#include <rdf4cpp/regex/Regex.hpp>
#include <rdf4cpp/regex/RegexImpl.hpp>
#include <rdf4cpp/regex/RegexReplacerImpl.hpp>

namespace rdf4cpp::regex {

Regex::Regex(std::string_view regex, flag_type const flags) : impl_{std::make_shared<Impl const>(regex, flags)} {
}

bool Regex::regex_match(std::string_view const str) const noexcept {
    return impl_->regex_match(str);
}

bool Regex::regex_search(std::string_view const str) const noexcept {
    return impl_->regex_search(str);
}

RegexReplacer Regex::make_replacer(std::string_view const rewrite) const {
    return RegexReplacer{std::make_shared<RegexReplacer::Impl>(impl_, rewrite)};
}

}  //namespace rdf4cpp::regex
