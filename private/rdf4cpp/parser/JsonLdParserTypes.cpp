#include "JsonLdParserTypes.hpp"
#include <rdf4cpp/util/CharMatcher.hpp>
#include <uni_algo/all.h>

namespace rdf4cpp::parser {
    std::optional<json_ld::BaseDirection> json_ld::try_parse_base_direction(std::string_view d) {
        if (d == "ltr") {
            return BaseDirection::Ltr;
        }
        if (d == "rtl") {
            return BaseDirection::Rtl;
        }
        return std::nullopt;
    }

    json_ld::TermDefinition *json_ld::Context::try_find_term(std::string_view key) {
        auto i = std::ranges::find_if(terms, [&](auto const &t) {
            return t.key == key;
        });
        if (i == terms.end()) {
            return nullptr;
        }
        return &*i;
    }

    json_ld::TermDefinition const *json_ld::Context::try_find_term(std::string_view key) const {
        auto i = std::ranges::find_if(terms, [&](auto const &t) {
            return t.key == key;
        });
        if (i == terms.end()) {
            return nullptr;
        }
        return &*i;
    }

    bool looks_like_keyword(std::string_view v) {
        if (v.size() <= 1) {
            return false;
        }
        if (v.at(0) != '@') {
            return false;
        }
        static constexpr util::char_matcher_detail::ASCIIAlphaMatcher m{};
        return rdf4cpp::util::char_matcher_detail::match<m, una::views::utf8>(v.substr(1));
    }

    namespace json_ld {
        ValueArrayIter::ValueArrayIter(simdjson::ondemand::value v) {
            if (v.type() == simdjson::ondemand::json_type::array) {
                a_ = v;
                current_ = a_.begin();
            } else {
                current_ = v;
            }
        }

        simdjson::ondemand::value &ValueArrayIter::operator*() {
            cache_ = std::visit([]<typename T>(T &t) -> simdjson::ondemand::value {
                if constexpr (std::same_as<T, simdjson::ondemand::value>) {
                    return t;
                } else if constexpr (std::same_as<T, simdjson::ondemand::array_iterator>) {
                    return *t;
                } else {
                    return {};
                }
            }, current_);
            return cache_;
        }

        simdjson::ondemand::value *ValueArrayIter::operator->() {
            return &**this;
        }

        ValueArrayIter &ValueArrayIter::operator++() {
            std::visit([&]<typename T>(T &t) {
                if constexpr (std::same_as<T, simdjson::ondemand::value>) {
                    current_ = std::monostate{};
                } else if constexpr (std::same_as<T, simdjson::ondemand::array_iterator>) {
                    ++t;
                    ++current_index_;
                }
            }, current_);
            return *this;
        }

        bool ValueArrayIter::operator==(std::default_sentinel_t) {
            return std::visit([&]<typename T>(T &t) -> bool {
                if constexpr (std::same_as<T, simdjson::ondemand::value>) {
                    return false;
                } else if constexpr (std::same_as<T, simdjson::ondemand::array_iterator>) {
                    return t == a_.end();
                } else {
                    return true;
                }
            }, current_);
        }

        ValueArrayIter &ValueArrayIter::begin() {
            return *this;
        }

        std::default_sentinel_t ValueArrayIter::end() {
            return std::default_sentinel;
        }

        void ValueArrayIter::push_index(json_ld::KeyPath &p) {
            return std::visit([&]<typename T>(T &) {
                if constexpr (std::same_as<T, simdjson::ondemand::array_iterator>) {
                    p.keys.emplace_back(current_index_);
                }
            }, current_);
        }

        ParsingError make_error(ParsingError::Type t, std::string msg) {
            return ParsingError{
                t,
                0, 0,
                std::move(msg),
            };
        }
    }  // namespace json_ld
}  // namespace rdf4cpp::parser
