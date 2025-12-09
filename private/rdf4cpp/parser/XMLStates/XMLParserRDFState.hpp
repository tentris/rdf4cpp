#ifndef RDF4CPP_XMLPARSERRDFSTATE_H
#define RDF4CPP_XMLPARSERRDFSTATE_H

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>

namespace rdf4cpp::parser {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#RDF
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::RDFState final : BaseState {
        StateTransition on_characters(XMLOutputQueue &out, std::string_view chars, Info const &info) override;
        StateTransition on_start_element(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes, Info const &info) override;
        StateTransition on_end_element(XMLOutputQueue &out, Info const &info) override;
        void move_to(BaseState *b) noexcept override;

        static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#RDF";

        using BaseState::BaseState;
    };
}

#endif  //RDF4CPP_XMLPARSERRDFSTATE_H
