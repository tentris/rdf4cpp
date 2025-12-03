#include <rdf4cpp/parser/XMLParser.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserDescriptionStateEnter.hpp>

namespace rdf4cpp::parser {
    void IStreamQuadIterator::ImplXML::CollectionState::on_characters(ImplXML &i, std::string_view const chars) {
        if (!ImplXML::trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected element, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::CollectionState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes) {
        DescriptionState::enter(i, local_name, uri, attributes, [&](Node const obj) {
            if (first) {
                first = false;
                last_bn = i.make_bn(std::nullopt);
                i.add_statement(subject, predicate, last_bn, reify);
            } else {
                auto const bn = i.make_bn(std::nullopt);
                i.add_statement(last_bn, i.make_hardcoded_iri(iri_rest), bn, IRI::make_null());
                last_bn = bn;
            }
            i.add_statement(last_bn, i.make_hardcoded_iri(iri_first), obj, IRI::make_null());
        });
    }

    void IStreamQuadIterator::ImplXML::CollectionState::on_end_element(ImplXML &i) {
        auto const nil = i.make_hardcoded_iri(iri_nil);
        if (first) {
            i.add_statement(subject, predicate, nil, reify);
        } else {
            i.add_statement(last_bn, i.make_hardcoded_iri(iri_rest), nil, IRI::make_null());
        }
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::CollectionState::move_to(BaseState *b) noexcept {
        new (b) CollectionState(std::move(*this));
    }
}