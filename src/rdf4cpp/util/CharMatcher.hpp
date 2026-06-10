#ifndef RDF4CPP_CHARMATCHER_HPP
#define RDF4CPP_CHARMATCHER_HPP

#include <array>
#include <optional>
#include <string_view>

#include <rdf4cpp/datatypes/registry/util/ConstexprString.hpp>

/**
 * purpose-build for IRIView::quick_validate and only included there.
 */
namespace rdf4cpp::util::char_matcher_detail {
struct CharRange {
    char first = '\0';
    char last = '\0';
};

template<typename T>
concept CharMatcher = requires(T const a, int c) {
    {
        a.match(c)
    } -> std::convertible_to<bool>;
    {
        T::simd_range_num
    } -> std::convertible_to<size_t>;
    {
        T::fail_if_unicode
    } -> std::convertible_to<bool>;
    {
        a.simd_ranges()
    } -> std::same_as<std::array<CharRange, T::simd_range_num>>;
    {
        a.simd_singles()
    } -> std::convertible_to<std::string_view>;
};

/**
 * tries to match each char of data to be in one of the ranges or contained in single.
 * fails (and returns std::nullopt) if non ascii is found.
 * each size variant needs to be explicitly specified (because it needs its own highway dispatch table).
 * @param data
 * @param ranges each range [first, last] needs to contain at least one character, first may also not be '\0'
 * @param single
 * @return
 */
template<size_t rn, size_t sn>
std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, rn> const &ranges, datatypes::registry::util::ConstexprString<sn> const &single) = delete;

template<>
std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<1> const &single);
template<>
std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<4> const &single);
template<>
std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<18> const &single);
template<>
std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<20> const &single);
template<>
std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<21> const &single);
template<>
std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 1> const &ranges, datatypes::registry::util::ConstexprString<1> const &single);
template<>
std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 2> const &ranges, datatypes::registry::util::ConstexprString<1> const &single);

/**
 * tries to check if data contains any of match.
 * @param data
 * @param match may not contain '\0'
 * @return
 */
template<size_t n>
bool contains_any(std::string_view data, datatypes::registry::util::ConstexprString<n> const &match) = delete;
template<>
bool contains_any(std::string_view data, datatypes::registry::util::ConstexprString<5> const &match);

/**
 * matches, if any char in pattern matches. does compare char by char, so no utf8.
 */
template<size_t n>
struct ASCIIPatternMatcher {
    datatypes::registry::util::ConstexprString<n> pattern;

    explicit constexpr ASCIIPatternMatcher(char const (&str)[n]) noexcept
        : pattern(str) {
    }

    [[nodiscard]] constexpr bool match(int c) const noexcept {
        auto ch = static_cast<char>(c);
        if (c != static_cast<int>(ch))  // not asciii
            return false;
        return static_cast<std::string_view>(pattern).find(ch) != std::string_view::npos;
    }

    static constexpr size_t simd_range_num = 0;
    static constexpr bool fail_if_unicode = true;
    [[nodiscard]] static consteval std::array<CharRange, simd_range_num> simd_ranges() noexcept {
        return {};
    }
    [[nodiscard]] consteval auto simd_singles() const noexcept {
        return pattern;
    }
};

/**
 * matches ascii numbers 0-9
 */
struct ASCIINumMatcher {
    [[nodiscard]] static constexpr bool match(int c) noexcept {
        auto ch = static_cast<char>(c);
        if (c != static_cast<int>(ch))  // not asciii
            return false;
        return c >= '0' && c <= '9';
    }

    static constexpr size_t simd_range_num = 1;
    static constexpr bool fail_if_unicode = true;
    [[nodiscard]] static consteval std::array<CharRange, simd_range_num> simd_ranges() noexcept {
        return std::array<CharRange, simd_range_num>{CharRange{'0', '9'}};
    }
    [[nodiscard]] static consteval auto simd_singles() noexcept {
        return datatypes::registry::util::ConstexprString("");
    }
};
/**
 * matches ascii a-z or A-Z
 */
