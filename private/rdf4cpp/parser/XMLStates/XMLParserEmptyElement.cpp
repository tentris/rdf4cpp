#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser::xml_states {
    StateTransition EmptyElement::on_characters(XMLOutputQueue &out, std::string_view const chars, XMLStateInfo const &info) {
        if (!trim_left(chars).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected end of element, found characters", info);
        }
        return {};
    }

    StateTransition EmptyElement::on_start_element(XMLOutputQueue &out, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<XMLAttribute> attributes, XMLStateInfo const &info) {
        out.add_error(ParsingError::Type::BadSyntax, "expected end of element, found ???", info);
        return {};
    }

    StateTransition EmptyElement::on_end_element([[maybe_unused]] XMLOutputQueue &out, [[maybe_unused]] XMLStateInfo const &info) {
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void EmptyElement::move_to(BaseState *b) noexcept {
        new (b) EmptyElement(std::move(*this));
    }
}  // namespace rdf4cpp::parser::xml_states