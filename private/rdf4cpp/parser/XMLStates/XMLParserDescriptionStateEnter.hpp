#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    template<class F>
    void IStreamQuadIterator::ImplXML::DescriptionState::enter(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> const attributes, F f) {
        auto const inherited_attribute_info = get_inherited_attributes(i, attributes);
        Node sub = Node::make_null();
        auto check_only_one = [&sub, &i]() {
            if (!sub.null()) {
                i.add_error(ParsingError::Type::BadSyntax, "expected only one of rdf:ID, rdf:about, and rdf:nodeID");
                return true;
            }
            return false;
        };
        for (auto const &att : attributes) {
            if (ImplXML::iri_equal_pieces(about_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = i.make_iri(att.value(), inherited_attribute_info.base);
            } else if (ImplXML::iri_equal_pieces(id_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = i.make_id(att.value(), inherited_attribute_info.base);
            } else if (ImplXML::iri_equal_pieces(node_id_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = i.make_bn(att.value());
            }
        }
        if (sub.null()) {
            sub = i.make_bn(std::nullopt);
        }
        if (!ImplXML::iri_equal_pieces(start_element, uri, local_name)) {
            IRI const obj = i.make_iri(uri, local_name, inherited_attribute_info.base);
            if (!obj.null()) {
                i.add_statement(sub, i.make_type_iri(), obj, IRI::make_null());
            }
        }
        for (auto const &att : attributes) {
            if (PredicateState::iri_reserved_predicate(att.uri(), att.local_name())) {
                continue;
            }
            if (ImplXML::iri_equal_pieces(type_attrib, att.uri(), att.local_name())) {
                IRI const obj = i.make_iri(att.value(), inherited_attribute_info.base);
                i.add_statement(sub, i.make_type_iri(), obj, IRI::make_null());
            } else {
                IRI const pred = i.make_iri(att.uri(), att.local_name(), inherited_attribute_info.base);
                Literal const obj = i.make_literal(att.value(), std::nullopt, inherited_attribute_info.lang_tag);
                i.add_statement(sub, pred, obj, IRI::make_null());
            }
        }
        f(sub);
        i.state_stack_.emplace_back(std::in_place_type_t<DescriptionState>{}, inherited_attribute_info, sub);
        i.update_current_state();
    }
}