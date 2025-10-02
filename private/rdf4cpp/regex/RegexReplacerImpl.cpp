#include "RegexReplacerImpl.hpp"

#include <cassert>
#include <cctype>
#include <sstream>

#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::regex {

namespace detail {

/**
 * Replaces the character at "pos" with '\'.
 *
 * @param s string to replace "pos" in
 * @param pos position to replace
 * @throws RegexError if the substring s.substr(pos, 2) is an invalid group reference
 */
static void replace(std::string &s, size_t pos) {
    RDF4CPP_ASSERT(s[pos] == '$');
    s[pos] = '\\';

    if (pos == s.size() - 1 || !std::isdigit(s[pos + 1])) {
        std::ostringstream oss;
        oss << "Illegal group reference at: " << pos;

        throw RegexError{oss.str()};
    }
}

/**
 * Translates a given libpcre rewrite-string into
 * the equivalent RE2 rewrite-string.
 *
 * Find all occurrences of '$' for each one:
 * check if it is escaped and if it is not translate it into the
 * equivalent construct in the RE2 replacement syntax
 *
 * @param s rewrite string in libpcre syntax
 * @return rewrite string in RE2 syntax
 */
static std::string translate_rewrite(std::string_view const s) {
    std::string res{s};

    auto pos = res.find_first_of('$');
    while (pos != std::string_view::npos) {
        if (pos == 0) {
            // is at start, cannot be escaped => replace
            replace(res, 0);
        } else {
            // find start of potential backslash sequence preceding the '$'
            size_t const end = pos;
            size_t before_start = end - 1;
            while (before_start != std::string_view::npos && res[before_start] == '\\') {
                --before_start;
            }

            if (size_t const count = end - (before_start + 1); count % 2 == 0) {
                // even number of '\' preceding '$' => not escaped => replace
                replace(res, pos);
            } else {
                // uneven number of '\' preceding '$' => escaped => remove escape char as RE2 does not require to escape '$'
                res.erase(pos - 1, 1);
                pos -= 1;
            }
        }

        pos = res.find_first_of('$', pos + 1);
    }

    return res;
}

} // namespace detail

RegexReplacer::Impl::Impl(std::shared_ptr<Regex::Impl> regex, std::string_view const rewrite) : regex{std::move(regex)},
                                                                                                rewrite{this->regex->flags.contains(RegexFlag::Literal)
                                                                                                      ? rewrite
                                                                                                      : detail::translate_rewrite(rewrite)} {
    std::string err{};
    if (!this->regex->regex.CheckRewriteString(this->rewrite, &err)) {
        throw RegexError(err);
    }
    if (this->regex->regex_match("")) {
        throw RegexError("replace matches empty string");
    }
}

void RegexReplacer::Impl::regex_replace(std::string &str) const noexcept {
    RE2::GlobalReplace(&str, this->regex->regex, this->rewrite);
}

}  // namespace rdf4cpp::regex

