#ifndef RDF4CPP_RDF_UTIL_PRIVATE_REGEX_IMPL_HPP
#define RDF4CPP_RDF_UTIL_PRIVATE_REGEX_IMPL_HPP

#include <rdf4cpp/regex/Regex.hpp>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace rdf4cpp::regex {
    struct Regex::Impl {
        // workaround for gcc-14 bug, erroneously warns on unsing a lambda here
        // see https://github.com/NVIDIA/stdexec/issues/1143
        template<auto f>
        struct CallFree {
            void operator()(auto* c) {
                f(c);
            }
        };

        using code_ptr = std::unique_ptr<pcre2_code_8, CallFree<pcre2_code_free_8>>;
        code_ptr match;
        code_ptr search;
        Regex::flag_type flags;

        Impl(std::string_view regex, flag_type flags);
        [[nodiscard]] bool regex_match(std::string_view str) const noexcept;
        [[nodiscard]] bool regex_search(std::string_view str) const noexcept;

    private:
        using match_data_ptr = std::unique_ptr<pcre2_match_data_8, CallFree<pcre2_match_data_free_8>>;
        [[nodiscard]] static bool apply(pcre2_code_8 &c, std::string_view str) noexcept;

        static code_ptr make_code(std::string_view regex, flag_type flags, int extra_flags);
        static std::string remove_whitespace(std::string_view str);
        static Impl make(std::string_view regex, flag_type flags);
        Impl(code_ptr m, code_ptr s, flag_type f);
    };
}  //namespace rdf4cpp::regex

#endif  //RDF4CPP_RDF_UTIL_PRIVATE_REGEX_IMPL_HPP
