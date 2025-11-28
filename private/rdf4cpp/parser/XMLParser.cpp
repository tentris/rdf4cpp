#include <deque>
#include <iterator>
#include <memory>

#include <rdf4cpp/Expected.hpp>

#include <rdf4cpp/Quad.hpp>

#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>
#include <rdf4cpp/parser/ParsingError.hpp>
#include <rdf4cpp/parser/ParsingState.hpp>

#include <ranges>

#include <libxml/parser.h>

#include <dice/sparse-map/sparse_set.hpp>

#include <dice/template-library/inplace_polymorphic.hpp>

namespace rdf4cpp::parser {
    // not in a header, because that would complicate linking it against libxml
    // probably even requiring to make libxml public
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


    xmlSAXHandler IStreamQuadIterator::ImplXML::make_sax_handler() {
        xmlSAXHandler r{};
        std::memset(&r, 0, sizeof(xmlSAXHandler));
        r.initialized = XML_SAX2_MAGIC;
        r.getParameterEntity = [](void *, xmlChar const *e) {
            return xmlGetPredefinedEntity(e);
        };
        r.getEntity = [](void *, xmlChar const *e) {
            return xmlGetPredefinedEntity(e);
        };
        r.characters = [](void *th, xmlChar const *e, int const len) {
            auto *t = static_cast<ImplXML *>(th);
            t->current_state_->on_characters(*t, from_xml_char(e, len));
        };
        r.startElementNs = [](void *th, xmlChar const *local_name, [[maybe_unused]] xmlChar const *prefix, xmlChar const *uri,
                              [[maybe_unused]] int n_namespaces, [[maybe_unused]] xmlChar const **namespaces,
                              int const n_attributes, [[maybe_unused]] int n_defaulted, xmlChar const **attributes) {
            auto *t = static_cast<ImplXML *>(th);
            t->current_state_->on_start_element(*t, from_xml_char(local_name), from_xml_char(uri),
                                                std::span{reinterpret_cast<Attribute *>(attributes), static_cast<size_t>(n_attributes)});
        };
        r.endElementNs = [](void *th, [[maybe_unused]] xmlChar const *local_name, [[maybe_unused]] xmlChar const *prefix, [[maybe_unused]] xmlChar const *uri) {
            auto *t = static_cast<ImplXML *>(th);
            t->current_state_->on_end_element(*t);
        };
        r.warning = on_error;
        r.error = on_error;
        return r;
    }

    void IStreamQuadIterator::ImplXML::add_error(ParsingError::Type const ty, std::string msg) {
        uint64_t const lin = xmlSAX2GetLineNumber(context_.get());
        uint64_t const col = xmlSAX2GetColumnNumber(context_.get());
        result_queue_.emplace_back(nonstd::unexpect, ty, lin, col, std::move(msg));
    }

    void IStreamQuadIterator::ImplXML::add_statement(Node const subject, IRI const predicate, Node const object, IRI const reify) {
        if (subject.null() || predicate.null() || object.null()) {
            return;
        }
        result_queue_.emplace_back(Quad(subject, predicate, object));
        if (!reify.null()) {
            result_queue_.emplace_back(Quad(reify, make_hardcoded_iri(reify_subject), subject));
            result_queue_.emplace_back(Quad(reify, make_hardcoded_iri(reify_predicate), predicate));
            result_queue_.emplace_back(Quad(reify, make_hardcoded_iri(reify_object), object));
            result_queue_.emplace_back(Quad(reify, make_type_iri(), make_hardcoded_iri(reify_type)));
        }
    }

    void IStreamQuadIterator::ImplXML::update_current_state() {
        if (state_stack_.empty()) {
            current_state_ = nullptr;
            return;
        }
        current_state_ = &state_stack_.back().get();
    }

    void IStreamQuadIterator::ImplXML::pop_state() {
        assert(!state_stack_.empty());
        state_stack_.pop_back();
        update_current_state();
    }

