#ifndef RDF4CPP_XMLPARSERRDFSTATE_H
#define RDF4CPP_XMLPARSERRDFSTATE_H

#include <rdf4cpp/parser/XMLParserUtility.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>

namespace rdf4cpp::parser::xml_states {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#RDF
     */
    struct RDFState final : BaseState {
        StateTransition on_characters(XMLOutputQueue &out, std::string_view chars, XMLStateInfo const &info) override;
        StateTransition on_start_element(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<XMLAttribute> attributes, XMLStateInfo const &info) override;
        StateTransition on_end_element(XMLOutputQueue &out, XMLStateInfo const &info) override;
        void move_to(BaseState *b) noexcept override;

        static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#RDF";

        using BaseState::BaseState;
    };
}  // namespace rdf4cpp::parser::xml_states

#endif  //RDF4CPP_XMLPARSERRDFSTATE_H
