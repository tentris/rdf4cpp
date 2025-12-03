#ifndef RDF4CPP_XMLPARSER_H
#define RDF4CPP_XMLPARSER_H

#include <deque>
#include <iterator>
#include <memory>

#include <rdf4cpp/Expected.hpp>

#include <rdf4cpp/Quad.hpp>

#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>
#include <rdf4cpp/parser/ParsingError.hpp>
#include <rdf4cpp/parser/ParsingState.hpp>

#include <libxml/parser.h>

#include <dice/sparse-map/sparse_set.hpp>

#include <dice/template-library/inplace_polymorphic.hpp>

namespace rdf4cpp::parser {
    struct IStreamQuadIterator::ImplXML final : Impl {
    private:
        xmlSAXHandler handler_;
        std::unique_ptr<xmlParserCtxt, decltype([](xmlParserCtxt *c) {
                            xmlFreeParserCtxt(c);
                        })>
                context_;
        void *reader_obj_;
        ReadFunc read_func_;
        ErrorFunc error_func_;
        EOFFunc eof_func_;
        std::deque<value_type> result_queue_;
        size_t next_bn_index_ = 0;
        state_type *state_;
        bool state_is_owned_ = false;
        dice::sparse_map::sparse_set<storage::identifier::NodeBackendID> reserved_ids_;

        static constexpr std::string_view reify_subject = "http://www.w3.org/1999/02/22-rdf-syntax-ns#subject";
        static constexpr std::string_view reify_predicate = "http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate";
        static constexpr std::string_view reify_object = "http://www.w3.org/1999/02/22-rdf-syntax-ns#object";
        static constexpr std::string_view reify_type = "http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement";

        static std::string_view from_xml_char(xmlChar const *s) {
            if (s == nullptr) {
                return "";
            }
            // ReSharper disable once CppDFALocalValueEscapesFunction
            return {reinterpret_cast<char const *>(s)};
        }

        static std::string_view from_xml_char(xmlChar const *s, xmlChar const *e) {
            if (s == nullptr) {
                return "";
            }
            // ReSharper disable once CppDFALocalValueEscapesFunction
            return {reinterpret_cast<char const *>(s), reinterpret_cast<char const *>(e)};
        }

        static std::string_view from_xml_char(xmlChar const *s, int const n) {
            if (s == nullptr) {
                return "";
            }
            // ReSharper disable once CppDFALocalValueEscapesFunction
            return {reinterpret_cast<char const *>(s), static_cast<size_t>(n)};
        }

        struct Attribute {
            xmlChar const *local_name_raw;
            xmlChar const *prefix_raw;
            xmlChar const *uri_raw;
            xmlChar const *value_start_raw;
            xmlChar const *value_end_raw;

            [[nodiscard]] std::string_view value() const {
                return from_xml_char(value_start_raw, value_end_raw);
            }

            [[nodiscard]] std::string_view local_name() const {
                return from_xml_char(local_name_raw);
            }

            [[nodiscard]] std::string_view uri() const {
                return from_xml_char(uri_raw);
            }
        };

        /**
         * most states handle one or more of the elements in https://www.w3.org/TR/rdf11-xml/#section-Infoset-Grammar .
         * note that the creation of a state is done by on_start_element of the previous state.
         * each state holds information on base iri and language tag defined on the corresponding xml element.
         */
        struct BaseState {  // NOLINT(*-special-member-functions)
            virtual ~BaseState() = default;
            virtual void on_characters(ImplXML &impl, std::string_view chars) = 0;
            virtual void on_start_element(ImplXML &impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) = 0;
            virtual void on_end_element(ImplXML &impl) = 0;
            virtual void move_to(BaseState *b) noexcept = 0;

            struct InheritedAttributeInfo {
                std::string_view base = "";
                std::string_view lang_tag = "";
            };

            std::string base;
            std::string lang_tag;

            explicit BaseState(InheritedAttributeInfo const &i)
                : base(i.base), lang_tag(i.lang_tag) {
            }

            static constexpr std::string_view base_attribute = "http://www.w3.org/XML/1998/namespacebase";
            static constexpr std::string_view lang_attribute = "http://www.w3.org/XML/1998/namespacelang";
            static InheritedAttributeInfo get_inherited_attributes(ImplXML &impl, std::span<Attribute> attributes);
        };

