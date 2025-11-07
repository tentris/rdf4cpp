#include "XMLParser.hpp"

#include <deque>
#include <libxml/parser.h>

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
        IRI try_make_iri(std::string_view iri);
        IRI try_make_iri(std::string_view uri, std::string_view local_name);
        BlankNode make_bn();

        static void on_error(void *th, char const *msg, ...);

    public:
        explicit Impl(void* obj, ReadFunc read, ErrorFunc err, EOFFunc eof);

        std::optional<value_type> next();
    };

    xmlSAXHandler XMLQuadIterator::Impl::make_sax_handler() {
        xmlSAXHandler r{};
        std::memset(&r, 0, sizeof(xmlSAXHandler));
        r.initialized = XML_SAX2_MAGIC;
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
    IRI XMLQuadIterator::Impl::try_make_iri(std::string_view const iri) {
        auto exp = IRIFactory::create_and_validate(iri);
        if (exp.has_value()) {
            return *exp;
        } else {
            add_error(ParsingError::Type::BadIri, std::format("{}: {}", iri, exp.error()));
            return IRI::make_null();
        }
    }
    IRI XMLQuadIterator::Impl::try_make_iri(std::string_view const uri, std::string_view const local_name) {
        std::string iri{uri};
        iri.append(local_name);
        return try_make_iri(iri);
    }
    BlankNode XMLQuadIterator::Impl::make_bn() {
        return BlankNode::make_unchecked(std::format("bn_{}", next_bn_index_++));
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

    void XMLQuadIterator::Impl::EmptyState::on_characters(Impl *impl, std::string_view const chars) {
        if (!trim(chars).empty()) {
            impl->add_error(ParsingError::Type::BadSyntax, "expected RDF, found characters");
        }
    }
    void XMLQuadIterator::Impl::EmptyState::on_start_element(Impl *impl, std::string_view const local_name, std::string_view const uri, [[maybe_unused]] std::span<Attribute> attributes) {
        if (iri_equal_pieces(RDFState::start_element, uri, local_name)) {
            impl->state_stack_.emplace_back(std::in_place_type_t<RDFState>{});
            impl->update_current_state();
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
        std::string s{uri};
        s.append(local_name);
        auto predicate = impl->try_make_iri(s);
        std::optional<IRI> datatype = std::nullopt;
        std::optional<IRI> ref = std::nullopt;
        for (auto const &att : attributes) {
            if (iri_equal_pieces(TypedLiteralPredicateState::datatype_attrib, att.uri(), att.local_name())) {
                datatype = impl->try_make_iri(att.value());
            } else if (iri_equal_pieces(PredicateState::resource_attrib, att.uri(), att.local_name())) {
                ref = impl->try_make_iri(att.value());
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
    }
    void XMLQuadIterator::Impl::DescriptionState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        impl->pop_state(subject);
    }
    template<class F>
    bool XMLQuadIterator::Impl::DescriptionState::try_enter(Impl *impl, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes, F f) {
        if (iri_equal_pieces(start_element, uri, local_name)) {
            for (auto const &att : attributes) {
                if (iri_equal_pieces(about_attrib, att.uri(), att.local_name())) {
                    IRI iri = impl->try_make_iri(att.value());
                    f(iri);
                    impl->state_stack_.emplace_back(std::in_place_type_t<DescriptionState>{}, iri);
                    impl->update_current_state();
                    return true;
                }
            }
            auto bn = impl->make_bn();
            f(bn);
            impl->state_stack_.emplace_back(std::in_place_type_t<DescriptionState>{}, bn);
            impl->update_current_state();
            return true;
        }
        return false;
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
            Literal lit = Literal::make_null();
            try {
                lit = Literal::make_simple(literal);
            } catch (std::runtime_error const &e) {  // InvalidNode is subclass
                impl->add_error(ParsingError::Type::BadLiteral, e.what());
            }
            impl->add_statement(subject, predicate, lit);
        }
        impl->pop_state(Node::make_null());
    }

    void XMLQuadIterator::Impl::TypedLiteralPredicateState::on_start_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        impl->add_error(ParsingError::Type::BadSyntax, "expected literal, found element");
    }
    void XMLQuadIterator::Impl::TypedLiteralPredicateState::on_end_element(Impl *impl, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri) {
        if (!datatype.null()) {
            Literal lit = Literal::make_null();
            try {
                lit = Literal::make_typed(literal, datatype);
            } catch (std::runtime_error const &e) {  // InvalidNode is subclass
                impl->add_error(ParsingError::Type::BadLiteral, e.what());
            }
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

    XMLQuadIterator::Impl::Impl(void* obj, ReadFunc read, ErrorFunc err, EOFFunc eof)
        : handler_(make_sax_handler()),
          context_(xmlCreatePushParserCtxt(&handler_, this, nullptr, 0, "mem")),
          reader_obj_(obj), read_func_(read), error_func_(err), eof_func_(eof) {
        state_stack_.emplace_back(std::in_place_type_t<EmptyState>{});
        update_current_state();
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


    XMLQuadIterator::XMLQuadIterator(void *stream, ReadFunc read, ErrorFunc error, EOFFunc eof)
        : impl_(std::make_unique<Impl>(stream, read, error, eof)), cur_(impl_->next()) {
    }
    XMLQuadIterator::XMLQuadIterator(std::istream &stream)
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
    })
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
