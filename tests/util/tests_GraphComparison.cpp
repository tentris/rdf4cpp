#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp;

namespace {
    Quad quad(std::string_view subject, std::string_view predicate, std::string_view object) {
        static constexpr auto node = [](std::string_view n) -> Node {
            if (n.starts_with("_:")) {
                return BlankNode::make(n.substr(2));
            }
            return IRI::make(n);
        };
        return Quad{IRI::default_graph(), node(subject), IRI::make(predicate), node(object)};
    }
}  // namespace

TEST_CASE("isomorphic graphs are mapped") {
    std::vector<Quad> a{quad("_:x", "http://example.com/p", "http://example.com/o")};
    std::vector<Quad> b{quad("_:y", "http://example.com/p", "http://example.com/o")};

    auto r = try_compare_graphs_fast<Quad>(a, b);
    REQUIRE(r.has_value());
    CHECK(r->size() == 1);
    CHECK(r->at(BlankNode::make("y")) == BlankNode::make("x"));
}

TEST_CASE("blank node mappings are injective") {
    // _:x <p> _:x is not isomorphic to _:y <p> _:z, mapping both y and z to x would be wrong
    std::vector<Quad> a{quad("_:x", "http://example.com/p", "_:x")};
    std::vector<Quad> b{quad("_:y", "http://example.com/p", "_:z")};

    CHECK(!try_compare_graphs_fast<Quad>(a, b).has_value());
    CHECK(!try_compare_graphs_fast<Quad>(b, a).has_value());
}

TEST_CASE("blank node mappings are injective over multiple quads") {
    std::vector<Quad> a{quad("_:x", "http://example.com/p", "http://example.com/o1"),
                        quad("_:x", "http://example.com/p", "http://example.com/o2")};
    std::vector<Quad> b{quad("_:y", "http://example.com/p", "http://example.com/o1"),
                        quad("_:z", "http://example.com/p", "http://example.com/o2")};

    CHECK(!try_compare_graphs_fast<Quad>(a, b).has_value());
}

TEST_CASE("graphs of different size are reported instead of aborting") {
    std::vector<Quad> a{quad("http://example.com/s", "http://example.com/p", "http://example.com/o")};
    std::vector<Quad> b{};

    auto r = try_compare_graphs_fast<Quad>(a, b);
    REQUIRE(!r.has_value());
    CHECK(r.error().first.null());
    CHECK(r.error().second.null());
}

TEST_CASE("empty graphs are equal") {
    std::vector<Quad> a{};
    std::vector<Quad> b{};

    auto r = try_compare_graphs_fast<Quad>(a, b);
    REQUIRE(r.has_value());
    CHECK(r->empty());
}

TEST_CASE("different graphs report the first mismatch") {
    std::vector<Quad> a{quad("http://example.com/s", "http://example.com/p", "http://example.com/o1")};
    std::vector<Quad> b{quad("http://example.com/s", "http://example.com/p", "http://example.com/o2")};

    auto r = try_compare_graphs_fast<Quad>(a, b);
    REQUIRE(!r.has_value());
    CHECK(r.error().first == IRI::make("http://example.com/o1"));
    CHECK(r.error().second == IRI::make("http://example.com/o2"));
}

TEST_CASE("blank nodes are matched over a bigger graph") {
    std::vector<Quad> a{};
    std::vector<Quad> b{};
    for (size_t i = 0; i < 100; ++i) {
        a.push_back(quad(std::format("_:a{}", i), "http://example.com/p", std::format("http://example.com/o{}", i)));
        b.push_back(quad(std::format("_:b{}", 99 - i), "http://example.com/p", std::format("http://example.com/o{}", 99 - i)));
    }

    auto r = try_compare_graphs_fast<Quad>(a, b);
    REQUIRE(r.has_value());
    CHECK(r->size() == 100);
    for (size_t i = 0; i < 100; ++i) {
        CHECK(r->at(BlankNode::make(std::format("b{}", i))) == BlankNode::make(std::format("a{}", i)));
    }
}
