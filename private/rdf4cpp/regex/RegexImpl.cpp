#include "RegexImpl.hpp"

namespace rdf4cpp::regex {

    Regex::Impl::Impl(std::string_view const regex, flag_type const flags)
        : Impl{make(regex, flags)} {
    }

    TriBool Regex::Impl::regex_match(std::string_view const str) const noexcept {
        return apply(*match, str);
    }

    TriBool Regex::Impl::regex_search(std::string_view const str) const noexcept {
        return apply(*search, str);
    }

    TriBool Regex::Impl::apply(pcre2_code_8 &c, std::string_view str) noexcept {
        match_data_ptr const m{pcre2_match_data_create_from_pattern_8(&c, nullptr)};
        auto ec =  pcre2_match_8(&c, reinterpret_cast<PCRE2_SPTR8>(str.data()), str.size(), 0, 0, m.get(), nullptr);
        if (ec == PCRE2_ERROR_NOMATCH) {
            return TriBool::False;
        }
        return ec >= 0 ? TriBool::True : TriBool::Err;
    }

    Regex::Impl::code_ptr Regex::Impl::make_code(std::string_view regex, flag_type flags, int extra_flags) {
        int error_code = 0;
        size_t err_off = 0;
        int f = PCRE2_UTF | extra_flags;
        if (flags.contains(RegexFlag::DotAll)) {
            f |= PCRE2_DOTALL;
        }
        if (flags.contains(RegexFlag::CaseInsensitive)) {
            f |= PCRE2_CASELESS;
        }
        if (flags.contains(RegexFlag::Literal)) {
            f |= PCRE2_LITERAL;
        } else {
            f |= PCRE2_UCP;
        }
        if (flags.contains(RegexFlag::Multiline)) {
            f |= PCRE2_MULTILINE;
        }
        code_ptr r{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR8>(regex.data()), regex.size(), f, &error_code, &err_off, nullptr)};
        if (r == nullptr) {
            std::string msg;
            msg.resize(120);
            msg.resize(pcre2_get_error_message_8(error_code, reinterpret_cast<PCRE2_UCHAR8 *>(msg.data()), msg.size()));
            throw RegexError{"Failed to compile regex: " + msg};
        }
        if (flags.contains(RegexFlag::Optimize)) {
            error_code = pcre2_jit_compile_8(r.get(), 0);
            if (error_code != 0) {
                std::string msg;
                msg.resize(120);
                msg.resize(pcre2_get_error_message_8(error_code, reinterpret_cast<PCRE2_UCHAR8 *>(msg.data()), msg.size()));
                throw RegexError{"Failed to compile jit regex: " + msg};
            }
        }
        return r;
    }

    std::string Regex::Impl::remove_whitespace(std::string_view str) {
        std::string r;
        r.reserve(str.size());
        uint64_t classes = 0;
        char prev = '\0';
        for (char const c : str) {
            if (c == '[' && prev != '\\') {
                ++classes;
            } else if (c == ']' && prev != '\\') {
                --classes;
            } else if (classes == 0 && (c == '\t' || c == '\r' || c == '\n' || c == ' ')) {
                continue;
            }
            r.append(1, c);
            prev = c;
        }
        return r;
    }
    Regex::Impl Regex::Impl::make(std::string_view regex, flag_type flags) {
        std::string buff = "";
        if (flags.contains(RegexFlag::RemoveWhitespace)) {
            buff = remove_whitespace(regex);
            regex = buff;
        }
        auto m = make_code(regex, flags, PCRE2_ANCHORED | PCRE2_ENDANCHORED);
        auto s = make_code(regex, flags, 0);
        return {std::move(m), std::move(s), flags};
    }
    Regex::Impl::Impl(code_ptr m, code_ptr s, flag_type f)
        : match(std::move(m)),
          search(std::move(s)),
          flags(f) {
    }

}  //namespace rdf4cpp::regex
