#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::EmptyElement::on_characters(XMLOutputQueue &out, std::string_view const chars, Info const &info) {
        if (!trim_left(chars).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected end of element, found characters", info);
        }
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::EmptyElement::on_start_element(XMLOutputQueue &out, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes, Info const &info) {
        out.add_error(ParsingError::Type::BadSyntax, "expected end of element, found ???", info);
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::EmptyElement::on_end_element([[maybe_unused]] XMLOutputQueue &out, [[maybe_unused]] Info const &info) {
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void IStreamQuadIterator::ImplXMLStateCollector::EmptyElement::move_to(BaseState *b) noexcept {
        new (b) EmptyElement(std::move(*this));
    }
}  // namespace rdf4cpp::parser