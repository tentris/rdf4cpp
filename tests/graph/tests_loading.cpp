#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_CASE("load Graph") {
    Graph set{};
    std::fstream file{"test_swdf/swdf.nt"};
    set.load_rdf_data(file, parser::ParsingFlags::none(), nullptr, [](parser::ParsingError e) {
        FAIL(std::format("{}", e));
    });
    CHECK(set.size() == 304592);
}

TEST_CASE("load dataset") {
    Dataset set{};
    std::fstream file{"tests_loading_simple.trig"};
    set.load_rdf_data(file, parser::ParsingFlags::none(), nullptr, [](parser::ParsingError e) {
        FAIL(std::format("{}", e));
    });
    CHECK(set.size() == 4);
}
