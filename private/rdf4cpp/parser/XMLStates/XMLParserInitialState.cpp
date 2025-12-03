#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    void IStreamQuadIterator::ImplXML::InitialState::on_characters(ImplXML &i, std::string_view const chars) {
        if (!ImplXML::trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected RDF, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::InitialState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, [[maybe_unused]] std::span<Attribute> attributes) {
        if (ImplXML::iri_equal_pieces(RDFState::start_element, uri, local_name)) {
            i.state_stack_.emplace_back(std::in_place_type_t<RDFState>{}, get_inherited_attributes(i, attributes));
            i.update_current_state();
            return;
        }
        i.add_error(ParsingError::Type::BadSyntax, "expected RDF, found ???");
    }

    void IStreamQuadIterator::ImplXML::InitialState::on_end_element(ImplXML &i) {
        i.add_error(ParsingError::Type::BadSyntax, "expected RDF, found end of ???");
    }
    void IStreamQuadIterator::ImplXML::InitialState::move_to(BaseState *b) noexcept {
        new (b) InitialState(std::move(*this));
    }
}