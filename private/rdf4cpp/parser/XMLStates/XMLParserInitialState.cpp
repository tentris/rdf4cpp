#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::InitialState::on_characters(XMLOutputQueue &out, std::string_view const chars, Info const &info) {
        if (!trim_left(chars).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected RDF, found characters", info);
        }
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::InitialState::on_start_element(XMLOutputQueue &out, std::string_view const local_name, std::string_view const uri, [[maybe_unused]] std::span<Attribute> attributes, Info const &info) {
        if (iri_equal_pieces(RDFState::start_element, uri, local_name)) {
            return StateTransition{
                    std::in_place_type_t<RDFState>{},
                    get_inherited_attributes(out, attributes, info),
            };
        }
        out.add_error(ParsingError::Type::BadSyntax, "expected RDF, found ???", info);
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::InitialState::on_end_element(XMLOutputQueue &out, Info const &info) {
        out.add_error(ParsingError::Type::BadSyntax, "expected RDF, found end of ???", info);
        return {};
    }
    void IStreamQuadIterator::ImplXMLStateCollector::InitialState::move_to(BaseState *b) noexcept {
        new (b) InitialState(std::move(*this));
    }
}  // namespace rdf4cpp::parser