#include "RegexImpl.hpp"

namespace rdf4cpp::regex {

Regex::Impl::Impl(std::string_view const regex, flag_type const flags) : compiled_regex{make_code(regex, flags, 0)}, flags{flags} {
}

bool Regex::Impl::regex_match(std::string_view const str) const noexcept {
    auto m = get_match_data();
    return pcre2_match_8(compiled_regex.get(), reinterpret_cast<PCRE2_SPTR8>(str.data()), str.size(), 0, PCRE2_ANCHORED | PCRE2_ENDANCHORED, m.get(), nullptr) >= 0;
}

bool Regex::Impl::regex_search(std::string_view const str) const noexcept {
    auto m = get_match_data();
    return pcre2_match_8(compiled_regex.get(), reinterpret_cast<PCRE2_SPTR8>(str.data()), str.size(), 0, 0, m.get(), nullptr) >= 0;
}

Regex::Impl::match_data_ptr Regex::Impl::get_match_data() const noexcept {
    return match_data_ptr{pcre2_match_data_create_from_pattern_8(compiled_regex.get(), nullptr)};
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
    }
    else {
        f |= PCRE2_UCP;
    }
    if (flags.contains(RegexFlag::Multiline)) {
        f |= PCRE2_MULTILINE;
    }
    if (flags.contains(RegexFlag::RemoveWhitespace)) {
        f |= PCRE2_EXTENDED;
    }
    decltype(compiled_regex) r{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR8>(regex.data()), regex.size(), f, &error_code, &err_off, nullptr)};
    if (r == nullptr) {
        std::string msg;
        msg.resize(120);
        msg.resize(pcre2_get_error_message_8(error_code, reinterpret_cast<PCRE2_UCHAR8 *>(msg.data()), msg.size()));
        throw RegexError{"Failed to compile regex: " + msg};
    }
    return r;
}

}  //namespace rdf4cpp::regex
