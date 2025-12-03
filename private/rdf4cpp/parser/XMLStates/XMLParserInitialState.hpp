#ifndef RDF4CPP_XMLPARSERINITIALSTATE_H
#define RDF4CPP_XMLPARSERINITIALSTATE_H

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>

namespace rdf4cpp::parser {
    /**
     * initial state, checks for start of https://www.w3.org/TR/rdf11-xml/#RDF
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::InitialState final : BaseState {
        void on_characters(ImplXML &i, std::string_view chars) override;
        void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
        void on_end_element(ImplXML &i) override;
        void move_to(BaseState *b) noexcept override;

        InitialState()
            : BaseState({}) {
        }
    };
}

#endif  //RDF4CPP_XMLPARSERINITIALSTATE_H