    std::string_view IStreamQuadIterator::ImplXML::trim_left(std::string_view v) {
        auto s = v.find_first_not_of(" \t\r\n");
        if (s == std::string_view::npos) {
            return "";
        }
        v.remove_prefix(s);
        // ReSharper disable once CppDFALocalValueEscapesFunction
        return v;
    }

    bool IStreamQuadIterator::ImplXML::iri_equal_pieces(std::string_view const full_iri, std::string_view const uri, std::string_view const local_name) {
        if (full_iri.size() != local_name.size() + uri.size()) {
            return false;
        }
        return full_iri.starts_with(uri) && full_iri.ends_with(local_name);
    }

    bool IStreamQuadIterator::ImplXML::iri_reserved(std::string_view uri, std::string_view local_name) {
        static constexpr std::array reserved = {
                RDFState::start_element,
                DescriptionState::id_attrib,
                DescriptionState::about_attrib,
                PredicateState::parse_type_attrib,
                PredicateState::resource_attrib,
                DescriptionState::node_id_attrib,
                TypedLiteralPredicateState::datatype_attrib,
                BaseState::base_attribute,
                BaseState::lang_attribute,
                std::string_view("http://www.w3.org/1999/02/22-rdf-syntax-ns#aboutEach"),
                std::string_view("http://www.w3.org/1999/02/22-rdf-syntax-ns#aboutEachPrefix"),
                std::string_view("http://www.w3.org/1999/02/22-rdf-syntax-ns#bagID"),
        };
        return std::ranges::any_of(reserved, [&](std::string_view const e) {
            return iri_equal_pieces(e, uri, local_name);
        });
    }

    template<typename NT>
    NT IStreamQuadIterator::ImplXML::inspect_node(NT node) {
        try {
            state_->inspect_node_func(node);
            return node;
        } catch (std::exception &e) {
            add_error(ParsingError::Type::BadSyntax, std::format("Triple explicitly skipped by inspect function: {}", e.what()));
        } catch (...) {
            add_error(ParsingError::Type::BadSyntax, "Triple explicitly skipped by inspect function");
        }
        return NT::make_null();
    }

    IRI IStreamQuadIterator::ImplXML::make_iri(std::string_view const iri, std::string_view const base) {
        if (base.empty()) {
            for (auto const &s : state_stack_ | std::ranges::views::reverse) {
                std::string_view const v = s.get().base;
                if (!v.empty()) {
                    state_->iri_factory.set_base_unchecked(v);
                    break;
                }
            }
        } else {
            state_->iri_factory.set_base_unchecked(base);
        }
        auto exp = state_->iri_factory.from_maybe_relative(iri, state_->node_storage);
        if (exp.has_value()) {
            return inspect_node(*exp);
        } else {
            add_error(ParsingError::Type::BadIri, std::format("{}: {}", iri, exp.error()));
            return IRI::make_null();
        }
    }

    IRI IStreamQuadIterator::ImplXML::make_iri(std::string_view const uri, std::string_view const local_name, std::string_view const base) {
        std::string iri{uri};
        iri.append(local_name);
        return make_iri(iri, base);
    }

    IRI IStreamQuadIterator::ImplXML::make_id(std::string_view const local_name, std::string_view const base) {
        std::string local = "#";
        local.append(local_name);
        auto iri = make_iri(local, base);
        if (reserved_ids_.contains(iri.backend_handle().id())) {
            add_error(ParsingError::Type::BadIri, std::format("{}: is already used as a rdf:ID", iri));
            return IRI::make_null();
        }
        reserved_ids_.insert(iri.backend_handle().id());
        return iri;
    }

    IRI IStreamQuadIterator::ImplXML::make_hardcoded_iri(std::string_view const iri) const {
        return IRI::make_unchecked(iri, state_->node_storage);
    }

    IRI IStreamQuadIterator::ImplXML::make_type_iri() const {
        return IRI::rdf_type(state_->node_storage);
    }

