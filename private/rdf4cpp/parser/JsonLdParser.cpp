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
            current_line(), current_column(),
            std::move(msg),
        };
    }
    json_ld::IRIMapping IStreamQuadIterator::ImplJsonLd::make_new_bn() {
        return {
            std::format("bn_{}", blank_node_index++),
            json_ld::IRIMappingType::BlankNode,
        };
    }
    nonstd::expected<IRI, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_iri(json_ld::IRIMapping const &m) {
        if (m.type != json_ld::IRIMappingType::IRI) {
            return nonstd::make_unexpected(make_error(ParsingError::Type::BadSyntax, "invalid type"));
        }
        auto r = state_->iri_factory.create_and_validate(m.data, state_->node_storage);
        if (r.has_value()) {
            return *r;
        }
        else {
            return nonstd::make_unexpected(make_error(ParsingError::Type::BadIri, std::format("{}", r.error())));
        }
    }
    nonstd::expected<Node, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_bn_or_iri(json_ld::IRIMapping const &m) {
        if (m.type == json_ld::IRIMappingType::BlankNode) {
            return BlankNode::make(m.data, state_->node_storage);
        }
        return make_iri(m);
    }
    nonstd::expected<IStreamQuadIterator::ok_type, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, json_ld::IRIMapping const &object) {
        Node o = Node::make_null();
        auto r = make_bn_or_iri(object);
        if (r.has_value()) {
            o = *r;
        }
        else {
            return nonstd::make_unexpected(r.error());
        }
        return make_quad(graph, subject, predicate, o);
    }
    nonstd::expected<IStreamQuadIterator::ok_type, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, Node object) {
        Node g = IRI::default_graph(state_->node_storage);
        Node s = Node::make_null();
        IRI p = IRI::make_null();

        if (graph.type != json_ld::IRIMappingType::None) {
            if (graph.type == json_ld::IRIMappingType::Keyword) {
                if (graph.data != keyword_default) {
                    return nonstd::make_unexpected(make_error(ParsingError::Type::BadSyntax, "found keyword as graph"));
                }
            }
            else {
                auto r = make_bn_or_iri(graph);
                if (r.has_value()) {
                    g = *r;
                }
                else {
                    return nonstd::make_unexpected(r.error());
                }
            }
        }
        {
            auto r = make_bn_or_iri(subject);
            if (r.has_value()) {
                s = *r;
            }
            else {
                return nonstd::make_unexpected(r.error());
            }
        }
        {
            if (predicate.type == json_ld::IRIMappingType::Keyword && predicate.data == keyword_type) {
                p = IRI::rdf_type(state_->node_storage);
            }
            else {
                auto r = make_iri(predicate);
                if (r.has_value()) {
                    p = *r;
                }
                else {
                    return nonstd::make_unexpected(r.error());
                }
            }
        }

        return Quad{g, s, p, object};
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
                        auto r = iri_expansion(result.value(), v, true, false, &result.value(), o);
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
            bool p = propagate;
            if (c == simdjson::SUCCESS) {
                p = prop;
            }
            if (p && result->previous_context == nullptr) {
                result->previous_context = &active_context;
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
            return make_error(ParsingError::Type::BadSyntax, "invalid term definition (empty term)");
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
                return make_error(ParsingError::Type::BadSyntax, "keyword redefinition (@type mapped to non-map)");
            }
            for (auto t : ob) {
                std::string_view const k = t.escaped_key();
                if (k == keyword_container) {
                    std::string_view v;
                    if (t.value().get(v) != simdjson::SUCCESS || v != keyword_set) {
                        return make_error(ParsingError::Type::BadSyntax, "keyword redefinition (@type invalid @container)");
                    }
                }
                else if (k == keyword_protected) {
                    bool v;
                    if (t.value().get(v) != simdjson::SUCCESS) {
                        return make_error(ParsingError::Type::BadSyntax, "keyword redefinition (@type invalid @protected)");
                    }
                }
                else {
                    return make_error(ParsingError::Type::BadSyntax, std::format("keyword redefinition (@type invalid entry: {})", k));
                }
            }
        }

        // 5
        if (is_keyword(term.key)) {
            return make_error(ParsingError::Type::BadSyntax, std::format("keyword redefinition ({})", term.key));
        }

        // 6
        json_ld::TermDefinition const *previous_definition = nullptr;
        {
            auto i = std::ranges::find_if(active_context.terms, [&](const auto& t) {return t.key == term.key; });
            if (i != active_context.terms.end()) {
                previous_definition = &*i;
            }
        }

        auto handle_id = [&](std::optional<std::string_view> v) -> std::optional<error_type> {
            if (!v.has_value()) {
                term.iri_mapping = {};
            }
            else {
                auto ex = iri_expansion(active_context, v, false, false, &local, json);
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
                return make_error(ParsingError::Type::BadSyntax, "invalid term definition (value not null, string or map)");
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
                    auto type = iri_expansion(local, v, false, false, &local, json);
                    if (!type.has_value()) {
                        return type.error();
                    }
                    if (type->type != json_ld::IRIMappingType::Keyword && type->type != json_ld::IRIMappingType::IRI) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid type mapping (not IRI or keyword)");
                    }
                    if (type->type == json_ld::IRIMappingType::Keyword && !any_of(v, {keyword_json, keyword_none, keyword_id, keyword_vocab})) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid type mapping (invalid keyword)");
                    }
                    term.type_mapping = std::move(type->data);
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
                        return make_error(ParsingError::Type::BadSyntax, "invalid reverse property (contains id)");
                    }
                    std::tie(nc, va) = try_get_field<simdjson::ondemand::value>(ob, keyword_nest);
                    if (nc != simdjson::NO_SUCH_FIELD) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid reverse property (contains nest)");
                    }

                    auto r = iri_expansion(active_context, v, false, false, &local, json);
                    if (!r.has_value()) {
                        return r.error();
                    }
                    if (r->type != json_ld::IRIMappingType::IRI && r->type != json_ld::IRIMappingType::BlankNode) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid IRI mapping");
                    }
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
                if (c != simdjson::NO_SUCH_FIELD && v != term.key) {
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
                    simdjson::pad(*term.context);
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
            if (!override_protected && previous_definition != nullptr && previous_definition->is_protected) {
                if (static_cast<const json_ld::TermDefinitionBase&>(term) != static_cast<const json_ld::TermDefinitionBase&>(*previous_definition)) {
                    return make_error(ParsingError::Type::BadSyntax, "protected term redefinition");
                }
                term = *previous_definition;
            }
        }
        // 28
        term.parse_state = json_ld::ParseState::Done;
        return std::nullopt;
    }
    nonstd::expected<json_ld::Context, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::parse_local_context(simdjson::padded_string_view json, json_ld::Context const &active_context, std::string_view base_iri, bool override_protected, bool propagate) {
        simdjson::ondemand::parser parser{};
        simdjson::ondemand::document doc = parser.iterate(json);
        return parse_context(doc, active_context, base_iri, override_protected, propagate);
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
    nonstd::expected<json_ld::ExpandedValue, IStreamQuadIterator::error_type>
    IStreamQuadIterator::ImplJsonLd::value_expansion(json_ld::Context const &active_conext,
                                                     json_ld::IRIMapping const &active_property,
                                                     simdjson::ondemand::value value) {
        // https://www.w3.org/TR/json-ld11-api/#value-expansion
        json_ld::TermDefinition const *active_term = active_conext.try_find_term(active_property.data);

        // 1
        if (active_term != nullptr && active_term->type_mapping.has_value() && *active_term->type_mapping == keyword_id && value.is_string()) {
            return iri_expansion(active_conext, value.get_string(), true, false);
        }
        // 2
        if (active_term != nullptr && active_term->type_mapping.has_value() && *active_term->type_mapping == keyword_vocab && value.is_string()) {
            return iri_expansion(active_conext, value.get_string(), false, true);
        }
        // 4
        if (active_term != nullptr && active_term->type_mapping.has_value() && !any_of(*active_term->type_mapping, {keyword_id, keyword_vocab, keyword_none})) {
            std::string d{};
            if (value.type() == simdjson::ondemand::json_type::number || value.type() == simdjson::ondemand::json_type::boolean) {
                d = static_cast<std::string_view>(value.raw_json());
            }
            else if (value.type() == simdjson::ondemand::json_type::string) {
                d = static_cast<std::string_view>(value.get_string());
            }
            else {
                return nonstd::unexpected{make_error(ParsingError::Type::BadSyntax, "invalid type for literal")};
            }
            return json_ld::TypedLiteralMapping{d, *active_term->type_mapping};
        }
        // 5
        if (value.is_string()) {
            return json_ld::LiteralMapping{
                std::string(static_cast<std::string_view>(value.get_string())),
                active_term != nullptr && active_term->language_mapping.has_value() ? active_term->language_mapping : active_conext.language,
                active_term != nullptr && active_term->direction_mapping != json_ld::BaseDirection::None ? active_term->direction_mapping : active_conext.base_direction,
            };
        }
        return value;
    }
    nonstd::expected<json_ld::ExpandedLevel, IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::expand_level(json_ld::Context const &active_context,
                                                                                                                            json_ld::IRIMapping const &active_property,
                                                                                                                            simdjson::ondemand::value element,
                                                                                                                            std::string_view base_iri,
                                                                                                                            bool frame_expansion,
                                                                                                                            bool ordered,
                                                                                                                            bool from_map,
                                                                                                                            bool assume_no_scalar) {
        static constexpr auto from_value_expansion = [](nonstd::expected<json_ld::ExpandedValue, error_type> v) -> nonstd::expected<json_ld::ExpandedLevel, error_type> {
            if (v.has_value()) {
                return std::visit([]<typename T>(T &&t) { return json_ld::ExpandedLevel{std::forward<T>(t)}; }, std::move(*v));
            }
            return nonstd::unexpected{std::move(v.error())};
        };

        // https://www.w3.org/TR/json-ld11-api/#expansion-algorithm
        // 1
        if (!assume_no_scalar && element.is_null()) {
            return json_ld::Null{};
        }
        // 2
        if (active_property.is_keyword(keyword_default)) {
            frame_expansion = false;
        }
        // 3
        json_ld::TermDefinition const *active_term = active_context.try_find_term(active_property.data);
        auto property_scoped_context = active_term != nullptr && active_term->context.has_value() ? &*active_term->context : nullptr;
        auto* active_ctx = &active_context;
        // 4
        if (element.is_scalar()) {
            if (active_property.type == json_ld::IRIMappingType::None || active_property.is_keyword(keyword_graph)) {
                return json_ld::Null{};
            }
            std::optional<json_ld::Context> ctx = std::nullopt;
            if (property_scoped_context != nullptr) {
                auto r = parse_local_context(simdjson::padded_string_view{*property_scoped_context}, active_context, active_term->base_iri.has_value() ? *active_term->base_iri : base_iri);
                if (!r.has_value()) {
                    return nonstd::unexpected(r.error());
                }
                ctx = std::move(*r);
                active_ctx = &*ctx;
            }
            // no need to keep the context alive
            return from_value_expansion(value_expansion(*active_ctx, active_property, element));
        }
        // 5
        if (element.type() == simdjson::ondemand::json_type::array) {
            // TODO
        }
        // 6
        auto elem_object = static_cast<simdjson::ondemand::object>(element);
        // 7
        if (active_ctx->previous_context != nullptr && !from_map) {
            bool clear = true;
            for (auto e : elem_object) {
                auto x = iri_expansion(*active_ctx, e.escaped_key());
                if (x.has_value() && x->type == json_ld::IRIMappingType::Keyword && any_of(x->data, {keyword_value, keyword_id})) {
                    clear = false;
                    break;
                }
            }
            elem_object.reset();
            if (clear) {
                active_ctx = active_ctx->previous_context;
            }
        }
        // 8
        json_ld::ExpandedMap result{};
        if (property_scoped_context != nullptr) {
            auto r = parse_local_context(simdjson::padded_string_view{*property_scoped_context}, *active_ctx, active_term->base_iri.has_value() ? *active_term->base_iri : base_iri, true);
            if (!r.has_value()) {
                return nonstd::unexpected(r.error());
            }
            result.active_context = std::move(*r);
            active_ctx = &*result.active_context;
        }
        // 9
        {
            auto [c, v] = try_get_field<simdjson::ondemand::value>(elem_object, keyword_context);
            if (c != simdjson::NO_SUCH_FIELD) {
                if (c != simdjson::SUCCESS) {
                    return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid context"));
                }
                auto r = parse_context(v, *active_ctx, base_iri);
                if (!r.has_value()) {
                    return nonstd::unexpected(r.error());
                }
                result.active_context = std::move(*r);
                active_ctx = &*result.active_context;
            }
        }
        // 10
        auto* type_scoped_ctx = active_ctx;
        // 11
        std::optional<std::string> input_type = std::nullopt;
        {
            std::vector<std::string> types{};
            elem_object.reset();
            for (auto kv : elem_object) {
                std::string_view k = kv.unescaped_key();
                auto ex = iri_expansion(*active_ctx, k);
                if (ex.has_value() && ex->type == json_ld::IRIMappingType::Keyword && ex->data == keyword_type) {
                    types.emplace_back(k);
                }
            }
            std::ranges::sort(types);
            elem_object.reset();

            auto handle_type = [&](std::string_view v) -> std::optional<error_type> {
                auto* term = type_scoped_ctx->try_find_term(v);
                if (term == nullptr || !term->context.has_value()) {
                    return std::nullopt;
                }
                auto r = parse_local_context(simdjson::padded_string_view{*term->context}, *active_ctx, base_iri, false, false);
                if (!r.has_value()) {
                    return r.error();
                }
                result.active_context = std::move(*r);
                active_ctx = &*result.active_context;
                return std::nullopt;
            };
            for (std::string_view const k : types) {
                if (!input_type.has_value()) {
                    input_type = k;
                }
                auto [c, v] = try_get_field<simdjson::ondemand::value>(elem_object, k);
                if (c == simdjson::SUCCESS) {
                    if (v.is_string()) {
                        auto e = handle_type(v.get_string());
                        if (e.has_value()) {
                            return nonstd::unexpected(*e);
                        }
                    }
                    else {
                        simdjson::ondemand::array a;
                        if (v.get(a) == simdjson::SUCCESS) {
                            for (auto e : a) {
                                if (e.is_string()) {
                                    auto err = handle_type(v.get_string());
                                    if (err.has_value()) {
                                        return nonstd::unexpected(*err);
                                    }
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
            auto r = expand_level_nested_recursive(result, elem_object, *active_ctx, *type_scoped_ctx, {}, active_property, input_type);
            if (r.has_value()) {
                return nonstd::unexpected(*r);
            }
        }

        // 15
        elem_object.reset();
        {
            auto* val = result.try_find_keyword(keyword_value);
            if (val != nullptr) {
                for (const auto& e : result.entries) {
                    if (e.key.type != json_ld::IRIMappingType::Keyword
                        || !any_of(e.key.data, {keyword_direction, keyword_index, keyword_language, keyword_type, keyword_value})) {
                        return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid value object"));
                    }
                }
                auto* type = result.try_find_keyword(keyword_type);
                auto [c, val_obj] = try_get_field<simdjson::ondemand::value>(elem_object, val->path);
                if (c != simdjson::SUCCESS) {
                    return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid value object"));
                }
                if (type != nullptr && (type->is_json_literal || type->has_keyword_value(keyword_json))) { // TODO double check if equivalent
                    std::string_view const s = val_obj.raw_json();
                    return json_ld::TypedLiteralMapping{std::string{s}, std::string{rdf_json_datatype}};
                }
                if (val_obj.is_null()) {
                    return json_ld::Null{};
                }
                auto* lang = result.try_find_keyword(keyword_language);
                if (lang != nullptr) {
                    if (val_obj.is_string()) {
                        auto [ec, lang_obj] = try_get_field<std::string_view>(elem_object, lang->path);
                        if (ec != simdjson::SUCCESS) {
                            return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid language-tagged value"));
                        }
                        json_ld::BaseDirection const d = json_ld::BaseDirection::None;
                        return json_ld::LiteralMapping{
                            std::string{std::string_view{val_obj.get_string()}},
                            std::string{lang_obj},
                            d,
                        };
                    }
                }
                std::string v{};
                simdjson::ondemand::json_type const ty = val_obj.type();
                if (val_obj.is_string()) {
                    v = std::string_view{val_obj.get_string()};
                }
                else if (ty == simdjson::ondemand::json_type::number || ty == simdjson::ondemand::json_type::boolean) {
                    v = std::string_view{val_obj.raw_json_token()};
                }
                if (type != nullptr) {
                    if (type->keyword_values.size() != 1 || type->keyword_values[0].type != json_ld::IRIMappingType::IRI) { // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                        return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid typed value"));
                    }
                    return json_ld::TypedLiteralMapping{
                        std::move(v),
                        std::move(type->keyword_values[0].data), // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                    };
                }
                return json_ld::LiteralMapping{
                    std::move(v),
                    std::nullopt,
                    json_ld::BaseDirection::None,
                };
            }
        }
        // 17
        {
            if (result.try_find_keyword(keyword_set) != nullptr || result.try_find_keyword(keyword_list) != nullptr) {
                size_t expected_size = 1;
                if (result.try_find_keyword(keyword_index) != nullptr) {
                    expected_size  = 2;
                }
                if (result.entries.size() != expected_size) {
                    return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "invalid set or list object"));
                }
                auto* set = result.try_find_keyword(keyword_set);
                if (set != nullptr) {
                    auto [c, el] = try_get_field<simdjson::ondemand::value>(elem_object, set->path);
                    if (c != simdjson::SUCCESS) {
                        return nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "set path not found?"));
                    }
                    return expand_level(active_context, active_property, el, base_iri, frame_expansion, ordered, from_map);
                }
            }
        }
        // 18
        if (result.try_find_keyword(keyword_language) != nullptr && result.entries.size() == 1) {
            return json_ld::Null{};
        }
        // 19
        if (active_property.type == json_ld::IRIMappingType::None || active_property.is_keyword(keyword_graph)) {
            if (result.entries.empty()) {
                return json_ld::Null{};
            }
            if (result.entries.size() == 1 && (result.try_find_keyword(keyword_value) != nullptr || result.try_find_keyword(keyword_list) != nullptr|| result.try_find_keyword(keyword_id) != nullptr)) {
                return json_ld::Null{};
            }
        }
        // 20
        return result;
    }
    std::optional<IStreamQuadIterator::error_type> IStreamQuadIterator::ImplJsonLd::expand_level_nested_recursive(json_ld::ExpandedMap &result, simdjson::ondemand::object elem_obj, json_ld::Context const &active_ctx, json_ld::Context const & type_scoped_context, json_ld::KeyPath const &active_path, json_ld::IRIMapping const &active_property, std::optional<std::string> const & input_type) {
        for (auto kv : elem_obj) {
            // 13.1
            std::string_view k = kv.unescaped_key();
            auto v = kv.value();
            if (k == keyword_context) {
                continue;
            }
            // 13.2
            auto expanded_property = iri_expansion(active_ctx, k);
            if (!expanded_property.has_value()) {
                return expanded_property.error();
            }
            // 13.3
            if (expanded_property->type == json_ld::IRIMappingType::None
                || !(expanded_property->type == json_ld::IRIMappingType::Keyword
                || expanded_property->data.find(':') != std::string::npos)) {
                continue;
            }
            // 13.4
            if (expanded_property->type == json_ld::IRIMappingType::Keyword) {
                // 13.4.1
                if (active_property.is_keyword(keyword_reverse)) {
                    return make_error(ParsingError::Type::BadSyntax, "invalid reverse property map (expanding map)");
                }
                // 13.4.2
                auto existing_entry = result.try_find_entry(*expanded_property);
                {
                    if (existing_entry != nullptr && !any_of(expanded_property->data, {keyword_type, keyword_included})) {
                        return make_error(ParsingError::Type::BadSyntax, "colliding keywords");
                    }
                }
                // 13.4.3
                json_ld::ExpandedMap::Entry expanded_value{};
                if (expanded_property->data == keyword_id) {
                    if (!v.is_string()) { // TODO double check frame expansion
                        return make_error(ParsingError::Type::BadSyntax, "invalid @id value");
                    }
                    auto r = iri_expansion(active_ctx, static_cast<std::string_view>(v), true, false);
                    if (!r.has_value()) {
                        return r.error();
                    }
                    expanded_value.keyword_values.emplace_back(std::move(*r));
                }
                // 13.4.4
                else if (expanded_property->data == keyword_type) {
                    auto& ent = existing_entry == nullptr ? expanded_value : *existing_entry;
                    if (v.is_string()) {
                        auto r = iri_expansion(type_scoped_context, static_cast<std::string_view>(v), true);
                        if (!r.has_value()) {
                            return r.error();
                        }
                        ent.keyword_values.emplace_back(std::move(*r));
                    }
                    else if (v.type() == simdjson::ondemand::json_type::array) {
                        for (auto e : static_cast<simdjson::ondemand::array>(v)) {
                            auto r = iri_expansion(type_scoped_context, e, true);
                            if (!r.has_value()) {
                                return r.error();
                            }
                            ent.keyword_values.emplace_back(std::move(*r));
                        }
                    }
                    else {
                        return make_error(ParsingError::Type::BadSyntax, "invalid type value");
                    }
                }
                // 13.4.5
                else if (expanded_property->data == keyword_graph) {
                    expanded_value.path = active_path;
                    expanded_value.path.keys.emplace_back(k);
                    expanded_value.active_property = keyword_graph;
                }
                // 13.4.6
                else if (expanded_property->data == keyword_included) {
                    expanded_value.path = active_path;
                    expanded_value.path.keys.emplace_back(k);
                }
                // 13.4.7
                else if (expanded_property->data == keyword_value) {
                    if (input_type == keyword_json || v.is_scalar()) {
                        expanded_value.path = active_path;
                        expanded_value.path.keys.emplace_back(k);
                        expanded_value.is_json_literal = input_type == keyword_json;
                    }
                    else {
                        return make_error(ParsingError::Type::BadSyntax, "invalid value object value");
                    }
                }
                // 13.4.8
                else if (expanded_property->data == keyword_language) {
                    if (!v.is_string()) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid language-tagged string");
                    }
                    expanded_value.path = active_path;
                    expanded_value.path.keys.emplace_back(k);
                }
                // 13.4.9
                else if (expanded_property->data == keyword_direction) {
                    if (!v.is_string()) {
                        return make_error(ParsingError::Type::BadSyntax, "invalid language-tagged string");
                    }
                    auto s = static_cast<std::string_view>(v);
                    if (s != "ltr" && s != "trl") {
                        return make_error(ParsingError::Type::BadSyntax, "invalid language-tagged string");
                    }
                    expanded_value.path = active_path;
                    expanded_value.path.keys.emplace_back(k);
                }
                // 13.4.10
                else if (expanded_property->data == keyword_index) {
                    if (!v.is_string()) {
                        return make_error(ParsingError::Type::BadSyntax, " invalid @index value");
                    }
                    expanded_value.path = active_path;
                    expanded_value.path.keys.emplace_back(k);
                }
                // 13.4.11
                else if (expanded_property->data == keyword_list) {
                    if (active_property.type == json_ld::IRIMappingType::None || active_property.is_keyword(keyword_graph)) {
                        continue;
                    }
                    expanded_value.path = active_path;
                    expanded_value.path.keys.emplace_back(k);
                }
                // 13.4.12/13
                else if (expanded_property->data == keyword_set || expanded_property->data == keyword_reverse) {
                    expanded_value.path = active_path;
                    expanded_value.path.keys.emplace_back(k);
                }
                // 13.4.14 + 14
                else if (expanded_property->data == keyword_nest) {
                    json_ld::KeyPath p = active_path;
                    p.keys.emplace_back(k);
                    auto e = expand_level_nested_recursive(result, v, active_ctx, type_scoped_context, p, active_property, input_type);
                    if (e.has_value()) {
                        return e;
                    }
                }
                // 13.4.16
                if (!expanded_value.path.keys.empty() || !expanded_value.keyword_values.empty()) {
                    expanded_value.key = std::move(*expanded_property);
                    result.entries.emplace_back(std::move(expanded_value));
                }
                continue;
            }
            // 13.5
            auto* term_definition = active_ctx.try_find_term(expanded_property->data);
            // 13.6
                json_ld::ExpandedMap::Entry expanded_value{};
            if (term_definition != nullptr && term_definition->type_mapping == keyword_json) {
                expanded_value.path = active_path;
                expanded_value.path.keys.emplace_back(k);
                expanded_value.is_json_literal = true;
            }
            // 13.7
            else if (term_definition != nullptr && term_definition->has_container_mapping(keyword_language)) {
                expanded_value.path = active_path;
                expanded_value.path.keys.emplace_back(k);
                expanded_value.language_map = active_ctx.base_direction;
                if (term_definition != nullptr && term_definition->direction_mapping != json_ld::BaseDirection::None) {
                    expanded_value.language_map = term_definition->direction_mapping;
                }
            }
            // 13.8
            else if (term_definition != nullptr && (term_definition->has_container_mapping(keyword_index)
                || term_definition->has_container_mapping(keyword_type) || term_definition->has_container_mapping(keyword_id))
                && v.type() == simdjson::ondemand::json_type::object) {
                continue; // TODO
            }
            // 13.10
            else if (v.is_null()) {
                continue;
            }
            // 13.9
            else {
                expanded_value.path = active_path;
                expanded_value.path.keys.emplace_back(k);
                expanded_value.active_property = k;
            }
            // 13.10
            if (expanded_value.path.keys.empty() && expanded_value.keyword_values.empty()) {
                continue;
            }
            // 13.11
            if (term_definition != nullptr && term_definition->has_container_mapping(keyword_list)) {
                expanded_value.as_list = true;
            }
            // 13.12
            if (term_definition != nullptr && term_definition->has_container_mapping(keyword_graph)
                && !term_definition->has_container_mapping(keyword_id) && !term_definition->has_container_mapping(keyword_index)) {
                expanded_value.as_graph = false;
            }
            // 13.13
            if (term_definition != nullptr && term_definition->is_reverse_property) {
                expanded_value.is_reverse = true;
            }
            if (!expanded_value.path.keys.empty() || !expanded_value.keyword_values.empty()) {
                expanded_value.key = std::move(*expanded_property);
                result.entries.emplace_back(std::move(expanded_value));
            }
        }

        return std::nullopt;
    }
    IStreamQuadIterator::ImplJsonLd::result_generator IStreamQuadIterator::ImplJsonLd::parse(simdjson::ondemand::value element,
                                                                                             json_ld::Context const &active_ctx,
                                                                                             std::string_view base_iri,
                                                                                             bool assume_no_scalar,
                                                                                             json_ld::IRIMapping const &active_graph,
                                                                                             json_ld::IRIMapping const &active_subject,
                                                                                             json_ld::IRIMapping const &active_property) {
        // follows https://www.w3.org/TR/json-ld11-api/#node-map-generation very roughly
        // 1
        if (element.type() == simdjson::ondemand::json_type::array) {
            for (simdjson::ondemand::value e : static_cast<simdjson::ondemand::array>(element)) {
                co_yield std::ranges::elements_of(parse(e, active_ctx, base_iri, false, active_graph, active_subject, active_property));
                co_return;
            }
        }
        auto expanded = expand_level(active_ctx, active_property, element, base_iri, false, false, false, assume_no_scalar);
        if (!expanded.has_value()) {
            co_yield nonstd::unexpected(expanded.error());
            co_return;
        }
        if (std::holds_alternative<json_ld::Null>(*expanded)) {
            co_return;
        }

        // 4
        if (std::holds_alternative<json_ld::LiteralMapping>(*expanded)) {
            auto& t = std::get<json_ld::LiteralMapping>(*expanded);
            auto l = t.language.has_value() ? Literal::make_lang_tagged(t.value, *t.language) : Literal::make_simple(t.value);
            co_yield make_quad(active_graph, active_subject, active_property, l);
            co_return;
        }
        if (std::holds_alternative<json_ld::TypedLiteralMapping>(*expanded)) {
            auto& t = std::get<json_ld::TypedLiteralMapping>(*expanded);
            auto dt = make_iri({std::move(t.type), json_ld::IRIMappingType::IRI});
            if (!dt.has_value()) {
                co_yield nonstd::unexpected(dt.error());
                co_return;
            }
            auto l = Literal::make_typed(t.value, *dt, state_->node_storage);
            co_yield make_quad(active_graph, active_subject, active_property, l);
            co_return;
        }

        if (std::holds_alternative<json_ld::ExpandedMap>(*expanded)) {
            auto& map = std::get<json_ld::ExpandedMap>(*expanded);
            const auto& ctx = map.active_context.value_or(active_ctx);

            { // 5
                auto list = map.try_find_keyword(keyword_list);
                if (list != nullptr) {
                    // TODO
                    co_return;
                }
            }

            // 6
            {
                json_ld::IRIMapping id{};
                auto id_entry = map.try_find_keyword(keyword_id);
                if (id_entry != nullptr && !id_entry->keyword_values.empty()) {
                    id = std::move(id_entry->keyword_values[0]);
                }
                if (id.type == json_ld::IRIMappingType::None) {
                    id = make_new_bn();
                }
                if (active_subject.type != json_ld::IRIMappingType::None && active_property.type != json_ld::IRIMappingType::None) {
                    co_yield make_quad(active_graph, active_subject, active_property, id);
                }

                {
                    auto type_entry = map.try_find_keyword(keyword_type);
                    if (type_entry != nullptr) {
                        for (const auto &e : type_entry->keyword_values) {
                            co_yield make_quad(active_graph, id, type_entry->key, e);
                        }
                    }
                }
                {
                    auto graph_entry = map.try_find_keyword(keyword_graph);
                    if (graph_entry != nullptr) {
                        auto [c, v] = try_get_field<simdjson::ondemand::value>(element, graph_entry->path);
                        if (c != simdjson::SUCCESS) {
                            co_yield nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "could not find graph value?"));
                        }
                        else {
                            co_yield std::ranges::elements_of(parse(v, ctx, base_iri, false, id));
                        }
                    }
                }
                {
                    auto included_entry = map.try_find_keyword(keyword_included);
                    if (included_entry != nullptr) {
                        auto [c, v] = try_get_field<simdjson::ondemand::value>(element, included_entry->path);
                        if (c != simdjson::SUCCESS) {
                            co_yield nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "could not find included value?"));
                        }
                        else {
                            co_yield std::ranges::elements_of(parse(v, ctx, base_iri, false, active_graph));
                        }
                    }
                }

                for (const auto &e : map.entries) {
                    if (e.key.type != json_ld::IRIMappingType::Keyword) {
                        auto [c, v] = try_get_field<simdjson::ondemand::value>(element, e.path);
                        if (c != simdjson::SUCCESS) {
                            co_yield nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "could not find property value?"));
                        }
                        else {
                            co_yield std::ranges::elements_of(parse(v, ctx, base_iri, false, active_graph, id, e.key));
                        }
                    }
                }
            }
        }

        co_return;
    }
    IStreamQuadIterator::ImplJsonLd::result_generator IStreamQuadIterator::ImplJsonLd::parse() {
        simdjson::ondemand::parser parser{};
        simdjson::ondemand::document doc = parser.iterate(json_data_);
        if (doc.is_scalar()) {
            co_yield nonstd::unexpected(make_error(ParsingError::Type::BadSyntax, "free floating scalar"));
            co_return;
        }
        json_ld::Context ctx{};
        ctx.base_iri = state_->iri_factory.get_base();
        co_yield std::ranges::elements_of(parse(doc, ctx, ctx.base_iri, true));
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
        return 0; // TODO
    }
    uint64_t IStreamQuadIterator::ImplJsonLd::current_column() const noexcept {
        return 0; // TODO
    }
    IStreamQuadIterator::ImplJsonLd::ImplJsonLd(std::string json, state_type *initial_state)
        : state_(initial_state == nullptr ? new state_type() : initial_state), state_is_owned_(initial_state == nullptr),
        json_data_(std::move(json)), current_iter_(parse().begin()) {
    }
    IStreamQuadIterator::ImplJsonLd::ImplJsonLd(void *stream, ReadFunc read, ErrorFunc error, EOFFunc eof, state_type *initial_state) :
        ImplJsonLd([&]() {
            std::string r{};
            while (error(stream) == 0 && eof(stream) == 0) {
                static constexpr size_t s = 1024;
                auto i = r.size();
                r.append(s, '\0');
                auto n = read(&r[i], 1, s, stream); // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                r.resize(i + n);
            }
            return r;
        }(), initial_state){
    }
    IStreamQuadIterator::ImplJsonLd::~ImplJsonLd() {
        if (state_is_owned_) {
            delete state_;
        }
    }
}  // namespace rdf4cpp::parser