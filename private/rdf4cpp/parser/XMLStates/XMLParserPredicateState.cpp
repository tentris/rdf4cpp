#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::PredicateState::on_characters([[maybe_unused]] XMLOutputQueue &out, std::string_view const chars, Info const &info) {
        if (done) {
            if (!trim_left(chars).empty()) {
                out.add_error(ParsingError::Type::BadSyntax, "expected end of element, found literal", info);
            }
            return {};
        }
        literal.append(chars);
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::PredicateState::on_start_element(XMLOutputQueue &out, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes, Info const &info) {
        if (!trim_left(literal).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected end of element or literal, found element", info);
            return {};
        }
        if (done) {
            out.add_error(ParsingError::Type::BadSyntax, "expected end of element, found element", info);
            return {};
        }
        auto [transition, obj] = DescriptionState::enter(out, local_name, uri, attributes, info);
        done = true;
        out.add_statement(subject, predicate, obj, reify);
        return transition;
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::PredicateState::on_end_element(XMLOutputQueue &out, Info const &info) {
        if (!done) {
            Literal const lit = out.make_literal(literal, std::nullopt, std::nullopt, info);
            out.add_statement(subject, predicate, lit, reify);
        }
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void IStreamQuadIterator::ImplXMLStateCollector::PredicateState::move_to(BaseState *b) noexcept {
        new (b) PredicateState(std::move(*this));
    }

    bool IStreamQuadIterator::ImplXMLStateCollector::PredicateState::iri_reserved_predicate(std::string_view const uri, std::string_view const local_name) {
        return iri_reserved(uri, local_name) || iri_equal_pieces(DescriptionState::start_element, uri, local_name) || iri_equal_pieces(list_start_element, uri, local_name);
    }

}  // namespace rdf4cpp::parser