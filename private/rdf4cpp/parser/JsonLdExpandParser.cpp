#include "JsonLdExpandParser.hpp"

namespace rdf4cpp::parser::json_ld {
    nonstd::expected<ExpandedValue, ExpandParser::error_type>
    ExpandParser::value_expansion(Context const &active_conext,
                                  IRIMapping const &active_property,
                                  simdjson::ondemand::value value) {
        // https://www.w3.org/TR/json-ld11-api/#value-expansion
        TermDefinition const *active_term = active_conext.try_find_term(active_property.get_search_key());

        // 1
        if (active_term != nullptr && active_term->type_mapping.has_value() && *active_term->type_mapping == keyword_id && value.is_string()) {
            return context_parser.iri_expansion(active_conext, value.get_string(), true, false);
        }
        // 2
        if (active_term != nullptr && active_term->type_mapping.has_value() && *active_term->type_mapping == keyword_vocab && value.is_string()) {
            return context_parser.iri_expansion(active_conext, value.get_string(), true, true);
        }
        // 4
        if (active_term != nullptr && active_term->type_mapping.has_value() && *active_term->type_mapping == keyword_json) {
            return to_json_literal(value);
        }
        static constexpr std::array inv_type = {keyword_id, keyword_vocab, keyword_none};
        if (active_term != nullptr && active_term->type_mapping.has_value() && !std::ranges::any_of(inv_type, [&](std::string_view a) {
                return a == *active_term->type_mapping;
            })) {
            auto t = *value.type();
            if (t != simdjson::ondemand::json_type::number && t != simdjson::ondemand::json_type::boolean && t != simdjson::ondemand::json_type::string) {
                return nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid type for literal")};
            }
            return TypedLiteralMapping{stringify(value, true, std::string_view(*active_term->type_mapping) == datatypes::registry::xsd_double).value, *active_term->type_mapping};
        }
        // 5
        if (value.is_string()) {
            auto const &l = active_term != nullptr ? active_term->language_mapping || active_conext.language : active_conext.language;
            return LiteralMapping{
                std::string(static_cast<std::string_view>(value.get_string())),
                l.output(),
                active_term != nullptr && active_term->direction_mapping != BaseDirection::None ? active_term->direction_mapping : active_conext.base_direction,
            };
        }
        // https://www.w3.org/TR/json-ld11-api/#object-to-rdf-conversion
        // 9
        if (value.type() == simdjson::ondemand::json_type::boolean || value.type() == simdjson::ondemand::json_type::number) {
            auto [v, t] = stringify(value, true);
            return TypedLiteralMapping{
                std::move(v),
                std::string(t)};
        }
        return value;
    }
    nonstd::expected<ExpandedValue, ExpandParser::error_type> ExpandParser::value_expansion(Context const &active_conext,
                                                                                            IRIMapping const &active_property,
                                                                                            std::string_view value) {
        // https://www.w3.org/TR/json-ld11-api/#value-expansion
        TermDefinition const *active_term = active_conext.try_find_term(active_property.get_search_key());

        // 1
        if (active_term != nullptr && active_term->type_mapping.has_value() && *active_term->type_mapping == keyword_id) {
            return context_parser.iri_expansion(active_conext, value, true, false);
        }
        // 2
        if (active_term != nullptr && active_term->type_mapping.has_value() && *active_term->type_mapping == keyword_vocab) {
            return context_parser.iri_expansion(active_conext, value, true, true);
        }
        // 4
        static constexpr std::array inv_type = {keyword_id, keyword_vocab, keyword_none};
        if (active_term != nullptr && active_term->type_mapping.has_value() && !std::ranges::any_of(inv_type, [&](std::string_view a) {
                return a == *active_term->type_mapping;
            })) {
            return TypedLiteralMapping{std::string{value}, *active_term->type_mapping};
        }
        // 5 (condition always true)
        auto const &l = active_term != nullptr ? active_term->language_mapping || active_conext.language : active_conext.language;
        return LiteralMapping{
            std::string(value),
            l.output(),
            active_term != nullptr && active_term->direction_mapping != BaseDirection::None ? active_term->direction_mapping : active_conext.base_direction,
        };
        // rest unreachable
    }
    StringifyResult ExpandParser::stringify(simdjson::ondemand::value v, bool normalize, bool force_double, bool escape_string) {
        auto t = *v.type();
        if (t == simdjson::ondemand::json_type::boolean) {
            return {std::string{v.get_bool() ? "true" : "false"}, datatypes::registry::xsd_boolean};
        }
        if (t == simdjson::ondemand::json_type::number) {
            auto str = v.raw_json_token();
            while (str.ends_with('\n') || str.ends_with(',') || str.ends_with(' ') || str.ends_with('\t')) {
                str = str.substr(0, str.length() - 1);
            }
            auto d = datatypes::registry::util::from_chars<double, "">(str);
            if (!normalize && d == 0.0) {
                return {"0", datatypes::registry::xsd_integer};
            }
            if (std::floor(d) != d || std::abs(d) >= 1.0e21 || force_double) {
                if (normalize) {
                    return {writer::StringWriter::oneshot([&](writer::StringWriter &w) {
                                return datatypes::registry::util::to_chars_canonical(d, w);
                            }),
                            datatypes::registry::xsd_double};
                }
                return {std::string(str), datatypes::registry::xsd_double};
            }
            if (normalize) {
                return {writer::StringWriter::oneshot([&](writer::StringWriter &w) {
                            return datatypes::registry::util::to_chars_canonical(static_cast<int64_t>(d), w);
                        }),
                        datatypes::registry::xsd_integer};
            }
            return {std::string(str), datatypes::registry::xsd_integer};
        }
        if (t == simdjson::ondemand::json_type::string) {
            auto s = *v.get_string();
            return {escape_string ? std::format("\"{}\"", s) : std::string{s}, datatypes::registry::xsd_string};
        }
        if (v.is_null()) {
            return {std::string{"null"}, ""};
        }
        auto str = *v.raw_json();
        while (str.ends_with('\n') || str.ends_with(',') || str.ends_with(' ') || str.ends_with('\t')) {
            str = str.substr(0, str.length() - 1);
        }
        return {std::string{str}, ""};
    }
    TypedLiteralMapping ExpandParser::to_json_literal(simdjson::ondemand::value v) {
        return TypedLiteralMapping{stringify(v, false, false, true).value, std::string{rdf_json_datatype}};
    }
    nonstd::expected<ExpandedLevel, ExpandParser::error_type> ExpandParser::expand_level(params::ExpandParams p) {
        if (p.is_json_literal) {
            return to_json_literal(p.element);
        }

        // https://www.w3.org/TR/json-ld11-api/#expansion-algorithm
        // 1
        if (!p.assume_no_scalar && !p.element_obj.has_value() && p.element.is_null()) {
            return Null{};
        }
        // 2
        // frame_expansion is unused
        // 3
        TermDefinition const *active_term = p.active_context.try_find_term(p.active_property.get_search_key());
        auto property_scoped_context = active_term != nullptr && active_term->context.has_value() ? &*active_term->context : nullptr;
        auto *active_ctx = &p.active_context;
        auto *active_ctx_for_local = &p.active_context;
        // 4
        if (!p.element_obj.has_value() && p.element.is_scalar()) {
            if (p.active_property.type == IRIMappingType::None || p.active_property.is_keyword(keyword_graph)) {
                return Null{};
            }
            std::optional<Context> ctx = std::nullopt;
            if (property_scoped_context != nullptr) {
                auto r = context_parser.parse_local_context(simdjson::padded_string_view{*property_scoped_context}, p.active_context, active_term->base_iri.has_value() ? *active_term->base_iri : p.base_iri);
                if (!r.has_value()) {
                    return nonstd::unexpected(r.error());
                }
                ctx = std::move(*r);
                active_ctx = &*ctx;
            }
            // no need to keep the context alive
            auto v = value_expansion(*active_ctx, p.active_property, p.element);
            if (v.has_value()) {
                return std::visit([&]<typename T>(T &&t) -> nonstd::expected<ExpandedLevel, error_type> {
                    if constexpr (std::same_as<T, IRIMapping>) {
                        ExpandedMap result{};
                        auto &e = result.entries.emplace_back(IRIMapping{std::string{keyword_id}, IRIMappingType::Keyword});
                        e.keyword_values.emplace_back(std::forward<T>(t));
                        result.expanded_from_no_map = true;
                        return ExpandedLevel{result};
                    } else {
                        return ExpandedLevel{std::forward<T>(t)};
                    }
                },
                                  std::move(*v));
            }
            return nonstd::unexpected{std::move(v.error())};
        }
        // 5
        if (p.element.type() == simdjson::ondemand::json_type::array) {
            // check for array happens before any call to expand
            return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "unexpected array in expand?"));
        }
        // 6
        auto elem_object = p.element_obj.has_value() ? *p.element_obj : static_cast<simdjson::ondemand::object>(p.element);
        // 7
        if (active_ctx->previous_context != nullptr && !p.from_map) {
            bool clear = true;
            bool has_id = false;
            bool any_other = false;
            for (auto e : elem_object) {
                auto x = context_parser.iri_expansion(*active_ctx, e.escaped_key(), false, true);
                if (x.has_value() && x->is_keyword(keyword_value)) {
                    clear = false;
                    break;
                }
                if (x.has_value() && x->is_keyword(keyword_id)) {
                    has_id = true;
                } else {
                    any_other = true;
                }
            }
            elem_object.reset();
            if (has_id && !any_other) {
                clear = false;
            }
            if (clear) {
                active_ctx = active_ctx->previous_context;
            }
        }
        // 8
        ExpandedMap result{};
        if (property_scoped_context != nullptr) {
            auto r = context_parser.parse_local_context(simdjson::padded_string_view{*property_scoped_context}, *active_ctx_for_local, active_term->base_iri.has_value() ? *active_term->base_iri : p.base_iri, true);
            if (!r.has_value()) {
                return nonstd::unexpected(r.error());
            }
            result.active_context = &result.context_storage.emplace_front(std::move(*r));
            active_ctx = result.active_context;
        }
        // 9
        {
            auto [c, v] = try_get_field<simdjson::ondemand::value>(elem_object, keyword_context);
            if (c != simdjson::NO_SUCH_FIELD) {
                if (c != simdjson::SUCCESS) {
                    return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid context"));
                }
                auto r = context_parser.parse_context(v, *active_ctx, p.base_iri);
                if (!r.has_value()) {
                    return nonstd::unexpected(r.error());
                }
                result.active_context = &result.context_storage.emplace_front(std::move(*r));
                active_ctx = result.active_context;
            }
        }
        // 10
        auto *type_scoped_ctx = active_ctx;
        // 11
        std::optional<std::string> input_type = std::nullopt;
        {
            std::vector<std::string> types{};
            elem_object.reset();
            for (auto kv : elem_object) {
                std::string_view k = kv.unescaped_key();
                auto ex = context_parser.iri_expansion(*active_ctx, k, false, true);
                if (ex.has_value() && ex->type == IRIMappingType::Keyword && ex->data == keyword_type) {
                    types.emplace_back(k);
                }
            }
            std::ranges::sort(types);
            elem_object.reset();

            auto handle_type = [&](std::string_view v) -> std::optional<error_type> {
                if (!input_type.has_value()) {
                    auto ex = context_parser.iri_expansion(*active_ctx, v, false, true);
                    if (ex.has_value() && ex->type != IRIMappingType::None) {
                        input_type = std::move(ex->data);
                    }
                }
                auto *term = type_scoped_ctx->try_find_term(v);
                if (term == nullptr || !term->context.has_value()) {
                    return std::nullopt;
                }
                auto r = context_parser.parse_local_context(simdjson::padded_string_view{*term->context}, *active_ctx, p.base_iri, false, false);
                if (!r.has_value()) {
                    return r.error();
                }
                result.active_context = &result.context_storage.emplace_front(std::move(*r));
                active_ctx = result.active_context;
                return std::nullopt;
            };
            for (std::string_view const k : types) {
                auto [c, v] = try_get_field<simdjson::ondemand::value>(elem_object, k);
                if (c == simdjson::SUCCESS) {
                    if (v.is_string()) {
                        auto e = handle_type(v.get_string());
                        if (e.has_value()) {
                            return nonstd::unexpected(*e);
                        }
                    } else {
                        simdjson::ondemand::array a;
                        if (v.get(a) == simdjson::SUCCESS) {
                            std::vector<std::string> t{};
                            for (auto e : a) {
                                if (e.is_string()) {
                                    t.emplace_back(e);
                                }
                            }
                            std::ranges::sort(t);
                            for (auto const &e : t) {
                                auto err = handle_type(e);
                                if (err.has_value()) {
                                    return nonstd::unexpected(*err);
                                }
                            }
                        }
                    }
                }
            }
        }
        // 12 + 13 + 14
        {
            elem_object.reset();
            auto r = expand_level_nested_recursive({
                .result = result,
                .elem_obj = elem_object,
                .active_ctx = *active_ctx,
                .type_scoped_context = *type_scoped_ctx,
                .active_path = {},
                .active_property = p.active_property,
                .input_type = input_type,
                .base_iri = p.base_iri,
            });
            if (r.has_value()) {
                return nonstd::unexpected(*r);
            }
        }

        // 15
        elem_object.reset();
        {
            auto *val = result.try_find_keyword(keyword_value);
            if (val != nullptr) {
                for (auto const &e : result.entries) {
                    static constexpr std::array kw = {keyword_direction, keyword_index, keyword_language, keyword_type, keyword_value};
                    if (e.key.type != IRIMappingType::Keyword
                        || !std::ranges::any_of(kw, [&](std::string_view a) {
                               return a == e.key.data;
                           })) {
                        return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid value object (invalid keyword)"));
                    }
                }
                auto *type = result.try_find_keyword(keyword_type);
                auto *lang = result.try_find_keyword(keyword_language);
                auto *dir = result.try_find_keyword(keyword_direction);
                if (type != nullptr && (lang != nullptr || dir != nullptr)) {
                    return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid value object (type and language or direction)"));
                }
                auto [c, val_obj] = try_get_field<simdjson::ondemand::value>(elem_object, val->path);
                if (c != simdjson::SUCCESS) {
                    return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid value object (val not found)"));
                }
                if (type != nullptr && (type->is_json_literal || type->has_keyword_value(keyword_json))) {
                    return to_json_literal(val_obj);
                }
                if (val_obj.is_null()) {
                    return Null{};
                }
                if (lang != nullptr || dir != nullptr) {
                    if (val_obj.is_string()) {
                        std::optional<std::string> lang_obj = std::nullopt;
                        if (lang != nullptr) {
                            auto [ec, l] = try_get_field<std::string_view>(elem_object, lang->path);
                            if (ec != simdjson::SUCCESS) {
                                return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid language-tagged value"));
                            }
                            lang_obj = std::string(l);
                        }
                        BaseDirection d = BaseDirection::None;
                        if (dir != nullptr && dir->keyword_values.size() == 1) {
                            auto &s = dir->keyword_values[0].data;  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                            if (s == "ltr") {
                                d = BaseDirection::Ltr;
                            } else if (s == "rtl") {
                                d = BaseDirection::Rtl;
                            }
                        }
                        return LiteralMapping{
                            std::string{std::string_view{val_obj.get_string()}},
                            std::move(lang_obj),
                            d,
                        };
                    }
                    return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid language-tagged value"));
                }
                bool fd = false;
                if (type != nullptr) {
                    if (type->keyword_values.size() != 1 || type->keyword_values[0].type != IRIMappingType::IRI) {  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                        return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid typed value"));
                    }
                    if (type->keyword_values[0].data == std::string_view(datatypes::registry::xsd_double)) {  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                        fd = true;
                    }
                }
                auto [s, t] = stringify(val_obj, true, fd, false);
                if (type != nullptr) {
                    return TypedLiteralMapping{
                        std::move(s),
                        std::move(type->keyword_values[0].data),  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                    };
                }
                if (!t.empty()) {
                    return TypedLiteralMapping{
                        std::move(s),
                        std::string(t),
                    };
                }
                return LiteralMapping{
                    std::move(s),
                    std::nullopt,
                    BaseDirection::None,
                };
            }
        }
        // 17
        {
            if (result.try_find_keyword(keyword_set) != nullptr || result.try_find_keyword(keyword_list) != nullptr) {
                size_t expected_size = 1;
                if (result.try_find_keyword(keyword_index) != nullptr) {
                    expected_size = 2;
                }
                if (result.entries.size() != expected_size) {
                    return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid set or list object"));
                }
                auto *set = result.try_find_keyword(keyword_set);
                if (set != nullptr) {
                    auto [c, el] = try_get_field<simdjson::ondemand::value>(elem_object, set->path);
                    if (c != simdjson::SUCCESS) {
                        return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "set path not found?"));
                    }
                    if (el.type() == simdjson::ondemand::json_type::array) {
                        return static_cast<simdjson::ondemand::array>(el);
                    }
                    return expand_level({
                        .active_context = p.active_context,
                        .active_property = p.active_property,
                        .element = el,
                        .base_iri = p.base_iri,
                        .from_map = p.from_map,
                    });
                }
            }
        }
        // 18
        if (result.try_find_keyword(keyword_language) != nullptr && result.entries.size() == 1) {
            return Null{};
        }
        // 19
        if (p.active_property.type == IRIMappingType::None || p.active_property.is_keyword(keyword_graph)) {
            if (result.entries.empty()) {
                return Null{};
            }
            if (result.entries.size() == 1 && (result.try_find_keyword(keyword_value) != nullptr || result.try_find_keyword(keyword_list) != nullptr || result.try_find_keyword(keyword_id) != nullptr)) {
                return Null{};
            }
        }
        // 20
        return result;
    }
    std::optional<ExpandParser::error_type> ExpandParser::expand_level_nested_recursive(params::ExpandNestedParams p) {
        for (auto kv : p.elem_obj) {
            // 13.1
            std::string_view k = kv.unescaped_key();
            simdjson::ondemand::value v = *kv.value();
            if (k == keyword_context) {
                continue;
            }
            // 13.2
            auto expanded_property = context_parser.iri_expansion(p.active_ctx, k, false, true);
            if (!expanded_property.has_value()) {
                return expanded_property.error();
            }
            // 13.3
            if (expanded_property->type == IRIMappingType::None
                || !(expanded_property->type == IRIMappingType::Keyword
                     || expanded_property->data.find(':') != std::string::npos)) {
                continue;
            }
            // 13.4
            if (expanded_property->type == IRIMappingType::Keyword) {
                // 13.4.1
                if (p.active_property.is_keyword(keyword_reverse)) {
                    return make_error(ParsingError::Type::BadSyntax, "invalid reverse property map (expanding map)");
                }
                // 13.4.2
                auto existing_entry = p.result.try_find_entry(*expanded_property);
                {
                    if (existing_entry != nullptr && expanded_property->data != keyword_type && expanded_property->data != keyword_included) {
                        return make_error(ParsingError::Type::BadSyntax, "colliding keywords");
                    }
                }
                // 13.4.3
                ExpandedMapEntry expanded_value{};
                expanded_value.is_reverse = p.reverse;
                if (expanded_property->data == keyword_id) {
                    if (!v.is_string()) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @id value");
                    }
                    auto r = context_parser.iri_expansion(p.active_ctx, static_cast<std::string_view>(v), true, false);
                    if (!r.has_value()) {
                        return r.error();
                    }
                    expanded_value.keyword_values.emplace_back(std::move(*r));
                }
                // 13.4.4
                else if (expanded_property->data == keyword_type) {
                    auto &ent = existing_entry == nullptr ? expanded_value : *existing_entry;
                    if (v.is_string()) {
                        auto r = context_parser.iri_expansion(p.type_scoped_context, static_cast<std::string_view>(v), true, true);
                        if (!r.has_value()) {
                            return r.error();
                        }
                        ent.keyword_values.emplace_back(std::move(*r));
                    } else if (v.type() == simdjson::ondemand::json_type::array) {
                        for (auto e : static_cast<simdjson::ondemand::array>(v)) {
                            auto r = context_parser.iri_expansion(p.type_scoped_context, e, true, true);
                            if (!r.has_value()) {
                                return r.error();
                            }
                            ent.keyword_values.emplace_back(std::move(*r));
                        }
                    } else {
                        return make_error(ParsingError::Type::BadSyntax, "invalid type value");
                    }
                }
                // 13.4.5
                else if (expanded_property->data == keyword_graph) {
                    expanded_value.path = p.active_path;
                    expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                    expanded_value.active_property = keyword_graph;
                }
                // 13.4.6
                else if (expanded_property->data == keyword_included) { // NOLINT(*-branch-clone)
                    expanded_value.path = p.active_path;
                    expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                }
                // 13.4.7
                else if (expanded_property->data == keyword_value) {
                    if (p.value_is_error) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @nest value (contains @value)");
                    }
                    if (p.input_type == keyword_json || v.is_scalar()) {
                        expanded_value.path = p.active_path;
                        expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                        expanded_value.is_json_literal = p.input_type == keyword_json;
                    } else {
                        return make_error(ParsingError::Type::BadSyntax, "invalid value object value");
                    }
                }
                // 13.4.8
                else if (expanded_property->data == keyword_language) {
                    if (!v.is_string()) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid language-tagged string");
                    }
                    expanded_value.path = p.active_path;
                    expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                }
                // 13.4.9
                else if (expanded_property->data == keyword_direction) {
                    if (!v.is_string()) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid language-tagged string");
                    }
                    auto s = static_cast<std::string_view>(v);
                    if (s != "ltr" && s != "rtl") {
                        return make_error(ParsingError::Type::BadSyntax, "invalid language-tagged string");
                    }
                    expanded_value.keyword_values.emplace_back(std::string{s}, IRIMappingType::None);
                }
                // 13.4.10
                else if (expanded_property->data == keyword_index) {
                    if (!v.is_string()) {
                        return make_error(ParsingError::Type::BadSyntax, " invalid @index value");
                    }
                    expanded_value.path = p.active_path;
                    expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                }
                // 13.4.11
                else if (expanded_property->data == keyword_list) {
                    if (p.active_property.type == IRIMappingType::None || p.active_property.is_keyword(keyword_graph)) {
                        continue;
                    }
                    expanded_value.path = p.active_path;
                    expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                }
                // 13.4.12
                else if (expanded_property->data == keyword_set) {
                    expanded_value.path = p.active_path;
                    expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                }
                // 13.4.13
                else if (expanded_property->data == keyword_reverse) {
                    simdjson::ondemand::object v_obj;
                    if (v.get(v_obj) != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @reverse value (not a object)");
                    }
                    KeyPath path = p.active_path;
                    path.keys.emplace_back(std::in_place_type<std::string>, k);
                    auto e = expand_level_nested_recursive({
                        .result = p.result,
                        .elem_obj = v_obj,
                        .active_ctx = p.active_ctx,
                        .type_scoped_context = p.type_scoped_context,
                        .active_path = path,
                        .active_property = p.active_property,
                        .input_type = p.input_type,
                        .base_iri = p.base_iri,
                        .reverse = !p.reverse,
                        .value_is_error = true,
                    });
                    if (e.has_value()) {
                        return e;
                    }
                }
                // 13.4.14 + 14
                else if (expanded_property->data == keyword_nest) {
                    for (ValueArrayIter iter{v}; iter != iter.end(); ++iter) {
                        simdjson::ondemand::object iter_obj;
                        if (iter->get(iter_obj) != simdjson::SUCCESS) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid @nest value (not a object)");
                        }
                        KeyPath path = p.active_path;
                        path.keys.emplace_back(std::in_place_type<std::string>, k);
                        iter.push_index(path);
                        auto e = expand_level_nested_recursive({
                            .result = p.result,
                            .elem_obj = iter_obj,
                            .active_ctx = p.active_ctx,
                            .type_scoped_context = p.type_scoped_context,
                            .active_path = path,
                            .active_property = p.active_property,
                            .input_type = p.input_type,
                            .base_iri = p.base_iri,
                            .reverse = p.reverse,
                            .value_is_error = true,
                        });
                        if (e.has_value()) {
                            return e;
                        }
                    }
                }
                // 13.4.16
                if (!expanded_value.path.keys.empty() || !expanded_value.keyword_values.empty()) {
                    expanded_value.key = std::move(*expanded_property);
                    p.result.entries.emplace_back(std::move(expanded_value));
                }
                continue;
            }
            // 13.5
            auto *term_definition = p.active_ctx.try_find_term(expanded_property->get_search_key());
            // 13.6
            ExpandedMapEntry expanded_value{};
            expanded_value.is_reverse = p.reverse;
            if (term_definition != nullptr && term_definition->type_mapping == keyword_json) {
                expanded_value.path = p.active_path;
                expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                expanded_value.is_json_literal = true;
            }
            // 13.7
            else if (term_definition != nullptr && term_definition->has_container_mapping(keyword_language) && v.type() == simdjson::ondemand::json_type::object) {
                expanded_value.path = p.active_path;
                expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                expanded_value.language_map = p.active_ctx.base_direction;
                if (term_definition != nullptr && term_definition->direction_mapping != BaseDirection::None) {
                    expanded_value.language_map = term_definition->direction_mapping;
                }
            }
            // 13.8
            else if (term_definition != nullptr && (term_definition->has_container_mapping(keyword_index) || term_definition->has_container_mapping(keyword_type) || term_definition->has_container_mapping(keyword_id))
                     && v.type() == simdjson::ondemand::json_type::object) {

                // 13.8.2
                IRIMapping index_key{std::string{keyword_index}, IRIMappingType::Keyword};
                {
                    auto *t = p.active_ctx.try_find_term(k);
                    if (t != nullptr && t->index_mapping.type != IRIMappingType::None) {
                        index_key = t->index_mapping;
                    }
                }

                // 13.8.3
                for (auto kw : simdjson::ondemand::object{v}) {
                    std::string_view index = kw.unescaped_key();
                    auto w = *kw.value();

                    Context const *map_context = nullptr;
                    // 13.8.3.1
                    if (term_definition->has_container_mapping(keyword_id) || term_definition->has_container_mapping(keyword_type)) {
                        map_context = p.active_ctx.previous_context;
                    }
                    // 13.8.3.1&3
                    if (map_context == nullptr) {
                        map_context = &p.active_ctx;
                    }

                    // 13.8.3.2
                    if (term_definition->has_container_mapping(keyword_type)) {
                        auto *index_term = map_context->try_find_term(index);
                        if (index_term != nullptr && index_term->context.has_value()) {
                            auto r = context_parser.parse_local_context(simdjson::padded_string_view{*index_term->context}, *map_context, index_term->base_iri.value_or(""));
                            if (!r.has_value()) {
                                return r.error();
                            }
                            map_context = &p.result.context_storage.emplace_front(std::move(*r));
                        }
                    }

                    // 13.8.3.4
                    auto expanded_index = context_parser.iri_expansion(p.active_ctx, index, false, true);
                    if (!expanded_index.has_value()) {
                        return expanded_index.error();
                    }

                    // 13.8.3.7
                    for (ValueArrayIter iter{w}; iter != iter.end(); ++iter) {
                        auto index_value = *iter;
                        auto ex = expanded_value;
                        ex.active_property = k;
                        ex.active_context = map_context;
                        ExpandedMap *next_lvl = nullptr;
                        std::optional<simdjson::ondemand::object> index_value_obj = std::nullopt;
                        // 13.8.3.7.1
                        if (term_definition->has_container_mapping(keyword_graph) && !is_graph_object(index_value, p.active_ctx, index_value_obj)) {
                            ex.next_level_pre_expanded = ExpandedMap{};
                            next_lvl = &std::get<ExpandedMap>(*ex.next_level_pre_expanded);
                            auto &ge = next_lvl->entries.emplace_back(IRIMapping{std::string{keyword_graph}, IRIMappingType::Keyword});

                            ex.path = p.active_path;
                            ex.path.keys.emplace_back(std::in_place_type<std::string>, k);

                            ge.path.keys.emplace_back(std::in_place_type<std::string>, index);
                            iter.push_index(ge.path);
                        } else {
                            auto nl = expand_level({
                                .active_context = *map_context,
                                .active_property = *expanded_property,
                                .element = index_value,
                                .base_iri = p.base_iri,
                                .from_map = true,
                                .element_obj = index_value_obj,
                            });
                            if (!nl.has_value()) {
                                return nl.error();
                            }
                            ex.next_level_pre_expanded = std::move(*nl);
                            if (std::holds_alternative<ExpandedMap>(*ex.next_level_pre_expanded)) {
                                next_lvl = &std::get<ExpandedMap>(*ex.next_level_pre_expanded);
                            }

                            ex.path = p.active_path;
                            ex.path.keys.emplace_back(std::in_place_type<std::string>, k);
                            ex.path.keys.emplace_back(std::in_place_type<std::string>, index);
                            iter.push_index(ex.path);
                        }
                        auto has_keyword = [&](std::string_view kwo) {
                            return next_lvl != nullptr && next_lvl->try_find_keyword(kwo) != nullptr;
                        };
                        // 13.8.3.7.2
                        if (term_definition->has_container_mapping(keyword_index) && !index_key.is_keyword(keyword_index) && !expanded_index->is_keyword(keyword_none)) {
                            auto reexpanded_index = value_expansion(p.active_ctx, index_key, index);
                            if (!reexpanded_index.has_value()) {
                                return reexpanded_index.error();
                            }
                            auto expanded_index_key = context_parser.iri_expansion(p.active_ctx, index_key.data, false, true);
                            if (!expanded_index_key.has_value()) {
                                return expanded_index_key.error();
                            }
                            if (next_lvl == nullptr) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid value object (attempting to add invalid index)");
                            }
                            auto &e = next_lvl->entries.emplace_back(std::move(*expanded_index_key));
                            e.pre_expanded_value = std::move(*reexpanded_index);
                        }
                        // 13.8.3.7.3
                        else if (term_definition->has_container_mapping(keyword_index) && !has_keyword(keyword_index)
                                 && !expanded_index->is_keyword(keyword_none)) {
                            // skipped, @index is ignored
                        }
                        // 13.8.3.7.4
                        else if (term_definition->has_container_mapping(keyword_id) && !has_keyword(keyword_id)
                                 && !expanded_index->is_keyword(keyword_none)) {
                            auto id = context_parser.iri_expansion(p.active_ctx, index, true, false);
                            if (!id.has_value()) {
                                return id.error();
                            }
                            if (next_lvl == nullptr) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid value object (attempting to add invalid id)");
                            }
                            auto &e = next_lvl->entries.emplace_back(IRIMapping{std::string{keyword_id}, IRIMappingType::Keyword});
                            e.keyword_values.emplace_back(std::move(*id));
                        }
                        // 13.8.3.7.5
                        else if (term_definition->has_container_mapping(keyword_type) && !expanded_index->is_keyword(keyword_none)) {
                            if (next_lvl == nullptr) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid value object (attempting to add invalid type)");
                            }
                            auto &e = next_lvl->entries.emplace_back(IRIMapping{std::string{keyword_type}, IRIMappingType::Keyword});
                            e.keyword_values.emplace_back(*expanded_index);
                        }
                        // 13.13
                        if (term_definition != nullptr && term_definition->is_reverse_property) {
                            ex.is_reverse = !p.reverse;
                        }
                        if (!ex.path.keys.empty() || !ex.keyword_values.empty()) {
                            ex.key = *expanded_property;
                            p.result.entries.emplace_back(std::move(ex));
                        }
                    }
                }
                continue;
            }
            // 13.10
            else if (v.is_null()) {
                continue;
            }
            // 13.9
            else {
                expanded_value.path = p.active_path;
                expanded_value.path.keys.emplace_back(std::in_place_type<std::string>, k);
                expanded_value.active_property = k;
            }
            // 13.10
            if (expanded_value.path.keys.empty() && expanded_value.keyword_values.empty()) {
                continue;
            }
            // 13.11
            if (term_definition != nullptr && term_definition->has_container_mapping(keyword_list) && !is_list_object(v, p.active_ctx)) {
                expanded_value.as_list = true;
            }
            // 13.12
            if (term_definition != nullptr && term_definition->has_container_mapping(keyword_graph)
                && !term_definition->has_container_mapping(keyword_id) && !term_definition->has_container_mapping(keyword_index)) {
                if (v.type() == simdjson::ondemand::json_type::array) {
                    expanded_value.as_multiple_graphs = true;
                } else {
                    expanded_value.as_graph = true;
                }
            }
            // 13.13
            if (term_definition != nullptr && term_definition->is_reverse_property) {
                expanded_value.is_reverse = !p.reverse;
            }
            if (!expanded_value.path.keys.empty() || !expanded_value.keyword_values.empty()) {
                expanded_value.key = std::move(*expanded_property);
                p.result.entries.emplace_back(std::move(expanded_value));
            }
        }

        return std::nullopt;
    }
    bool ExpandParser::is_list_object(simdjson::ondemand::value v, Context const &active_context) {
        if (v.type() != simdjson::ondemand::json_type::object) {
            return false;
        }
        bool list = false;
        bool index = false;
        auto ob = static_cast<simdjson::ondemand::object>(v);
        for (auto kv : ob) {
            std::string_view k = kv.unescaped_key();
            auto expanded_property = context_parser.iri_expansion(active_context, k, false, true);
            if (!expanded_property.has_value()) {
                return false;
            }
            if (expanded_property->is_keyword(keyword_list)) {
                if (list) {
                    return false;
                }
                list = true;
            } else if (expanded_property->is_keyword(keyword_index)) {
                if (index) {
                    return false;
                }
                index = true;
            } else {
                return false;
            }
        }
        return list;
    }
    bool ExpandParser::is_graph_object(simdjson::ondemand::value v, Context const &active_context, std::optional<simdjson::ondemand::object> &obj_out) {
        if (v.type() != simdjson::ondemand::json_type::object) {
            return false;
        }
        bool graph = false;
        bool index = false;
        bool id = false;
        auto ob = static_cast<simdjson::ondemand::object>(v);
        obj_out = ob;
        for (auto kv : ob) {
            std::string_view k = kv.unescaped_key();
            auto expanded_property = context_parser.iri_expansion(active_context, k, false, true);
            if (!expanded_property.has_value()) {
                ob.reset();
                return false;
            }
            if (expanded_property->is_keyword(keyword_graph)) {
                if (graph) {
                    ob.reset();
                    return false;
                }
                graph = true;
            } else if (expanded_property->is_keyword(keyword_index)) {
                if (index) {
                    ob.reset();
                    return false;
                }
                index = true;
            } else if (expanded_property->is_keyword(keyword_index)) {
                if (id) {
                    ob.reset();
                    return false;
                }
                id = true;
            } else {
                ob.reset();
                return false;
            }
        }
        ob.reset();
        return graph;
    }
}  // namespace rdf4cpp::parser::json_ld