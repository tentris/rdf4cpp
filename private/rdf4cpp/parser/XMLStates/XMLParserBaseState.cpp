#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    IStreamQuadIterator::ImplXML::BaseState::InheritedAttributeInfo IStreamQuadIterator::ImplXML::BaseState::get_inherited_attributes(ImplXML &impl, std::span<Attribute> const attributes) {
        InheritedAttributeInfo r{};
        for (auto const &a : attributes) {
            if (ImplXML::iri_equal_pieces(base_attribute, a.uri(), a.local_name())) {
                if (auto e = IRIView(a.value()).quick_validate(); e != IRIFactoryError::Ok) {
                    impl.add_error(ParsingError::Type::BadIri, std::format("invalid base IRI ({}): {}", e, a.value()));
                }
                r.base = a.value();
            } else if (ImplXML::iri_equal_pieces(lang_attribute, a.uri(), a.local_name())) {
                r.lang_tag = a.value();
            }
        }
        return r;
    }
}