#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::TypedLiteralPredicateState::on_start_element(XMLOutputQueue &out, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes, Info const &info) {
        out.add_error(ParsingError::Type::BadSyntax, "expected literal, found element", info);
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::TypedLiteralPredicateState::on_end_element(XMLOutputQueue &out, Info const &info) {
        if (!datatype.null()) {
            Literal const lit = out.make_literal(literal, datatype, std::nullopt, info);
            out.add_statement(subject, predicate, lit, reify);
        }
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void IStreamQuadIterator::ImplXMLStateCollector::TypedLiteralPredicateState::move_to(BaseState *b) noexcept {
        new (b) TypedLiteralPredicateState(std::move(*this));
    }
}  // namespace rdf4cpp::parser