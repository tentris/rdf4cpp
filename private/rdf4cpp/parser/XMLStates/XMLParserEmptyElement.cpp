#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    void IStreamQuadIterator::ImplXML::EmptyElement::on_characters(ImplXML &i, std::string_view const chars) {
        if (!ImplXML::trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected end of element, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::EmptyElement::on_start_element(ImplXML &i, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        i.add_error(ParsingError::Type::BadSyntax, "expected end of element, found ???");
    }

    void IStreamQuadIterator::ImplXML::EmptyElement::on_end_element(ImplXML &i) {
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::EmptyElement::move_to(BaseState *b) noexcept {
        new (b) EmptyElement(std::move(*this));
    }
}