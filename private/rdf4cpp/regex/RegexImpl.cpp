#include "RegexImpl.hpp"

#include <uni_algo/conv.h>

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

    pcre2_match_context_8 &Regex::Impl::get_match_context() {
        using match_ctx_ptr = std::unique_ptr<pcre2_match_context_8, CallFree<pcre2_match_context_free_8>>;
        static auto match_ctx = []() {
            match_ctx_ptr r{pcre2_match_context_create_8(nullptr)};
            pcre2_set_heap_limit_8(r.get(), 32); // 32 KB
            pcre2_set_match_limit_8(r.get(), 100000);
            // jit stack limit is 32 KB (to change it, match_ctx needs to be thread local, then add a jit stack)
            return r;
        }();
        return *match_ctx;
    }
    std::string Regex::Impl::translate_error_code(int error_code) {
        std::string msg;
        msg.resize(120); // https://pcre2project.github.io/pcre2/doc/pcre2api/#geterrormessage says 120 chars is enough for any error message
        msg.resize(pcre2_get_error_message_8(error_code, reinterpret_cast<PCRE2_UCHAR8 *>(msg.data()), msg.size()));
        return msg;
    }

    TriBool Regex::Impl::apply(pcre2_code_8 &c, std::string_view str) noexcept {
        assert(una::is_valid_utf8(str));
        match_data_ptr const m{pcre2_match_data_create_from_pattern_8(&c, nullptr)};
        auto ec =  pcre2_match_8(&c, reinterpret_cast<PCRE2_SPTR8>(str.data()), str.size(), 0, PCRE2_NO_UTF_CHECK, m.get(), &get_match_context());
        if (ec == PCRE2_ERROR_NOMATCH) {
            return TriBool::False;
        }
        return ec >= 0 ? TriBool::True : TriBool::Err;
    }

    Regex::Impl::code_ptr Regex::Impl::make_code(std::string_view regex, flag_type flags, int extra_flags) {
        using compile_ctr_ptr = std::unique_ptr<pcre2_compile_context_8, CallFree<pcre2_compile_context_free_8>>;
        static auto compile_ctx = []() {
            compile_ctr_ptr r{pcre2_compile_context_create_8(nullptr)};
            pcre2_set_max_pattern_length_8(r.get(), 1024ul * 1024 * 4); // 4 MB
            pcre2_set_max_pattern_compiled_length_8(r.get(), 1024ul * 1024 * 4); // 4 MB
            // lookbehind limit defaults to 255
            // parens nest limit defaults to 250
            return r;
        }();

        assert(una::is_valid_utf8(regex));
        int error_code = 0;
        size_t err_off = 0;
        int f = PCRE2_UTF | PCRE2_NO_UTF_CHECK | extra_flags;
        if (flags.contains(RegexFlag::DotAll)) {
            f |= PCRE2_DOTALL;
        }
        if (flags.contains(RegexFlag::CaseInsensitive)) {
            f |= PCRE2_CASELESS;
        }
        if (flags.contains(RegexFlag::Literal)) {
            f |= PCRE2_LITERAL;
        } else {
            f |= PCRE2_UCP | PCRE2_NEVER_BACKSLASH_C;
        }
        if (flags.contains(RegexFlag::Multiline)) {
            f |= PCRE2_MULTILINE;
        }
        code_ptr r{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR8>(regex.data()), regex.size(), f, &error_code, &err_off, compile_ctx.get())};
        if (r == nullptr) {
            throw RegexError{"Failed to compile regex: " + translate_error_code(error_code)};
        }
        if (flags.contains(RegexFlag::Optimize)) {
            error_code = pcre2_jit_compile_8(r.get(), 0);
            if (error_code != 0) {
                throw RegexError{"Failed to compile jit regex: " + translate_error_code(error_code)};
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
    Regex::Impl::Impl(code_ptr match, code_ptr search, flag_type flags)
        : match(std::move(match)),
          search(std::move(search)),
          flags(flags) {
    }

}  //namespace rdf4cpp::regex
