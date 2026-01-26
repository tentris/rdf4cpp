#ifndef RDF4CPP_XMLPARSERTYPEDLITERALPREDICATESTATE_H
#define RDF4CPP_XMLPARSERTYPEDLITERALPREDICATESTATE_H

#include <rdf4cpp/parser/XMLParserUtility.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserPredicateState.hpp>

namespace rdf4cpp::parser::xml_states {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#literalPropertyElt (with datatype attribute)
     *
     * example:
     * <rdf:Description>
     *  <ex:literalPropertyElt rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">10</ex:literalPropertyElt>
     * </rdf:Description>
     */
    struct TypedLiteralPredicateState final : PredicateState {
        StateTransition on_start_element(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<XMLAttribute> attributes, XMLStateInfo const &info) override;
        StateTransition on_end_element(XMLOutputQueue &out, XMLStateInfo const &info) override;
        void move_to(BaseState *b) noexcept override;

        IRI datatype;

        TypedLiteralPredicateState(InheritedAttributeInfo const &i, Node iri, IRI predicate, IRI reify, IRI datatype)
            : PredicateState(i, iri, predicate, reify), datatype(datatype) {
        }

        static constexpr std::string_view datatype_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#datatype";
    };
}  // namespace rdf4cpp::parser::xml_states

#endif  //RDF4CPP_XMLPARSERTYPEDLITERALPREDICATESTATE_H
