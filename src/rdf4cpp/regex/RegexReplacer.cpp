#include "RegexReplacer.hpp"
#include <rdf4cpp/regex/RegexReplacerImpl.hpp>

namespace rdf4cpp::regex {

RegexReplacer::RegexReplacer(std::shared_ptr<Impl> impl) noexcept : impl_{std::move(impl)} {
}

void RegexReplacer::regex_replace(std::string &str) const {
    impl_->regex_replace(str);
}

}  //namespace rdf4cpp::regex
