#include <rdf4cpp/parser/XMLParser.hpp>

#include <rdf4cpp/parser/XMLParserStateTransition.hpp>

namespace rdf4cpp::parser::xml_states {
    StateTransition DescriptionState::on_characters(XMLOutputQueue &out, std::string_view const chars, XMLStateInfo const &info) {
        if (!trim_left(chars).empty()) {
            out.add_error(ParsingError::Type::BadSyntax, "expected predicate, found characters", info);
        }
        return {};
    }

    StateTransition DescriptionState::on_start_element(XMLOutputQueue &out, std::string_view const local_name, std::string_view const uri, std::span<XMLAttribute> attributes, XMLStateInfo const &info) {
        auto const inherited_attribute_info = get_inherited_attributes(out, attributes, info);
        IRI predicate;
        if (iri_equal_pieces(PredicateState::list_start_element, uri, local_name)) {
            predicate = out.make_iri(std::format("http://www.w3.org/1999/02/22-rdf-syntax-ns#_{}", list_current++), inherited_attribute_info.base, info);
        } else {
            predicate = out.make_iri(uri, local_name, inherited_attribute_info.base, info);
        }
        std::optional<IRI> datatype = std::nullopt;
        std::optional<Node> sub = std::nullopt;
        IRI reify = IRI::make_null();
        bool parse_resource = false;
        bool parse_literal = false;
        bool parse_collection = false;
        for (auto const &att : attributes) {
            if (iri_equal_pieces(TypedLiteralPredicateState::datatype_attrib, att.uri(), att.local_name())) {
                datatype = out.make_iri(att.value(), inherited_attribute_info.base, info);
            } else if (iri_equal_pieces(PredicateState::resource_attrib, att.uri(), att.local_name())) {
                sub = out.make_iri(att.value(), inherited_attribute_info.base, info);
            } else if (iri_equal_pieces(node_id_attrib, att.uri(), att.local_name())) {
                sub = out.make_bn(att.value(), info);
            } else if (iri_equal_pieces(id_attrib, att.uri(), att.local_name())) {
                reify = out.make_id(att.value(), inherited_attribute_info.base, info);
            } else if (iri_equal_pieces(PredicateState::parse_type_attrib, att.uri(), att.local_name())) {
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
                sub = out.make_bn(std::nullopt, info);
            }
            if (iri_equal_pieces(type_attrib, att.uri(), att.local_name())) {
                IRI const obj = out.make_iri(att.value(), base, info);
                out.add_statement(*sub, out.make_type_iri(), obj, IRI::make_null());
            } else {
                IRI const pred = out.make_iri(att.uri(), att.local_name(), base, info);
                Literal const obj = out.make_literal(att.value(), std::nullopt, inherited_attribute_info.lang_tag, info);
                out.add_statement(*sub, pred, obj, IRI::make_null());
            }
        }
        if (sub.has_value() && (parse_collection || parse_literal || parse_resource)) {
            out.add_error(ParsingError::Type::BadSyntax, "expected only one of rdf:parseType, rdf:nodeID and rdf:resource", info);
        }
        if (datatype.has_value()) {
            return StateTransition(std::in_place_type_t<TypedLiteralPredicateState>{}, inherited_attribute_info, subject, predicate, reify, *datatype);
        } else if (sub.has_value()) {
            out.add_statement(subject, predicate, *sub, reify);
            return StateTransition(std::in_place_type_t<EmptyElement>{}); // predicate is expected to be empty, object defined as attribute
            // example: https://www.w3.org/2013/RDFXMLTests/rdfms-empty-property-elements/test013.rdf
        } else if (parse_resource) {
            Node const obj = out.make_bn(std::nullopt, info);
            out.add_statement(subject, predicate, obj, reify);
            return StateTransition(std::in_place_type_t<DescriptionState>{}, inherited_attribute_info, obj);
        } else if (parse_literal) {
            return StateTransition(std::in_place_type_t<XMLLiteralState>{}, inherited_attribute_info, subject, predicate, reify, info);
        } else if (parse_collection) {
            return StateTransition(std::in_place_type_t<CollectionState>{}, inherited_attribute_info, subject, predicate, reify);
        } else {
            return StateTransition(std::in_place_type_t<PredicateState>{}, inherited_attribute_info, subject, predicate, reify);
        }
    }

    StateTransition DescriptionState::on_end_element([[maybe_unused]] XMLOutputQueue &out, [[maybe_unused]] XMLStateInfo const &info) {
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void DescriptionState::move_to(BaseState *b) noexcept {
        new (b) DescriptionState(std::move(*this));
    }
    std::pair<StateTransition, Node> DescriptionState::enter(XMLOutputQueue &out, std::string_view local_name, std::string_view uri, std::span<XMLAttribute> attributes, XMLStateInfo const &info) {
        auto const inherited_attribute_info = get_inherited_attributes(out, attributes, info);
        Node sub = Node::make_null();
        auto check_only_one = [&sub, &out, &info]() {
            if (!sub.null()) {
                out.add_error(ParsingError::Type::BadSyntax, "expected only one of rdf:ID, rdf:about, and rdf:nodeID", info);
                return true;
            }
            return false;
        };
        for (auto const &att : attributes) {
            if (iri_equal_pieces(about_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = out.make_iri(att.value(), inherited_attribute_info.base, info);
            } else if (iri_equal_pieces(id_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = out.make_id(att.value(), inherited_attribute_info.base, info);
            } else if (iri_equal_pieces(node_id_attrib, att.uri(), att.local_name())) {
                if (check_only_one()) {
                    continue;
                }
                sub = out.make_bn(att.value(), info);
            }
        }
        if (sub.null()) {
            sub = out.make_bn(std::nullopt, info);
        }
        if (!iri_equal_pieces(start_element, uri, local_name)) {
            IRI const obj = out.make_iri(uri, local_name, inherited_attribute_info.base, info);
            if (!obj.null()) {
                out.add_statement(sub, out.make_type_iri(), obj, IRI::make_null());
            }
        }
        for (auto const &att : attributes) {
            if (PredicateState::iri_reserved_predicate(att.uri(), att.local_name())) {
                continue;
            }
            if (iri_equal_pieces(type_attrib, att.uri(), att.local_name())) {
                IRI const obj = out.make_iri(att.value(), inherited_attribute_info.base, info);
                out.add_statement(sub, out.make_type_iri(), obj, IRI::make_null());
            } else {
                IRI const pred = out.make_iri(att.uri(), att.local_name(), inherited_attribute_info.base, info);
                Literal const obj = out.make_literal(att.value(), std::nullopt, inherited_attribute_info.lang_tag, info);
                out.add_statement(sub, pred, obj, IRI::make_null());
            }
        }
        return {
                StateTransition{std::in_place_type_t<DescriptionState>{}, inherited_attribute_info, sub},
                sub,
        };
    }
}  // namespace rdf4cpp::parser::xml_states