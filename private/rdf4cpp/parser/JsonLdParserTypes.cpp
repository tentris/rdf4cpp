#include "JsonLdParserTypes.hpp"
#include <rdf4cpp/util/CharMatcher.hpp>
#include <uni_algo/all.h>

namespace rdf4cpp::parser {
    std::optional<json_ld::BaseDirection> json_ld::try_parse_base_direction(std::string_view d) {
        if (d == "ltr") {
            return BaseDirection::Ltr;
        }
        if (d == "trl") {
            return BaseDirection::Rtl;
        }
        return std::nullopt;
    }
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
}  // namespace rdf4cpp::parser
