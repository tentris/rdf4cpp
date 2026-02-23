#include "JsonLdParser.hpp"
#include <rdf4cpp/util/CharMatcher.hpp>
#include <uni_algo/all.h>

namespace rdf4cpp::parser {
    json_ld::TermDefinition *json_ld::Context::try_find_term(std::string_view key) {
        auto i = std::ranges::find_if(terms, [&](auto const& t) {return t.key == key; });
        if (i == terms.end()) {
            return nullptr;
        }
        return &*i;
    }
    json_ld::TermDefinition const *json_ld::Context::try_find_term(std::string_view key) const {
        auto i = std::ranges::find_if(terms, [&](auto const& t) {return t.key == key; });
        if (i == terms.end()) {
            return nullptr;
        }
        return &*i;
    }
    bool IStreamQuadIterator::ImplJsonLd::looks_like_keyword(std::string_view v) {
        if (v.empty()) {
            return false;
        }
        if (v.at(0) != '@') {
            return false;
        }
        static constexpr rdf4cpp::util::char_matcher_detail::ASCIIAlphaMatcher m{};
        return rdf4cpp::util::char_matcher_detail::match<m, una::views::utf8>(v.substr(1));
    }
    IStreamQuadIterator::error_type IStreamQuadIterator::ImplJsonLd::make_error(ParsingError::Type t, std::string msg) {
        return error_type{
            t,
            0, 0, // TODO
            std::move(msg),
        };
    }
    nonstd::expected<json_ld::Context, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::parse_context(simdjson::ondemand::value json, json_ld::Context const &active_context, std::string_view base_iri, bool override_protected, bool propagate) {
        // https://www.w3.org/TR/json-ld11-api/#context-processing-algorithm
        // 1
        nonstd::expected<json_ld::Context, error_type> result{active_context};

        auto handle_ctx = [&](simdjson::ondemand::object o) {
            { // 5.5
                auto [c, v] = try_get_field<double>(o, keyword_version);
                if (c != simdjson::NO_SUCH_FIELD && (c != simdjson::SUCCESS || v != 1.1)) {
                    result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid @version value")};
                    return true;
                }
            }
            { // 5.6
                auto [c, v] = try_get_field<std::string_view>(o, keyword_import);
                if (c != simdjson::NO_SUCH_FIELD) {
                    result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, c == simdjson::SUCCESS ? "invalid @import value " : "import context not supported")};
                    return true;
                }
            }
            { // 5.7
                auto [c, v] = try_get_optional_field<std::string_view>(o, keyword_base);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base IRI")};
                        return true;
                    }
                    if (!v.has_value()) {
                        result->base_iri = "";
                    }
                    else {
                        if (IRIView{*v}.is_relative()) {
                            if (state_->iri_factory.set_base(result->base_iri) != IRIFactoryError::Ok) {
                                result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base IRI")};
                                return true;
                            }
                            auto r = state_->iri_factory.from_maybe_relative_as_string(*v);
                            if (r.has_value()) {
                                result->base_iri = *r;
                            }
                            else {
                                result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base IRI")};
                                return true;
                            }
                        }
                        else {
                            if (IRIView{*v}.quick_validate() == IRIFactoryError::Ok) {
                                result->base_iri = *v;
                            }
                            else {
                                result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base IRI")};
                                return true;
                            }
                        }
                    }
                }
            }
            { // 5.12/5.13 part 1
                o.reset();
                for (auto x : o) {
                    std::string_view key = x.escaped_key();
                    if (any_of(key, {keyword_base, keyword_direction, keyword_import, keyword_language, keyword_propagate, keyword_protected,
                        keyword_version, keyword_vocab})) {
                        continue;
                    }
                    auto i = std::ranges::find_if(result->terms, [&](const auto& t) { return t.key == key; });
                    if (i == result->terms.end()) {
                        result->terms.emplace_back(key);
                    }
                    else {
                        *i = json_ld::TermDefinition{key};
                    }
                }
            }
            { // 5.8
                auto [c, v] = try_get_optional_field<std::string_view>(o, keyword_vocab);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid vocab mapping")};
                        return true;
                    }
                    if (!v.has_value()) {
                        result->vocab = std::nullopt;
                    }
                    else {
                        auto r = iri_expansion(active_context, *v, true, false, &result.value(), o);
                        if (!r.has_value() || r->type != json_ld::IRIMappingType::IRI) { // technically blank node is only deprecated, not removed
                            result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid vocab mapping")};
                            return true;
                        }
                        result->vocab = std::move(r->data);
                    }
                }
            }
            { // 5.9
                auto [c, v] = try_get_optional_field<std::string_view>(o, keyword_language);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid default language")};
                        return true;
                    }
                    if (!v.has_value()) {
                        result->language = std::nullopt;
                    }
                    else {
                        result->language = *v;
                    }
                }
            }
            { // 5.10
                auto [c, v] = try_get_optional_field<std::string_view>(o, keyword_direction);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base direction")};
                        return true;
                    }
                    if (!v.has_value()) {
                        result->base_direction = json_ld::BaseDirection::None;
                    }
                    else {
                        if (*v == "ltr") {
                            result->base_direction = json_ld::BaseDirection::Ltr;
                        }
                        else if (*v == "rtl") {
                            result->base_direction = json_ld::BaseDirection::Rtl;
                        }
                        else {
                            result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid base direction")};
                            return true;
                        }
                    }
                }
            }
            { // 5.11
                auto [c, v] = try_get_field<bool>(o, keyword_propagate);
                if (c != simdjson::NO_SUCH_FIELD && c != simdjson::SUCCESS) {
                    result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid @propagate value")};
                    return true;
                }
                // setting field earlier
            }
            { // 5.13
                auto [c, prot] = try_get_field<bool>(o, keyword_protected);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid @protected value")};
                        return true;
                    }
                }
                else {
                    prot = false;
                }

                for (auto &term : result->terms) {
                    auto e = parse_context_term(o, *result, term, active_context, prot, override_protected);
                    if (e.has_value()) {
                        result = nonstd::unexpected{e.value()};
                        return true;
                    }
                }
            }
            return false;
        };

        if (json.type() == simdjson::ondemand::json_type::object) {
            // 2 & 3
            simdjson::ondemand::object const o = json.get_object();
            auto [c, prop] = try_get_field<bool>(o, keyword_propagate);
            if (c == simdjson::SUCCESS) {
                result->propagate = prop;
            }
            else {
                result->propagate = propagate;
            }
            //error handling later

            handle_ctx(o); // 4
        }
        else {
            simdjson::ondemand::array a{};
            if (json.get(a) != simdjson::SUCCESS) {
                result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, " invalid local context")};
                return result;
            }
            for (auto v : a) {
                switch (v.type()) {
                    case simdjson::ondemand::json_type::null: // 5.1
                        v.is_null();
                        for (const auto &t : active_context.terms) {
                            if (t.is_protected) {
                                result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid context nullification")};
                                return result;
                            }
                        }
                        result = json_ld::Context{
                            .base_iri{base_iri},
                        };
                        break;
                    case simdjson::ondemand::json_type::object: // 5.4
                        if (handle_ctx(v.get_object()))
                            return result;
                        break;
                    case simdjson::ondemand::json_type::string: // 5.2
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "remote context not supported")};
                        return result;
                    default: // 5.3
                        result = nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, " invalid local context")};
                        return result;
                }
            }
        }

        return result;
    }
    std::optional<IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::parse_context_term(simdjson::ondemand::object json, json_ld::Context &local, json_ld::TermDefinition &term, json_ld::Context const &active_context, bool is_protected, bool override_protected) {
        // https://www.w3.org/TR/json-ld11-api/#create-term-definition
        // 1
        if (term.parse_state == json_ld::ParseState::Done) {
            return std::nullopt;
        }
        if (term.parse_state == json_ld::ParseState::InProgress) {
            return make_error(ParsingError::Type::BadSyntax, "cyclic IRI mapping");
        }

        // 2
        if (term.key.empty()) {
            return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
        }
        term.parse_state = json_ld::ParseState::InProgress;

        // 3
        auto [value_ec, value] = try_get_field<simdjson::ondemand::value>(json, term.key);
        if (value_ec != simdjson::SUCCESS) {
            return make_error(ParsingError::Type::BadSyntax, "unknown key?"); // should not happen
        }

        // 4
        if (term.key == keyword_type) {
            simdjson::ondemand::object ob;
            if (value.get(ob) != simdjson::SUCCESS) {
                return make_error(ParsingError::Type::BadSyntax, "keyword redefinition");
            }
            for (auto t : ob) {
                std::string_view const k = t.escaped_key();
                if (k == keyword_container) {
                    std::string_view v;
                    if (t.value().get(v) != simdjson::SUCCESS || v != keyword_set) {
                        return make_error(ParsingError::Type::BadSyntax, "keyword redefinition");
                    }
                }
                else if (k == keyword_protected) {
                    bool v;
                    if (t.value().get(v) != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "keyword redefinition");
                    }
                }
                else {
                    return make_error(ParsingError::Type::BadSyntax, "keyword redefinition");
                }
            }
        }

        // 5
        if (is_keyword(term.key)) {
            return make_error(ParsingError::Type::BadSyntax, "keyword redefinition");
        }

        // 6
        json_ld::TermDefinition const *previous = nullptr;
        {
            auto i = std::ranges::find_if(active_context.terms, [&](const auto& t) {return t.key == term.key; });
            if (i != active_context.terms.end()) {
                previous = &*i;
            }
        }

        auto handle_id = [&](std::optional<std::string_view> v) -> std::optional<error_type> {
            if (!v.has_value()) {
                term.iri_mapping = {};
            }
            else {
                auto ex = iri_expansion(active_context, *v, false, false, &local, json);
                if (!ex.has_value()) {
                    return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping");
                }
                if (ex->data == keyword_context && ex->type == json_ld::IRIMappingType::Keyword) {
                    return make_error(ParsingError::Type::BadSyntax, "invalid keyword alias");
                }
                term.iri_mapping = *ex;

                bool const has_slash = term.key.find('/') != std::string::npos;
                bool const has_colon = std::string_view(term.key).substr(0, term.key.length()-2).substr(1).find(':') != std::string_view::npos;
                if (has_colon || has_slash) {
                    term.parse_state = json_ld::ParseState::Done;
                    if (iri_expansion(active_context, term.key, false, false, &local, json) != term.iri_mapping) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping");
                    }
                }
                else if (term.is_simple) {
                    bool const iri = !term.iri_mapping.data.empty() && term.iri_mapping.type == json_ld::IRIMappingType::IRI &&
                        std::string_view(":/?#[]@").find(term.iri_mapping.data.at(term.iri_mapping.data.length()-1)) != std::string::npos;
                    if (iri || term.iri_mapping.type == json_ld::IRIMappingType::BlankNode) {
                        term.is_prefix = true;
                    }
                }
            }
            return std::nullopt;
        };

        bool skip_object = false;
        // 7
        if (value.is_null()) {
            auto e = handle_id(std::nullopt);
            if (e.has_value())
                return e;
            skip_object = true;
        }

        // 8
        if (value.is_string()) {
            term.is_simple = true;
            auto e = handle_id(static_cast<std::string_view>(value));
            if (e.has_value())
                return e;
            skip_object = true;
        }

        if (!skip_object) {
            // 9
            simdjson::ondemand::object ob;
            if (value.get(ob) != simdjson::SUCCESS) {
                return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
            }

            // 10
            term.is_simple = false;
            term.is_protected = is_protected;

            { // 11
                auto [c, v] = try_get_field<bool>(ob, keyword_protected);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @protected value");
                    }
                    is_protected = v;
                }
            }

            bool has_type = false;
            { // 12
                auto [c, v] = try_get_field<std::string_view>(ob, keyword_type);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid type mapping");
                    }
                    if (!any_of(v, {keyword_json, keyword_none, keyword_id, keyword_vocab}) && IRIView{v}.quick_validate() != IRIFactoryError::Ok) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid type mapping");
                    }
                    term.type_mapping = v;
                    has_type = true;
                }
            }

            { // 13
                auto [c, v] = try_get_field<std::string_view>(ob, keyword_reverse);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping");
                    }
                    auto [nc, va] = try_get_field<simdjson::ondemand::value>(ob, keyword_id);
                    if (nc != simdjson::NO_SUCH_FIELD) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid reverse property");
                    }
                    std::tie(nc, va) = try_get_field<simdjson::ondemand::value>(ob, keyword_nest);
                    if (nc != simdjson::NO_SUCH_FIELD) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid reverse property");
                    }

                    auto r = iri_expansion(active_context, v, false, false, &local, json);
                    term.iri_mapping = *r;


                    auto [c_cont, v_cont] = try_get_field<simdjson::ondemand::value>(ob, keyword_container);
                    if (c_cont != simdjson::NO_SUCH_FIELD) {
                        if (c_cont != simdjson::SUCCESS) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid reverse property");
                        }
                        if (v_cont.is_null()) {
                            term.container_mapping.clear();
                        }
                        else {
                            std::string_view cont;
                            if (v_cont.get(cont) != simdjson::SUCCESS) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid reverse property");
                            }
                            if (!any_of(cont, {keyword_set, keyword_index})) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid reverse property");
                            }
                            term.container_mapping.clear();
                            term.container_mapping.emplace_back(cont);
                        }
                    }

                    // TODO double check protected
                    term.is_reverse_property = true;
                    term.parse_state = json_ld::ParseState::Done;
                    return std::nullopt;
                }
            }
            { // 14
                auto [c, v] = try_get_optional_field<std::string_view>(ob, keyword_id);
                if (c != simdjson::NO_SUCH_FIELD && (!v.has_value() || *v == term.key)) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping");
                    }
                    auto e = handle_id(v);
                    if (e.has_value()) {
                        return e;
                    }
                }
                else {
                    { // 15
                        auto const colon_pos = std::string_view(term.key).substr(1).find(':');
                        if (colon_pos != std::string_view::npos) {
                            auto prefix = std::string_view(term.key).substr(0, colon_pos);
                            auto other_term = std::ranges::find_if(local.terms, [&](auto const &x) { return x.key == prefix; });
                            if (other_term != local.terms.end()) {
                                auto rec = parse_context_term(json, local, *other_term, active_context, is_protected, override_protected);
                                if (rec.has_value()) {
                                    return rec;
                                }
                                term.iri_mapping = other_term->iri_mapping;
                                term.iri_mapping.data.append(std::string_view(term.key).substr(colon_pos + 1));
                            }
                            else {
                                term.iri_mapping.data = term.key;
                                term.iri_mapping.type = std::string_view(term.key).substr(2) == "_:" ? json_ld::IRIMappingType::BlankNode : json_ld::IRIMappingType::IRI;
                            }
                        }
                        // 16
                        else if (std::string_view(term.key).find('/') != std::string_view::npos) {
                            auto m = iri_expansion(active_context, term.key, false, false, &local, json);
                            if (!m.has_value()) {
                                return m.error();
                            }
                            if (m->type != json_ld::IRIMappingType::IRI) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping");
                            }
                            term.iri_mapping = *m;
                        }
                        // 17
                        else if (term.key == keyword_type) {
                            term.iri_mapping.data = keyword_type;
                            term.iri_mapping.type = json_ld::IRIMappingType::Keyword;
                        }
                        // 18
                        else if (active_context.vocab.has_value()) {
                            term.iri_mapping.data = *active_context.vocab;
                            term.iri_mapping.data.append(term.key);
                            term.iri_mapping.type = json_ld::IRIMappingType::IRI;
                        }
                        else {
                            return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping");
                        }
                    }
                }
            }
            auto container_contains = [&](std::string_view x) {
                return std::ranges::find(term.container_mapping, x) != term.container_mapping.end();
            };
            { // 19
                auto [c, v] = try_get_field<simdjson::ondemand::value>(ob, keyword_container);
                if (c != simdjson::NO_SUCH_FIELD) {
                    auto is_valid_keyword = [](std::string_view v) {
                        return any_of(v, {keyword_graph, keyword_id, keyword_index, keyword_language, keyword_list, keyword_set, keyword_type});
                    };
                    if (v.is_string()) {
                        auto d = static_cast<std::string_view>(v);
                        if (!is_valid_keyword(d)) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                        }
                        term.container_mapping.clear();
                        term.container_mapping.emplace_back(d);
                    }
                    else {
                        simdjson::ondemand::array a;
                        if (v.get(a) != simdjson::SUCCESS) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                        }
                        term.container_mapping.clear();
                        for (auto w : a) {
                            std::string_view x;
                            if (w.get(x) != simdjson::SUCCESS) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                            }
                            if (!is_valid_keyword(x)) {
                                return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                            }
                            term.container_mapping.emplace_back(x);
                        }
                    }
                    if (term.container_mapping.empty()) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid container mapping");
                    }
                    if (term.container_mapping.size() > 1) {
                        auto only = [&](std::initializer_list<std::string_view> x) {
                            for (std::string_view const d : term.container_mapping) { // NOLINT(*-use-anyofallof)
                                if (!any_of(d, x)) {
                                    return false;
                                }
                            }
                            return true;
                        };
                        bool graph = container_contains(keyword_graph);
                        if (graph) {
                            auto id = container_contains(keyword_id);
                            auto index = container_contains(keyword_index);
                            if (index == id) { // xor
                                graph = false;
                            }
                            else {
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
                        if (!term.type_mapping.has_value()) {
                            term.type_mapping = keyword_id;
                        }
                        if (term.type_mapping != keyword_id && term.type_mapping != keyword_vocab) {
                            return make_error(ParsingError::Type::BadSyntax, "invalid type mapping");
                        }
                    }
                }
            }
            { // 20
                auto [c, v] = try_get_field<std::string_view>(ob, keyword_index);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                    if (!container_contains(keyword_index)) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                    auto r = iri_expansion(active_context, v, false, false, &local, json);
                    if (!r.has_value()) {
                        return r.error();
                    }
                    if (r->type != json_ld::IRIMappingType::IRI) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                    term.index_mapping = std::move(r->data);
                }
            }
            { // 21
                auto [c, v] = try_get_field<simdjson::ondemand::object>(ob, keyword_context);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid scoped context");
                    }
                    term.context = std::string{static_cast<std::string_view>(v.raw_json())};
                    // skip parsing it more than necessary, will be parsed when actually used
                }
            }
            if (!has_type) { // 22
                auto [c, v] = try_get_optional_field<std::string_view>(ob, keyword_language);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid language mapping");
                    }
                    term.language_mapping = v;
                }
            }
            if (!has_type) { // 23
                auto [c, v] = try_get_optional_field<std::string_view>(ob, keyword_direction);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid base direction");
                    }
                    if (!v.has_value()) {
                        term.direction_mapping = json_ld::BaseDirection::None;
                    }
                    else if (*v == "ltr") {
                        term.direction_mapping = json_ld::BaseDirection::Ltr;
                    }
                    else if (*v == "rtl") {
                        term.direction_mapping = json_ld::BaseDirection::Rtl;
                    }
                    else {
                        return make_error(ParsingError::Type::BadSyntax, "invalid base direction");
                    }
                }
            }
            { // 24
                auto [c, v] = try_get_field<std::string_view>(ob, keyword_nest);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @nest value");
                    }
                    if (is_keyword(v) && v != keyword_nest) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @nest value");
                    }
                    term.nest_value = v;
                }
            }
            { // 25
                auto [c, v] = try_get_field<bool>(ob, keyword_prefix);
                if (c != simdjson::NO_SUCH_FIELD) {
                    if (c != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid @prefix value");
                    }
                    if (term.key.find_first_of("/:") != std::string::npos) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                    term.is_prefix = v;
                    if (v && term.iri_mapping.type == json_ld::IRIMappingType::Keyword) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                    }
                }
            }
            { // 26
                ob.reset();
                for (auto e : ob) {
                    if (!any_of(e.escaped_key(), {keyword_id, keyword_reverse, keyword_container, keyword_context, keyword_direction,
                        keyword_index, keyword_language, keyword_nest, keyword_prefix, keyword_protected, keyword_type})) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid term definition");
                        }
                }
            }
        }
        { // 27
            if (!override_protected && previous != nullptr && previous->is_protected) {
                if (static_cast<const json_ld::TermDefinitionBase&>(term) != static_cast<const json_ld::TermDefinitionBase&>(*previous)) {
                    return make_error(ParsingError::Type::BadSyntax, "protected term redefinition");
                }
                term = *previous;
            }
        }
        // 28
        term.parse_state = json_ld::ParseState::Done;
        return std::nullopt;
    }
    nonstd::expected<json_ld::IRIMapping, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::iri_expansion(json_ld::Context const &active_context, std::optional<std::string_view> value, bool document_relative, bool vocab, json_ld::Context *local_context, std::optional<simdjson::ondemand::object> local_context_json) {
        // https://www.w3.org/TR/json-ld11-api/#iri-expansion
        // 1
        if (!value.has_value()) {
            return json_ld::IRIMapping{};
        }
        if (is_keyword(*value)) {
            return json_ld::IRIMapping{std::string{*value}, json_ld::IRIMappingType::Keyword};
        }
        // 2
        if (looks_like_keyword(*value)) {
            return json_ld::IRIMapping{};
        }
        // 3
        if (local_context != nullptr) {
            auto i = local_context->try_find_term(*value);
            if (i != nullptr && i->parse_state != json_ld::ParseState::Done) {
                auto e = parse_context_term(local_context_json.value(), *local_context, *i, active_context);
                if (e.has_value()) {
                    return nonstd::make_unexpected(*e);
                }
            }
        }
        // 4
        auto* in_active = active_context.try_find_term(*value);
        if (in_active != nullptr && in_active->iri_mapping.type == json_ld::IRIMappingType::Keyword) {
            return in_active->iri_mapping;
        }
        // 5
        if (vocab && in_active != nullptr) {
            return in_active->iri_mapping;
        }
        // 6
        {
            auto i = value->find(':');
            if (i != std::string_view::npos) {
                auto pre = value->substr(0, i);
                auto post = value->substr(i + 1);
                if (pre == "_") {
                    return json_ld::IRIMapping{std::string(*value), json_ld::IRIMappingType::BlankNode};
                }
                if (post.starts_with("//")) {
                    return json_ld::IRIMapping{std::string(*value), json_ld::IRIMappingType::IRI};
                }
                if (local_context != nullptr) {
                    auto term = local_context->try_find_term(pre);
                    if (term != nullptr && term->parse_state != json_ld::ParseState::Done) {
                        auto e = parse_context_term(local_context_json.value(), *local_context, *term, active_context);
                        if (e.has_value()) {
                            return nonstd::make_unexpected(*e);
                        }
                    }
                }
                auto term = active_context.try_find_term(pre);
                if (term != nullptr && term->iri_mapping.type == json_ld::IRIMappingType::None && term->is_prefix) {
                    std::string v = term->iri_mapping.data;
                    v.append(post);
                    return json_ld::IRIMapping{std::move(v), json_ld::IRIMappingType::IRI};
                }
                if (IRIView(*value).quick_validate() == IRIFactoryError::Ok) {
                    return json_ld::IRIMapping{std::string(*value), json_ld::IRIMappingType::IRI};
                }
            }
        }
        // 7
        if (vocab && active_context.vocab.has_value()) {
            std::string v = *active_context.vocab;
            v.append(*value);
            return json_ld::IRIMapping{std::move(v), json_ld::IRIMappingType::IRI};
        }
        // 8
        if (document_relative) {
            if (state_->iri_factory.set_base(active_context.base_iri) != IRIFactoryError::Ok) {
                return nonstd::make_unexpected(make_error(ParsingError::Type::BadIri, "invalid base iri"));
            }
            auto r = state_->iri_factory.from_maybe_relative_as_string(*value);
            if (r.has_value()) {
                return json_ld::IRIMapping{std::string(*r), json_ld::IRIMappingType::IRI};
            }
            else {
                return nonstd::make_unexpected(make_error(ParsingError::Type::BadIri, std::format("invalid relative iri: {}", r.error())));
            }
        }
        // 9
        // no keyword, bn or valid iri, would have passed any further check
        return json_ld::IRIMapping{std::string(""), json_ld::IRIMappingType::None};
    }
    nonstd::expected<std::variant<json_ld::IRIMapping, json_ld::LiteralMapping, json_ld::TypedLiteralMapping, simdjson::ondemand::value>,
                     IStreamQuadIterator::error_type>
    IStreamQuadIterator::ImplJsonLd::value_expansion(json_ld::Context const &active_conext,
                                                     json_ld::TermDefinition const &active_property,
                                                     simdjson::ondemand::value value) {
        if (active_property.type_mapping.has_value() && *active_property.type_mapping == keyword_id && value.is_string()) {
            return iri_expansion(active_conext, value.get_string(), true, false);
        }
        if (active_property.type_mapping.has_value() && *active_property.type_mapping == keyword_vocab && value.is_string()) {
            return iri_expansion(active_conext, value.get_string(), false, true);
        }
        if (active_property.type_mapping.has_value() && !any_of(*active_property.type_mapping, {keyword_id, keyword_vocab, keyword_none})) {
            return json_ld::TypedLiteralMapping{value, *active_property.type_mapping};
        }
        if (value.is_string()) {
            return json_ld::LiteralMapping{
                std::string(static_cast<std::string_view>(value.get_string())),
                active_property.language_mapping.has_value() ? active_property.language_mapping : active_conext.language,
                active_property.direction_mapping != json_ld::BaseDirection::None ? active_property.direction_mapping : active_conext.base_direction,
            };
        }
        return value;
    }
}  // namespace rdf4cpp::parser