    Node IStreamQuadIterator::ImplXML::make_bn(std::optional<std::string_view> name) {
        std::string n = "";
        if (!name.has_value()) {
            n = std::format("bn_{}", next_bn_index_++);
            name = n;
        }
        try {
            if (state_->blank_node_scope_manager == nullptr) {
                return inspect_node(BlankNode::make(*name, state_->node_storage));
            } else {
                return inspect_node(state_->blank_node_scope_manager.scope("").get_or_generate_node(*name, state_->node_storage));
            }
        } catch (InvalidNode const &e) {
            add_error(ParsingError::Type::BadBlankNode, e.what());
            return BlankNode::make_null();
        } catch (...) {
            add_error(ParsingError::Type::BadBlankNode, "unknown error");
            return BlankNode::make_null();
        }
    }

    Literal IStreamQuadIterator::ImplXML::make_literal(std::string_view value, std::optional<IRI> datatype, std::optional<std::string_view> lang_tag) {
        Literal l = Literal::make_null();
        try {
            if (datatype.has_value()) {
                l = Literal::make_typed(value, *datatype, state_->node_storage);
            } else {
                if (!lang_tag.has_value() || lang_tag->empty()) {
                    for (auto const &s : state_stack_ | std::ranges::views::reverse) {
                        std::string_view const v = s.get().lang_tag;
                        if (!v.empty()) {
                            lang_tag = v;
                            break;
                        }
                    }
                }
                if (lang_tag.has_value() && !lang_tag->empty()) {
                    l = Literal::make_lang_tagged(value, *lang_tag, state_->node_storage);
                } else {
                    l = Literal::make_simple(value, state_->node_storage);
                }
            }
        } catch (InvalidNode const &e) {
            add_error(ParsingError::Type::BadLiteral, e.what());
        } catch (...) {
            add_error(ParsingError::Type::BadLiteral, "unknown error");
        }
        return inspect_node(l);
    }

    void IStreamQuadIterator::ImplXML::on_error(void *th, char const *msg, ...) {  // NOLINT(*-dcl50-cpp)
        va_list args;
        auto t = static_cast<ImplXML *>(th);
        va_start(args, msg);  // NOLINT(*-pro-bounds-array-to-pointer-decay)
        std::string out{};
        out.resize(1024, '\0');
        auto l = vsnprintf(out.data(), out.size(), msg, args);  // NOLINT(*-pro-bounds-array-to-pointer-decay)
        if (l > 0) {
            out.resize(l);
        } else {
            out = "unknown error, too long to fit";
        }
        t->add_error(ParsingError::Type::BadSyntax, std::move(out));
        va_end(args);  // NOLINT(*-pro-bounds-array-to-pointer-decay)
    }

    IStreamQuadIterator::ImplXML::BaseState::InheritedAttributeInfo IStreamQuadIterator::ImplXML::BaseState::get_inherited_attributes(ImplXML &impl, std::span<Attribute> const attributes) {
        InheritedAttributeInfo r{};
        for (auto const &a : attributes) {
            if (iri_equal_pieces(base_attribute, a.uri(), a.local_name())) {
                if (auto e = IRIView(a.value()).quick_validate(); e != IRIFactoryError::Ok) {
                    impl.add_error(ParsingError::Type::BadIri, std::format("invalid base IRI ({}): {}", e, a.value()));
                }
                r.base = a.value();
            } else if (iri_equal_pieces(lang_attribute, a.uri(), a.local_name())) {
                r.lang_tag = a.value();
            }
        }
        return r;
    }

