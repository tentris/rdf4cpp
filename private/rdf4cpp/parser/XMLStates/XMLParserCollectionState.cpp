#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::CollectionState::on_characters(XMLOutputQueue &out, std::string_view const chars, Info const &info) {
        if (!trim_left(chars).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected element, found characters", info);
        }
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::CollectionState::on_start_element(XMLOutputQueue &out, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes, Info const &info) {
        auto [transition, obj] = DescriptionState::enter(out, local_name, uri, attributes, info);
        if (first) {
            first = false;
            last_bn = out.make_bn(std::nullopt, info);
            out.add_statement(subject, predicate, last_bn, reify);
        } else {
            auto const bn = out.make_bn(std::nullopt, info);
            out.add_statement(last_bn, out.make_hardcoded_iri(iri_rest), bn, IRI::make_null());
            last_bn = bn;
        }
        out.add_statement(last_bn, out.make_hardcoded_iri(iri_first), obj, IRI::make_null());
        return transition;
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::CollectionState::on_end_element(XMLOutputQueue &out, [[maybe_unused]] Info const &info) {
        auto const nil = out.make_hardcoded_iri(iri_nil);
        if (first) {
            out.add_statement(subject, predicate, nil, reify);
        } else {
            out.add_statement(last_bn, out.make_hardcoded_iri(iri_rest), nil, IRI::make_null());
        }
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void IStreamQuadIterator::ImplXMLStateCollector::CollectionState::move_to(BaseState *b) noexcept {
        new (b) CollectionState(std::move(*this));
    }
}  // namespace rdf4cpp::parser