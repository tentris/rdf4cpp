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
        void on_characters(ImplXML &i, std::string_view chars) override;
        void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
        void on_end_element(ImplXML &i) override;
        void move_to(BaseState *b) noexcept override;

        size_t depth = 0;
        size_t data_start = 0;
        size_t last_offset = 0;
        size_t last_size = 0;

        using PredicateState::PredicateState;

        void source_input(ImplXML &i);
    };
}

#endif  //RDF4CPP_XMLPARSERXMLLITERALESTATE_H
