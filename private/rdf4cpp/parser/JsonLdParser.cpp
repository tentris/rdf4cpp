#include "JsonLdParser.hpp"
#include <uni_algo/case.h>

namespace rdf4cpp::parser {
    json_ld::IRIMapping IStreamQuadIterator::ImplJsonLd::make_new_bn() {
        return {
            std::format("bn_{}", blank_node_index_++),
            json_ld::IRIMappingType::BlankNode,
        };
    }
    nonstd::expected<IRI, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_iri(std::string_view iri) {
        auto result = state_->iri_factory.create_and_validate(iri, state_->node_storage);
        if (result.has_value()) {
            return *result;
        } else {
            return nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadIri, std::format("{}", result.error())));
        }
    }
    nonstd::expected<IRI, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_iri(json_ld::IRIMapping const &iri) {
        if (iri.type != json_ld::IRIMappingType::IRI) {
            return nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "invalid type"));
        }
        return make_iri(iri.data);
    }
    nonstd::expected<Node, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_bn_or_iri(json_ld::IRIMapping const &mapping) {
        if (mapping.type == json_ld::IRIMappingType::BlankNode) {
            if (state_->blank_node_scope_manager == nullptr) {
                return BlankNode::make(mapping.data, state_->node_storage);
            } else {
                return state_->blank_node_scope_manager.scope("").get_or_generate_node(mapping.data, state_->node_storage);
            }
        }
        return make_iri(mapping);
    }
    nonstd::expected<json_ld::DirectionLiteralResult, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_literal(json_ld::StringLikeLiteralMapping const &lit, json_ld::IRIMapping const &graph) {
        try {
            if (lit.direction != json_ld::BaseDirection::None) {
                std::string_view dir = lit.direction == json_ld::BaseDirection::Ltr ? "ltr" : "rtl";
                if (direction_ == ParsingFlag::JsonLdDirectionI18n) {
                    auto lang = lit.language.has_value() ? static_cast<std::string_view>(*lit.language) : "";
                    auto dt = std::format("https://www.w3.org/ns/i18n#{}_{}", una::cases::to_lowercase_utf8(lang), dir);
                    auto iri = make_iri(dt);
                    if (!iri.has_value()) {
                        return nonstd::make_unexpected(iri.error());
                    }
                    return json_ld::DirectionLiteralResult{Literal::make_typed(lit.value, *iri, state_->node_storage)};
                }
                if (direction_ == ParsingFlag::JsonLdDirectionCompound) {
                    auto bn = make_new_bn();
                    auto compound = make_bn_or_iri(bn);
                    if (!compound.has_value()) {
                        return nonstd::make_unexpected(compound.error());
                    }
                    json_ld::DirectionLiteralResult r{*compound, std::array<Quad, 3>{}};
                    auto tmp = make_quad(graph, bn, json_ld::IRIMapping{std::string{"http://www.w3.org/1999/02/22-rdf-syntax-ns#value"}, json_ld::IRIMappingType::IRI}, Literal::make_simple(lit.value, state_->node_storage));
                    if (!tmp.has_value()) {
                        return nonstd::make_unexpected(tmp.error());
                    }
                    r.extra_quads->at(0) = *tmp;
                    tmp = make_quad(graph, bn, json_ld::IRIMapping{std::string{"http://www.w3.org/1999/02/22-rdf-syntax-ns#direction"}, json_ld::IRIMappingType::IRI}, Literal::make_simple(dir, state_->node_storage));
                    if (!tmp.has_value()) {
                        return nonstd::make_unexpected(tmp.error());
                    }
                    r.extra_quads->at(1) = *tmp;
                    if (lit.language.has_value()) {
                        tmp = make_quad(graph, bn, json_ld::IRIMapping{std::string{"http://www.w3.org/1999/02/22-rdf-syntax-ns#language"}, json_ld::IRIMappingType::IRI}, Literal::make_simple(una::cases::to_lowercase_utf8(*lit.language), state_->node_storage));
                        if (!tmp.has_value()) {
                            return nonstd::make_unexpected(tmp.error());
                        }
                        r.extra_quads->at(2) = *tmp;
                    }
                    else {
                        r.extra_quads->at(2) = Quad{Node::make_null(), Node::make_null(), Node::make_null(), Node::make_null()};
                    }
                    return r;
                }
            }
            if (lit.language.has_value()) {
                return json_ld::DirectionLiteralResult{Literal::make_lang_tagged(lit.value, *lit.language, state_->node_storage)};
            }
            return json_ld::DirectionLiteralResult{Literal::make_simple(lit.value, state_->node_storage)};
        } catch (InvalidNode const &e) {
            return nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadLiteral, e.what()));
        }
    }
    nonstd::expected<Literal, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_literal(json_ld::TypedLiteralMapping const &lit) {
        try {
            auto dt = make_iri(lit.type);
            if (!dt.has_value()) {
                return nonstd::unexpected(dt.error());
            }
            return Literal::make_typed(lit.value, *dt, state_->node_storage);
        } catch (InvalidNode const &e) {
            return nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadLiteral, e.what()));
        }
    }
    nonstd::expected<IStreamQuadIterator::ok_type, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, json_ld::IRIMapping const &object) {
        Node obj = Node::make_null();
        auto r = make_bn_or_iri(object);
        if (r.has_value()) {
            obj = *r;
        } else {
            return nonstd::make_unexpected(r.error());
        }
        return make_quad(graph, subject, predicate, obj);
    }
    nonstd::expected<IStreamQuadIterator::ok_type, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, Node object) {
        Node gra = IRI::default_graph(state_->node_storage);
        Node sub = Node::make_null();
        IRI pred = IRI::make_null();

        if (graph.type != json_ld::IRIMappingType::None) {
            if (graph.type == json_ld::IRIMappingType::Keyword) {
                if (graph.data != keyword_default) {
                    return nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "found keyword as graph"));
                }
            } else {
                auto r = make_bn_or_iri(graph);
                if (r.has_value()) {
                    gra = *r;
                } else {
                    return nonstd::make_unexpected(r.error());
                }
            }
        }
        {
            auto r = make_bn_or_iri(subject);
            if (r.has_value()) {
                sub = *r;
            } else {
                return nonstd::make_unexpected(r.error());
            }
        }
        {
            if (predicate.type == json_ld::IRIMappingType::Keyword && predicate.data == keyword_type) {
                pred = IRI::rdf_type(state_->node_storage);
            } else {
                auto r = make_iri(predicate);
                if (r.has_value()) {
                    pred = *r;
                } else {
                    return nonstd::make_unexpected(r.error());
                }
            }
        }

        return Quad{gra, sub, pred, object};
    }
    IStreamQuadIterator::ImplJsonLd::result_generator IStreamQuadIterator::ImplJsonLd::parse(params::ParseParams p) {
        if (p.obj_out != nullptr) {
            *p.obj_out = std::monostate{};
        }

        // follows https://www.w3.org/TR/json-ld11-api/#node-map-generation and https://www.w3.org/TR/json-ld11-api/#deserialize-json-ld-to-rdf-algorithm very roughly
        // this code and the source are different enough to make mapping out the single steps hard/impossible
        if (p.element.type() == simdjson::ondemand::json_type::array && !p.is_json_literal) {
            for (auto element : static_cast<simdjson::ondemand::array>(p.element)) {
                co_yield std::ranges::elements_of(parse({
                    .element = *element,
                    .active_ctx = p.active_ctx,
                    .base_iri = p.base_iri,
                    .active_graph = p.active_graph,
                    .active_subject = p.active_subject,
                    .active_property = p.active_property,
                    .obj_out = p.obj_out,
                    .is_top_level = p.is_top_level,
                    .is_reverse = p.is_reverse,
                    .is_json_literal = p.is_json_literal,
                    .is_included = p.is_included,
                }));
            }
            co_return;
        }
        auto expanded = expand_parser_.expand_level({
            .active_context = p.active_ctx,
            .active_property = p.active_property,
            .element = p.element,
            .base_iri = p.base_iri,
            .assume_no_scalar = p.is_top_level,
            .is_json_literal = p.is_json_literal,
        });
        if (!expanded.has_value()) {
            co_yield nonstd::unexpected(expanded.error());
            co_return;
        }
        co_yield std::ranges::elements_of(parse(p, *expanded));
    }
    IStreamQuadIterator::ImplJsonLd::result_generator IStreamQuadIterator::ImplJsonLd::parse(params::ParseParams p, json_ld::ExpandedLevel &expanded) {
        if (std::holds_alternative<json_ld::Null>(expanded)) {
            if (p.is_included) {
                co_yield nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "invalid @included value"));
                co_return;
            }
            co_return;
        }

        if (std::holds_alternative<simdjson::ondemand::array>(expanded)) {
            for (auto element : std::get<simdjson::ondemand::array>(expanded)) {
                co_yield std::ranges::elements_of(parse({
                    .element = *element,
                    .active_ctx = p.active_ctx,
                    .base_iri = p.base_iri,
                    .active_graph = p.active_graph,
                    .active_subject = p.active_subject,
                    .active_property = p.active_property,
                    .obj_out = p.obj_out,
                    .is_top_level = p.is_top_level,
                    .is_reverse = p.is_reverse,
                    .is_json_literal = p.is_json_literal,
                    .is_included = p.is_included,
                }));
            }
            co_return;
        }

        // 4
        if (std::holds_alternative<json_ld::StringLikeLiteralMapping>(expanded)) {
            if (p.is_included) {
                co_yield nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "invalid @included value"));
                co_return;
            }
            if (p.is_reverse) {
                co_yield nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "invalid reverse property value"));
                co_return;
            }
            auto &literal_mapping = std::get<json_ld::StringLikeLiteralMapping>(expanded);
            auto lit = make_literal(literal_mapping, p.active_graph);
            if (!lit.has_value()) {
                co_yield nonstd::make_unexpected(lit.error());
                co_return;
            }
            if (p.active_subject.type != json_ld::IRIMappingType::None && p.active_property.type != json_ld::IRIMappingType::None) {
                co_yield make_quad(p.active_graph, p.active_subject, p.active_property, lit->object);
                if (lit->extra_quads.has_value()) {
                    co_yield std::ranges::elements_of(*lit->extra_quads | std::views::filter([](Quad const &e) {
                        return !e.graph().null() && !e.subject().null() && !e.predicate().null() && !e.object().null();
                    }));
                }
            }
            if (p.obj_out != nullptr) {
                *p.obj_out = lit->object;
            }
            co_return;
        }
        if (std::holds_alternative<json_ld::TypedLiteralMapping>(expanded)) {
            if (p.is_included) {
                co_yield nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "invalid @included value"));
                co_return;
            }
            if (p.is_reverse) {
                co_yield nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "invalid reverse property value"));
                co_return;
            }
            auto &literal_mapping = std::get<json_ld::TypedLiteralMapping>(expanded);
            auto lit = make_literal(literal_mapping);
            if (!lit.has_value()) {
                co_yield nonstd::make_unexpected(lit.error());
                co_return;
            }
            if (p.active_subject.type != json_ld::IRIMappingType::None && p.active_property.type != json_ld::IRIMappingType::None) {
                co_yield make_quad(p.active_graph, p.active_subject, p.active_property, *lit);
            }
            if (p.obj_out != nullptr) {
                *p.obj_out = *lit;
            }
            co_return;
        }

        if (std::holds_alternative<json_ld::ExpandedMap>(expanded)) {
            auto &map = std::get<json_ld::ExpandedMap>(expanded);
            auto const &ctx = map.active_context == nullptr ? p.active_ctx : *map.active_context;

            simdjson::ondemand::object obj;
            if (!map.expanded_from_no_map) {
                obj = p.element;
                obj.reset();
            }

            // https://www.w3.org/TR/json-ld11/#named-graphs (default graph section)
            if (p.is_top_level && !map.expanded_from_no_map) {
                auto graph = map.try_find_keyword(keyword_graph);
                if (graph != nullptr) {
                    bool no_context = true;
                    for (auto const &entry : map.entries) {
                        static constexpr std::array gc = {keyword_graph, keyword_context};
                        if (entry.key.type != json_ld::IRIMappingType::Keyword || !std::ranges::any_of(gc, [&](std::string_view a) { return a == entry.key.data; })) {
                            no_context = false;
                            break;
                        }
                    }
                    if (no_context) {
                        auto [error_code, element] = try_get_field<simdjson::ondemand::value>(obj, graph->path);
                        if (error_code != simdjson::SUCCESS) {
                            co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "could not find graph value?"));
                        } else {
                            co_yield std::ranges::elements_of(parse({
                                .element = element,
                                .active_ctx = ctx,
                                .base_iri = p.base_iri,
                                .active_graph = p.active_graph,
                                .active_subject = p.active_subject,
                                .active_property = p.active_property,
                                .is_reverse = p.is_reverse,
                            }));
                        }
                        co_return;
                    }
                }
            }

            if (!map.expanded_from_no_map) {  // 5
                auto list = map.try_find_keyword(keyword_list);
                if (list != nullptr) {
                    if (p.is_reverse) {
                        co_yield nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "invalid reverse property value"));
                        co_return;
                    }
                    auto [error_code, list_array] = try_get_field<simdjson::ondemand::value>(obj, list->path);
                    if (error_code != simdjson::SUCCESS) {
                        co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "could not find list value?"));
                    } else {
                        co_yield std::ranges::elements_of(parse_list({
                            .ar = list_array,
                            .active_ctx = ctx,
                            .base_iri = p.base_iri,
                            .active_graph = p.active_graph,
                            .active_subject = p.active_subject,
                            .active_property = p.active_property,
                            .obj_out = p.obj_out,
                            .recursive_list = false,
                        }));
                    }
                    co_return;
                }
            }

            // 6
            {
                json_ld::IRIMapping id{};
                auto id_entry = map.try_find_keyword(keyword_id);
                if (id_entry != nullptr && !id_entry->keyword_values.empty()) {
                    id = std::move(id_entry->keyword_values[0]);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                    if (id.type == json_ld::IRIMappingType::None) {
                        co_return;
                    }
                }
                if (id.type == json_ld::IRIMappingType::None) {
                    id = make_new_bn();
                }
                if (p.active_subject.type != json_ld::IRIMappingType::None && p.active_property.type != json_ld::IRIMappingType::None) {
                    if (p.is_reverse) {
                        co_yield make_quad(p.active_graph, id, p.active_property, p.active_subject);
                    } else {
                        co_yield make_quad(p.active_graph, p.active_subject, p.active_property, id);
                    }
                }

                {
                    for (auto const &type_entry : map.entries) {
                        if (type_entry.key.is_keyword(keyword_type)) {
                            for (auto const &keyword : type_entry.keyword_values) {
                                if (keyword.type != json_ld::IRIMappingType::None) {
                                    co_yield make_quad(p.active_graph, id, type_entry.key, keyword);
                                }
                            }
                        }
                    }
                }

                if (!map.expanded_from_no_map) {
                    auto graph_entry = map.try_find_keyword(keyword_graph);
                    if (graph_entry != nullptr) {
                        auto [error_code, element] = try_get_field<simdjson::ondemand::value>(obj, graph_entry->path);
                        if (error_code != simdjson::SUCCESS) {
                            co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "could not find graph value?"));
                            co_return;
                        }
                        auto const *context = graph_entry->active_context;
                        if (context == nullptr) {
                            context = &ctx;
                        }
                        co_yield std::ranges::elements_of(parse({
                            .element = element,
                            .active_ctx = *context,
                            .base_iri = p.base_iri,
                            .active_graph = id,
                            .active_subject = {},
                            .active_property = {},
                        }));
                    }
                }

                for (auto &entry : map.entries) {
                    if (entry.key.is_keyword(keyword_included)) {
                        if (map.expanded_from_no_map) {
                            continue;
                        }
                        auto [error_code, element] = try_get_field<simdjson::ondemand::value>(obj, entry.path);
                        if (error_code != simdjson::SUCCESS) {
                            co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "could not find included value?"));
                        } else {
                            auto const *context = entry.active_context;
                            if (context == nullptr) {
                                context = &ctx;
                            }
                            co_yield std::ranges::elements_of(parse({
                                .element = element,
                                .active_ctx = *context,
                                .base_iri = p.base_iri,
                                .active_graph = p.active_graph,
                                .active_subject = {},
                                .active_property = {},
                                .is_json_literal = entry.is_json_literal,
                                .is_included = true,
                            }));
                        }
                    } else if (entry.key.type != json_ld::IRIMappingType::Keyword) {
                        auto const *context = entry.active_context;
                        if (context == nullptr) {
                            context = &ctx;
                        }
                        if (entry.pre_expanded_value.has_value()) {
                            auto &val = *entry.pre_expanded_value;
                            if (std::holds_alternative<json_ld::StringLikeLiteralMapping>(val)) {
                                auto &literal_mapping = std::get<json_ld::StringLikeLiteralMapping>(val);
                                auto lit = make_literal(literal_mapping, p.active_graph);
                                if (!lit.has_value()) {
                                    co_yield nonstd::make_unexpected(lit.error());
                                    co_return;
                                }
                                if (id.type != json_ld::IRIMappingType::None && entry.key.type != json_ld::IRIMappingType::None) {
                                    co_yield make_quad(p.active_graph, id, entry.key, lit->object);
                                    if (lit->extra_quads.has_value()) {
                                        co_yield std::ranges::elements_of(*lit->extra_quads | std::views::filter([](Quad const &e) {
                                            return !e.graph().null() && !e.subject().null() && !e.predicate().null() && !e.object().null();
                                        }));
                                    }
                                }
                            } else if (std::holds_alternative<json_ld::TypedLiteralMapping>(val)) {
                                auto &literal_mapping = std::get<json_ld::TypedLiteralMapping>(val);
                                auto lit = make_literal(literal_mapping);
                                if (!lit.has_value()) {
                                    co_yield nonstd::make_unexpected(lit.error());
                                    co_return;
                                }
                                if (id.type != json_ld::IRIMappingType::None && entry.key.type != json_ld::IRIMappingType::None) {
                                    co_yield make_quad(p.active_graph, id, entry.key, *lit);
                                }
                            } else if (std::holds_alternative<json_ld::IRIMapping>(val)) {
                                auto &iri = std::get<json_ld::IRIMapping>(val);
                                if (id.type != json_ld::IRIMappingType::None && entry.key.type != json_ld::IRIMappingType::None) {
                                    if (p.is_reverse) {
                                        co_yield make_quad(p.active_graph, iri, entry.key, id);
                                    } else {
                                        co_yield make_quad(p.active_graph, id, entry.key, iri);
                                    }
                                }
                            }
                            continue;
                        }
                        if (map.expanded_from_no_map) {
                            continue;
                        }
                        if (entry.as_list) {
                            if (p.is_reverse) {
                                co_yield nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "invalid reverse property value"));
                                co_return;
                            }
                            auto [error_code, list_value] = try_get_field<simdjson::ondemand::value>(obj, entry.path);
                            if (error_code != simdjson::SUCCESS) {
                                co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "could not find property value?"));
                            } else {
                                co_yield std::ranges::elements_of(parse_list({
                                    .ar = list_value,
                                    .active_ctx = *context,
                                    .base_iri = p.base_iri,
                                    .active_graph = p.active_graph,
                                    .active_subject = id,
                                    .active_property = entry.key,
                                    .obj_out = p.obj_out,
                                    .recursive_list = true,
                                }));
                            }
                            continue;
                        }
                        auto [error_code, value] = try_get_field<simdjson::ondemand::value>(obj, entry.path);
                        if (error_code != simdjson::SUCCESS) {
                            co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "could not find property value?"));
                        } else {
                            if (entry.as_multiple_graphs) {
                                simdjson::ondemand::array array;
                                if (value.get(array) != simdjson::SUCCESS) {
                                    co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "multigraph not array?"));
                                    co_return;
                                }
                                for (auto array_entry : array) {
                                    auto graph = make_new_bn();
                                    if (id.type != json_ld::IRIMappingType::None && entry.key.type != json_ld::IRIMappingType::None) {
                                        if (p.is_reverse) {
                                            co_yield make_quad(p.active_graph, graph, entry.key, id);
                                        } else {
                                            co_yield make_quad(p.active_graph, id, entry.key, graph);
                                        }
                                    }
                                    co_yield std::ranges::elements_of(parse({
                                        .element = *array_entry,
                                        .active_ctx = *context,
                                        .base_iri = p.base_iri,
                                        .active_graph = graph,
                                        .active_subject = {},
                                        .active_property = {},
                                        .is_json_literal = entry.is_json_literal,
                                    }));
                                }
                            } else if (entry.as_graph || entry.as_named_graph.has_value()) {
                                auto graph = entry.as_named_graph.has_value() ? *entry.as_named_graph : make_new_bn();
                                if (id.type != json_ld::IRIMappingType::None && entry.key.type != json_ld::IRIMappingType::None) {
                                    if (p.is_reverse) {
                                        co_yield make_quad(p.active_graph, graph, entry.key, id);
                                    } else {
                                        co_yield make_quad(p.active_graph, id, entry.key, graph);
                                    }
                                }
                                co_yield std::ranges::elements_of(parse({
                                    .element = value,
                                    .active_ctx = *context,
                                    .base_iri = p.base_iri,
                                    .active_graph = graph,
                                    .active_subject = {},
                                    .active_property = {},
                                    .is_json_literal = entry.is_json_literal,
                                }));
                            } else if (entry.language_map.has_value()) {
                                if (value.type() != simdjson::ondemand::json_type::object) {
                                    co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "not a map?"));
                                    co_return;
                                }
                                for (auto language_to_str : simdjson::ondemand::object{value}) {
                                    std::string_view const lang = language_to_str.unescaped_key();
                                    for (auto str : json_ld::ValueArrayIter{*language_to_str.value()}) {
                                        std::string_view string;
                                        if (str.get(string) != simdjson::SUCCESS) {
                                            co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "invalid language map value"));
                                            co_return;
                                        }
                                        auto expanded_lang = expand_parser_.context_parser.iri_expansion(*context, lang, false, true);
                                        auto lit = expanded_lang.has_value() && expanded_lang->is_keyword(keyword_none) ? Literal::make_simple(string) : Literal::make_lang_tagged(string, lang);
                                        co_yield make_quad(p.active_graph, id, entry.key, lit);
                                    }
                                }
                            } else {
                                params::ParseParams const pa{
                                    .element = value,
                                    .active_ctx = *context,
                                    .base_iri = p.base_iri,
                                    .active_graph = p.active_graph,
                                    .active_subject = id,
                                    .active_property = entry.key,
                                    .is_reverse = entry.is_reverse,
                                    .is_json_literal = entry.is_json_literal,
                                };
                                if (entry.next_level_pre_expanded.has_value()) {
                                    co_yield std::ranges::elements_of(parse(pa, *entry.next_level_pre_expanded));
                                } else {
                                    co_yield std::ranges::elements_of(parse(pa));
                                }
                            }
                        }
                    }
                }

                if (p.obj_out != nullptr) {
                    *p.obj_out = std::move(id);
                }
            }
        }

        co_return;
    }
    IStreamQuadIterator::ImplJsonLd::result_generator IStreamQuadIterator::ImplJsonLd::parse_list(params::ParseListParams p) {
        json_ld::IRIMapping current_bn{};
        json_ld::IRIMapping const first{std::string{list_iris::iri_first}, json_ld::IRIMappingType::IRI};
        json_ld::IRIMapping const rest{std::string{list_iris::iri_rest}, json_ld::IRIMappingType::IRI};
        json_ld::IRIMapping const nil{std::string{list_iris::iri_nil}, json_ld::IRIMappingType::IRI};
        json_ld::IRIMapping const *curr_sub = &p.active_subject;
        json_ld::IRIMapping const *curr_pred = &p.active_property;
        for (auto list_elem : json_ld::ValueArrayIter{p.ar}) {
            params::ListObjOut curr_obj{};
            {
                if (p.recursive_list && *list_elem.type() == simdjson::ondemand::json_type::array) {
                    co_yield std::ranges::elements_of(parse_list({
                        .ar = list_elem,
                        .active_ctx = p.active_ctx,
                        .base_iri = p.base_iri,
                        .active_graph = p.active_graph,
                        .active_subject = {},
                        .active_property = p.active_property,
                        .obj_out = &curr_obj,
                        .recursive_list = true,
                    }));
                } else {
                    co_yield std::ranges::elements_of(parse({
                        .element = list_elem,
                        .active_ctx = p.active_ctx,
                        .base_iri = p.base_iri,
                        .active_graph = p.active_graph,
                        .active_subject = {},
                        .active_property = p.active_property,
                        .obj_out = &curr_obj,
                    }));
                }
            }
            if (std::holds_alternative<std::monostate>(curr_obj)) {
                continue;
            }
            auto next_bn = make_new_bn();
            if (p.obj_out != nullptr) {
                *p.obj_out = next_bn;
                p.obj_out = nullptr;
            }
            if (curr_sub->type != json_ld::IRIMappingType::None && curr_sub->type != json_ld::IRIMappingType::None) {
                co_yield make_quad(p.active_graph, *curr_sub, *curr_pred, next_bn);
            }
            if (std::holds_alternative<json_ld::IRIMapping>(curr_obj)) {
                co_yield make_quad(p.active_graph, next_bn, first, std::get<json_ld::IRIMapping>(curr_obj));
            } else if (std::holds_alternative<Node>(curr_obj)) {
                co_yield make_quad(p.active_graph, next_bn, first, std::get<Node>(curr_obj));
            }
            current_bn = std::move(next_bn);
            curr_sub = &current_bn;
            curr_pred = &rest;
        }
        if (p.obj_out != nullptr) {
            *p.obj_out = nil;
            p.obj_out = nullptr;
        }
        if (curr_sub->type != json_ld::IRIMappingType::None && curr_sub->type != json_ld::IRIMappingType::None) {
            co_yield make_quad(p.active_graph, *curr_sub, *curr_pred, nil);
        }
    }
    IStreamQuadIterator::ImplJsonLd::result_generator IStreamQuadIterator::ImplJsonLd::parse() {
        simdjson::ondemand::parser parser{};
        auto c = parser.allocate(json_data_.size() * 5);
        if (c != simdjson::SUCCESS) {
            co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "failed to allocate parser"));
            co_return;
        }
        simdjson::ondemand::document doc = parser.iterate(simdjson::pad(json_data_));
        if (doc.is_scalar()) {
            co_yield nonstd::unexpected(json_ld::make_error(ParsingError::Type::BadSyntax, "free floating scalar"));
            co_return;
        }
        json_ld::Context ctx{};
        ctx.base_iri = state_->iri_factory.get_base();
        co_yield std::ranges::elements_of(parse({
            .element = doc,
            .active_ctx = ctx,
            .base_iri = ctx.base_iri,
            .active_graph = {std::string{keyword_default}, json_ld::IRIMappingType::Keyword},
            .active_subject = {},
            .active_property = {},
            .is_top_level = true,
        }));
        co_return;
    }
    std::optional<nonstd::expected<IStreamQuadIterator::ok_type, IStreamQuadIterator::error_type>> IStreamQuadIterator::ImplJsonLd::next() {
        if (current_iter_ == std::default_sentinel) {
            return std::nullopt;
        }
        auto r = *current_iter_;
        ++current_iter_;
        return r;
    }
    uint64_t IStreamQuadIterator::ImplJsonLd::current_line() const noexcept {
        return 0;
    }
    uint64_t IStreamQuadIterator::ImplJsonLd::current_column() const noexcept {
        return 0;
    }
    IStreamQuadIterator::ImplJsonLd::ImplJsonLd(std::string json, ParsingFlags flags, state_type *initial_state)
        : state_(initial_state == nullptr ? new state_type() : initial_state),
          state_is_owned_(initial_state == nullptr),
          json_data_(std::move(json)),
          expand_parser_(state_->iri_factory),
          direction_(flags.get_direction()),
          active_generator_(parse()),
          current_iter_(active_generator_.begin()) {
    }
    IStreamQuadIterator::ImplJsonLd::ImplJsonLd(void *stream, ReadFunc read, ErrorFunc error, EOFFunc eof, ParsingFlags flags, state_type *initial_state)
        : ImplJsonLd([&]() {
              std::string buff{};
              while (error(stream) == 0 && eof(stream) == 0) {
                  static constexpr size_t s = 1024;
                  auto i = buff.size();
                  buff.append(s, '\0');
                  auto n = read(&buff[i], 1, s, stream);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                  buff.resize(i + n);
              }
              return buff;
          }(),
                     flags, initial_state) {
    }
    IStreamQuadIterator::ImplJsonLd::~ImplJsonLd() {
        if (state_is_owned_) {
            delete state_;
        }
    }
}  // namespace rdf4cpp::parser