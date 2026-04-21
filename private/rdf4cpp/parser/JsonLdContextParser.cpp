#include "JsonLdContextParser.hpp"

namespace rdf4cpp::parser::json_ld {
    nonstd::expected<Context, ContextParser::error_type> ContextParser::parse_context(simdjson::ondemand::value local_context, Context const &active_context, std::string_view base_iri, bool override_protected, bool propagate) {
        // https://www.w3.org/TR/json-ld11-api/#context-processing-algorithm
        // 1
        nonstd::expected<Context, error_type> result{active_context};
        for (auto &t : result->terms) {
            t.needs_context_check = false;
        }

        auto handle_ctx = [&](simdjson::ondemand::object o) {
            {  // 5.5
                auto [c, v] = try_get_field<double>(o, keyword_version);
                if (c != simdjson::NO_SUCH_FIELD && (c != simdjson::SUCCESS || v != 1.1)) {
                    result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid @version value")};
                    return true;
                }
            }
            {  // 5.6
                auto [c, v] = try_get_field<std::string_view>(o, keyword_import);
                if (c != simdjson::NO_SUCH_FIELD) {
                    result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, c != simdjson::SUCCESS ? "invalid @import value " : "import context not supported")};
                    return true;
                }
            }
            {  // 5.7
                auto [c, v] = try_get_optional_field<std::string_view>(o, keyword_base);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base IRI")};
                        return true;
                    }
                    if (!v.has_value()) {
                        result->base_iri = "";
                    } else {
                        if (IRIView{*v}.is_relative()) {
                            if (iri_factory->set_base(result->base_iri) != IRIFactoryError::Ok) {
                                result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base IRI")};
                                return true;
                            }
                            auto r = iri_factory->from_maybe_relative_as_string(*v);
                            if (r.has_value()) {
                                result->base_iri = *r;
                            } else {
                                result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base IRI")};
                                return true;
                            }
                        } else {
                            if (IRIView{*v}.quick_validate() == IRIFactoryError::Ok) {
                                result->base_iri = *v;
                            } else {
                                result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base IRI")};
                                return true;
                            }
                        }
                    }
                }
            }
            std::vector<TermDefinition> previous_terms{};
            {  // 5.12/5.13 part 1
                o.reset();
                for (auto x : o) {
                    std::string_view key = x.escaped_key();
                    static constexpr std::array check = {
                        keyword_base,
                        keyword_direction,
                        keyword_import,
                        keyword_language,
                        keyword_propagate,
                        keyword_protected,
                        keyword_version,
                        keyword_vocab};
                    if (std::ranges::any_of(check, [&](std::string_view v) {
                            return key == v;
                        })) {
                        continue;
                    }
                    auto i = std::ranges::find_if(result->terms, [&](auto const &t) {
                        return t.key == key;
                    });
                    if (i == result->terms.end()) {
                        result->terms.emplace_back(key);
                    } else {
                        previous_terms.emplace_back(std::move(*i));
                        *i = TermDefinition{key};
                    }
                }
            }
            {  // 5.8
                auto [c, v] = try_get_optional_field<std::string_view>(o, keyword_vocab);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid vocab mapping")};
                        return true;
                    }
                    if (!v.has_value()) {
                        result->vocab = std::nullopt;
                    } else {
                        params::ParseContextIRIExpansionParams p_ctx{
                            .active_context = *result,
                            .local_context = o,
                            .previous_terms = previous_terms,
                        };
                        auto r = iri_expansion(result.value(), v, true, true, nullptr, &p_ctx);
                        if (!r.has_value() || r->type != IRIMappingType::IRI) {  // technically blank node is only deprecated, not removed
                            result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid vocab mapping")};
                            return true;
                        }
                        result->vocab = std::move(r->data);
                    }
                }
            }
            {  // 5.9
                auto [c, v] = try_get_optional_field<std::string_view>(o, keyword_language);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid default language")};
                        return true;
                    }
                    if (!v.has_value()) {
                        result->language = Null{};
                    } else {
                        result->language = std::string(*v);
                    }
                }
            }
            {  // 5.10
                auto [c, v] = try_get_optional_field<std::string_view>(o, keyword_direction);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base direction")};
                        return true;
                    }
                    if (!v.has_value()) {
                        result->base_direction = BaseDirection::None;
                    } else {
                        if (*v == "ltr") {
                            result->base_direction = BaseDirection::Ltr;
                        } else if (*v == "rtl") {
                            result->base_direction = BaseDirection::Rtl;
                        } else {
                            result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base direction")};
                            return true;
                        }
                    }
                }
            }
            {  // 5.11
                auto [c, v] = try_get_field<bool>(o, keyword_propagate);
                if (c != simdjson::NO_SUCH_FIELD && c != simdjson::SUCCESS) {
                    result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid @propagate value")};
                    return true;
                }
                // setting field earlier
            }
            {  // 5.13
                auto [c, prot] = try_get_field<bool>(o, keyword_protected);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid @protected value")};
                        return true;
                    }
                } else {
                    prot = false;
                }

                for (auto &term : result->terms) {
                    auto e = parse_context_term({
                        .local_context = o,
                        .active_context = *result,
                        .term = term,
                        .previous_terms = previous_terms,
                        .base_iri = base_iri,
                        .is_protected = prot,
                        .override_protected = override_protected,
                    });
                    if (e.has_value()) {
                        result = nonstd::unexpected{e.value()};
                        return true;
                    }
                }
            }
            return false;
        };

        if (local_context.type() == simdjson::ondemand::json_type::object) {
            // 2 & 3
            simdjson::ondemand::object const o = local_context.get_object();
            auto [c, prop] = try_get_field<bool>(o, keyword_propagate);
            bool p = propagate;
            if (c == simdjson::SUCCESS) {
                p = prop;
            }
            if (!p && result->previous_context == nullptr) {
                result->previous_context = &active_context;
            }
            //error handling later

            handle_ctx(o);  // 4
        } else if (local_context.is_scalar() && local_context.is_null()) {
            for (auto const &t : active_context.terms) {
                if (t.is_protected) {
                    result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid context nullification")};
                    return result;
                }
            }
            result = Context{
                .base_iri{base_iri},
            };
            return result;
        } else {
            if (!propagate && result->previous_context == nullptr) {
                result->previous_context = &active_context;
            }
            simdjson::ondemand::array a{};
            if (local_context.get(a) != simdjson::SUCCESS) {
                result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid local context")};
                return result;
            }
            for (auto v : a) {
                switch (v.type()) {
                    case simdjson::ondemand::json_type::null:  // 5.1
                        v.is_null();
                        if (!override_protected) {
                            for (auto const &t : active_context.terms) {
                                if (t.is_protected) {
                                    result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid context nullification")};
                                    return result;
                                }
                            }
                        }
                        result = Context{
                            .base_iri{base_iri},
                        };
                        break;
                    case simdjson::ondemand::json_type::object:  // 5.4
                        if (handle_ctx(v.get_object())) {
                            return result;
                        }
                        break;
                    case simdjson::ondemand::json_type::string:  // 5.2
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "remote context not supported")};
                        return result;
                    default:  // 5.3
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid local context")};
                        return result;
                }
            }
        }

        // check scoped context for errors, moved here from create_term
        if (result.has_value()) {
            for (auto const &t : result->terms) {
                if (t.needs_context_check && t.context.has_value()) {
                    auto lc = parse_local_context(simdjson::padded_string_view{*t.context}, *result, base_iri, true);
                    if (!lc.has_value()) {
                        return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, std::format("invalid scoped context ({})", lc.error().message)));
                    }
                }
            }
        }

        return result;
    }
    std::optional<ContextParser::error_type> ContextParser::parse_context_term(params::ParseContextTermParams p) {
        // https://www.w3.org/TR/json-ld11-api/#create-term-definition
        // 1
        if (p.term.parse_state == ParseState::Done) {
            return std::nullopt;
        }
        if (p.term.parse_state == ParseState::InProgress) {
            return make_error(ParsingError::Type::BadSyntax, "cyclic IRI mapping");
        }

        // 2
        if (p.term.key.empty()) {
            return make_error(ParsingError::Type::BadSyntax, "invalid term definition (empty term)");
        }
        p.term.parse_state = ParseState::InProgress;

        // 3
        auto [value_ec, value] = try_get_field<simdjson::ondemand::value>(p.local_context, p.term.key);
        if (value_ec != simdjson::SUCCESS) {
            return make_error(ParsingError::Type::BadSyntax, "unknown key?");  // should not happen
        }

        // 6 (out of order, because type gets handled differently)
        TermDefinition const *previous_definition = nullptr;
        {
            auto i = std::ranges::find_if(p.previous_terms, [&](TermDefinition const &t) {
                return t.key == p.term.key;
            });
            if (i != p.previous_terms.end()) {
                previous_definition = &*i;
            }
        }

        auto check_protected = [&]() -> std::optional<error_type> {
            if (!p.override_protected && previous_definition != nullptr && previous_definition->is_protected) {
                if (static_cast<TermDefinitionBase const &>(p.term) != static_cast<TermDefinitionBase const &>(*previous_definition)) {
                    return make_error(ParsingError::Type::BadSyntax, "protected term redefinition");
                }
                p.term = *previous_definition;
            }
            return std::nullopt;
        };

        // 4
        if (p.term.key == keyword_type) {
            p.term.is_protected = p.is_protected;
            simdjson::ondemand::object ob;
            if (value.get(ob) != simdjson::SUCCESS) {
                return make_error(ParsingError::Type::BadSyntax, "keyword redefinition (@type mapped to non-map)");
            }
            bool any = false;
            for (auto t : ob) {
                std::string_view const k = t.escaped_key();
                if (k == keyword_container) {
                    std::string_view v;
                    if (t.value().get(v) != simdjson::SUCCESS || v != keyword_set) {
                        return make_error(ParsingError::Type::BadSyntax, "keyword redefinition (@type invalid @container)");
                    }
                    p.term.container_mapping.emplace_back(keyword_set);
                    any = true;
                } else if (k == keyword_protected) {
                    bool v;
                    if (t.value().get(v) != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "keyword redefinition (@type invalid @protected)");
                    }
                    p.term.is_protected = v;
                    any = true;
                } else {
                    return make_error(ParsingError::Type::BadSyntax, std::format("keyword redefinition (@type invalid entry: {})", k));
                }
            }
            if (!any) {
                return make_error(ParsingError::Type::BadSyntax, "keyword redefinition (empty @type)");
            }
            p.term.parse_state = ParseState::Done;
            return check_protected();
        }

        // 5
        if (is_keyword(p.term.key)) {
            return make_error(ParsingError::Type::BadSyntax, std::format("keyword redefinition ({})", p.term.key));
        }

        auto handle_id = [&](std::optional<std::string_view> v) -> std::optional<error_type> {
            if (!v.has_value()) {
                p.term.iri_mapping = {};
            } else {
                params::ParseContextIRIExpansionParams p_ctx{
                    .active_context = p.active_context,
                    .local_context = p.local_context,
                    .previous_terms = p.previous_terms,
                };
                auto ex = iri_expansion(p.active_context, v, false, true, v == p.term.key ? &p.term : nullptr, &p_ctx);
                if (!ex.has_value()) {
                    return ex.error();
                }
                if (ex->type != IRIMappingType::IRI && ex->type != IRIMappingType::BlankNode && ex->type != IRIMappingType::Keyword) {
                    return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping (@id not IRI, bn or keyword)");
                }
                if (ex->is_keyword(keyword_context)) {
                    return make_error(ParsingError::Type::BadSyntax, "invalid keyword alias (to @context)");
                }
                p.term.iri_mapping = *ex;

                bool const has_slash = p.term.key.find('/') != std::string::npos;
                std::string_view colon_check_part = p.term.key;
                colon_check_part = colon_check_part.substr(0, colon_check_part.length() - 2);
                if (!colon_check_part.empty()) {
                    colon_check_part = colon_check_part.substr(1);
                }
                bool const has_colon = colon_check_part.find(':') != std::string_view::npos;
                if (has_colon || has_slash) {
                    p.term.parse_state = ParseState::Done;
                    params::ParseContextIRIExpansionParams p_ctx2{
                        .active_context = p.active_context,
                        .local_context = p.local_context,
                        .previous_terms = p.previous_terms,
                    };
                    if (iri_expansion(p.active_context, p.term.key, false, true, &p.term, &p_ctx2) != p.term.iri_mapping) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping (@id)");
                    }
                } else if (p.term.is_simple) {
                    bool const iri = !p.term.iri_mapping.data.empty() && p.term.iri_mapping.type == IRIMappingType::IRI && std::string_view(":/?#[]@").find(p.term.iri_mapping.data.at(p.term.iri_mapping.data.length() - 1)) != std::string::npos;
                    if (iri || p.term.iri_mapping.type == IRIMappingType::BlankNode) {
                        p.term.is_prefix = true;
                    }
                }
            }
            return std::nullopt;
        };

        bool skip_object = false;
        // 7
        if (value.is_null()) {
            p.term.is_protected = p.is_protected;
            auto e = handle_id(std::nullopt);
            if (e.has_value()) {
                return e;
            }
            skip_object = true;
        }

        // 8
        if (value.is_string()) {
            p.term.is_simple = true;
            p.term.is_protected = p.is_protected;
            auto e = handle_id(static_cast<std::string_view>(value));
            if (e.has_value()) {
                return e;
            }
            skip_object = true;
        }

        if (!skip_object) {
            // 9
            simdjson::ondemand::object ob;
            if (value.get(ob) != simdjson::SUCCESS) {
                return make_error(ParsingError::Type::BadSyntax, "invalid term definition (value not null, string or map)");
            }

            // 10
            p.term.is_simple = false;
            p.term.is_protected = p.is_protected;

            {  // 11
                auto [c, v] = try_get_field<bool>(ob, keyword_protected);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @protected value");
                    }
                    p.term.is_protected = v;
                }
            }

            bool has_type = false;
            {  // 12
                auto [c, v] = try_get_field<std::string_view>(ob, keyword_type);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid type mapping");
                    }
                    params::ParseContextIRIExpansionParams p_ctx{
                        .active_context = p.active_context,
                        .local_context = p.local_context,
                        .previous_terms = p.previous_terms,
                    };
                    auto type = iri_expansion(p.active_context, v, false, true, nullptr, &p_ctx);
                    if (!type.has_value()) {
                        return type.error();
                    }
                    if (type->type != IRIMappingType::Keyword && type->type != IRIMappingType::IRI) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid type mapping (not IRI or keyword)");
                    }
                    static constexpr std::array invalid = {keyword_json, keyword_none, keyword_id, keyword_vocab};
                    if (type->type == IRIMappingType::Keyword && !std::ranges::any_of(invalid, [&](std::string_view a) {
                            return a == v;
                        })) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid type mapping (invalid keyword)");
                    }
                    p.term.type_mapping = std::move(type->data);
                    has_type = true;
                }
            }

            {  // 13
                auto [c, v] = try_get_field<std::string_view>(ob, keyword_reverse);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping (@reverse)");
                    }
                    auto [nc, va] = try_get_field<simdjson::ondemand::value>(ob, keyword_id);
                    if (nc != simdjson::NO_SUCH_FIELD) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid reverse property (contains id)");
                    }
                    std::tie(nc, va) = try_get_field<simdjson::ondemand::value>(ob, keyword_nest);
                    if (nc != simdjson::NO_SUCH_FIELD) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid reverse property (contains nest)");
                    }

                    params::ParseContextIRIExpansionParams p_ctx{
                        .active_context = p.active_context,
                        .local_context = p.local_context,
                        .previous_terms = p.previous_terms,
                    };
                    auto r = iri_expansion(p.active_context, v, false, true, nullptr, &p_ctx);
                    if (!r.has_value()) {
                        return r.error();
                    }
                    if (r->type != IRIMappingType::IRI && r->type != IRIMappingType::BlankNode) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping (in @reverse)");
                    }
                    p.term.iri_mapping = *r;


                    auto [c_cont, v_cont] = try_get_field<simdjson::ondemand::value>(ob, keyword_container);
                    if (c_cont != simdjson::NO_SUCH_FIELD) {
                        if (c_cont != simdjson::SUCCESS) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid reverse property");
                        }
                        if (v_cont.is_null()) {
                            p.term.container_mapping.clear();
                        } else {
                            std::string_view cont;
                            if (v_cont.get(cont) != simdjson::SUCCESS) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid reverse property");
                            }
                            if (cont != keyword_set && cont != keyword_index) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid reverse property");
                            }
                            p.term.container_mapping.clear();
                            p.term.container_mapping.emplace_back(cont);
                        }
                    }

                    // TODO double check protected
                    p.term.is_reverse_property = true;
                    p.term.parse_state = ParseState::Done;
                    return std::nullopt;
                }
            }
            {  // 14
                auto [c, v] = try_get_optional_field<std::string_view>(ob, keyword_id);
                if (c != simdjson::NO_SUCH_FIELD && v != p.term.key) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping (@id)");
                    }
                    auto e = handle_id(v);
                    if (e.has_value()) {
                        return e;
                    }
                } else {
                    {  // 15
                        auto const colon_pos = std::string_view(p.term.key).substr(1).find(':');
                        if (colon_pos != std::string_view::npos) {
                            auto prefix = std::string_view(p.term.key).substr(0, colon_pos + 1);
                            auto other_term = p.active_context.try_find_term(prefix);
                            if (other_term != nullptr) {
                                auto rec = parse_context_term({
                                    .local_context = p.local_context,
                                    .active_context = p.active_context,
                                    .term = *other_term,
                                    .previous_terms = p.previous_terms,
                                    .base_iri = p.base_iri,
                                    .is_protected = p.is_protected,
                                    .override_protected = p.override_protected,
                                });
                                if (rec.has_value()) {
                                    return rec;
                                }
                                p.term.iri_mapping = other_term->iri_mapping;
                                p.term.iri_mapping.data.append(std::string_view(p.term.key).substr(colon_pos + 2));
                            } else {
                                p.term.iri_mapping.data = p.term.key;
                                p.term.iri_mapping.type = std::string_view(p.term.key).substr(2) == "_:" ? IRIMappingType::BlankNode : IRIMappingType::IRI;
                            }
                        }
                        // 16
                        else if (std::string_view(p.term.key).find('/') != std::string_view::npos) {
                            params::ParseContextIRIExpansionParams p_ctx{
                                .active_context = p.active_context,
                                .local_context = p.local_context,
                                .previous_terms = p.previous_terms,
                            };
                            auto m = iri_expansion(p.active_context, p.term.key, false, true, nullptr, &p_ctx);
                            if (!m.has_value()) {
                                return m.error();
                            }
                            if (m->type != IRIMappingType::IRI) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping");
                            }
                            p.term.iri_mapping = *m;
                        }
                        // 17
                        else if (p.term.key == keyword_type) {
                            p.term.iri_mapping.data = keyword_type;
                            p.term.iri_mapping.type = IRIMappingType::Keyword;
                        }
                        // 18
                        else if (p.active_context.vocab.has_value()) {
                            p.term.iri_mapping.data = *p.active_context.vocab;
                            p.term.iri_mapping.data.append(p.term.key);
                            p.term.iri_mapping.type = IRIMappingType::IRI;
                        } else {
                            return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping (no mapping available)");
                        }
                    }
                }
            }
            auto container_contains = [&](std::string_view x) {
                return std::ranges::find(p.term.container_mapping, x) != p.term.container_mapping.end();
            };
            {  // 19
                auto [c, v] = try_get_field<simdjson::ondemand::value>(ob, keyword_container);
                if (c != simdjson::NO_SUCH_FIELD) {
                    auto is_valid_keyword = [](std::string_view v) {
                        static constexpr std::array valid = {keyword_graph, keyword_id, keyword_index, keyword_language, keyword_list, keyword_set, keyword_type};
                        return std::ranges::any_of(valid, [v](std::string_view a) {
                            return a == v;
                        });
                    };
                    if (v.is_string()) {
                        auto d = static_cast<std::string_view>(v);
                        if (!is_valid_keyword(d)) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                        }
                        p.term.container_mapping.clear();
                        p.term.container_mapping.emplace_back(d);
                    } else {
                        simdjson::ondemand::array a;
                        if (v.get(a) != simdjson::SUCCESS) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                        }
                        p.term.container_mapping.clear();
                        for (auto w : a) {
                            std::string_view x;
                            if (w.get(x) != simdjson::SUCCESS) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                            }
                            if (!is_valid_keyword(x)) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                            }
                            p.term.container_mapping.emplace_back(x);
                        }
                    }
                    if (p.term.container_mapping.empty()) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                    }
                    if (p.term.container_mapping.size() > 1) {
                        auto only = [&](std::initializer_list<std::string_view> x) {
                            return std::ranges::all_of(p.term.container_mapping, [&](std::string_view d) {
                                return std::ranges::any_of(x, [&](std::string_view a) {
                                    return a == d;
                                });
                            });
                        };
                        bool graph = container_contains(keyword_graph);
                        if (graph) {
                            auto id = container_contains(keyword_id);
                            auto index = container_contains(keyword_index);
                            if (index == id) {  // xor
                                graph = false;
                            } else {
                                graph = only({keyword_graph, keyword_id, keyword_index, keyword_set});
                            }
                        }
                        bool set = container_contains(keyword_set);
                        if (set) {
                            set = only({keyword_set, keyword_index, keyword_graph, keyword_id, keyword_type, keyword_language});
                        }
                        if (!set && !graph) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                        }
                    }
                    if (container_contains(keyword_type)) {
                        if (!p.term.type_mapping.has_value()) {
                            p.term.type_mapping = keyword_id;
                        }
                        if (p.term.type_mapping != keyword_id && p.term.type_mapping != keyword_vocab) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid type mapping");
                        }
                    }
                }
            }
            {  // 20
                auto [c, v] = try_get_field<std::string_view>(ob, keyword_index);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                    if (!container_contains(keyword_index)) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                    params::ParseContextIRIExpansionParams p_ctx{
                        .active_context = p.active_context,
                        .local_context = p.local_context,
                        .previous_terms = p.previous_terms,
                    };
                    auto r = iri_expansion(p.active_context, v, false, true, nullptr, &p_ctx);
                    if (!r.has_value()) {
                        return r.error();
                    }
                    if (r->type != IRIMappingType::IRI) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                    p.term.index_mapping = std::move(*r);
                }
            }
            {  // 21
                auto [c, v] = try_get_field<simdjson::ondemand::value>(ob, keyword_context);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid scoped context");
                    }
                    auto t = *v.type();
                    if (t == simdjson::ondemand::json_type::string) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid scoped context, remote");
                    }
                    p.term.context = std::string{static_cast<std::string_view>(v.raw_json())};
                    if (t != simdjson::ondemand::json_type::array && t != simdjson::ondemand::json_type::object) {
                        p.term.context = std::format("[{}]", *p.term.context);
                    }
                    simdjson::pad(*p.term.context);
                    // check moved to end of context processing
                    p.term.needs_context_check = true;
                }
            }
            if (!has_type) {  // 22
                auto [c, v] = try_get_optional_field<std::string_view>(ob, keyword_language);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid language mapping");
                    }
                    if (v.has_value()) {
                        p.term.language_mapping = std::string(*v);
                    } else {
                        p.term.language_mapping = Null{};
                    }
                }
            }
            if (!has_type) {  // 23
                auto [c, v] = try_get_optional_field<std::string_view>(ob, keyword_direction);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid base direction");
                    }
                    if (!v.has_value()) {
                        p.term.direction_mapping = BaseDirection::None;
                    } else if (*v == "ltr") {
                        p.term.direction_mapping = BaseDirection::Ltr;
                    } else if (*v == "rtl") {
                        p.term.direction_mapping = BaseDirection::Rtl;
                    } else {
                        return make_error(ParsingError::Type::BadSyntax, "invalid base direction");
                    }
                }
            }
            {  // 24
                auto [c, v] = try_get_field<std::string_view>(ob, keyword_nest);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @nest value");
                    }
                    if (is_keyword(v) && v != keyword_nest) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @nest value");
                    }
                    p.term.nest_value = v;
                }
            }
            {  // 25
                auto [c, v] = try_get_field<bool>(ob, keyword_prefix);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @prefix value");
                    }
                    if (p.term.key.find_first_of("/:") != std::string::npos) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                    p.term.is_prefix = v;
                    if (v && p.term.iri_mapping.type == IRIMappingType::Keyword) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                }
            }
            {  // 26
                ob.reset();
                for (auto e : ob) {
                    static constexpr std::array invalid = {keyword_id, keyword_reverse, keyword_container, keyword_context, keyword_direction, keyword_index, keyword_language, keyword_nest, keyword_prefix, keyword_protected, keyword_type};
                    auto esc = *e.escaped_key();
                    if (!std::ranges::any_of(invalid, [&](std::string_view a) {
                            return a == esc;
                        })) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                }
            }
        }
        {  // 27
            auto e = check_protected();
            if (e.has_value()) {
                return e;
            }
        }
        // 28
        p.term.parse_state = ParseState::Done;
        return std::nullopt;
    }
    nonstd::expected<Context, ContextParser::error_type> ContextParser::parse_local_context(simdjson::padded_string_view json, Context const &active_context, std::string_view base_iri, bool override_protected, bool propagate) {
        simdjson::ondemand::parser parser{};
        simdjson::ondemand::document doc = parser.iterate(json);
        if (doc.is_scalar()) {
            return nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, doc.is_string() ? "remote context not supported" : "context free floating scalar")};
        }
        return parse_context(doc, active_context, base_iri, override_protected, propagate);
    }
    nonstd::expected<IRIMapping, ContextParser::error_type> ContextParser::iri_expansion(Context const &active_context,
                                                                                         std::optional<std::string_view> value,
                                                                                         bool document_relative,
                                                                                         bool vocab,
                                                                                         TermDefinition const *ignore_local,
                                                                                         params::ParseContextIRIExpansionParams *parse_ctx) {
        // https://www.w3.org/TR/json-ld11-api/#iri-expansion
        // 1
        if (!value.has_value()) {
            return IRIMapping{};
        }
        if (is_keyword(*value)) {
            return IRIMapping{std::string{*value}, IRIMappingType::Keyword};
        }
        // 2
        if (looks_like_keyword(*value)) {
            return IRIMapping{};
        }
        // 3
        if (parse_ctx != nullptr) {
            auto i = parse_ctx->active_context.try_find_term(*value);
            if (i != nullptr && i->parse_state != ParseState::Done && i != ignore_local) {
                auto e = parse_context_term({
                    .local_context = parse_ctx->local_context,
                    .active_context = parse_ctx->active_context,
                    .term = *i,
                    .previous_terms = parse_ctx->previous_terms,
                    .base_iri = "",
                });
                if (e.has_value()) {
                    return nonstd::make_unexpected(*e);
                }
            }
        }
        // 4
        auto *in_active = active_context.try_find_term(*value);
        if (in_active != nullptr && in_active->iri_mapping.type == IRIMappingType::Keyword && in_active != ignore_local) {
            return IRIMapping{in_active->iri_mapping.data, in_active->iri_mapping.type, std::string{*value}};
        }
        // 5
        if (vocab && in_active != nullptr && in_active != ignore_local) {
            return IRIMapping{in_active->iri_mapping.data, in_active->iri_mapping.type, std::string{*value}};
        }
        // 6
        {
            auto i = value->find(':');
            if (i != std::string_view::npos && i > 0) {
                auto pre = value->substr(0, i);
                auto post = value->substr(i + 1);
                if (pre == "_") {
                    return IRIMapping{std::format("bn_n{}", post), IRIMappingType::BlankNode};
                }
                if (post.starts_with("//")) {
                    return IRIMapping{std::string(*value), IRIMappingType::IRI};
                }
                if (parse_ctx != nullptr) {
                    auto term = parse_ctx->active_context.try_find_term(pre);
                    if (term != nullptr && term->parse_state != ParseState::Done) {
                        auto e = parse_context_term({
                            .local_context = parse_ctx->local_context,
                            .active_context = parse_ctx->active_context,
                            .term = *term,
                            .previous_terms = parse_ctx->previous_terms,
                            .base_iri = "",
                        });
                        if (e.has_value()) {
                            return nonstd::make_unexpected(*e);
                        }
                    }
                }
                auto term = active_context.try_find_term(pre);
                if (term != nullptr && term->iri_mapping.type != IRIMappingType::None && term->is_prefix) {
                    std::string v = term->iri_mapping.data;
                    v.append(post);
                    return IRIMapping{std::move(v), IRIMappingType::IRI, std::string{*value}};
                }
                if (IRIView(*value).quick_validate() == IRIFactoryError::Ok) {
                    return IRIMapping{std::string(*value), IRIMappingType::IRI};
                }
            }
        }
        // 7
        if (vocab && active_context.vocab.has_value()) {
            std::string v = *active_context.vocab;
            v.append(*value);
            return IRIMapping{std::move(v), IRIMappingType::IRI};
        }
        // 8
        if (document_relative) {
            if (active_context.base_iri.empty()) {
                return IRIMapping{std::string(""), IRIMappingType::None};
            }
            if (iri_factory->set_base(active_context.base_iri) != IRIFactoryError::Ok) {
                return nonstd::make_unexpected(make_error(ParsingError::Type::BadIri, "invalid base iri"));
            }
            auto r = iri_factory->from_maybe_relative_as_string(*value);
            if (r.has_value()) {
                return IRIMapping{std::string(*r), IRIMappingType::IRI};
            } else {
                return nonstd::make_unexpected(json_ld::make_error(ParsingError::Type::BadIri, std::format("invalid relative iri: {}", r.error())));
            }
        }
        // 9
        // no keyword, bn or valid iri, would have passed any further check
        return IRIMapping{std::string(""), IRIMappingType::None};
    }
}  // namespace rdf4cpp::parser::json_ld