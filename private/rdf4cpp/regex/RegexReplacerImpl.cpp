#include "RegexReplacerImpl.hpp"

#include <rdf4cpp/Assert.hpp>
#include <uni_algo/conv.h>

namespace rdf4cpp::regex {
    std::string RegexReplacer::Impl::translate_rewrite(std::string_view const s) {
        std::string res{s};

        auto pos = res.find_first_of("$\\");
        while (pos < res.size()) {
            if (res[pos] == '\\') {  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                if (res.size() <= pos) {
                    throw RegexError{"invalid escape sequence in replacement string"};
                }
                if (res.at(pos + 1) == '$') {
                    res[pos] = '$';  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                    ++pos;
                } else if (res.at(pos + 1) == '\\') {
                    res.erase(pos, 1);
                } else {
                    throw RegexError{"incomplete escape sequence in replacement string"};
                }
            } else {  // == '$
                if (pos + 1 == res.size()) {
                    throw RegexError{"invalid capture sequence in replacement string"};
                }
            }

            pos = res.find_first_of("$\\", pos + 1);
        }

        return res;
    }

    RegexReplacer::Impl::Impl(std::shared_ptr<Regex::Impl const> regex, std::string_view const rewrite)
        : regex{std::move(regex)},
          rewrite{this->regex->flags.contains(RegexFlag::Literal)
                      ? rewrite
                      : translate_rewrite(rewrite)} {
        assert(una::is_valid_utf8(rewrite));
    }

    void RegexReplacer::Impl::regex_replace(std::string &str) const {
        assert(una::is_valid_utf8(str));
        std::string r{};
        r.resize(str.size() * 2);
        size_t outsize = 0;
        int opt = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH | PCRE2_SUBSTITUTE_GLOBAL | PCRE2_NO_UTF_CHECK;
        if (regex->flags.contains(RegexFlag::Literal)) {
            opt |= PCRE2_SUBSTITUTE_LITERAL;
        }
        auto rep = [&] {
            outsize = r.size();
            return pcre2_substitute_8(regex->search.get(), reinterpret_cast<PCRE2_SPTR8>(str.data()), str.size(), 0, opt, nullptr, &Regex::Impl::get_match_context(), reinterpret_cast<PCRE2_SPTR8>(rewrite.data()), rewrite.size(), reinterpret_cast<PCRE2_UCHAR8 *>(r.data()), &outsize);
        };
        auto e = rep();
        if (e == PCRE2_ERROR_NOMEMORY) {
            r.resize(outsize + 1);
            e = rep();
        }
        if (e < 0) {
            std::string msg;
            msg.resize(120);
            msg.resize(pcre2_get_error_message_8(e, reinterpret_cast<PCRE2_UCHAR8 *>(msg.data()), msg.size()));
            throw RegexError{"replacement error: " + msg};
        }
        r.resize(outsize);
        str = std::move(r);
    }

}  // namespace rdf4cpp::regex
