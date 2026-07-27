#include <rdf4cpp/parser/XMLParser.hpp>

#include <rdf4cpp/parser/XMLParserStateTransition.hpp>

namespace rdf4cpp::parser::xml_states {
    StateTransition CollectionState::on_characters(XMLOutputQueue &out, std::string_view const chars, XMLStateInfo const &info) {
        if (!trim_left(chars).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected element, found characters", info);
        }
        return {};
    }

    StateTransition CollectionState::on_start_element(XMLOutputQueue &out, std::string_view const local_name, std::string_view const uri, std::span<XMLAttribute> const attributes, XMLStateInfo const &info) {
        // https://www.w3.org/TR/rdf11-xml/#parseTypeCollectionPropertyElt
        // only node elements (=>DescriptionState) can appear in the list
        auto [transition, obj] = DescriptionState::enter(out, local_name, uri, attributes, info);
        if (first) {
            first = false;
            last_bn = out.make_bn(std::nullopt, info);
            out.add_statement(subject, predicate, last_bn, reify);
        } else {
            auto const bn = out.make_bn(std::nullopt, info);
            out.add_statement(last_bn, out.make_hardcoded_iri(list_iris::iri_rest), bn, IRI::make_null());
            last_bn = bn;
        }
        out.add_statement(last_bn, out.make_hardcoded_iri(list_iris::iri_first), obj, IRI::make_null());
        return transition;
    }

    StateTransition CollectionState::on_end_element(XMLOutputQueue &out, [[maybe_unused]] XMLStateInfo const &info) {
        auto const nil = out.make_hardcoded_iri(list_iris::iri_nil);
        if (first) {
            out.add_statement(subject, predicate, nil, reify);
        } else {
            out.add_statement(last_bn, out.make_hardcoded_iri(list_iris::iri_rest), nil, IRI::make_null());
        }
        return StateTransition{std::in_place_type<PopState>};
    }
    void CollectionState::move_to(BaseState *b) noexcept {
        new (b) CollectionState(std::move(*this));
    }
}  // namespace rdf4cpp::parser::xml_states