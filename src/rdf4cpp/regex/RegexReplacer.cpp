#include "RegexReplacer.hpp"
#include <rdf4cpp/regex/RegexReplacerImpl.hpp>

namespace rdf4cpp::regex {

RegexReplacer::RegexReplacer(std::unique_ptr<Impl> &&impl) noexcept : impl{std::move(impl)} {
}

RegexReplacer::RegexReplacer() noexcept = default;
RegexReplacer::RegexReplacer(RegexReplacer &&other) noexcept = default;
RegexReplacer &RegexReplacer::operator=(RegexReplacer &&other) noexcept = default;
RegexReplacer::~RegexReplacer() noexcept = default;

void RegexReplacer::regex_replace(std::string &str) const noexcept {
    if (this->impl != nullptr) {
        this->impl->regex_replace(str);
    }
}

}  //namespace rdf4cpp::regex