struct ASCIIAlphaMatcher {
    [[nodiscard]] static constexpr bool match(int c) noexcept {
        auto ch = static_cast<char>(c);
        if (c != static_cast<int>(ch))  // not asciii
            return false;
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    static constexpr size_t simd_range_num = 2;
    static constexpr bool fail_if_unicode = true;
    [[nodiscard]] static consteval std::array<CharRange, simd_range_num> simd_ranges() noexcept {
        return std::array<CharRange, simd_range_num>{
                CharRange{'a', 'z'},
                CharRange{'A', 'Z'}};
    }
    [[nodiscard]] static consteval auto simd_singles() noexcept {
        return datatypes::registry::util::ConstexprString("");
    }
};

/**
 * matches if a or b matches.
 * @tparam A
 * @tparam B
 */
template<CharMatcher A, CharMatcher B>
struct OrMatcher {
    A a;
    B b;

    constexpr OrMatcher(A a, B b) : a(a), b(b) {}
    constexpr OrMatcher() = default;

    [[nodiscard]] constexpr bool match(int c) const noexcept {
        return a.match(c) || b.match(c);
    }

    static constexpr size_t simd_range_num = A::simd_range_num + B::simd_range_num;
    static constexpr bool fail_if_unicode = A::fail_if_unicode && B::fail_if_unicode;
    [[nodiscard]] consteval std::array<CharRange, simd_range_num> simd_ranges() const noexcept {
        std::array<CharRange, simd_range_num> r{};
        if constexpr (A::simd_range_num > 0) {
            auto aa = a.simd_ranges();
            for (size_t s = 0; s < A::simd_range_num; ++s) {
                r[s] = aa[s];
            }
        }
        if constexpr (B::simd_range_num > 0) {
            auto ba = b.simd_ranges();
            for (size_t s = 0; s < B::simd_range_num; ++s) {
                r[s + A::simd_range_num] = ba[s];
            }
        }
        return r;
    }
    [[nodiscard]] consteval auto simd_singles() const noexcept {
        return a.simd_singles() + b.simd_singles();
    }
};

/**
 * shortcut to create a OrMatcher.
 * @tparam A
 * @tparam B
 * @param a
 * @param b
 * @return
 */
template<CharMatcher A, CharMatcher B>
constexpr OrMatcher<A, B> operator|(A a, B b) {
    return OrMatcher{a, b};
}

/**
 * matches ascii 0-9, a-z or A-Z.
 */
constexpr auto ascii_alphanum_matcher = ASCIIAlphaMatcher{} | ASCIINumMatcher{};

/**
 * matches ucschar of the IRI specification.
 */
struct UCSCharMatcher {
    [[nodiscard]] static constexpr bool match(int c) noexcept {
        return (c >= 0xA0 && c <= 0xD7FF) ||
               (c >= 0xF900 && c <= 0xFDCF) ||
               (c >= 0xFDF0 && c <= 0xFFEF) ||
               (c >= 0x10000 && c <= 0x1FFFD) ||
               (c >= 0x20000 && c <= 0x2FFFD) ||
               (c >= 0x30000 && c <= 0x3FFFD) ||
               (c >= 0x40000 && c <= 0x4FFFD) ||
               (c >= 0x50000 && c <= 0x5FFFD) ||
               (c >= 0x60000 && c <= 0x6FFFD) ||
               (c >= 0x70000 && c <= 0x7FFFD) ||
               (c >= 0x80000 && c <= 0x8FFFD) ||
               (c >= 0x90000 && c <= 0x9FFFD) ||
               (c >= 0xA0000 && c <= 0xAFFFD) ||
               (c >= 0xB0000 && c <= 0xBFFFD) ||
               (c >= 0xC0000 && c <= 0xCFFFD) ||
               (c >= 0xD0000 && c <= 0xDFFFD) ||
               (c >= 0xE0000 && c <= 0xEFFFD);
    }

