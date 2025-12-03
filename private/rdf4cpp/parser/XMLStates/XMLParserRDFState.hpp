#ifndef RDF4CPP_XMLPARSERRDFSTATE_H
#define RDF4CPP_XMLPARSERRDFSTATE_H

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>

namespace rdf4cpp::parser {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#RDF
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::RDFState final : BaseState {
        void on_characters(ImplXML &i, std::string_view chars) override;
        void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
        void on_end_element(ImplXML &i) override;
        void move_to(BaseState *b) noexcept override;

        static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#RDF";

        using BaseState::BaseState;
    };
}

#endif  //RDF4CPP_XMLPARSERRDFSTATE_H
