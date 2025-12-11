#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser::xml_states {
    StateTransition InitialState::on_characters(XMLOutputQueue &out, std::string_view const chars, XMLStateInfo const &info) {
        if (!trim_left(chars).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected RDF, found characters", info);
        }
        return {};
    }

    StateTransition InitialState::on_start_element(XMLOutputQueue &out, std::string_view const local_name, std::string_view const uri, [[maybe_unused]] std::span<XMLAttribute> attributes, XMLStateInfo const &info) {
        if (iri_equal_pieces(RDFState::start_element, uri, local_name)) {
            return StateTransition{
                    std::in_place_type_t<RDFState>{},
                    get_inherited_attributes(out, attributes, info),
            };
        }
        out.add_error(ParsingError::Type::BadSyntax, "expected RDF, found ???", info);
        return {};
    }

    StateTransition InitialState::on_end_element(XMLOutputQueue &out, XMLStateInfo const &info) {
        out.add_error(ParsingError::Type::BadSyntax, "expected RDF, found end of ???", info);
        return {};
    }
    void InitialState::move_to(BaseState *b) noexcept {
        new (b) InitialState(std::move(*this));
    }
}  // namespace rdf4cpp::parser::xml_states