#include <rdf4cpp/parser/XMLParser.hpp>

#include <ranges>

namespace rdf4cpp::parser {
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

    IStreamQuadIterator::ImplXML::ImplXML(void *obj, ReadFunc const read, ErrorFunc const err, EOFFunc const eof, state_type *state)
        : handler_(make_sax_handler()),
          context_(xmlCreatePushParserCtxt(&handler_, this, nullptr, 0, "rdf/xml")),
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
}  // namespace rdf4cpp::parser