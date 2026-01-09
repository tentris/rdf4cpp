#ifndef RDF4CPP_XMLPARSERSTATETRANSITION_H
#define RDF4CPP_XMLPARSERSTATETRANSITION_H

#include <rdf4cpp/parser/XMLParserUtility.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserRDFState.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserDescriptionState.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserPredicateState.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserTypedLiteralPredicateState.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserXMLLiteralState.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserCollectionState.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserEmptyElement.hpp>

namespace rdf4cpp::parser {
    struct StateTransition {
        using ModifyStateStack = std::variant<NoStateChange, PopState, xml_states::RDFState, xml_states::DescriptionState,
                                              xml_states::PredicateState, xml_states::TypedLiteralPredicateState, xml_states::XMLLiteralState,
                                              xml_states::CollectionState, xml_states::EmptyElement>;

        ModifyStateStack modify_state;

        template<typename... T>
        explicit StateTransition(T &&...a) : modify_state(std::forward<T>(a)...) {
        }

        StateTransition() : StateTransition(std::in_place_type_t<NoStateChange>{}) {
        }
    };
}

#endif  //RDF4CPP_XMLPARSERSTATETRANSITION_H
