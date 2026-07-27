#ifndef XMLPARSERCOLLECTIONSTATE_HPP
#define XMLPARSERCOLLECTIONSTATE_HPP

#include <rdf4cpp/parser/XMLParserUtility.hpp>

namespace rdf4cpp::parser::xml_states {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#parseTypeCollectionPropertyElt
     *
     * example:
     * <rdf:Description>
     *  <ex:parseTypeCollectionPropertyElt rdf:parseType="Collection">
     *    <rdf:Description>...</rdf:Description>
     *    <rdf:Description>...</rdf:Description>
     *    <rdf:Description>...</rdf:Description>
     *  </ex:parseTypeCollectionPropertyElt>
     * </rdf:Description>
     */
    struct CollectionState final : BaseState {
        StateTransition on_characters(XMLOutputQueue &out, std::string_view chars, XMLStateInfo const &info) override;
        StateTransition on_start_element(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<XMLAttribute> attributes, XMLStateInfo const &info) override;
        StateTransition on_end_element(XMLOutputQueue &out, XMLStateInfo const &info) override;
        void move_to(BaseState *b) noexcept override;

        Node subject;
        IRI predicate;
        Node last_bn = Node::make_null();
        IRI reify;
        bool first = true;

        CollectionState(InheritedAttributeInfo const &i, Node sub, IRI pred, IRI reify)
            : BaseState(i), subject(sub), predicate(pred), reify(reify) {
        }
    };
}  // namespace rdf4cpp::parser::xml_states

#endif  // XMLPARSERCOLLECTIONSTATE_HPP