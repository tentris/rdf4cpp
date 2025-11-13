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

            std::string base;

            static constexpr std::string_view base_attribute = "http://www.w3.org/XML/1998/namespacebase";
            static std::string_view try_handle_base_attrib(Impl* impl, std::span<Attribute> attributes);
        };

        struct EmptyState final : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;
        };
        struct RDFState final : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#RDF";
        };
        struct DescriptionState final : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            Node subject;

            explicit DescriptionState(Node sub)
                : subject(sub) {
            }

            template<class F>
            static bool try_enter(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes, F f);

            static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#Description";
            static constexpr std::string_view about_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#about";
            static constexpr std::string_view id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#ID";
            static constexpr std::string_view node_id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#nodeID";
            static constexpr std::string_view type_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
        };
        struct PredicateState : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            Node subject;
            IRI predicate;
            std::string literal;
            bool done = false;

            PredicateState(Node sub, IRI predicate)
                : subject(sub), predicate(predicate) {
            }

            static constexpr std::string_view resource_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#resource";
        };
        struct TypedLiteralPredicateState final : PredicateState {
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;

            IRI datatype;

            TypedLiteralPredicateState(Node iri, IRI predicate, IRI datatype)
                : PredicateState(iri, predicate), datatype(datatype) {
            }

            static constexpr std::string_view datatype_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#datatype";
        };

        struct EmptyElement final : BaseState {
            void on_characters(Impl *impl, std::string_view chars) override;
            void on_start_element(Impl *impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
            void on_end_element(Impl *impl, std::string_view local_name, std::string_view uri) override;
        };

        BaseState *current_state_ = nullptr;
        std::vector<std::variant<EmptyState, RDFState, DescriptionState, PredicateState, TypedLiteralPredicateState, EmptyElement>> state_stack_;

        static xmlSAXHandler make_sax_handler();

        void add_error(ParsingError::Type ty, std::string msg);
        /**
         * add statement to the output list, if none of the components is null
         * (null is used to track an already inserted parse error for that component)
         * @param subject
         * @param predicate
         * @param object
         */
        void add_statement(Node subject, IRI predicate, Node object);
        void update_current_state();
        void pop_state(Node object);
        static std::string_view trim(std::string_view v);
        static bool iri_equal_pieces(std::string_view full_iri, std::string_view uri, std::string_view local_name);
        template<typename NT>
        NT inspect_node(NT node);
        IRI try_make_iri(std::string_view iri, std::string_view base);
        IRI try_make_iri(std::string_view uri, std::string_view local_name, std::string_view base);
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
            t->current_state_->on_start_element(t, reinterpret_cast<char const *>(local_name), reinterpret_cast<char const *>(uri),
                                                std::span{reinterpret_cast<Attribute *>(attributes), static_cast<size_t>(n_attributes)});
        };
        r.endElementNs = [](void *th, xmlChar const *local_name, [[maybe_unused]] xmlChar const *prefix, xmlChar const *uri) {
            auto *t = static_cast<Impl *>(th);
            t->current_state_->on_end_element(t, reinterpret_cast<char const *>(local_name), reinterpret_cast<char const *>(uri));
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
    void XMLQuadIterator::Impl::add_statement(Node subject, IRI predicate, Node object) {
        if (subject.null() || predicate.null() || object.null()) {
            return;
        }
        result_queue_.emplace_back(Quad(subject, predicate, object));
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
    IRI XMLQuadIterator::Impl::try_make_iri(std::string_view const iri, std::string_view base) {
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
    IRI XMLQuadIterator::Impl::try_make_iri(std::string_view const uri, std::string_view const local_name, std::string_view base) {
        std::string iri{uri};
        iri.append(local_name);
        return try_make_iri(iri, base);
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
            }
            else if (lang_tag.has_value()) {
                l = Literal::make_lang_tagged(value, *lang_tag, state_->node_storage);
            }
            else {
                l = Literal::make_simple(value);
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

    std::string_view XMLQuadIterator::Impl::BaseState::try_handle_base_attrib(Impl* impl, std::span<Attribute> attributes) {
        for (const auto& a : attributes) {
            if (iri_equal_pieces(base_attribute, a.uri(), a.local_name()))
            {
                if (auto r = IRIView(a.value()).quick_validate(); r != IRIFactoryError::Ok) {
                    impl->add_error(ParsingError::Type::BadIri, std::format("invalid base IRI ({}): {}", r, a.value()));
                }
                return a.value();
            }
        }
        return "";
    }
    void XMLQuadIterator::Impl::EmptyState::on_characters(Impl *impl, std::string_view const chars) {
        if (!trim(chars).empty()) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected RDF, found characters");
        }
    }
    void XMLQuadIterator::Impl::EmptyState::on_start_element(Impl *impl, std::string_view const local_name, std::string_view const uri, [[maybe_unused]] std::span<Attribute> attributes) {
        if (iri_equal_pieces(RDFState::start_element, uri, local_name)) {
            auto const base = try_handle_base_attrib(impl, attributes);
            impl->state_stack_.emplace_back(std::in_place_type_t<RDFState>{});
            impl->update_current_state();
            if (!base.empty()) {
                impl->current_state_->base = base;
            }
            return;
        }
        impl->add_error(ParsingError::Type::BadSyntax, "expected RDF, found ???");
    }
    void XMLQuadIterator::Impl::EmptyState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        impl->add_error(ParsingError::Type::BadSyntax, "expected RDF, found end of ???");
    }

    void XMLQuadIterator::Impl::RDFState::on_characters(Impl *impl, std::string_view const chars) {
        if (!trim(chars).empty()) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected Description, found characters");
        }
    }
    void XMLQuadIterator::Impl::RDFState::on_start_element(Impl *impl, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes) {
        if (DescriptionState::try_enter(impl, local_name, uri, attributes, [](auto) {})) {
            return;
        }
        impl->add_error(ParsingError::Type::BadSyntax, "expected Description, found ???");
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
        auto base = try_handle_base_attrib(impl, attributes);
        std::string s{uri};
        s.append(local_name);
        auto predicate = impl->try_make_iri(s, base);
        std::optional<IRI> datatype = std::nullopt;
        std::optional<IRI> ref = std::nullopt;
        for (auto const &att : attributes) {
            if (iri_equal_pieces(TypedLiteralPredicateState::datatype_attrib, att.uri(), att.local_name())) {
                datatype = impl->try_make_iri(att.value(), base);
            } else if (iri_equal_pieces(PredicateState::resource_attrib, att.uri(), att.local_name())) {
                ref = impl->try_make_iri(att.value(), base);
            }
        }
        if (datatype.has_value()) {
            impl->state_stack_.emplace_back(std::in_place_type_t<TypedLiteralPredicateState>{}, subject, predicate, *datatype);
        } else if (ref.has_value()) {
            impl->add_statement(subject, predicate, *ref);
            impl->state_stack_.emplace_back(std::in_place_type_t<EmptyElement>{});
        } else {
            impl->state_stack_.emplace_back(std::in_place_type_t<PredicateState>{}, subject, predicate);
        }
        impl->update_current_state();
        if (!base.empty()) {
            impl->current_state_->base = base;
        }
    }
    void XMLQuadIterator::Impl::DescriptionState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        impl->pop_state(subject);
    }
    template<class F>
    bool XMLQuadIterator::Impl::DescriptionState::try_enter(Impl *impl, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes, F f) {
        auto base = try_handle_base_attrib(impl, attributes);
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
                sub = impl->try_make_iri(att.value(), base);
            } else if (iri_equal_pieces(id_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                std::string i = "#";
                i.append(att.value());
                sub = impl->try_make_iri(i, base);
            } else if (iri_equal_pieces(node_id_attrib, att.uri(), att.local_name())) {  // TODO test case
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
            IRI const obj = impl->try_make_iri(uri, local_name, base);
            if (!obj.null())
            {
                impl->add_statement(sub, IRI::rdf_type(), obj);
            }
        }
        for (auto const &att : attributes) {
            if (iri_equal_pieces(type_attrib, att.uri(), att.local_name())) {  // TODO needs test
                IRI const obj = impl->try_make_iri(att.value(), base);
                if (obj.null())
                {
                    continue;
                }
                impl->add_statement(sub, IRI::rdf_type(), obj);
            } else if (iri_equal_pieces(about_attrib, att.uri(), att.local_name()) || iri_equal_pieces(node_id_attrib, att.uri(), att.local_name()) || iri_equal_pieces(id_attrib, att.uri(), att.local_name()) || iri_equal_pieces(base_attribute, att.uri(), att.local_name())) {
                continue;
            } else {  // TODO tests say this is correct, spec does not???
                IRI const pred = impl->try_make_iri(att.uri(), att.local_name(), base);
                Literal const obj = impl->make_literal(att.value(), std::nullopt, std::nullopt);
                if (pred.null() || obj.null())
                {
                    continue;
                }
                impl->add_statement(sub, pred, obj);
            }
        }
        f(sub);
        impl->state_stack_.emplace_back(std::in_place_type_t<DescriptionState>{}, sub);
        impl->update_current_state();
        if (!base.empty()) {
            impl->current_state_->base = base;
        }
        return true;
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
        if (DescriptionState::try_enter(impl, local_name, uri, attributes, [&](Node obj) {
            done = true;
            impl->add_statement(subject, predicate, obj);
        })) {
            return;
        }
        impl->add_error(ParsingError::Type::BadSyntax, "expected Description or literal, found element");
    }
    void XMLQuadIterator::Impl::PredicateState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        if (!done) {
            Literal const lit = impl->make_literal(literal, std::nullopt, std::nullopt);
            impl->add_statement(subject, predicate, lit);
        }
        impl->pop_state(Node::make_null());
    }

    void XMLQuadIterator::Impl::TypedLiteralPredicateState::on_start_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        impl->add_error(ParsingError::Type::BadSyntax, "expected literal, found element");
    }
    void XMLQuadIterator::Impl::TypedLiteralPredicateState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        if (!datatype.null()) {
            Literal const lit = impl->make_literal(literal, datatype, std::nullopt);
            impl->add_statement(subject, predicate, lit);
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
        state_stack_.emplace_back(std::in_place_type_t<EmptyState>{});
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
