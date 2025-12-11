#ifndef XMLPARSEREMPTYELEMENT_HPP
#define XMLPARSEREMPTYELEMENT_HPP

#include <rdf4cpp/parser/XMLParserUtility.hpp>

namespace rdf4cpp::parser::xml_states {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#emptyPropertyElt (if attributes are present)
     */
    struct EmptyElement final : BaseState {
        StateTransition on_characters(XMLOutputQueue &out, std::string_view chars, XMLStateInfo const &info) override;
        StateTransition on_start_element(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<XMLAttribute> attributes, XMLStateInfo const &info) override;
        StateTransition on_end_element(XMLOutputQueue &out, XMLStateInfo const &info) override;
        void move_to(BaseState *b) noexcept override;

        EmptyElement()
            : BaseState({}) {
        }
    };
}  // namespace rdf4cpp::parser::xml_states

#endif  // XMLPARSEREMPTYELEMENT_HPP