#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

#include <iostream>
#include <filesystem>
#include <curl/curl.h>

using namespace rdf4cpp;
using namespace rdf4cpp::parser;

void jsonld_test_positive([[maybe_unused]] std::string json_str, [[maybe_unused]] std::string nt_str, [[maybe_unused]] std::string_view base_iri) {

}

void jsonld_test_negative([[maybe_unused]] std::string json_str, [[maybe_unused]] std::string_view base_iri) {

}