    void IStreamQuadIterator::ImplXML::InitialState::on_characters(ImplXML &i, std::string_view const chars) {
        if (!trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected RDF, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::InitialState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, [[maybe_unused]] std::span<Attribute> attributes) {
        if (iri_equal_pieces(RDFState::start_element, uri, local_name)) {
            i.state_stack_.emplace_back(std::in_place_type_t<RDFState>{}, get_inherited_attributes(i, attributes));
            i.update_current_state();
            return;
        }
        i.add_error(ParsingError::Type::BadSyntax, "expected RDF, found ???");
    }

    void IStreamQuadIterator::ImplXML::InitialState::on_end_element(ImplXML &i) {
        i.add_error(ParsingError::Type::BadSyntax, "expected RDF, found end of ???");
    }
    void IStreamQuadIterator::ImplXML::InitialState::move_to(BaseState *b) noexcept {
        new (b) InitialState(std::move(*this));
    }

    void IStreamQuadIterator::ImplXML::RDFState::on_characters(ImplXML &i, std::string_view const chars) {
        if (!trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected Description, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::RDFState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes) {
        DescriptionState::enter(i, local_name, uri, attributes, [](auto) {
        });
    }

    void IStreamQuadIterator::ImplXML::RDFState::on_end_element(ImplXML &i) {
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::RDFState::move_to(BaseState *b) noexcept {
        new (b) RDFState(std::move(*this));
    }

    void IStreamQuadIterator::ImplXML::DescriptionState::on_characters(ImplXML &i, std::string_view const chars) {
        if (!trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected predicate, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::DescriptionState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> attributes) {
        auto const inherited_attribute_info = get_inherited_attributes(i, attributes);
        IRI predicate;
        if (iri_equal_pieces(PredicateState::list_start_element, uri, local_name)) {
            predicate = i.make_iri(std::format("http://www.w3.org/1999/02/22-rdf-syntax-ns#_{}", list_current++), inherited_attribute_info.base);
        } else {
            predicate = i.make_iri(uri, local_name, inherited_attribute_info.base);
        }
        std::optional<IRI> datatype = std::nullopt;
        std::optional<Node> sub = std::nullopt;
        IRI reify = IRI::make_null();
        bool parse_resource = false;
        bool parse_literal = false;
        bool parse_collection = false;
        for (auto const &att : attributes) {
            if (iri_equal_pieces(TypedLiteralPredicateState::datatype_attrib, att.uri(), att.local_name())) {
                datatype = i.make_iri(att.value(), inherited_attribute_info.base);
            } else if (iri_equal_pieces(PredicateState::resource_attrib, att.uri(), att.local_name())) {
                sub = i.make_iri(att.value(), inherited_attribute_info.base);
            } else if (iri_equal_pieces(node_id_attrib, att.uri(), att.local_name())) {
                sub = i.make_bn(att.value());
            } else if (iri_equal_pieces(id_attrib, att.uri(), att.local_name())) {
                reify = i.make_id(att.value(), inherited_attribute_info.base);
            } else if (iri_equal_pieces(PredicateState::parse_type_attrib, att.uri(), att.local_name())) {
                if (att.value() == PredicateState::parse_type_resource) {
                    parse_resource = true;
                } else if (att.value() == PredicateState::parse_type_collection) {
                    parse_collection = true;
                } else {
                    parse_literal = true;
                }
            }
        }
        for (auto const &att : attributes) {
            if (PredicateState::iri_reserved_predicate(att.uri(), att.local_name())) {
                continue;
            }
            if (!sub.has_value()) {
                sub = i.make_bn(std::nullopt);
            }
            if (iri_equal_pieces(type_attrib, att.uri(), att.local_name())) {
                IRI const obj = i.make_iri(att.value(), base);
                i.add_statement(*sub, i.make_type_iri(), obj, IRI::make_null());
            } else {
                IRI const pred = i.make_iri(att.uri(), att.local_name(), base);
                Literal const obj = i.make_literal(att.value(), std::nullopt, inherited_attribute_info.lang_tag);
                i.add_statement(*sub, pred, obj, IRI::make_null());
            }
        }
        if (sub.has_value() && (parse_collection || parse_literal || parse_resource)) {
            i.add_error(ParsingError::Type::BadSyntax, "expected only one of rdf:parseType, rdf:nodeID and rdf:resource");
        }
        if (datatype.has_value()) {
            i.state_stack_.emplace_back(std::in_place_type_t<TypedLiteralPredicateState>{}, inherited_attribute_info, subject, predicate, reify, *datatype);
        } else if (sub.has_value()) {
            i.add_statement(subject, predicate, *sub, reify);
            i.state_stack_.emplace_back(std::in_place_type_t<EmptyElement>{});
        } else if (parse_resource) {
            Node const obj = i.make_bn(std::nullopt);
            i.add_statement(subject, predicate, obj, reify);
            i.state_stack_.emplace_back(std::in_place_type_t<DescriptionState>{}, inherited_attribute_info, obj);
        } else if (parse_literal) {
            auto &xml_state = i.state_stack_.emplace_back(std::in_place_type_t<XMLLiteralState>{}, inherited_attribute_info, subject, predicate, reify);
            static_cast<XMLLiteralState &>(xml_state.get()).source_input(i);  // NOLINT(*-pro-type-static-cast-downcast)
        } else if (parse_collection) {
            i.state_stack_.emplace_back(std::in_place_type_t<CollectionState>{}, inherited_attribute_info, subject, predicate, reify);
        } else {
            i.state_stack_.emplace_back(std::in_place_type_t<PredicateState>{}, inherited_attribute_info, subject, predicate, reify);
        }
        i.update_current_state();
    }

    void IStreamQuadIterator::ImplXML::DescriptionState::on_end_element(ImplXML &i) {
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::DescriptionState::move_to(BaseState *b) noexcept {
        new (b) DescriptionState(std::move(*this));
    }

    template<class F>
    void IStreamQuadIterator::ImplXML::DescriptionState::enter(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes, F f) {
        auto const inherited_attribute_info = get_inherited_attributes(i, attributes);
        Node sub = Node::make_null();
        auto check_only_one = [&sub, &i]() {
            if (!sub.null()) {
                i.add_error(ParsingError::Type::BadSyntax, "expected only one of rdf:ID, rdf:about, and rdf:nodeID");
                return true;
            }
            return false;
        };
        for (auto const &att : attributes) {
            if (iri_equal_pieces(about_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = i.make_iri(att.value(), inherited_attribute_info.base);
            } else if (iri_equal_pieces(id_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = i.make_id(att.value(), inherited_attribute_info.base);
            } else if (iri_equal_pieces(node_id_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = i.make_bn(att.value());
            }
        }
        if (sub.null()) {
            sub = i.make_bn(std::nullopt);
        }
        if (!iri_equal_pieces(start_element, uri, local_name)) {
            IRI const obj = i.make_iri(uri, local_name, inherited_attribute_info.base);
            if (!obj.null()) {
                i.add_statement(sub, i.make_type_iri(), obj, IRI::make_null());
            }
        }
        for (auto const &att : attributes) {
            if (PredicateState::iri_reserved_predicate(att.uri(), att.local_name())) {
                continue;
            }
            if (iri_equal_pieces(type_attrib, att.uri(), att.local_name())) {
                IRI const obj = i.make_iri(att.value(), inherited_attribute_info.base);
                i.add_statement(sub, i.make_type_iri(), obj, IRI::make_null());
            } else {
                IRI const pred = i.make_iri(att.uri(), att.local_name(), inherited_attribute_info.base);
                Literal const obj = i.make_literal(att.value(), std::nullopt, inherited_attribute_info.lang_tag);
                i.add_statement(sub, pred, obj, IRI::make_null());
            }
        }
        f(sub);
        i.state_stack_.emplace_back(std::in_place_type_t<DescriptionState>{}, inherited_attribute_info, sub);
        i.update_current_state();
    }

    void IStreamQuadIterator::ImplXML::PredicateState::on_characters([[maybe_unused]] ImplXML &i, std::string_view const chars) {
        if (done) {
            if (!trim_left(chars).empty()) {
                i.add_error(ParsingError::Type::BadSyntax, "expected end of element, found literal");
            }
            return;
        }
        literal.append(chars);
    }

    void IStreamQuadIterator::ImplXML::PredicateState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes) {
        if (!trim_left(literal).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected end of element or literal, found element");
            return;
        }
        if (done) {
            i.add_error(ParsingError::Type::BadSyntax, "expected end of element, found element");
            return;
        }
        DescriptionState::enter(i, local_name, uri, attributes, [&](Node obj) {
            done = true;
            i.add_statement(subject, predicate, obj, reify);
        });
    }

    void IStreamQuadIterator::ImplXML::PredicateState::on_end_element(ImplXML &i) {
        if (!done) {
            Literal const lit = i.make_literal(literal, std::nullopt, std::nullopt);
            i.add_statement(subject, predicate, lit, reify);
        }
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::PredicateState::move_to(BaseState *b) noexcept {
        new (b) PredicateState(std::move(*this));
    }

    bool IStreamQuadIterator::ImplXML::PredicateState::iri_reserved_predicate(std::string_view const uri, std::string_view const local_name) {
        return iri_reserved(uri, local_name) || iri_equal_pieces(DescriptionState::start_element, uri, local_name) || iri_equal_pieces(list_start_element, uri, local_name);
    }

    void IStreamQuadIterator::ImplXML::TypedLiteralPredicateState::on_start_element(ImplXML &i, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        i.add_error(ParsingError::Type::BadSyntax, "expected literal, found element");
    }

    void IStreamQuadIterator::ImplXML::TypedLiteralPredicateState::on_end_element(ImplXML &i) {
        if (!datatype.null()) {
            Literal const lit = i.make_literal(literal, datatype, std::nullopt);
            i.add_statement(subject, predicate, lit, reify);
        }
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::TypedLiteralPredicateState::move_to(BaseState *b) noexcept {
        new (b) TypedLiteralPredicateState(std::move(*this));
    }

    void IStreamQuadIterator::ImplXML::XMLLiteralState::on_characters(ImplXML &i, [[maybe_unused]] std::string_view chars) {
        source_input(i);
    }

    void IStreamQuadIterator::ImplXML::XMLLiteralState::on_start_element(ImplXML &i, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        ++depth;
        source_input(i);
    }

    void IStreamQuadIterator::ImplXML::XMLLiteralState::on_end_element(ImplXML &i) {
        if (depth > 0) {
            --depth;
            source_input(i);
            return;
        }
        IRI datatype = i.make_hardcoded_iri("http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral");
        std::string_view l = literal;
        l = l.substr(0, last_offset);
        l.remove_prefix(data_start);
        if (!l.empty() && l[0] == '/') {
            l.remove_prefix(1);
        }
        if (!l.empty() && l[0] == '>') {
            l.remove_prefix(1);
        }
        Literal const lit = i.make_literal(l, datatype, std::nullopt);
        i.add_statement(subject, predicate, lit, reify);
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::XMLLiteralState::move_to(BaseState *b) noexcept {
        new (b) XMLLiteralState(std::move(*this));
    }

    void IStreamQuadIterator::ImplXML::XMLLiteralState::source_input(ImplXML &i) {
        xmlChar const *data;
        int size = 1024;
        int off = 0;
        xmlCtxtGetInputWindow(i.context_.get(), 0, &data, &size, &off);
        std::string_view const sv{reinterpret_cast<char const *>(data), static_cast<size_t>(size)};
        if (literal.empty()) {
            data_start = off;
        }
        if (!static_cast<std::string_view>(literal).ends_with(sv)) {
            last_size = literal.size();
            literal += sv;
        }
        last_offset = static_cast<size_t>(off) + last_size;
    }

    void IStreamQuadIterator::ImplXML::CollectionState::on_characters(ImplXML &i, std::string_view const chars) {
        if (!trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected element, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::CollectionState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes) {
        DescriptionState::enter(i, local_name, uri, attributes, [&](Node const obj) {
            if (first) {
                first = false;
                last_bn = i.make_bn(std::nullopt);
                i.add_statement(subject, predicate, last_bn, reify);
            } else {
                auto const bn = i.make_bn(std::nullopt);
                i.add_statement(last_bn, i.make_hardcoded_iri(iri_rest), bn, IRI::make_null());
                last_bn = bn;
            }
            i.add_statement(last_bn, i.make_hardcoded_iri(iri_first), obj, IRI::make_null());
        });
    }

    void IStreamQuadIterator::ImplXML::CollectionState::on_end_element(ImplXML &i) {
        auto const nil = i.make_hardcoded_iri(iri_nil);
        if (first) {
            i.add_statement(subject, predicate, nil, reify);
        } else {
            i.add_statement(last_bn, i.make_hardcoded_iri(iri_rest), nil, IRI::make_null());
        }
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::CollectionState::move_to(BaseState *b) noexcept {
        new (b) CollectionState(std::move(*this));
    }

    void IStreamQuadIterator::ImplXML::EmptyElement::on_characters(ImplXML &i, std::string_view const chars) {
        if (!trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected end of element, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::EmptyElement::on_start_element(ImplXML &i, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        i.add_error(ParsingError::Type::BadSyntax, "expected end of element, found ???");
    }

    void IStreamQuadIterator::ImplXML::EmptyElement::on_end_element(ImplXML &i) {
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::EmptyElement::move_to(BaseState *b) noexcept {
        new (b) EmptyElement(std::move(*this));
    }

    IStreamQuadIterator::ImplXML::ImplXML(void *obj, ReadFunc const read, ErrorFunc const err, EOFFunc const eof, state_type *state)
        : handler_(make_sax_handler()),
          context_(xmlCreatePushParserCtxt(&handler_, this, nullptr, 0, "mem")),
          reader_obj_(obj), read_func_(read), error_func_(err), eof_func_(eof),
          state_(state) {
        xmlCtxtSetOptions(context_.get(), XML_PARSE_NOENT | XML_PARSE_PEDANTIC | XML_PARSE_NOCDATA | XML_PARSE_NO_XXE | XML_PARSE_BIG_LINES);
        state_stack_.reserve(10);
        state_stack_.emplace_back(std::in_place_type_t<InitialState>{});
        update_current_state();

        if (state_ == nullptr) {
            state_ = new state_type();
            state_is_owned_ = true;
        }

        current_state_->base = state_->iri_factory.get_base();
    }
    IStreamQuadIterator::ImplXML::~ImplXML() {
        if (state_is_owned_) {
            delete state_;
        }
    }

    std::optional<IStreamQuadIterator::value_type> IStreamQuadIterator::ImplXML::next() {
        std::array<char, 1024> buffer;  // NOLINT(*-pro-type-member-init)
        while (result_queue_.empty() && error_func_(reader_obj_) == 0 && eof_func_(reader_obj_) == 0) {
            auto const read = read_func_(buffer.data(), sizeof(char), buffer.size(), reader_obj_);
            xmlParseChunk(context_.get(), buffer.data(), static_cast<int>(read), eof_func_(reader_obj_) != 0);
        }
        if (result_queue_.empty()) {
            return std::nullopt;
        }
        auto r = result_queue_.front();
        result_queue_.pop_front();
        return r;
    }

    uint64_t IStreamQuadIterator::ImplXML::current_line() const noexcept {
        return xmlSAX2GetLineNumber(context_.get());
    }

    uint64_t IStreamQuadIterator::ImplXML::current_column() const noexcept {
        return xmlSAX2GetColumnNumber(context_.get());
    }


    std::unique_ptr<IStreamQuadIterator::Impl> IStreamQuadIterator::make_xml_impl(void *obj, ReadFunc read, ErrorFunc err, EOFFunc eof, state_type *state) {
        return std::make_unique<ImplXML>(obj, read, err, eof, state);
    }
}  // namespace rdf4cpp::parser