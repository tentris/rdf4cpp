#ifndef XMLPARSEREMPTYELEMENT_HPP
#define XMLPARSEREMPTYELEMENT_HPP

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>

namespace rdf4cpp::parser {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#emptyPropertyElt (if attributes are present)
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::EmptyElement final : BaseState {
        void on_characters(ImplXML &i, std::string_view chars) override;
        void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
        void on_end_element(ImplXML &i) override;
        void move_to(BaseState *b) noexcept override;

        EmptyElement()
            : BaseState({}) {
        }
    };
}  // namespace rdf4cpp::parser

#endif  // XMLPARSEREMPTYELEMENT_HPP