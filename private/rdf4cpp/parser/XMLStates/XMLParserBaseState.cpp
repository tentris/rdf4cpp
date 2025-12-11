#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser::xml_states {
    BaseState::InheritedAttributeInfo BaseState::get_inherited_attributes(XMLOutputQueue &out, std::span<XMLAttribute> const attributes, XMLStateInfo const &info) {
        InheritedAttributeInfo r{};
        for (auto const &a : attributes) {
            if (iri_equal_pieces(base_attribute, a.uri(), a.local_name())) {
                if (auto e = IRIView(a.value()).quick_validate(); e != IRIFactoryError::Ok) {
                    out.add_error(ParsingError::Type::BadIri, std::format("invalid base IRI ({}): {}", e, a.value()), info);
                }
                r.base = a.value();
            } else if (iri_equal_pieces(lang_attribute, a.uri(), a.local_name())) {
                r.lang_tag = a.value();
            }
        }
        return r;
    }
}  // namespace rdf4cpp::parser::xml_states