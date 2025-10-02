#include "Regex.hpp"
#include <rdf4cpp/regex/RegexImpl.hpp>
#include <rdf4cpp/regex/RegexReplacerImpl.hpp>

namespace rdf4cpp::regex {

Regex::Regex(std::string_view regex, flag_type const flags) : impl(std::make_shared<Impl const>(regex, flags)) {
}
Regex::Regex(Regex const &other) noexcept = default;
Regex::Regex(Regex &&other) noexcept = default;
Regex &Regex::operator=(Regex const &other) noexcept = default;
Regex &Regex::operator=(Regex &&other) noexcept = default;
Regex::~Regex() noexcept = default;

bool Regex::regex_match(std::string_view const str) const noexcept {
    return this->impl->regex_match(str);
}

bool Regex::regex_search(std::string_view const str) const noexcept {
    return this->impl->regex_search(str);
}

RegexReplacer Regex::make_replacer(std::string_view const rewrite) const {
    return RegexReplacer{std::make_unique<RegexReplacer::Impl>(this->impl, rewrite)};
}

}  //namespace rdf4cpp::regex
