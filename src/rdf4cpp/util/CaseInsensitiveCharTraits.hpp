#ifndef RDF4CPP_RDF_UTIL_CASEINSENSITIVECHARTRAITS_HPP
#define RDF4CPP_RDF_UTIL_CASEINSENSITIVECHARTRAITS_HPP

#include <string_view>

namespace rdf4cpp::util {

/**
 * Case insensitive char traits.
 * Identical to std::char_traits<char> except that CiCharTraits treats comparisons as case insensitive
 */
struct CiCharTraits : std::char_traits<char> {
    using char_type = std::char_traits<char>::char_type;
    using pos_type = std::char_traits<char>::pos_type;
    using int_type = std::char_traits<char>::int_type;
    using off_type = std::char_traits<char>::off_type;
    using state_type = std::char_traits<char>::state_type;
    using comparison_category = std::char_traits<char>::comparison_category;

    static bool eq(char_type const c1, char_type const c2) noexcept {
        return std::tolower(c1) == std::tolower(c2);
    }

    static bool ne(char_type const c1, char_type const c2) noexcept {
        return std::tolower(c1) != std::tolower(c2);
    }

    static bool lt(char_type const c1, char_type const c2) noexcept {
        return std::tolower(c1) < std::tolower(c2);
    }

    static int compare(char_type const *s1, char_type const *s2, size_t const len) noexcept {
        for (size_t ix = 0; ix < len; ++ix) {
            auto const c1 = std::tolower(s1[ix]);
            auto const c2 = std::tolower(s2[ix]);

            if (c1 < c2) {
                return -1;
            }

            if (c1 > c2) {
                return 1;
            }
        }

        return 0;
    }

    static char_type const *find(char_type const *s, size_t const len, char const needle) noexcept {
        auto const ineedle = std::tolower(needle);

        for (size_t ix = 0; ix < len; ++ix) {
            if (std::tolower(s[ix]) == ineedle) {
                return &s[ix];
            }
        }

        return nullptr;
    }
};

/**
 * Case insensitive view to a string
 */
using CiStringView = std::basic_string_view<char, CiCharTraits>;

}  //namespace rdf4cpp::util

#endif  //RDF4CPP_RDF_UTIL_CASEINSENSITIVECHARTRAITS_HPP
