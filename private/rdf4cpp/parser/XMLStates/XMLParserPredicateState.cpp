#include <rdf4cpp/parser/XMLParser.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserDescriptionStateEnter.hpp>

namespace rdf4cpp::parser {

    void IStreamQuadIterator::ImplXML::PredicateState::on_characters([[maybe_unused]] ImplXML &i, std::string_view const chars) {
        if (done) {
            if (!ImplXML::trim_left(chars).empty()) {
                i.add_error(ParsingError::Type::BadSyntax, "expected end of element, found literal");
            }
            return;
        }
        literal.append(chars);
    }

    void IStreamQuadIterator::ImplXML::PredicateState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes) {
        if (!ImplXML::trim_left(literal).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected end of element or literal, found element");
            return;
        }
        if (done) {
            i.add_error(ParsingError::Type::BadSyntax, "expected end of element, found element");
            return;
        }
        DescriptionState::enter(i, local_name, uri, attributes, [&](Node obj) {
            done = true;
            i.add_statement(subject, predicate, obj, reify);
        });
    }

    void IStreamQuadIterator::ImplXML::PredicateState::on_end_element(ImplXML &i) {
        if (!done) {
            Literal const lit = i.make_literal(literal, std::nullopt, std::nullopt);
            i.add_statement(subject, predicate, lit, reify);
        }
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::PredicateState::move_to(BaseState *b) noexcept {
        new (b) PredicateState(std::move(*this));
    }

    bool IStreamQuadIterator::ImplXML::PredicateState::iri_reserved_predicate(std::string_view const uri, std::string_view const local_name) {
        return ImplXML::iri_reserved(uri, local_name) || ImplXML::iri_equal_pieces(DescriptionState::start_element, uri, local_name) || ImplXML::iri_equal_pieces(list_start_element, uri, local_name);
    }

}