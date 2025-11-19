#include "XMLParser.hpp"

#include <deque>
#include <libxml/parser.h>
#include <ranges>

namespace rdf4cpp::parser {
    struct XMLQuadIterator::Impl {
    private:
        xmlSAXHandler handler_;
        std::unique_ptr<xmlParserCtxt, decltype([](xmlParserCtxt *c) { xmlFreeParserCtxt(c); })> context_;
        void* reader_obj_;
        ReadFunc read_func_;
        ErrorFunc error_func_;
        EOFFunc eof_func_;
        std::deque<value_type> result_queue_;
        size_t next_bn_index_ = 0;
        std::unique_ptr<state_type> owned_state_;
        state_type *state_;
        std::set<IRI> reserved_ids_; // TODO faster alternative

        static constexpr std::string_view reify_subject = "http://www.w3.org/1999/02/22-rdf-syntax-ns#subject";
        static constexpr std::string_view reify_predicate = "http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate";
        static constexpr std::string_view reify_object = "http://www.w3.org/1999/02/22-rdf-syntax-ns#object";
        static constexpr std::string_view reify_type = "http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement";

        struct Attribute {
            xmlChar const *local_name_raw;
            xmlChar const *prefix_raw;
            xmlChar const *uri_raw;
            xmlChar const *value_start_raw;
            xmlChar const *value_end_raw;

            [[nodiscard]] std::string_view value() const {
                return {reinterpret_cast<char const *>(value_start_raw), reinterpret_cast<char const *>(value_end_raw)};
            }
            [[nodiscard]] std::string_view local_name() const {
                return {reinterpret_cast<char const *>(local_name_raw)};
            }
            [[nodiscard]] std::string_view uri() const {
                return {reinterpret_cast<char const *>(uri_raw)};
            }
        };

        struct BaseState {  // NOLINT(*-special-member-functions)
            virtual ~BaseState() = default;
            virtual void on_characters(Impl *impl, std::string_view chars) = 0;
            virtual void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) = 0;
            virtual void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) = 0; // TODO remove params?
            virtual void re_enter([[maybe_unused]] Impl *impl, [[maybe_unused]] Node obj) { // TODO remove if not needed for something
            }

            struct InheritedAttributeInfo {
                std::string_view base = "";
                std::string_view lang_tag = "";
            };

            std::string base;
            std::string lang_tag;

            explicit BaseState(InheritedAttributeInfo const &i) : base(i.base), lang_tag(i.lang_tag) {}

