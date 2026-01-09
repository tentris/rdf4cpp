#ifndef RDF4CPP_XMLPARSERDESCRIPTIONSTATE_H
#define RDF4CPP_XMLPARSERDESCRIPTIONSTATE_H

#include <rdf4cpp/parser/XMLParserUtility.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>

namespace rdf4cpp::parser::xml_states {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#nodeElementList and https://www.w3.org/TR/rdf11-xml/#nodeElement
     * on_start_element checks and dispatches for the different options in https://www.w3.org/TR/rdf11-xml/#propertyEltList
     * (https://www.w3.org/TR/rdf11-xml/#parseTypeResourcePropertyElt has no own state, instead gets handled directly by on_start_element)
     *
     * example:
     * <rdf:Description>
     *  ...
     * </rdf:Description>
     */
    struct DescriptionState final : BaseState {
        StateTransition on_characters(XMLOutputQueue &out, std::string_view chars, XMLStateInfo const &info) override;
        StateTransition on_start_element(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<XMLAttribute> attributes, XMLStateInfo const &info) override;
        StateTransition on_end_element(XMLOutputQueue &out, XMLStateInfo const &info) override;
        void move_to(BaseState *b) noexcept override;

        Node subject;
        size_t list_current = 1;

        explicit DescriptionState(InheritedAttributeInfo const &i, Node sub)
            : BaseState(i), subject(sub) {
        }

        /**
         * enters a description state
         * @param out
         * @param local_name
         * @param uri
         * @param attributes
         * @param info
         * @return transition & the node this state represents, to be used as object in parent states
         */
        static std::pair<StateTransition, Node> enter(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<XMLAttribute> attributes, XMLStateInfo const &info);

        static constexpr std::string_view start_element = "http://www.w3.org/1999/02/22-rdf-syntax-ns#Description";
        static constexpr std::string_view about_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#about";
        static constexpr std::string_view id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#ID";
        static constexpr std::string_view node_id_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#nodeID";
        static constexpr std::string_view type_attrib = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
    };
}  // namespace rdf4cpp::parser::xml_states

#endif  //RDF4CPP_XMLPARSERDESCRIPTIONSTATE_H
