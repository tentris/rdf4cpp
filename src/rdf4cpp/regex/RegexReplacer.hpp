#ifndef RDF4CPP_RDF_REGEX_REGEXREPLACER_HPP
#define RDF4CPP_RDF_REGEX_REGEXREPLACER_HPP

#include <memory>
#include <string>

namespace rdf4cpp::regex {

struct RegexReplacer {
    using value_type = char;
private:
    friend struct Regex;

    struct Impl;
    std::unique_ptr<Impl> impl;

    explicit RegexReplacer(std::unique_ptr<Impl> &&impl) noexcept;
public:
    RegexReplacer() noexcept;
    RegexReplacer(RegexReplacer &&other) noexcept;
    RegexReplacer &operator=(RegexReplacer &&other) noexcept;
    ~RegexReplacer() noexcept;

    /**
     * Replaces all matches of the regex this replacer was constructed from in "str" with "rewrite" it
     * was constructed with.
     * Within "rewrite", dollar-escaped digits ($1 to $9) can be
     * used to insert text matching corresponding parenthesized group
     * from the pattern. $0 in "rewrite" refers to the entire matching
     *
     * @param str the string to replace all matches with
     * @warning behaviour is undefined if this replacer lives longer than the regex it was created from
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
    void regex_replace(std::string &str) const noexcept;
};

}  //namespace rdf4cpp::regex

#endif  //RDF4CPP_RDF_REGEX_REGEXREPLACER_HPP
