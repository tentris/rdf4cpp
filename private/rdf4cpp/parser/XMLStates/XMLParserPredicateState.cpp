#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser::xml_states {

    StateTransition PredicateState::on_characters([[maybe_unused]] XMLOutputQueue &out, std::string_view const chars, XMLStateInfo const &info) {
        if (done) {
            if (!trim_left(chars).empty()) {
                out.add_error(ParsingError::Type::BadSyntax, "expected end of element, found literal", info);
            }
            return {};
        }
        literal.append(chars);
        return {};
    }

    StateTransition PredicateState::on_start_element(XMLOutputQueue &out, std::string_view const local_name, std::string_view const uri, std::span<XMLAttribute> const attributes, XMLStateInfo const &info) {
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

    StateTransition PredicateState::on_end_element(XMLOutputQueue &out, XMLStateInfo const &info) {
        if (!done) {
            Literal const lit = out.make_literal(literal, std::nullopt, std::nullopt, info);
            out.add_statement(subject, predicate, lit, reify);
        }
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void PredicateState::move_to(BaseState *b) noexcept {
        new (b) PredicateState(std::move(*this));
    }

    bool PredicateState::iri_reserved_predicate(std::string_view const uri, std::string_view const local_name) {
        return iri_reserved(uri, local_name) || iri_equal_pieces(DescriptionState::start_element, uri, local_name) || iri_equal_pieces(list_start_element, uri, local_name);
    }

}  // namespace rdf4cpp::parser::xml_states