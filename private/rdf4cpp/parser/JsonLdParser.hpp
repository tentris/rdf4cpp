#ifndef RDF4CPP_JSONLDPARSER_HPP
#define RDF4CPP_JSONLDPARSER_HPP

#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/Quad.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>

#include <generator>
#include <vector>
#include <forward_list>

#include <simdjson.h>

namespace rdf4cpp::parser {
    namespace json_ld {
        enum class BaseDirection : uint8_t {
            None,
            Ltr,
            Rtl,
        };
        enum class ParseState : uint8_t {
            NotStarted,
            InProgress,
            Done,
        };

        enum class IRIMappingType : uint8_t {
            None,
            IRI,
            BlankNode,
            Keyword,
        };
        struct IRIMapping {
            std::string data;
            IRIMappingType type = IRIMappingType::None;
            std::string search_key = "";

            constexpr auto operator<=>(IRIMapping const &r) const noexcept {
                return std::tie(data, type) <=> std::tie(r.data, r.type);
            }
            constexpr bool operator==(IRIMapping const &r) const noexcept {
                return (*this <=> r) == std::strong_ordering::equal;
            }

            [[nodiscard]] constexpr bool is_keyword(std::string_view k) const {
                return type == IRIMappingType::Keyword && data == k;
            }
            [[nodiscard]] constexpr std::string_view get_search_key() const {
                if (search_key.empty()) {
                    return data;
                }
                return search_key;
            }
        };
        struct TypedLiteralMapping {
            std::string value;
            std::string type;
        };
        struct LiteralMapping {
            std::string value;
            std::optional<std::string> language;
            BaseDirection direction;
        };
        using ExpandedValue = std::variant<IRIMapping, LiteralMapping, TypedLiteralMapping, simdjson::ondemand::value>;

        struct TermDefinitionBase {
            std::string key;
            IRIMapping iri_mapping;
            bool is_prefix = false;
            bool is_reverse_property = false;
            bool is_simple = false;
            std::optional<std::string> base_iri;
            std::optional<std::string> context;
            std::vector<std::string> container_mapping;
            std::optional<std::string> index_mapping;
            std::optional<std::string> language_mapping;
            std::optional<std::string> nest_value;
            std::optional<std::string> type_mapping;
            BaseDirection direction_mapping = BaseDirection::None;

            constexpr auto operator<=>(TermDefinitionBase const &) const = default;

            constexpr explicit TermDefinitionBase(std::string_view k)
                : key{k} {
            }

            [[nodiscard]] constexpr bool has_container_mapping(std::string_view m) const {
                for (const auto& e : container_mapping) {
                    if (m == e) {
                        return true;
                    }
                }
                return false;
            }
        };
        struct TermDefinition : TermDefinitionBase {
            bool is_protected = false;
            ParseState parse_state = ParseState::NotStarted;

            constexpr explicit TermDefinition(std::string_view k)
                : TermDefinitionBase{k} {
            }
        };

        struct Context {
            std::vector<TermDefinition> terms{};  // TODO map? one of the term members as key
            std::string base_iri;
            std::optional<std::string> vocab = std::nullopt;
            std::optional<std::string> language = std::nullopt;
            BaseDirection base_direction = BaseDirection::None;
            Context const* previous_context = nullptr;

            TermDefinition *try_find_term(std::string_view key);
            [[nodiscard]] TermDefinition const *try_find_term(std::string_view key) const;
        };

        struct Null {};
        struct KeyPath {
            std::vector<std::string> keys;
        };
        struct ContainerData {
            std::string_view index_key;
            TermDefinition const *term_definition;
            Context const * active_context;
            Context const * map_context;
            IRIMapping expanded_index;
            std::optional<ExpandedValue> reexpanded_index;
            IRIMapping expanded_index_key;
            IRIMapping index;
        };
        struct ExpandedMap {
            struct Entry {
                IRIMapping key;
                KeyPath path;
                std::vector<IRIMapping> keyword_values;
                std::string active_property = "";
                bool is_json_literal = false;
                bool as_list = false;
                bool as_graph = false;
                bool is_reverse = false;
                std::optional<BaseDirection> language_map = std::nullopt;
                std::optional<ContainerData> container_data = std::nullopt;
                std::optional<ExpandedValue> pre_expanded_value = std::nullopt;
                Context const *active_context = nullptr;

                constexpr bool has_keyword_value(std::string_view key) {
                    return std::ranges::any_of(keyword_values, [&](auto const& t) { return t.type == IRIMappingType::Keyword && t.data == key; });
                }

                [[nodiscard]] constexpr ContainerData const * container() const {
                    return container_data.has_value() ? &*container_data : nullptr;
                }
            };

            std::vector<Entry> entries;
            Context* active_context = nullptr;
            std::forward_list<Context> context_storage; // moving the contained objects is not allowed

