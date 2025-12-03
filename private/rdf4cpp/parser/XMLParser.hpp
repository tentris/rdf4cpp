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

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserInitialState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserPredicateState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserXMLLiteralState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserCollectionState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserDescriptionState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserEmptyElement.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserRDFState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserTypedLiteralPredicateState.hpp>

namespace rdf4cpp::parser {
    struct IStreamQuadIterator::ImplXML final : ImplXMLStateCollector {
    private:
        xmlSAXHandler handler_;
        // workaround for gcc-14 bug, erroneously warns on unsing a lambda here
        // see https://github.com/NVIDIA/stdexec/issues/1143
        struct XmlParserCtxtDtorLambda {
            void operator()(xmlParserCtxt* c) const {
                xmlFreeParserCtxt(c);
            }
        };
        std::unique_ptr<xmlParserCtxt, XmlParserCtxtDtorLambda> context_;
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

        friend struct BaseState;
        friend struct InitialState;
        friend struct RDFState;
        friend struct DescriptionState;
        friend struct PredicateState;
        friend struct TypedLiteralPredicateState;
        friend struct EmptyElement;
        friend struct XMLLiteralState;
        friend struct CollectionState;

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
