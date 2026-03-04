#ifndef RDF4CPP_JSONLDPARSER_HPP
#define RDF4CPP_JSONLDPARSER_HPP

#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/Quad.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>

#include <vector>
#include <generator>

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

            constexpr auto operator<=>(IRIMapping const &r) const noexcept = default;
        };
        struct TypedLiteralMapping {
            simdjson::ondemand::value value;
            std::string_view type;
        };
        struct LiteralMapping {
            std::string value;
            std::optional<std::string> language;
            BaseDirection direction;
        };

        struct Context;
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
            bool propagate = true;

            TermDefinition *try_find_term(std::string_view key);
            [[nodiscard]] TermDefinition const *try_find_term(std::string_view key) const;
        };
    }  // namespace json_ld

    struct IStreamQuadIterator::ImplJsonLd final : Impl {
    private:
        state_type *state_;
        bool state_is_owned_;
        std::string json_data_;

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
        nonstd::expected<json_ld::Context, error_type> parse_context(simdjson::ondemand::value json,
                                                                     json_ld::Context const &active_context,
                                                                     std::string_view base_iri,
                                                                     bool override_protected = false,
                                                                     bool propagate = true);
        std::optional<error_type> parse_context_term(simdjson::ondemand::object json,
                                                     json_ld::Context &local,
                                                     json_ld::TermDefinition &term,
                                                     json_ld::Context const &active_context,
                                                     bool is_protected = false,
                                                     bool override_protected = false);

        nonstd::expected<json_ld::IRIMapping, error_type> iri_expansion(json_ld::Context const &active_context,
                                                                        std::optional<std::string_view> value,
                                                                        bool document_relative = false,
                                                                        bool vocab = false,
                                                                        json_ld::Context *local_context = nullptr,
                                                                        std::optional<simdjson::ondemand::object> local_context_json
                                                                        = std::nullopt);
        nonstd::expected<std::variant<json_ld::IRIMapping, json_ld::LiteralMapping, json_ld::TypedLiteralMapping, simdjson::ondemand::value>, error_type>
        value_expansion(json_ld::Context const &active_conext,
                        json_ld::TermDefinition const &active_property,
                        simdjson::ondemand::value value);

        using result_generator = std::generator<nonstd::expected<ok_type, error_type>>;

        result_generator parse();

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