            constexpr Entry* try_find_entry(IRIMapping const &key) {
                auto i = std::ranges::find_if(entries, [&](auto const& t) { return t.key == key; });
                if (i == entries.end()) {
                    return nullptr;
                }
                return &*i;
            }
            constexpr Entry* try_find_keyword(std::string_view key) {
                auto i = std::ranges::find_if(entries, [&](auto const& t)
                    { return t.key.type == IRIMappingType::Keyword && t.key.data == key; });
                if (i == entries.end()) {
                    return nullptr;
                }
                return &*i;
            }
        };
        using ExpandedLevel = std::variant<ExpandedMap, IRIMapping, LiteralMapping, TypedLiteralMapping, Null, simdjson::ondemand::value>;
    }  // namespace json_ld

    struct IStreamQuadIterator::ImplJsonLd final : Impl {
    private:
        state_type *state_;
        bool state_is_owned_;
        std::string json_data_;
        uint64_t blank_node_index = 0;

        static constexpr std::string_view keyword_base = "@base";
        static constexpr std::string_view keyword_container = "@container";
        static constexpr std::string_view keyword_context = "@context";
        static constexpr std::string_view keyword_direction = "@direction";
        static constexpr std::string_view keyword_graph = "@graph";
        static constexpr std::string_view keyword_id = "@id";
        static constexpr std::string_view keyword_import = "@import";
        static constexpr std::string_view keyword_included = "@included";
        static constexpr std::string_view keyword_index = "@index";
        static constexpr std::string_view keyword_json = "@json";
        static constexpr std::string_view keyword_language = "@language";
        static constexpr std::string_view keyword_list = "@list";
        static constexpr std::string_view keyword_nest = "@nest";
        static constexpr std::string_view keyword_none = "@none";
        static constexpr std::string_view keyword_prefix = "@prefix";
        static constexpr std::string_view keyword_propagate = "@propagate";
        static constexpr std::string_view keyword_protected = "@protected";
        static constexpr std::string_view keyword_reverse = "@reverse";
        static constexpr std::string_view keyword_set = "@set";
        static constexpr std::string_view keyword_type = "@type";
        static constexpr std::string_view keyword_value = "@value";
        static constexpr std::string_view keyword_version = "@version";
        static constexpr std::string_view keyword_vocab = "@vocab";

        // not treated as keyword in other places
        static constexpr std::string_view keyword_default = "@default";

        static constexpr std::string_view rdf_json_datatype = "http://www.w3.org/1999/02/22-rdf-syntax-ns#JSON";
        static constexpr std::string_view iri_nil = "http://www.w3.org/1999/02/22-rdf-syntax-ns#nil";
        static constexpr std::string_view iri_rest = "http://www.w3.org/1999/02/22-rdf-syntax-ns#rest";
        static constexpr std::string_view iri_first = "http://www.w3.org/1999/02/22-rdf-syntax-ns#first";

        static constexpr bool any_of(std::string_view v, std::initializer_list<std::string_view> l) {
            for (auto const x : l) {
                if (v == x) {
                    return true;
                }
            }
            return false;
        }
        static constexpr bool is_keyword(std::string_view v) {
            return any_of(v,
                          {keyword_base,
                           keyword_container,
                           keyword_context,
                           keyword_direction,
                           keyword_graph,
                           keyword_id,
                           keyword_import,
                           keyword_included,
                           keyword_index,
                           keyword_json,
                           keyword_language,
                           keyword_list,
                           keyword_nest,
                           keyword_none,
                           keyword_prefix,
                           keyword_propagate,
                           keyword_protected,
                           keyword_reverse,
                           keyword_set,
                           keyword_type,
                           keyword_value,
                           keyword_version,
                           keyword_vocab});
        }
        static bool looks_like_keyword(std::string_view v);

        template<typename T>
        static auto try_get_field(simdjson::ondemand::object o, std::string_view key) {
            T r{};
            auto c = o[key].get(r);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
            return std::tuple(c, r);
        }
        template<typename T>
        static auto try_get_field(simdjson::ondemand::object o, json_ld::KeyPath const &key) {
            if (key.keys.empty()) {
                if constexpr (std::same_as<T, simdjson::ondemand::object>) {
                    return std::tuple(simdjson::SUCCESS, o);
                }
                else {
                    return std::tuple(simdjson::INCORRECT_TYPE, T{});
                }
            }
            simdjson::error_code c;
            for (size_t i = 0; i < key.keys.size() - 1; ++i) {
                std::tie(c, o) = try_get_field<simdjson::ondemand::object>(o, key.keys[i]);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                if (c != simdjson::SUCCESS) {
                    return std::tuple(c, T{});
                }
            }
            return try_get_field<T>(o, key.keys[key.keys.size() - 1]);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
        }
        template<typename T>
        static std::tuple<simdjson::error_code, std::optional<T>> try_get_optional_field(simdjson::ondemand::object o,
                                                                                         std::string_view key) {
            simdjson::ondemand::value v;
            auto c = o[key].get(v);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
            if (c != simdjson::SUCCESS) {
                return std::tuple(c, std::nullopt);
            }
            if (v.is_null()) {
                return std::tuple(c, std::nullopt);
            }
            T r{};
            c = v.get(r);
            return std::tuple(c, r);
        }