        /**
         * initial state, checks for start of https://www.w3.org/TR/rdf11-xml/#RDF
         */
        struct InitialState final : BaseState {
            void on_characters(ImplXML &i, std::string_view chars) override;
            void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(ImplXML &i) override;
            void move_to(BaseState *b) noexcept override;

            InitialState()
                : BaseState({}) {
            }
        };

        /**
         * state for https://www.w3.org/TR/rdf11-xml/#RDF
         */
        struct RDFState final : BaseState {
            void on_characters(ImplXML &i, std::string_view chars) override;
            void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(ImplXML &i) override;
            void move_to(BaseState *b) noexcept override;

            static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#RDF";

            using BaseState::BaseState;
        };

        /**
         * state for https://www.w3.org/TR/rdf11-xml/#nodeElementList and https://www.w3.org/TR/rdf11-xml/#nodeElement
         * on_start_element checks and dispatches for the different options in https://www.w3.org/TR/rdf11-xml/#propertyEltList
         * (https://www.w3.org/TR/rdf11-xml/#parseTypeResourcePropertyElt has no own state, instead gets handled directly by on_start_element)
         */
        struct DescriptionState final : BaseState {
            void on_characters(ImplXML &i, std::string_view chars) override;
            void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(ImplXML &i) override;
            void move_to(BaseState *b) noexcept override;

            Node subject;
            size_t list_current = 1;

            explicit DescriptionState(InheritedAttributeInfo const &i, Node sub)
                : BaseState(i), subject(sub) {
            }

            template<class F>
            static void enter(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes, F f);

            static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#Description";
            static constexpr std::string_view about_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#about";
            static constexpr std::string_view id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#ID";
            static constexpr std::string_view node_id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#nodeID";
            static constexpr std::string_view type_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
        };

        /**
         * state for https://www.w3.org/TR/rdf11-xml/#resourcePropertyElt (nested nodeElement / DescriptionState
         * and https://www.w3.org/TR/rdf11-xml/#literalPropertyElt (literal with no datatype attribute)
         * and https://www.w3.org/TR/rdf11-xml/#emptyPropertyElt (with no attributes (empty literal))
         */
        struct PredicateState : BaseState {
            void on_characters(ImplXML &i, std::string_view chars) override;
            void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(ImplXML &i) override;
            void move_to(BaseState *b) noexcept override;

            Node subject;
            IRI predicate;
            IRI reify;
            std::string literal;
            bool done = false;

            PredicateState(InheritedAttributeInfo const &i, Node sub, IRI predicate, IRI reify)
                : BaseState(i), subject(sub), predicate(predicate), reify(reify) {
            }

            static constexpr std::string_view resource_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#resource";
            static constexpr std::string_view parse_type_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#parseType";
            static constexpr std::string_view parse_type_resource = "Resource";
            static constexpr std::string_view parse_type_literal = "Literal";
            static constexpr std::string_view parse_type_collection = "Collection";
            static constexpr std::string_view list_start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#li";

            static bool iri_reserved_predicate(std::string_view uri, std::string_view local_name);
        };

        /**
         * state for https://www.w3.org/TR/rdf11-xml/#literalPropertyElt (with datatype attribute)
         */
        struct TypedLiteralPredicateState final : PredicateState {
            void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(ImplXML &i) override;
            void move_to(BaseState *b) noexcept override;

            IRI datatype;

            TypedLiteralPredicateState(InheritedAttributeInfo const &i, Node iri, IRI predicate, IRI reify, IRI datatype)
                : PredicateState(i, iri, predicate, reify), datatype(datatype) {
            }

            static constexpr std::string_view datatype_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#datatype";
        };

        /**
         * state for https://www.w3.org/TR/rdf11-xml/#parseTypeLiteralPropertyElt
         * (and https://www.w3.org/TR/rdf11-xml/#parseTypeOtherPropertyElt)
         */
        struct XMLLiteralState final : PredicateState {
            void on_characters(ImplXML &i, std::string_view chars) override;
            void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(ImplXML &i) override;
            void move_to(BaseState *b) noexcept override;

            size_t depth = 0;
            size_t data_start = 0;
            size_t last_offset = 0;
            size_t last_size = 0;

