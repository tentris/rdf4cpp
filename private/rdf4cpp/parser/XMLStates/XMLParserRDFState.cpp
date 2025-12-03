#include <rdf4cpp/parser/XMLParser.hpp>

#include <rdf4cpp/parser/XMLStates/XMLParserDescriptionStateEnter.hpp>

namespace rdf4cpp::parser {
    void IStreamQuadIterator::ImplXML::RDFState::on_characters(ImplXML &i, std::string_view const chars) {
        if (!ImplXML::trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected Description, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::RDFState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes) {
        DescriptionState::enter(i, local_name, uri, attributes, [](auto) {
        });
    }

    void IStreamQuadIterator::ImplXML::RDFState::on_end_element(ImplXML &i) {
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::RDFState::move_to(BaseState *b) noexcept {
        new (b) RDFState(std::move(*this));
    }
}  // namespace rdf4cpp::parser