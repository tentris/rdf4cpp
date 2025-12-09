#ifndef RDF4CPP_XMLPARSERXMLLITERALESTATE_H
#define RDF4CPP_XMLPARSERXMLLITERALESTATE_H

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserPredicateState.hpp>

namespace rdf4cpp::parser {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#parseTypeLiteralPropertyElt
     * (and https://www.w3.org/TR/rdf11-xml/#parseTypeOtherPropertyElt)
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::XMLLiteralState final : PredicateState {
        StateTransition on_characters(XMLOutputQueue &out, std::string_view chars, Info const &info) override;
        StateTransition on_start_element(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes, Info const &info) override;
        StateTransition on_end_element(XMLOutputQueue &out, Info const &info) override;
        void move_to(BaseState *b) noexcept override;

        size_t depth = 0;
        size_t data_start = 0;
        size_t last_offset = 0;
        size_t last_size = 0;

        void source_input(Info const &info);

        XMLLiteralState(InheritedAttributeInfo const &i, Node sub, IRI predicate, IRI reify, Info const &info) : PredicateState(i, sub, predicate, reify) {
            source_input(info);
        }
    };
}

#endif  //RDF4CPP_XMLPARSERXMLLITERALESTATE_H
