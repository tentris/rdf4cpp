#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::RDFState::on_characters(XMLOutputQueue &out, std::string_view const chars, Info const &info) {
        if (!trim_left(chars).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected Description, found characters", info);
        }
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::RDFState::on_start_element(XMLOutputQueue &out, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes, Info const &info) {
        auto [trans, _] = DescriptionState::enter(out, local_name, uri, attributes, info);
        return trans;
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::RDFState::on_end_element([[maybe_unused]] XMLOutputQueue &out, [[maybe_unused]] Info const &info) {
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void IStreamQuadIterator::ImplXMLStateCollector::RDFState::move_to(BaseState *b) noexcept {
        new (b) RDFState(std::move(*this));
    }
}  // namespace rdf4cpp::parser