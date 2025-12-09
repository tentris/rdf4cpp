#ifndef XMLPARSEREMPTYELEMENT_HPP
#define XMLPARSEREMPTYELEMENT_HPP

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>

namespace rdf4cpp::parser {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#emptyPropertyElt (if attributes are present)
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::EmptyElement final : BaseState {
        StateTransition on_characters(XMLOutputQueue &out, std::string_view chars, Info const &info) override;
        StateTransition on_start_element(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes, Info const &info) override;
        StateTransition on_end_element(XMLOutputQueue &out, Info const &info) override;
        void move_to(BaseState *b) noexcept override;

        EmptyElement()
            : BaseState({}) {
        }
    };
}  // namespace rdf4cpp::parser

#endif  // XMLPARSEREMPTYELEMENT_HPP