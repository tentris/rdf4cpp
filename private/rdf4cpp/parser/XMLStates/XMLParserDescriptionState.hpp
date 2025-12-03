#ifndef RDF4CPP_XMLPARSERDESCRIPTIONSTATE_H
#define RDF4CPP_XMLPARSERDESCRIPTIONSTATE_H

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>

namespace rdf4cpp::parser {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#nodeElementList and https://www.w3.org/TR/rdf11-xml/#nodeElement
     * on_start_element checks and dispatches for the different options in https://www.w3.org/TR/rdf11-xml/#propertyEltList
     * (https://www.w3.org/TR/rdf11-xml/#parseTypeResourcePropertyElt has no own state, instead gets handled directly by on_start_element)
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::DescriptionState final : BaseState {
        void on_characters(ImplXML &i, std::string_view chars) override;
        void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
        void on_end_element(ImplXML &i) override;
        void move_to(BaseState *b) noexcept override;

        Node subject;
        size_t list_current = 1;

        explicit DescriptionState(InheritedAttributeInfo const &i, Node sub)
            : BaseState(i), subject(sub) {
        }

        /**
         * include XMLParserDescriptionStateEnter.hpp to use it
         */
        template<class F>
        static void enter(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes, F f);

        static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#Description";
        static constexpr std::string_view about_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#about";
        static constexpr std::string_view id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#ID";
        static constexpr std::string_view node_id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#nodeID";
        static constexpr std::string_view type_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
    };
}

#endif  //RDF4CPP_XMLPARSERDESCRIPTIONSTATE_H
