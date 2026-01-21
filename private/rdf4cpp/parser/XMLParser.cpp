#include <rdf4cpp/parser/XMLParser.hpp>

#include <rdf4cpp/parser/XMLParserStateTransition.hpp>

#include <ranges>

namespace rdf4cpp::parser {
    xmlSAXHandler IStreamQuadIterator::ImplXML::make_sax_handler() {
        xmlSAXHandler r{};
        std::memset(&r, 0, sizeof(xmlSAXHandler));
        r.initialized = XML_SAX2_MAGIC;
        r.getParameterEntity = get_entity;
        r.getEntity = get_entity;
        r.characters = on_characters;
        r.startElementNs = on_start_element;
        r.endElementNs = on_end_element;
        r.warning = on_error;
        r.error = on_error;
        return r;
    }

    void IStreamQuadIterator::ImplXML::handle_state_transition(StateTransition transition) {
        dice::template_library::match(std::move(transition.modify_state),
            [](NoStateChange) {
                // noop
            },
            [this](PopState) {
                state_stack_.pop_back();
            },
            [this]<typename S>(S &&new_state) {
                state_stack_.emplace_back(std::in_place_type<S>, std::forward<S>(new_state));
            }
        );
    }

    // implemented here, to have access to states
    bool iri_reserved(std::string_view const uri, std::string_view const local_name) {
        static constexpr std::array reserved = {
                xml_states::RDFState::start_element,
                xml_states::DescriptionState::id_attrib,
                xml_states::DescriptionState::about_attrib,
                xml_states::PredicateState::parse_type_attrib,
                xml_states::PredicateState::resource_attrib,
                xml_states::DescriptionState::node_id_attrib,
                xml_states::TypedLiteralPredicateState::datatype_attrib,
                xml_states::BaseState::base_attribute,
                xml_states::BaseState::lang_attribute,
                std::string_view("http://www.w3.org/1999/02/22-rdf-syntax-ns#aboutEach"),
                std::string_view("http://www.w3.org/1999/02/22-rdf-syntax-ns#aboutEachPrefix"),
                std::string_view("http://www.w3.org/1999/02/22-rdf-syntax-ns#bagID"),
        };
        return std::ranges::any_of(reserved, [&](std::string_view const e) {
            return iri_equal_pieces(e, uri, local_name);
        });
    }
    bool iri_core_syntax(std::string_view const uri, std::string_view const local_name) {
        static constexpr std::array reserved = {
            xml_states::RDFState::start_element,
            xml_states::DescriptionState::id_attrib,
            xml_states::DescriptionState::about_attrib,
            xml_states::PredicateState::parse_type_attrib,
            xml_states::PredicateState::resource_attrib,
            xml_states::DescriptionState::node_id_attrib,
        };
        return std::ranges::any_of(reserved, [&](std::string_view const e) {
            return iri_equal_pieces(e, uri, local_name);
        });
    }
    bool iri_old_term(std::string_view const uri, std::string_view const local_name) {
        static constexpr std::array reserved = {
            std::string_view{"http://www.w3.org/1999/02/22-rdf-syntax-ns#aboutEach"},
            std::string_view{"http://www.w3.org/1999/02/22-rdf-syntax-ns#aboutEachPrefix"},
            std::string_view{"http://www.w3.org/1999/02/22-rdf-syntax-ns#bagID"},
        };
        return std::ranges::any_of(reserved, [&](std::string_view const e) {
            return iri_equal_pieces(e, uri, local_name);
        });
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
        t->output_.add_error(ParsingError::Type::BadSyntax, std::move(out), t->make_info());
        va_end(args);  // NOLINT(*-pro-bounds-array-to-pointer-decay)
    }
    xmlEntity *IStreamQuadIterator::ImplXML::get_entity(void *, xmlChar const *e) {
        return xmlGetPredefinedEntity(e);
    }
    void IStreamQuadIterator::ImplXML::on_characters(void *th, xmlChar const *e, int const len) {
        auto *t = static_cast<ImplXML *>(th);
        t->handle_state_transition(t->current_state().on_characters(t->output_, from_xml_char(e, len), t->make_info()));
    }
    void IStreamQuadIterator::ImplXML::on_start_element(void *th, xmlChar const *local_name, [[maybe_unused]] xmlChar const *prefix, xmlChar const *uri,
                                                        [[maybe_unused]] int n_namespaces, [[maybe_unused]] xmlChar const **namespaces,
                                                        int const n_attributes, [[maybe_unused]] int n_defaulted, xmlChar const **attributes) {
        auto *t = static_cast<ImplXML *>(th);
        t->handle_state_transition(t->current_state().on_start_element(t->output_, from_xml_char(local_name), from_xml_char(uri),
                                                                       std::span{reinterpret_cast<XMLAttribute *>(attributes), static_cast<size_t>(n_attributes)}, t->make_info()));
    }
    void IStreamQuadIterator::ImplXML::on_end_element(void *th, [[maybe_unused]] xmlChar const *local_name, [[maybe_unused]] xmlChar const *prefix, [[maybe_unused]] xmlChar const *uri) {
        auto *t = static_cast<ImplXML *>(th);
        t->handle_state_transition(t->current_state().on_end_element(t->output_, t->make_info()));
    }

