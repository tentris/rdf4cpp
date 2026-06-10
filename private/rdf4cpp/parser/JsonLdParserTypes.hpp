#ifndef RDF4CPP_JSONLDPARSERTYPES_HPP
#define RDF4CPP_JSONLDPARSERTYPES_HPP

#include "rdf4cpp/parser/ParsingError.hpp"


#include <rdf4cpp/parser/JsonLdParserPath.hpp>

#include <forward_list>
#include <string_view>
#include <vector>

#include <simdjson.h>

namespace rdf4cpp::parser {
    // ReSharper disable CppEvaluationInternalFailure
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
    // ReSharper restore CppEvaluationInternalFailure

    static constexpr bool is_keyword(std::string_view v) {
        static constexpr std::array all_kw = {
            keyword_base,
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
            keyword_vocab,
        };
        return std::ranges::any_of(all_kw, [&](std::string_view k) {
            return v == k;
        });
    }
    bool looks_like_keyword(std::string_view v);

    namespace json_ld {
        struct Null {
            constexpr auto operator<=>(Null const &) const noexcept = default;
        };
        struct NotSet {
            constexpr auto operator<=>(NotSet const &) const = default;
        };

        enum class BaseDirection : uint8_t {
            None,
            Ltr,
            Rtl,
        };
        std::optional<BaseDirection> try_parse_base_direction(std::string_view d);

        // corresponds to the defined map from the context parsing algorithms.
        enum class ParseState : uint8_t {
            // not in the map
            NotStarted,
            // false
            InProgress,
            // true
            Done,
        };

        enum class IRIMappingType : uint8_t {
            None,
            IRI,
            BlankNode,
            Keyword,
        };
        /**
         * result of the https://www.w3.org/TR/json-ld11-api/#iri-expansion algorythm.
         * contrary to its name, not necessarily a IRI.
         */
        struct IRIMapping {
            std::string data;
            IRIMappingType type = IRIMappingType::None;
            std::string search_key = "";

            // ReSharper disable once CppDFAUnreachableFunctionCall
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
        struct StringLikeLiteralMapping {
            std::string value;
            std::optional<std::string> language;
            BaseDirection direction;
        };

        // result of the value expansion function
        using ExpandedValue = std::variant<IRIMapping, StringLikeLiteralMapping, TypedLiteralMapping, simdjson::ondemand::value>;

        /**
         * Null is used for no language tag (raw xsd:string)
         * NotSet is used to indicate no value, use the next value from the hierarchy.
         */
        struct LanguageMapping : std::variant<NotSet, Null, std::string> {
            using Base = std::variant<NotSet, Null, std::string>;

            using Base::Base;
            using Base::operator=;
            constexpr auto operator<=>(LanguageMapping const &) const = default;

            constexpr LanguageMapping const &operator||(LanguageMapping const &other) const noexcept {
                if (std::holds_alternative<NotSet>(*this)) {
                    return other;  // NOLINT(*-return-const-ref-from-parameter)
                }
                return *this;
            }

            [[nodiscard]] constexpr std::optional<std::string> output() const {
                return std::visit([]<typename T>(T const &e) -> std::optional<std::string> {
                    if constexpr (std::same_as<T, std::string>) {
                        return e;
                    } else {
                        return std::nullopt;
                    }
                }, *this);
            }
        };

        // part of the term definition that needs to be compared for protection checks
        struct TermDefinitionBase {
            std::string key;
            IRIMapping iri_mapping;
            std::optional<std::string> base_iri;
            // needs to be padded during parent context parse
            std::optional<std::string> context;
            std::vector<std::string> container_mapping;
            IRIMapping index_mapping;
            LanguageMapping language_mapping = NotSet{};
            std::optional<std::string> nest_value;
            std::optional<std::string> type_mapping;
            BaseDirection direction_mapping = BaseDirection::None;
            bool is_prefix = false;
            bool is_reverse_property = false;
            bool is_simple = false;

            constexpr auto operator<=>(TermDefinitionBase const &) const = default;

            constexpr explicit TermDefinitionBase(std::string_view k)
                : key{k} {
            }

