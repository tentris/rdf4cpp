#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_CASE("load dataset") {
    Dataset set{};
    for (const auto& v : rdf4cpp::parser::RDFFileParser{"test_swdf/swdf.nt"}) {
        if (v.has_value()) {
            set.add(v.value());
        }
        else {
            FAIL(v.error());
        }
    }
    CHECK(set.size() == 304592);
}