    static constexpr size_t simd_range_num = 0;
    static constexpr bool fail_if_unicode = false;
    [[nodiscard]] static consteval std::array<CharRange, simd_range_num> simd_ranges() noexcept {
        return {};
    }
    [[nodiscard]] static consteval auto simd_singles() noexcept {
        return datatypes::registry::util::ConstexprString("");
    }
};

/**
 * matches iunreserved of the IRI specification.
 */
constexpr auto i_unreserved_matcher = ascii_alphanum_matcher | ASCIIPatternMatcher{"-._~"} | UCSCharMatcher{};
/**
 * matches sub-delim of the IRI (and URI) specification.
 */
constexpr auto sub_delims_matcher = ASCIIPatternMatcher{"!$&'()*+,;="};

/**
 * matches iprivate of the IRI specification.
 */
struct IPrivateMatcher {
    [[nodiscard]] static constexpr bool match(int c) noexcept {
        return (c >= 0xE000 && c <= 0xF8FF) ||
               (c >= 0xF0000 && c <= 0xFFFFD) ||
               (c >= 0x100000 && c <= 0x10FFFD);
    }

    static constexpr size_t simd_range_num = 0;
    static constexpr bool fail_if_unicode = false;
    [[nodiscard]] static consteval std::array<CharRange, simd_range_num> simd_ranges() noexcept {
        return {};
    }
    [[nodiscard]] static consteval auto simd_singles() noexcept {
        return datatypes::registry::util::ConstexprString("");
    }
};

/**
 * Matches the unicode part (the characters listed as numbers) of PN_CHARS_BASE of the Turtle/SPARQL specification
 */
struct PNCharsBase_UnicodePartMatcher {
    [[nodiscard]] static constexpr bool match(int c) noexcept {
        return (c >= 0xC0 && c <= 0xD6) ||
               (c >= 0xD8 && c <= 0xF6) ||
               (c >= 0xF8 && c <= 0x02FF) ||
               (c >= 0x0370 && c <= 0x037D) ||
               (c >= 0x037F && c <= 0x1FFF) ||
               (c >= 0x200C && c <= 0x200D) ||
               (c >= 0x2070 && c <= 0x218F) ||
               (c >= 0x2C00 && c <= 0x2FEF) ||
               (c >= 0x3001 && c <= 0xD7FF) ||
               (c >= 0xF900 && c <= 0xFDCF) ||
               (c >= 0xFDF0 && c <= 0xFFFD) ||
               (c >= 0x00010000 && c <= 0x000EFFFF);
    }

    static constexpr size_t simd_range_num = 0;
    static constexpr bool fail_if_unicode = false;
    [[nodiscard]] static consteval std::array<CharRange, simd_range_num> simd_ranges() noexcept {
        return {};
    }
    [[nodiscard]] static consteval auto simd_singles() noexcept {
        return datatypes::registry::util::ConstexprString("");
    }
};

/**
 * matches PN_CHARS_BASE of the Turtle/SPARQL specification
 */
constexpr auto PNCharsBaseMatcher = ASCIIAlphaMatcher{} | PNCharsBase_UnicodePartMatcher{};

/**
 * matches PN_CHARS_U of the Turtle/SPARQL specificiation
 */
constexpr auto PNCharsUMatcher = ASCIIPatternMatcher{"_"} | PNCharsBaseMatcher;

/**
 * Matches the unicode part (the characters listed as numbers) of PN_CHARS of the Turtle/SPARQL specification
 */
struct PNChars_UnicodePartMatcher {
    [[nodiscard]] static constexpr bool match(int c) noexcept {
        return c == 0xB7 ||
               (c >= 0x0300 && c <= 0x036F) ||
               (c >= 0x203F && c <= 0x2040);
    }

