#include <rdf4cpp/parser/XMLParser.hpp>

#include <rdf4cpp/parser/XMLParserStateTransition.hpp>

namespace rdf4cpp::parser::xml_states {
    StateTransition TypedLiteralPredicateState::on_start_element(XMLOutputQueue &out, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<XMLAttribute> attributes, XMLStateInfo const &info) {
        out.add_error(ParsingError::Type::BadSyntax, "expected literal, found element", info);
        return {};
    }

    StateTransition TypedLiteralPredicateState::on_end_element(XMLOutputQueue &out, XMLStateInfo const &info) {
        if (!datatype.null()) {
            Literal const lit = out.make_literal(literal, datatype, std::nullopt, info);
            out.add_statement(subject, predicate, lit, reify);
        }
        return StateTransition{std::in_place_type<PopState>};
    }
    void TypedLiteralPredicateState::move_to(BaseState *b) noexcept {
        new (b) TypedLiteralPredicateState(std::move(*this));
    }
}  // namespace rdf4cpp::parser::xml_states