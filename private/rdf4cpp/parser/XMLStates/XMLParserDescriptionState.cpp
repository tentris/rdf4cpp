#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    void IStreamQuadIterator::ImplXML::DescriptionState::on_characters(ImplXML &i, std::string_view const chars) {
        if (!ImplXML::trim_left(chars).empty()) {
            i.add_error(ParsingError::Type::BadSyntax, "expected predicate, found characters");
        }
    }

    void IStreamQuadIterator::ImplXML::DescriptionState::on_start_element(ImplXML &i, std::string_view const local_name, std::string_view const uri, std::span<Attribute> attributes) {
        auto const inherited_attribute_info = get_inherited_attributes(i, attributes);
        IRI predicate;
        if (ImplXML::iri_equal_pieces(PredicateState::list_start_element, uri, local_name)) {
            predicate = i.make_iri(std::format("http://www.w3.org/1999/02/22-rdf-syntax-ns#_{}", list_current++), inherited_attribute_info.base);
        } else {
            predicate = i.make_iri(uri, local_name, inherited_attribute_info.base);
        }
        std::optional<IRI> datatype = std::nullopt;
        std::optional<Node> sub = std::nullopt;
        IRI reify = IRI::make_null();
        bool parse_resource = false;
        bool parse_literal = false;
        bool parse_collection = false;
        for (auto const &att : attributes) {
            if (ImplXML::iri_equal_pieces(TypedLiteralPredicateState::datatype_attrib, att.uri(), att.local_name())) {
                datatype = i.make_iri(att.value(), inherited_attribute_info.base);
            } else if (ImplXML::iri_equal_pieces(PredicateState::resource_attrib, att.uri(), att.local_name())) {
                sub = i.make_iri(att.value(), inherited_attribute_info.base);
            } else if (ImplXML::iri_equal_pieces(node_id_attrib, att.uri(), att.local_name())) {
                sub = i.make_bn(att.value());
            } else if (ImplXML::iri_equal_pieces(id_attrib, att.uri(), att.local_name())) {
                reify = i.make_id(att.value(), inherited_attribute_info.base);
            } else if (ImplXML::iri_equal_pieces(PredicateState::parse_type_attrib, att.uri(), att.local_name())) {
                if (att.value() == PredicateState::parse_type_resource) {
                    parse_resource = true;
                } else if (att.value() == PredicateState::parse_type_collection) {
                    parse_collection = true;
                } else {
                    parse_literal = true;
                }
            }
        }
        for (auto const &att : attributes) {
            if (PredicateState::iri_reserved_predicate(att.uri(), att.local_name())) {
                continue;
            }
            if (!sub.has_value()) {
                sub = i.make_bn(std::nullopt);
            }
            if (ImplXML::iri_equal_pieces(type_attrib, att.uri(), att.local_name())) {
                IRI const obj = i.make_iri(att.value(), base);
                i.add_statement(*sub, i.make_type_iri(), obj, IRI::make_null());
            } else {
                IRI const pred = i.make_iri(att.uri(), att.local_name(), base);
                Literal const obj = i.make_literal(att.value(), std::nullopt, inherited_attribute_info.lang_tag);
                i.add_statement(*sub, pred, obj, IRI::make_null());
            }
        }
        if (sub.has_value() && (parse_collection || parse_literal || parse_resource)) {
            i.add_error(ParsingError::Type::BadSyntax, "expected only one of rdf:parseType, rdf:nodeID and rdf:resource");
        }
        if (datatype.has_value()) {
            i.state_stack_.emplace_back(std::in_place_type_t<TypedLiteralPredicateState>{}, inherited_attribute_info, subject, predicate, reify, *datatype);
        } else if (sub.has_value()) {
            i.add_statement(subject, predicate, *sub, reify);
            i.state_stack_.emplace_back(std::in_place_type_t<EmptyElement>{});
        } else if (parse_resource) {
            Node const obj = i.make_bn(std::nullopt);
            i.add_statement(subject, predicate, obj, reify);
            i.state_stack_.emplace_back(std::in_place_type_t<DescriptionState>{}, inherited_attribute_info, obj);
        } else if (parse_literal) {
            auto &xml_state = i.state_stack_.emplace_back(std::in_place_type_t<XMLLiteralState>{}, inherited_attribute_info, subject, predicate, reify);
            static_cast<XMLLiteralState &>(xml_state.get()).source_input(i);  // NOLINT(*-pro-type-static-cast-downcast)
        } else if (parse_collection) {
            i.state_stack_.emplace_back(std::in_place_type_t<CollectionState>{}, inherited_attribute_info, subject, predicate, reify);
        } else {
            i.state_stack_.emplace_back(std::in_place_type_t<PredicateState>{}, inherited_attribute_info, subject, predicate, reify);
        }
        i.update_current_state();
    }

    void IStreamQuadIterator::ImplXML::DescriptionState::on_end_element(ImplXML &i) {
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::DescriptionState::move_to(BaseState *b) noexcept {
        new (b) DescriptionState(std::move(*this));
    }
}