    static constexpr size_t simd_range_num = 0;
    static constexpr bool fail_if_unicode = false;
    [[nodiscard]] static consteval std::array<CharRange, simd_range_num> simd_ranges() noexcept {
        return {};
    }
    [[nodiscard]] static consteval auto simd_singles() noexcept {
        return datatypes::registry::util::ConstexprString("");
    }
};

/**
 * matches PN_CHARS of the Turtle/SPARQL specification.
 */
constexpr auto PNCharsMatcher = ASCIINumMatcher{} | ASCIIPatternMatcher{"-"} | PNCharsUMatcher | PNChars_UnicodePartMatcher{};

namespace xml {
    /**
     * Matches the unicode part (the characters listed as numbers) of NCNameStartChar of the XML specification
     */
    struct NCNameStartChar_UnicodePartMatcher {
        [[nodiscard]] static constexpr bool match(int c) noexcept {
            return (c >= 0xC0 && c <= 0xD6) ||
                   (c >= 0xD8 && c <= 0xF6) ||
                   (c >= 0xF8 && c <= 0x2FF) ||
                   (c >= 0x370 && c <= 0x37D) ||
                   (c >= 0x37F && c <= 0x1FFF) ||
                   (c >= 0x200C && c <= 0x200D) ||
                   (c >= 0x2070 && c <= 0x218F) ||
                   (c >= 0x2C00 && c <= 0x2FEF) ||
                   (c >= 0x3001 && c <= 0xD7FF) ||
                   (c >= 0xF900 && c <= 0xFDCF) ||
                   (c >= 0xFDF0 && c <= 0xFFFD) ||
                   (c >= 0x10000 && c <= 0xEFFFF);
        }

        static constexpr size_t simd_range_num = 0;
        static constexpr bool fail_if_unicode = false;
        [[nodiscard]] static consteval std::array<CharRange, simd_range_num> simd_ranges() noexcept {
            return {};
        }
        [[nodiscard]] static consteval auto simd_singles() noexcept {
            return datatypes::registry::util::ConstexprString("");
        }
    };


    /**
     * Matches the unicode part (the characters listed as numbers) of NCNameChar of the XML specification
     */
    struct NCNameChar_UnicodePartMatcher {
        [[nodiscard]] static constexpr bool match(int c) noexcept {
            return c == 0xB7 ||
                   (c >= 0x0300 && c <= 0x036F) ||
                   (c >= 0x203F && c <= 0x2040);
        }

        static constexpr size_t simd_range_num = 0;
        static constexpr bool fail_if_unicode = false;
        [[nodiscard]] static consteval std::array<CharRange, simd_range_num> simd_ranges() noexcept {
            return {};
        }
        [[nodiscard]] static consteval auto simd_singles() noexcept {
            return datatypes::registry::util::ConstexprString("");
        }
    };

    constexpr auto NCNameStartChar = ASCIIAlphaMatcher{} | ASCIIPatternMatcher{"_"} | NCNameStartChar_UnicodePartMatcher{};
    constexpr auto NCNameChar = ASCIIAlphaMatcher{} | ASCIINumMatcher{} | ASCIIPatternMatcher{"_-."} | NCNameStartChar_UnicodePartMatcher{} | NCNameChar_UnicodePartMatcher{};
}

/**
  * iterates over s and tries to match all in m.
  * attempts to do an ASCII SIMD match first, if that does not decide the matching, decodes the utf-8 and matches char by char.
  * @tparam m a CharMatcher
  * @tparam utf8_range_decoder needs to be una::views::utf8 (if i include it here, una technically becomes part of our public API)
  * @param s
  * @return
  */
template<auto const &m, auto utf8_range_decoder>
bool match(std::string_view s) noexcept {
    auto ranges = m.simd_ranges();
    static constexpr auto singles = m.simd_singles();
    auto simd_r = try_match_simd(s, ranges, singles);
    if (simd_r.has_value()) {
        return *simd_r;
    }
    if constexpr (m.fail_if_unicode) {
        return false;
    }
    for (int c : s | utf8_range_decoder) {
        if (!m.match(c)) {
            return false;
        }
    }
    return true;
}
} // namespace rdf4cpp::util::char_matcher_detail

#endif  //RDF4CPP_CHARMATCHER_HPP