    XMLStateInfo IStreamQuadIterator::ImplXML::make_info() const {
        std::string_view base = "";
        for (auto const &s : state_stack_ | std::views::reverse) {
            std::string_view const v = s->base;
            if (!v.empty()) {
                base = v;
                break;
            }
        }

        std::string_view lang_tag = "";
        for (auto const &s : state_stack_ | std::views::reverse) {
            std::string_view const v = s->lang_tag;
            if (!v.empty()) {
                lang_tag = v;
                break;
            }
        }

        xmlChar const *data;
        int size = 1024;
        int off = 0;
        xmlCtxtGetInputWindow(context_.get(), 0, &data, &size, &off);
        std::string_view const source{reinterpret_cast<char const *>(data), static_cast<size_t>(size)};

        return XMLStateInfo{
                current_line(),
                current_column(),
                base,
                lang_tag,
                source,
                off,
        };
    }

    IStreamQuadIterator::ImplXML::ImplXML(void *obj, ReadFunc const read, ErrorFunc const err, EOFFunc const eof, state_type *state)
        : handler_(make_sax_handler()),
          context_(xmlCreatePushParserCtxt(&handler_, this, nullptr, 0, "rdf/xml")),
          reader_obj_(obj), read_func_(read), error_func_(err), eof_func_(eof),
          output_(state) {
        xmlCtxtSetOptions(context_.get(), XML_PARSE_NOENT | XML_PARSE_PEDANTIC | XML_PARSE_NOCDATA | XML_PARSE_NO_XXE | XML_PARSE_BIG_LINES);
        state_stack_.reserve(10);
        state_stack_.emplace_back(std::in_place_type_t<xml_states::InitialState>{});

        current_state().base = output_.current_base_iri();
    }

    std::optional<IStreamQuadIterator::value_type> IStreamQuadIterator::ImplXML::next() {
        std::array<char, 8192> buffer;  // NOLINT(*-pro-type-member-init)
        while (output_.empty() && error_func_(reader_obj_) == 0 && eof_func_(reader_obj_) == 0) {
            auto const read = read_func_(buffer.data(), sizeof(char), buffer.size(), reader_obj_);
            xmlParseChunk(context_.get(), buffer.data(), static_cast<int>(read), eof_func_(reader_obj_) != 0);
        }
        return output_.next();
    }

    uint64_t IStreamQuadIterator::ImplXML::current_line() const noexcept {
        return xmlSAX2GetLineNumber(context_.get());
    }

    uint64_t IStreamQuadIterator::ImplXML::current_column() const noexcept {
        return xmlSAX2GetColumnNumber(context_.get());
    }
}  // namespace rdf4cpp::parser