            [[nodiscard]] constexpr bool has_container_mapping(std::string_view m) const {
                return std::ranges::any_of(container_mapping, [&](auto const &e) {
                    return e == m;
                });
            }
        };
        struct TermDefinition : TermDefinitionBase {
            bool is_protected = false;
            bool needs_context_check = false;
            ParseState parse_state = ParseState::NotStarted;

            using TermDefinitionBase::TermDefinitionBase;
        };

        struct Context {
            std::vector<TermDefinition> terms{};
            std::string base_iri;
            std::optional<std::string> vocab = std::nullopt;
            Context const *previous_context = nullptr;
            LanguageMapping language = NotSet{};
            BaseDirection base_direction = BaseDirection::None;

            TermDefinition *try_find_term(std::string_view key);
            [[nodiscard]] TermDefinition const *try_find_term(std::string_view key) const;
        };


        struct ExpandedMapEntry;
        struct ExpandedMap {
            std::vector<ExpandedMapEntry> entries;
            Context *active_context = nullptr;
            // moving the contained objects is not allowed
            std::forward_list<Context> context_storage;
            // if set, this map was expanded from something that is not a map (example: a string being converted to a map containing only an @id entry)
            // therefore parse may not cast the input element to an object
            // if set, only entries consisting of keyword_values may be in this map
            bool expanded_from_no_map = false;

            constexpr ExpandedMapEntry *try_find_entry(IRIMapping const &key);
            constexpr ExpandedMapEntry *try_find_keyword(std::string_view key);
        };
        // result of the expand level function
        using ExpandedLevel = std::variant<ExpandedMap, StringLikeLiteralMapping, TypedLiteralMapping, Null, simdjson::ondemand::value, simdjson::ondemand::array>;


        struct ExpandedMapEntry {
            IRIMapping key;
            KeyPath path;
            std::vector<IRIMapping> keyword_values;
            std::string active_property = "";
            bool is_json_literal = false;
            bool as_list = false;
            bool as_graph = false;
            bool is_reverse = false;
            bool as_multiple_graphs = false;
            std::optional<IRIMapping> as_named_graph = std::nullopt;
            std::optional<BaseDirection> language_map = std::nullopt;
            std::optional<ExpandedValue> pre_expanded_value = std::nullopt;
            Context const *active_context = nullptr;
            std::optional<ExpandedLevel> next_level_pre_expanded = std::nullopt;

            constexpr bool has_keyword_value(std::string_view k) {
                return std::ranges::any_of(keyword_values, [&](auto const &t) {
                    return t.type == IRIMappingType::Keyword && t.data == k;
                });
            }
        };

        constexpr ExpandedMapEntry *ExpandedMap::try_find_entry(IRIMapping const &key) {
            auto i = std::ranges::find_if(entries, [&](auto const &t) {
                return t.key == key;
            });
            if (i == entries.end()) {
                return nullptr;
            }
            return &*i;
        }
        constexpr ExpandedMapEntry *ExpandedMap::try_find_keyword(std::string_view key) {
            auto i = std::ranges::find_if(entries, [&](auto const &t) {
                return t.key.type == IRIMappingType::Keyword && t.key.data == key;
            });
            if (i == entries.end()) {
                return nullptr;
            }
            return &*i;
        }

        struct StringifyResult {
            std::string value;
            std::string_view datatype;
        };

        // if passed in value is an array, iterates over its content
        // otherwise iterates over [value]
        struct ValueArrayIter {
        private:
            simdjson::ondemand::array a_{};
            std::variant<std::monostate, simdjson::ondemand::value, simdjson::ondemand::array_iterator> current_;
            size_t current_index_ = 0;
            simdjson::ondemand::value cache_;

        public:
            explicit ValueArrayIter(simdjson::ondemand::value v);
            simdjson::ondemand::value& operator*();
            simdjson::ondemand::value* operator->();
            ValueArrayIter& operator++();
            bool operator==(std::default_sentinel_t);
            ValueArrayIter& begin();
            std::default_sentinel_t end();
            // if this is iterating over an array, add its key to the path (so a path pointing at the array now points at the current object).
            // otherwise do nothing (as the key path is already pointing directly at the object).
            void push_index(json_ld::KeyPath& p);
        };

        [[nodiscard]] ParsingError make_error(ParsingError::Type t, std::string msg);
    }  // namespace json_ld
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_JSONLDPARSERTYPES_HPP
