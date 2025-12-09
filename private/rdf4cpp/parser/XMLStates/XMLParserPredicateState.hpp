#ifndef RDF4CPP_XMLPARSERPREDICATESTATE_H
#define RDF4CPP_XMLPARSERPREDICATESTATE_H

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>

namespace rdf4cpp::parser {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#resourcePropertyElt (nested nodeElement / DescriptionState
     * and https://www.w3.org/TR/rdf11-xml/#literalPropertyElt (literal with no datatype attribute)
     * and https://www.w3.org/TR/rdf11-xml/#emptyPropertyElt (with no attributes (empty literal))
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::PredicateState : BaseState {
        StateTransition on_characters(XMLOutputQueue &out, std::string_view chars, Info const &info) override;
        StateTransition on_start_element(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes, Info const &info) override;
        StateTransition on_end_element(XMLOutputQueue &out, Info const &info) override;
        void move_to(BaseState *b) noexcept override;

        Node subject;
        IRI predicate;
        IRI reify;
        std::string literal;
        bool done = false;

        PredicateState(InheritedAttributeInfo const &i, Node sub, IRI predicate, IRI reify)
            : BaseState(i), subject(sub), predicate(predicate), reify(reify) {
        }

        static constexpr std::string_view resource_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#resource";
        static constexpr std::string_view parse_type_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#parseType";
        static constexpr std::string_view parse_type_resource = "Resource";
        static constexpr std::string_view parse_type_literal = "Literal";
        static constexpr std::string_view parse_type_collection = "Collection";
        static constexpr std::string_view list_start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#li";

        static bool iri_reserved_predicate(std::string_view uri, std::string_view local_name);
    };
}

#endif  //RDF4CPP_XMLPARSERPREDICATESTATE_H
