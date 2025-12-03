#ifndef RDF4CPP_XMLPARSERTYPEDLITERALPREDICATESTATE_H
#define RDF4CPP_XMLPARSERTYPEDLITERALPREDICATESTATE_H

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserPredicateState.hpp>

namespace rdf4cpp::parser {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#literalPropertyElt (with datatype attribute)
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::TypedLiteralPredicateState final : PredicateState {
        void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
        void on_end_element(ImplXML &i) override;
        void move_to(BaseState *b) noexcept override;

        IRI datatype;

        TypedLiteralPredicateState(InheritedAttributeInfo const &i, Node iri, IRI predicate, IRI reify, IRI datatype)
            : PredicateState(i, iri, predicate, reify), datatype(datatype) {
        }

        static constexpr std::string_view datatype_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#datatype";
    };
}

#endif  //RDF4CPP_XMLPARSERTYPEDLITERALPREDICATESTATE_H
