#ifndef RDF4CPP_RDF_UTIL_PRIVATE_REGEX_IMPL_HPP
#define RDF4CPP_RDF_UTIL_PRIVATE_REGEX_IMPL_HPP

#include <rdf4cpp/regex/Regex.hpp>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace rdf4cpp::regex {

struct Regex::Impl {
    using code_ptr = std::unique_ptr<pcre2_code_8, decltype([](pcre2_code_8* c) { pcre2_code_free_8(c); })>;
    code_ptr compiled_regex;
    Regex::flag_type flags;

    Impl(std::string_view regex, flag_type flags);
    [[nodiscard]] bool regex_match(std::string_view str) const noexcept;
    [[nodiscard]] bool regex_search(std::string_view str) const noexcept;

private:
    using match_data_ptr = std::unique_ptr<pcre2_match_data_8, decltype([](pcre2_match_data_8* c) { pcre2_match_data_free_8(c); })>;
    [[nodiscard]] match_data_ptr get_match_data() const noexcept;

    static code_ptr make_code(std::string_view regex, flag_type flags, int extra_flags);
};

}  //namespace rdf4cpp::regex

#endif  //RDF4CPP_RDF_UTIL_PRIVATE_REGEX_IMPL_HPP