        error_type make_error(ParsingError::Type t, std::string msg);
        json_ld::IRIMapping make_new_bn();
        nonstd::expected<IRI, error_type> make_iri(json_ld::IRIMapping const & m);
        nonstd::expected<Node, error_type> make_bn_or_iri(json_ld::IRIMapping const & m);
        nonstd::expected<ok_type, error_type> make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, json_ld::IRIMapping const &object);
        nonstd::expected<ok_type, error_type> make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, Node object);

        nonstd::expected<json_ld::Context, error_type> parse_context(simdjson::ondemand::value local_context,
                                                                     json_ld::Context const &active_context,
                                                                     std::string_view base_iri,
                                                                     bool override_protected = false,
                                                                     bool propagate = true);
        std::optional<error_type> parse_context_term(simdjson::ondemand::object local_context,
                                                     json_ld::Context &active_context,
                                                     json_ld::TermDefinition &term,
                                                     json_ld::Context const &parent_context,
                                                     bool is_protected = false,
                                                     bool override_protected = false);

        nonstd::expected<json_ld::Context, error_type> parse_local_context(simdjson::padded_string_view json,
                                                                     json_ld::Context const &active_context,
                                                                     std::string_view base_iri,
                                                                     bool override_protected = false,
                                                                     bool propagate = true);


        nonstd::expected<json_ld::IRIMapping, error_type> iri_expansion(json_ld::Context const &active_context,
                                                                        std::optional<std::string_view> value,
                                                                        bool document_relative,
                                                                        bool vocab,
                                                                        json_ld::Context *local_context = nullptr,
                                                                        std::optional<simdjson::ondemand::object> local_context_json
                                                                        = std::nullopt);

        nonstd::expected<json_ld::ExpandedValue, error_type> value_expansion(json_ld::Context const &active_conext,
                                                                             json_ld::IRIMapping const &active_property,
                                                                             simdjson::ondemand::value value);
        nonstd::expected<json_ld::ExpandedValue, error_type> value_expansion(json_ld::Context const &active_conext,
                                                                             std::string_view active_property,
                                                                             std::string_view value);

        nonstd::expected<json_ld::ExpandedLevel, error_type> expand_level(json_ld::Context const &active_context,
                                                                          json_ld::IRIMapping const &active_property,
                                                                          simdjson::ondemand::value element,
                                                                          std::string_view base_iri,
                                                                          bool frame_expansion = false,
                                                                          bool ordered = false,
                                                                          bool from_map = false,
                                                                          bool assume_no_scalar = false,
                                                                          json_ld::ContainerData const *container_data = nullptr);
        std::optional<error_type> expand_level_nested_recursive(json_ld::ExpandedMap& result,
                                                                simdjson::ondemand::object elem_obj,
                                                                json_ld::Context const & active_ctx,
                                                                json_ld::Context const & type_scoped_context,
                                                                json_ld::KeyPath const &active_path,
                                                                json_ld::IRIMapping const &active_property,
                                                                std::optional<std::string> const & input_type,
                                                                json_ld::ContainerData const *container_data);
        static std::optional<error_type> expand_transform_container(json_ld::ExpandedMap &result,
                                                             json_ld::ContainerData const *container_data);

        using result_generator = std::generator<nonstd::expected<ok_type, error_type>>;

        result_generator parse(simdjson::ondemand::value element,
                               json_ld::Context const &active_ctx,
                               std::string_view base_iri,
                               bool is_top_level = false,
                               json_ld::IRIMapping const &active_graph = {std::string{keyword_default}, json_ld::IRIMappingType::Keyword},
                               json_ld::IRIMapping const &active_subject = {},
                               json_ld::IRIMapping const &active_property = {},
                               bool is_reverse = false,
                               std::variant<std::monostate, json_ld::IRIMapping, Literal> *obj_out = nullptr,
                               json_ld::ContainerData const *container_data = nullptr);
        result_generator parse_list(simdjson::ondemand::array ar,
                               json_ld::Context const &active_ctx,
                               std::string_view base_iri,
                               json_ld::IRIMapping const &active_graph,
                               json_ld::IRIMapping const &active_subject,
                               json_ld::IRIMapping const &active_property);

        result_generator parse();

        result_generator active_generator_;
        std::ranges::iterator_t<result_generator> current_iter_;

    public:
        [[nodiscard]] std::optional<nonstd::expected<ok_type, error_type>> next() override;
        [[nodiscard]] uint64_t current_line() const noexcept override;
        [[nodiscard]] uint64_t current_column() const noexcept override;

        explicit ImplJsonLd(std::string json, state_type *initial_state = nullptr);
        ImplJsonLd(void *stream,
                   ReadFunc read,
                   ErrorFunc error,
                   EOFFunc eof,
                   state_type *initial_state = nullptr);

        ~ImplJsonLd() override;
    };
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_JSONLDPARSER_HPP