            using PredicateState::PredicateState;

            void source_input(ImplXML &i);
        };

        /**
         * state for https://www.w3.org/TR/rdf11-xml/#parseTypeCollectionPropertyElt
         */
        struct CollectionState final : BaseState {
            void on_characters(ImplXML &i, std::string_view chars) override;
            void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(ImplXML &i) override;
            void move_to(BaseState *b) noexcept override;

            Node subject;
            IRI predicate;
            Node last_bn = Node::make_null();
            IRI reify;
            bool first = true;

            CollectionState(InheritedAttributeInfo const &i, Node sub, IRI pred, IRI reify)
                : BaseState(i), subject(sub), predicate(pred), reify(reify) {
            }

            static constexpr std::string_view iri_nil = "http://www.w3.org/1999/02/22-rdf-syntax-ns#nil";
            static constexpr std::string_view iri_rest = "http://www.w3.org/1999/02/22-rdf-syntax-ns#rest";
            static constexpr std::string_view iri_first = "http://www.w3.org/1999/02/22-rdf-syntax-ns#first";
        };

        /**
         * state for https://www.w3.org/TR/rdf11-xml/#emptyPropertyElt (if attributes are present)
         */
        struct EmptyElement final : BaseState {
            void on_characters(ImplXML &i, std::string_view chars) override;
            void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(ImplXML &i) override;
            void move_to(BaseState *b) noexcept override;

            EmptyElement()
                : BaseState({}) {
            }
        };

        BaseState *current_state_ = nullptr;
        std::vector<dice::template_library::inplace_polymorphic<BaseState, InitialState, RDFState, DescriptionState, PredicateState, TypedLiteralPredicateState, EmptyElement, XMLLiteralState, CollectionState>> state_stack_;

        static xmlSAXHandler make_sax_handler();

        void add_error(ParsingError::Type ty, std::string msg);
        /**
         * add statement to the output list, if none of the components is null
         * (null is used to track an already inserted parse error for that component)
         */
        void add_statement(Node subject, IRI predicate, Node object, IRI reify);
        void update_current_state();
        void pop_state();
        /**
         * removes whitespace according to xml spec
         */
        [[nodiscard]] static std::string_view trim_left(std::string_view v);
        [[nodiscard]] static bool iri_equal_pieces(std::string_view full_iri, std::string_view uri, std::string_view local_name);
        [[nodiscard]] static bool iri_reserved(std::string_view uri, std::string_view local_name);
        template<typename NT>
        [[nodiscard]] NT inspect_node(NT node);
        [[nodiscard]] IRI make_iri(std::string_view iri, std::string_view base);
        [[nodiscard]] IRI make_iri(std::string_view uri, std::string_view local_name, std::string_view base);
        /**
         * create the IRI for an id_attrib, including uniqueness check
         */
        [[nodiscard]] IRI make_id(std::string_view local_name, std::string_view base);
        /**
         * create an IRI with no checks, intended for hardcoded IRIs like reify_subject
         */
        [[nodiscard]] IRI make_hardcoded_iri(std::string_view iri) const;
        [[nodiscard]] IRI make_type_iri() const;
        [[nodiscard]] Node make_bn(std::optional<std::string_view> name);
        /**
         * creates a literal
         * @param value
         * @param datatype
         * @param lang_tag (ignored, if datatype is set)
         * @return
         */
        [[nodiscard]] Literal make_literal(std::string_view value, std::optional<IRI> datatype, std::optional<std::string_view> lang_tag);

        static void on_error(void *th, char const *msg, ...);

    public:
        ImplXML(void *obj, ReadFunc read, ErrorFunc err, EOFFunc eof, state_type *state);
        ~ImplXML() override;

        ImplXML(ImplXML const &) = delete;
        ImplXML &operator=(ImplXML const &) = delete;
        ImplXML(ImplXML &&) = delete;
        ImplXML &operator=(ImplXML &&) = delete;

        [[nodiscard]] std::optional<value_type> next() override;

        [[nodiscard]] uint64_t current_line() const noexcept override;
        [[nodiscard]] uint64_t current_column() const noexcept override;
    };
}

#endif  //RDF4CPP_XMLPARSER_H