            static constexpr std::string_view base_attribute = "http://www.w3.org/XML/1998/namespacebase";
            static constexpr std::string_view lang_attribute = "http://www.w3.org/XML/1998/namespacelang";
            static InheritedAttributeInfo get_inherited_attributes(Impl* impl, std::span<Attribute> attributes);
        };

        struct InitialState final : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            InitialState() : BaseState({}) {}
        };
        struct RDFState final : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#RDF";

            using BaseState::BaseState;
        };
        struct DescriptionState final : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            Node subject;

            explicit DescriptionState(InheritedAttributeInfo const &i, Node sub)
                : BaseState(i), subject(sub) {
            }

            template<class F>
            static void enter(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes, F f);

            static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#Description";
            static constexpr std::string_view about_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#about";
            static constexpr std::string_view id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#ID";
            static constexpr std::string_view node_id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#nodeID";
            static constexpr std::string_view type_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
            static constexpr std::string_view list_start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#li";
        };
        struct PredicateState : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

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

            static bool iri_reserved_predicate(std::string_view uri, std::string_view local_name);
        };
        struct TypedLiteralPredicateState final : PredicateState {
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            IRI datatype;

            TypedLiteralPredicateState(InheritedAttributeInfo const &i, Node iri, IRI predicate, IRI reify, IRI datatype)
                : PredicateState(i, iri, predicate, reify), datatype(datatype) {
            }

            static constexpr std::string_view datatype_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#datatype";
        };
        struct XMLLiteralState final : PredicateState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            size_t depth = 0;
            size_t data_start = 0;
            size_t last_offset = 0;
            size_t last_size = 0;

            using PredicateState::PredicateState;

            void source_input(Impl *impl);
        };
        struct CollectionState final : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            Node subject;
            IRI predicate;
            Node last_bn = Node::make_null();
            IRI reify;
            bool first = true;

            CollectionState(InheritedAttributeInfo const &i, Node sub, IRI pred, IRI reify) : BaseState(i), subject(sub), predicate(pred), reify(reify) {}

            static constexpr std::string_view iri_nil = "http://www.w3.org/1999/02/22-rdf-syntax-ns#nil";
            static constexpr std::string_view iri_rest = "http://www.w3.org/1999/02/22-rdf-syntax-ns#rest";
            static constexpr std::string_view iri_first = "http://www.w3.org/1999/02/22-rdf-syntax-ns#first";
        };
        struct EmptyElement final : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            EmptyElement() :BaseState({}) {}
        };

        BaseState *current_state_ = nullptr;
        std::vector<std::variant<InitialState, RDFState, DescriptionState, PredicateState, TypedLiteralPredicateState, EmptyElement, XMLLiteralState, CollectionState>> state_stack_;

        static xmlSAXHandler make_sax_handler();

        void add_error(ParsingError::Type ty, std::string msg);
        /**
         * add statement to the output list, if none of the components is null
         * (null is used to track an already inserted parse error for that component)
         * @param subject
         * @param predicate
         * @param object
         */
        void add_statement(Node subject, IRI predicate, Node object, IRI reify);
        void update_current_state();
        void pop_state(Node object);
        static std::string_view trim(std::string_view v);
        static bool iri_equal_pieces(std::string_view full_iri, std::string_view uri, std::string_view local_name);
        static bool iri_reserved(std::string_view uri, std::string_view local_name);
        template<typename NT>
        NT inspect_node(NT node);
        IRI make_iri(std::string_view iri, std::string_view base);
        IRI make_iri(std::string_view uri, std::string_view local_name, std::string_view base);
        IRI make_id(std::string_view local_name, std::string_view base);
        [[nodiscard]] IRI make_hardcoded_iri(std::string_view iri) const;
        Node make_bn(std::optional<std::string_view> name);
        Literal make_literal(std::string_view value, std::optional<IRI> datatype, std::optional<std::string_view> lang_tag);

        static void on_error(void *th, char const *msg, ...);

    public:
        explicit Impl(void* obj, ReadFunc read, ErrorFunc err, EOFFunc eof, state_type* state);

        std::optional<value_type> next();
    };

    xmlSAXHandler XMLQuadIterator::Impl::make_sax_handler() {
        xmlSAXHandler r{};
        std::memset(&r, 0, sizeof(xmlSAXHandler));
        r.initialized = XML_SAX2_MAGIC;
        r.getParameterEntity = [](void *, xmlChar const *e) {
            return xmlGetPredefinedEntity(e);
        };
        r.getEntity = [](void *, xmlChar const *e) {
            return xmlGetPredefinedEntity(e);
        };
        r.characters = [](void *th, xmlChar const *e, int len) {
            auto *t = static_cast<Impl *>(th);
            t->current_state_->on_characters(t, std::string_view(reinterpret_cast<char const *>(e), static_cast<size_t>(len)));
        };
        r.startElementNs = [](void *th, xmlChar const *local_name, [[maybe_unused]] xmlChar const *prefix, xmlChar const *uri,
                              [[maybe_unused]] int n_namespaces, [[maybe_unused]] xmlChar const **namespaces,
                              int const n_attributes, [[maybe_unused]] int n_defaulted, xmlChar const **attributes) {
            auto *t = static_cast<Impl *>(th);
            t->current_state_->on_start_element(t, reinterpret_cast<char const *>(local_name), uri == nullptr ? "" : reinterpret_cast<char const *>(uri),
                                                std::span{reinterpret_cast<Attribute *>(attributes), static_cast<size_t>(n_attributes)});
        };
        r.endElementNs = [](void *th, xmlChar const *local_name, [[maybe_unused]] xmlChar const *prefix, xmlChar const *uri) {
            auto *t = static_cast<Impl *>(th);
            t->current_state_->on_end_element(t, reinterpret_cast<char const *>(local_name), uri == nullptr ? "" : reinterpret_cast<char const *>(uri));
        };
        r.warning = on_error;
        r.error = on_error;
        return r;
    }

    void XMLQuadIterator::Impl::add_error(ParsingError::Type const ty, std::string msg) {
        uint64_t const lin = xmlSAX2GetLineNumber(context_.get());
        uint64_t const col = xmlSAX2GetColumnNumber(context_.get());
        result_queue_.emplace_back(nonstd::unexpect, ty, lin, col, std::move(msg));
    }
    void XMLQuadIterator::Impl::add_statement(Node const subject, IRI const predicate, Node const object, IRI const reify) {
        if (subject.null() || predicate.null() || object.null()) {
            return;
        }
        result_queue_.emplace_back(Quad(subject, predicate, object));
        if (!reify.null()) {
            result_queue_.emplace_back(Quad(reify, IRI::make_unchecked(reify_subject, state_->node_storage), subject));
            result_queue_.emplace_back(Quad(reify, IRI::make_unchecked(reify_predicate, state_->node_storage), predicate));
            result_queue_.emplace_back(Quad(reify, IRI::make_unchecked(reify_object, state_->node_storage), object));
            result_queue_.emplace_back(Quad(reify, IRI::rdf_type(state_->node_storage), IRI::make_unchecked(reify_type, state_->node_storage)));
        }
    }
    void XMLQuadIterator::Impl::update_current_state() {
        if (state_stack_.empty()) {
            current_state_ = nullptr;
            return;
        }
        current_state_ = std::visit([](auto &s) -> BaseState * { return &s; }, state_stack_.back());
    }
    void XMLQuadIterator::Impl::pop_state(Node object) {
        assert(!state_stack_.empty());
        state_stack_.pop_back();
        update_current_state();
        current_state_->re_enter(this, object);
    }
    std::string_view XMLQuadIterator::Impl::trim(std::string_view v) {
        auto s = v.find_first_not_of(" \t\r\n");
        if (s == std::string_view::npos) {
            return "";
        }
        v.remove_prefix(s);
        // ReSharper disable once CppDFALocalValueEscapesFunction
        return v;
    }
    bool XMLQuadIterator::Impl::iri_equal_pieces(std::string_view const full_iri, std::string_view const uri, std::string_view const local_name) {
        if (full_iri.size() != local_name.size() + uri.size()) {
            return false;
        }
        return full_iri.starts_with(uri) && full_iri.ends_with(local_name);
    }
    bool XMLQuadIterator::Impl::iri_reserved(std::string_view uri, std::string_view local_name) {
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
    NT XMLQuadIterator::Impl::inspect_node(NT node) {
        try {
            state_->inspect_node_func(node);
            return node;
        }
        catch (std::exception &e) {
            add_error(ParsingError::Type::BadSyntax, std::format("Triple explicitly skipped by inspect function: {}", e.what()));
        }
        catch (...) {
            add_error(ParsingError::Type::BadSyntax, "Triple explicitly skipped by inspect function");
        }
        return NT::make_null();
    }
    IRI XMLQuadIterator::Impl::make_iri(std::string_view const iri, std::string_view const base) {
        if (base.empty()) {
            for (const auto &s : state_stack_ | std::ranges::views::reverse) {
                auto const v = std::visit([](const auto& s) -> std::string_view { return s.base; }, s);
                if (!v.empty()) {
                    state_->iri_factory.set_base_unchecked(v);
                    break;
                }
            }
        }
        else {
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
    IRI XMLQuadIterator::Impl::make_iri(std::string_view const uri, std::string_view const local_name, std::string_view const base) {
        std::string iri{uri};
        iri.append(local_name);
        return make_iri(iri, base);
    }
    IRI XMLQuadIterator::Impl::make_id(std::string_view const local_name, std::string_view const base) {
        std::string local = "#";
        local.append(local_name);
        auto iri = make_iri(local, base);
        if (reserved_ids_.contains(iri)) {
            add_error(ParsingError::Type::BadIri, std::format("{}: is already used as a rdf:ID", iri));
            return IRI::make_null();
        }
        reserved_ids_.insert(iri);
        return iri;
    }
    IRI XMLQuadIterator::Impl::make_hardcoded_iri(std::string_view const iri) const {
        return IRI::make_unchecked(iri, state_->node_storage);
    }
    Node XMLQuadIterator::Impl::make_bn(std::optional<std::string_view> name) {
        std::string n = "";
        if (!name.has_value()) {
            n = std::format("bn_{}", next_bn_index_++);
            name = n;
        }
        try {
            if (state_->blank_node_scope_manager == nullptr)
            {
                return inspect_node(BlankNode::make(*name));
            }
            else {
                return inspect_node(state_->blank_node_scope_manager.scope("").get_or_generate_node(*name, state_->node_storage));
            }
        }
        catch (InvalidNode const &e) {
            add_error(ParsingError::Type::BadBlankNode, e.what());
            return BlankNode::make_null();
        }
        catch (...) {
            add_error(ParsingError::Type::BadBlankNode, "unknown error");
            return BlankNode::make_null();
        }
    }
    Literal XMLQuadIterator::Impl::make_literal(std::string_view value, std::optional<IRI> datatype, std::optional<std::string_view> lang_tag) {
        Literal l = Literal::make_null();
        try {
            if (datatype.has_value()) {
                l = Literal::make_typed(value, *datatype, state_->node_storage);
            } else {
                if (!lang_tag.has_value() || lang_tag->empty()) {
                    for (const auto &s : state_stack_ | std::ranges::views::reverse) {
                        auto const v = std::visit([](const auto& s) -> std::string_view { return s.lang_tag; }, s);
                        if (!v.empty()) {
                            lang_tag = v;
                            break;
                        }
                    }
                }
                if (lang_tag.has_value() && !lang_tag->empty()) {
                    l = Literal::make_lang_tagged(value, *lang_tag, state_->node_storage);
                }
                else {
                    l = Literal::make_simple(value);
                }
            }
        }
        catch (InvalidNode const &e) {
            add_error(ParsingError::Type::BadLiteral, e.what());
        }
        catch (...) {
            add_error(ParsingError::Type::BadLiteral, "unknown error");
        }
        return inspect_node(l);
    }
    void XMLQuadIterator::Impl::on_error(void *th, char const *msg, ...) {  // NOLINT(*-dcl50-cpp)
        va_list args;
        auto t = static_cast<Impl *>(th);
        va_start(args, msg);  // NOLINT(*-pro-bounds-array-to-pointer-decay)
        std::string out{};
        out.resize(1024, '\0');
        auto l = vsnprintf(out.data(), out.size(), msg, args);  // NOLINT(*-pro-bounds-array-to-pointer-decay)
        if (l > 0)
        {
            out.resize(l);
        } else {
            out = "unknown error, too long to fit";
        }
        t->add_error(ParsingError::Type::BadSyntax, std::move(out));
        va_end(args);  // NOLINT(*-pro-bounds-array-to-pointer-decay)
    }

    XMLQuadIterator::Impl::BaseState::InheritedAttributeInfo XMLQuadIterator::Impl::BaseState::get_inherited_attributes(Impl *impl, std::span<Attribute> attributes) {
        InheritedAttributeInfo r{};
        for (const auto& a : attributes) {
            if (iri_equal_pieces(base_attribute, a.uri(), a.local_name()))
            {
                if (auto e = IRIView(a.value()).quick_validate(); e != IRIFactoryError::Ok) {
                    impl->add_error(ParsingError::Type::BadIri, std::format("invalid base IRI ({}): {}", e, a.value()));
                }
                r.base = a.value();
            }
            else if (iri_equal_pieces(lang_attribute, a.uri(), a.local_name()))
            {
                r.lang_tag = a.value();
            }
        }
        return r;
    }
    void XMLQuadIterator::Impl::InitialState::on_characters(Impl *impl, std::string_view const chars) {
        if (!trim(chars).empty()) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected RDF, found characters");
        }
    }
    void XMLQuadIterator::Impl::InitialState::on_start_element(Impl *impl, std::string_view const local_name, std::string_view const uri, [[maybe_unused]] std::span<Attribute> attributes) {
        if (iri_equal_pieces(RDFState::start_element, uri, local_name)) {
            impl->state_stack_.emplace_back(std::in_place_type_t<RDFState>{}, get_inherited_attributes(impl, attributes));
            impl->update_current_state();
            return;
        }
        impl->add_error(ParsingError::Type::BadSyntax, "expected RDF, found ???");
    }
    void XMLQuadIterator::Impl::InitialState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        impl->add_error(ParsingError::Type::BadSyntax, "expected RDF, found end of ???");
    }

    void XMLQuadIterator::Impl::RDFState::on_characters(Impl *impl, std::string_view const chars) {
        if (!trim(chars).empty()) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected Description, found characters");
        }
    }
    void XMLQuadIterator::Impl::RDFState::on_start_element(Impl *impl, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes) {
        DescriptionState::enter(impl, local_name, uri, attributes, [](auto) {});
    }
    void XMLQuadIterator::Impl::RDFState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        impl->pop_state(Node::make_null());
    }

    void XMLQuadIterator::Impl::DescriptionState::on_characters(Impl *impl, std::string_view const chars) {
        if (!trim(chars).empty()) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected predicate, found characters");
        }
    }
    void XMLQuadIterator::Impl::DescriptionState::on_start_element(Impl *impl, std::string_view const local_name, std::string_view const uri, std::span<Attribute> attributes) {
        auto const i = get_inherited_attributes(impl, attributes);
        auto predicate = impl->make_iri(uri, local_name, i.base);
        std::optional<IRI> datatype = std::nullopt;
        std::optional<Node> sub = std::nullopt;
        IRI reify = IRI::make_null();
        bool parse_resource = false;
        bool parse_literal = false;
        bool parse_collection = false;
        for (auto const &att : attributes) {
            if (iri_equal_pieces(TypedLiteralPredicateState::datatype_attrib, att.uri(), att.local_name())) {
                datatype = impl->make_iri(att.value(), i.base);
            } else if (iri_equal_pieces(PredicateState::resource_attrib, att.uri(), att.local_name())) {
                sub = impl->make_iri(att.value(), i.base);
            } else if (iri_equal_pieces(node_id_attrib, att.uri(), att.local_name())) {
                sub = impl->make_bn(att.value());
            } else if (iri_equal_pieces(id_attrib, att.uri(), att.local_name())) {
                reify = impl->make_id(att.value(), i.base);
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
                sub = impl->make_bn(std::nullopt);
            }
            if (iri_equal_pieces(type_attrib, att.uri(), att.local_name())) {
                IRI const obj = impl->make_iri(att.value(), base);
                impl->add_statement(*sub, IRI::rdf_type(impl->state_->node_storage), obj, IRI::make_null());
            } else {
                IRI const pred = impl->make_iri(att.uri(), att.local_name(), base);
                Literal const obj = impl->make_literal(att.value(), std::nullopt, i.lang_tag);
                impl->add_statement(*sub, pred, obj, IRI::make_null());
            }
        }
        if (sub.has_value() && (parse_collection || parse_literal || parse_resource)) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected only one of rdf:parseType, rdf:nodeID and rdf:resource");
        }
        if (datatype.has_value()) {
            impl->state_stack_.emplace_back(std::in_place_type_t<TypedLiteralPredicateState>{}, i, subject, predicate, reify, *datatype);
        } else if (sub.has_value()) {
            impl->add_statement(subject, predicate, *sub, reify);
            impl->state_stack_.emplace_back(std::in_place_type_t<EmptyElement>{});
        } else if (parse_resource) {
            Node obj = impl->make_bn(std::nullopt);
            impl->add_statement(subject, predicate, obj, reify);
            impl->state_stack_.emplace_back(std::in_place_type_t<DescriptionState>{}, i, obj);
        } else if (parse_literal) { // TODO tests
            auto& xml_state = impl->state_stack_.emplace_back(std::in_place_type_t<XMLLiteralState>{}, i, subject, predicate, reify);
            std::visit([&]<typename T>(T& o) { if constexpr (std::same_as<T, XMLLiteralState>) { o.source_input(impl); }}, xml_state);
        } else if (parse_collection) {
            impl->state_stack_.emplace_back(std::in_place_type_t<CollectionState>{}, i, subject, predicate, reify);
        } else {
            impl->state_stack_.emplace_back(std::in_place_type_t<PredicateState>{}, i, subject, predicate, reify);
        }
        impl->update_current_state();
    }
    void XMLQuadIterator::Impl::DescriptionState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        impl->pop_state(subject);
    }
    template<class F>
    void XMLQuadIterator::Impl::DescriptionState::enter(Impl *impl, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes, F f) {
        auto const i = get_inherited_attributes(impl, attributes);
        Node sub = Node::make_null();
        auto check_only_one = [&sub, impl]() {
            if (!sub.null()) {
                impl->add_error(ParsingError::Type::BadSyntax, "expected only one of rdf:ID, rdf:about, and rdf:nodeID");
                return true;
            }
            return false;
        };
        for (auto const &att : attributes) {
            if (iri_equal_pieces(about_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = impl->make_iri(att.value(), i.base);
            } else if (iri_equal_pieces(id_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = impl->make_id(att.value(), i.base);
            } else if (iri_equal_pieces(node_id_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = impl->make_bn(att.value());
            }
        }
        if (sub.null())
        {
            sub = impl->make_bn(std::nullopt);
        }
        if (!iri_equal_pieces(start_element, uri, local_name)) {
            IRI const obj = impl->make_iri(uri, local_name, i.base);
            if (!obj.null())
            {
                impl->add_statement(sub, IRI::rdf_type(impl->state_->node_storage), obj, IRI::make_null());
            }
        }
        for (auto const &att : attributes) {
            if (PredicateState::iri_reserved_predicate(att.uri(), att.local_name())) {
                continue;
            }
            if (iri_equal_pieces(type_attrib, att.uri(), att.local_name())) {
                IRI const obj = impl->make_iri(att.value(), i.base);
                impl->add_statement(sub, IRI::rdf_type(impl->state_->node_storage), obj, IRI::make_null());
            } else {
                IRI const pred = impl->make_iri(att.uri(), att.local_name(), i.base);
                Literal const obj = impl->make_literal(att.value(), std::nullopt, i.lang_tag);
                impl->add_statement(sub, pred, obj, IRI::make_null());
            }
        }
        f(sub);
        impl->state_stack_.emplace_back(std::in_place_type_t<DescriptionState>{}, i, sub);
        impl->update_current_state();
    }

    void XMLQuadIterator::Impl::PredicateState::on_characters([[maybe_unused]] Impl *impl, std::string_view const chars) {
        if (done) {
            if (!trim(chars).empty()) {
                impl->add_error(ParsingError::Type::BadSyntax, "expected end of element, found literal");
            }
            return;
        }
        literal.append(chars);
    }
    void XMLQuadIterator::Impl::PredicateState::on_start_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        if (!trim(literal).empty()) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected end of element or literal, found element");
            return;
        }
        if (done) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected end of element, found element");
            return;
        }
        DescriptionState::enter(impl, local_name, uri, attributes, [&](Node obj) {
            done = true;
            impl->add_statement(subject, predicate, obj, reify);
        });
    }
    void XMLQuadIterator::Impl::PredicateState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        if (!done) {
            Literal const lit = impl->make_literal(literal, std::nullopt, std::nullopt);
            impl->add_statement(subject, predicate, lit, reify);
        }
        impl->pop_state(Node::make_null());
    }

    bool XMLQuadIterator::Impl::PredicateState::iri_reserved_predicate(std::string_view const uri, std::string_view const local_name) {
        return iri_reserved(uri, local_name) || iri_equal_pieces(DescriptionState::start_element, uri, local_name) || iri_equal_pieces(DescriptionState::list_start_element, uri, local_name);
    }
    void XMLQuadIterator::Impl::TypedLiteralPredicateState::on_start_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        impl->add_error(ParsingError::Type::BadSyntax, "expected literal, found element");
    }
    void XMLQuadIterator::Impl::TypedLiteralPredicateState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        if (!datatype.null()) {
            Literal const lit = impl->make_literal(literal, datatype, std::nullopt);
            impl->add_statement(subject, predicate, lit, reify);
        }
        impl->pop_state(Node::make_null());
    }
    void XMLQuadIterator::Impl::XMLLiteralState::on_characters(Impl *impl, [[maybe_unused]] std::string_view chars) {
        source_input(impl);
    }
    void XMLQuadIterator::Impl::XMLLiteralState::on_start_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        ++depth;
        source_input(impl);
    }
    void XMLQuadIterator::Impl::XMLLiteralState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        if (depth > 0) {
            --depth;
            source_input(impl);
            return;
        }
        IRI datatype = impl->make_hardcoded_iri("http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral");
        std::string_view l = literal;
        l = l.substr(0, last_offset);
        l.remove_prefix(data_start);
        if (!l.empty() && l[0] == '/')
        {
            l.remove_prefix(1);
        }
        if (!l.empty() && l[0] == '>')
        {
            l.remove_prefix(1);
        }
        Literal const lit = impl->make_literal(l, datatype, std::nullopt);
        impl->add_statement(subject, predicate, lit, reify);
        impl->pop_state(Node::make_null());
    }
    void XMLQuadIterator::Impl::XMLLiteralState::source_input(Impl *impl) {
        const xmlChar* data;
        int size = 1024;
        int off = 0;
        xmlCtxtGetInputWindow(impl->context_.get(), 0, &data, &size, &off);
        std::string_view const sv{reinterpret_cast<const char*>(data), static_cast<size_t>(size)};
        if (literal.empty()) {
            data_start = off;
        }
        if (!static_cast<std::string_view>(literal).ends_with(sv)) {
            last_size = literal.size();
            literal += sv;
        }
        last_offset = static_cast<size_t>(off) + last_size;
    }
    void XMLQuadIterator::Impl::CollectionState::on_characters(Impl *impl, std::string_view const chars) {
        if (!trim(chars).empty()) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected element, found characters");
        }
    }
    void XMLQuadIterator::Impl::CollectionState::on_start_element(Impl *impl, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes) {
        DescriptionState::enter(impl, local_name, uri, attributes, [&](Node const obj) {
            if (first) {
                first = false;
                last_bn = impl->make_bn(std::nullopt);
                impl->add_statement(subject, predicate, last_bn, reify);
            } else {
                auto const bn = impl->make_bn(std::nullopt);
                impl->add_statement(last_bn, impl->make_hardcoded_iri(iri_rest), bn, IRI::make_null());
                last_bn = bn;
            }
            impl->add_statement(last_bn, impl->make_hardcoded_iri(iri_first), obj, IRI::make_null());
        });
    }
    void XMLQuadIterator::Impl::CollectionState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        auto const nil = impl->make_hardcoded_iri(iri_nil);
        if (first) {
            impl->add_statement(subject, predicate, nil, reify);
        } else {
            impl->add_statement(last_bn, impl->make_hardcoded_iri(iri_rest), nil, IRI::make_null());
        }
        impl->pop_state(Node::make_null());
    }
    void XMLQuadIterator::Impl::EmptyElement::on_characters(Impl *impl, std::string_view const chars) {
        if (!trim(chars).empty()) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected end of element, found characters");
        }
    }
    void XMLQuadIterator::Impl::EmptyElement::on_start_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        impl->add_error(ParsingError::Type::BadSyntax, "expected end of element, found ???");
    }
    void XMLQuadIterator::Impl::EmptyElement::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        impl->pop_state(Node::make_null());
    }

    XMLQuadIterator::Impl::Impl(void* obj, ReadFunc read, ErrorFunc err, EOFFunc eof, state_type* state)
        : handler_(make_sax_handler()),
          context_(xmlCreatePushParserCtxt(&handler_, this, nullptr, 0, "mem")),
          reader_obj_(obj), read_func_(read), error_func_(err), eof_func_(eof),
          owned_state_(state == nullptr ? std::make_unique<state_type>() : nullptr), state_(state == nullptr ? owned_state_.get() : state){
        xmlCtxtSetOptions(context_.get(), XML_PARSE_NOENT | XML_PARSE_PEDANTIC | XML_PARSE_NOCDATA | XML_PARSE_NO_XXE | XML_PARSE_BIG_LINES);
        state_stack_.emplace_back(std::in_place_type_t<InitialState>{});
        update_current_state();
        current_state_->base = IRIFactory::default_base;
    }

    std::optional<XMLQuadIterator::value_type> XMLQuadIterator::Impl::next() {
        std::array<char, 1024> buffer; // NOLINT(*-pro-type-member-init)
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


    XMLQuadIterator::XMLQuadIterator(void *stream, ReadFunc read, ErrorFunc error, EOFFunc eof, state_type* state)
        : impl_(std::make_unique<Impl>(stream, read, error, eof, state)), cur_(impl_->next()) {
    }
    XMLQuadIterator::XMLQuadIterator(std::istream &stream, state_type* state)
        : XMLQuadIterator(&stream,
    [](void *buf, [[maybe_unused]] size_t elem_size, size_t count, void *voided_self) noexcept -> size_t {
        RDF4CPP_ASSERT(elem_size == 1);

        auto *self = static_cast<std::istream *>(voided_self);
        self->read(static_cast<char *>(buf), static_cast<std::streamsize>(count));
        return self->gcount();
    },
    [](void *voided_self) noexcept {
        auto *self = static_cast<std::istream *>(voided_self);
        return static_cast<int>(self->fail() && !self->eof());
    },
    [](void *voided_self) noexcept {
        auto *self = static_cast<std::istream *>(voided_self);
        return static_cast<int>(self->eof());
    }, state)
    {
    }
    XMLQuadIterator::~XMLQuadIterator() noexcept = default;

    XMLQuadIterator::reference XMLQuadIterator::operator*() const noexcept {
        return *cur_;
    }
    XMLQuadIterator::pointer XMLQuadIterator::operator->() const noexcept {
        return &*cur_;
    }
    XMLQuadIterator &XMLQuadIterator::operator++() {
        cur_ = impl_->next();
        return *this;
    }
    bool XMLQuadIterator::operator==(std::default_sentinel_t) const noexcept {
        return !cur_.has_value();
    }
}  // namespace rdf4cpp::parser
