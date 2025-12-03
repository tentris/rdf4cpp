#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    void IStreamQuadIterator::ImplXML::TypedLiteralPredicateState::on_start_element(ImplXML &i, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        i.add_error(ParsingError::Type::BadSyntax, "expected literal, found element");
    }

    void IStreamQuadIterator::ImplXML::TypedLiteralPredicateState::on_end_element(ImplXML &i) {
        if (!datatype.null()) {
            Literal const lit = i.make_literal(literal, datatype, std::nullopt);
            i.add_statement(subject, predicate, lit, reify);
        }
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::TypedLiteralPredicateState::move_to(BaseState *b) noexcept {
        new (b) TypedLiteralPredicateState(std::move(*this));
    }
}