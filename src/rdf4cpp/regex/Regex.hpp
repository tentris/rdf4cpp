#ifndef RDF4CPP_RDF_REGEX_REGEX_HPP
#define RDF4CPP_RDF_REGEX_REGEX_HPP

#include <string_view>
#include <memory>

#include <rdf4cpp/regex/RegexError.hpp>
#include <rdf4cpp/regex/RegexFlags.hpp>
#include <rdf4cpp/regex/RegexReplacer.hpp>
#include <rdf4cpp/TriBool.hpp>

namespace rdf4cpp::regex {

struct Regex {
    using value_type = char;
    using flag_type = RegexFlags;
    using replacer_type = RegexReplacer;

private:
    friend struct RegexReplacer;

    struct Impl;
    std::shared_ptr<Impl const> impl_;

public:
    /**
     * Tries to compile a regex.
     * The syntax follows google's RE2 library.
     *
     * @param regex the regex to compile
     * @param flags flags to configure the regex behaviour
     * @throws RegexError if the regex compilation was not successful
     */
    explicit Regex(std::string_view regex, flag_type flags = flag_type::none());

    /**
     * Similar to std::regex_match. Tries to match this regex against the
     * whole string.
     *
     * @param str the string to match against
     * @return true if the regex matched the whole string, false otherwise
     *
     * @example
     * @code
     * Regex const r{"[0-9]+"};
     * assert(!r.regex_search("123456789a"));
     * @endcode
     */
    [[nodiscard]] TriBool regex_match(std::string_view str) const noexcept;

    /**
     * Similar to std::regex_search. Tries to match this regex against a subsequence
     * of str.
     *
     * @param str the string to match against
     * @return true if the regex matched any substring in str, false otherwise
     *
     * @example
     * @code
     * Regex const r{"bra"};
     * assert(r.regex_search("abracadabra"));
     * @endcode
     */
    [[nodiscard]] TriBool regex_search(std::string_view str) const noexcept;

    /**
     * Constructs a regex replacer for this regex by possibly compiling
     * the rewrite string.
     *
     * @param rewrite the string to replace all matches with
     * @throws RegexError if an invalid rewrite string is encountered
     * @warning behaviour is undefined if the replacer lives longer than the regex it was created from
     *
     * @example
     * @code
     * Regex const r{"[0-9]+"};
     * RegexReplacer const repl = r.make_replacer("$0th");
     *
     * std::string s = "Hello 13 World";
     * repl.regex_replace(s);
     *
     * assert(s == "Hello 13th World);
     * @endcode
     */
    [[nodiscard]] replacer_type make_replacer(std::string_view rewrite) const;
};

}  //namespace rdf4cpp::regex

#endif  //RDF4CPP_RDF_REGEX_REGEX_HPP
