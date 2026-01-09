#include <rdf4cpp/parser/XMLParser.hpp>

#include <rdf4cpp/parser/XMLParserStateTransition.hpp>

namespace rdf4cpp::parser::xml_states {
    StateTransition RDFState::on_characters(XMLOutputQueue &out, std::string_view const chars, XMLStateInfo const &info) {
        if (!trim_left(chars).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected Description, found characters", info);
        }
        return {};
    }

    StateTransition RDFState::on_start_element(XMLOutputQueue &out, std::string_view const local_name, std::string_view const uri, std::span<XMLAttribute> const attributes, XMLStateInfo const &info) {
        auto [trans, _] = DescriptionState::enter(out, local_name, uri, attributes, info);
        return trans;
    }

    StateTransition RDFState::on_end_element([[maybe_unused]] XMLOutputQueue &out, [[maybe_unused]] XMLStateInfo const &info) {
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void RDFState::move_to(BaseState *b) noexcept {
        new (b) RDFState(std::move(*this));
    }
}  // namespace rdf4cpp::parser::xml_states