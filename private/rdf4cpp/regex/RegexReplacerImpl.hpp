#ifndef RDF4CPP_RDF_UTIL_PRIVATE_REGEXREPLACER_IMPL_HPP
#define RDF4CPP_RDF_UTIL_PRIVATE_REGEXREPLACER_IMPL_HPP

#include <rdf4cpp/regex/RegexReplacer.hpp>
#include "RegexImpl.hpp"

namespace rdf4cpp::regex {

struct RegexReplacer::Impl {
    std::shared_ptr<Regex::Impl> regex;
    std::string rewrite;

    Impl(std::shared_ptr<Regex::Impl> regex, std::string_view rewrite);
    void regex_replace(std::string &str) const noexcept;
};

}  //namespace rdf4cpp::regex

#endif   //RDF4CPP_RDF_UTIL_PRIVATE_REGEXREPLACER_IMPL_HPP
