#define DOCTEST_CONFIG_IMPLEMENT

#include <charconv>
#include <set>
#include <unordered_set>

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>
#include <rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.hpp>
#include <rdf4cpp/storage/reference_node_storage/SyncReferenceNodeStorage.hpp>

using namespace rdf4cpp;
using namespace rdf4cpp::storage;

int main(int argc, char **argv) {
    {
        reference_node_storage::SyncReferenceNodeStorage syncns{};
        default_node_storage = syncns;
        auto ret = doctest::Context{argc, argv}.run();
        if (ret != 0) {
            return ret;
        }
    }

    {
        reference_node_storage::UnsyncReferenceNodeStorage unsyncns{};
        default_node_storage = unsyncns;
        return doctest::Context{argc, argv}.run();
    }
}


namespace rdf4cpp::datatypes::registry {

constexpr static util::ConstexprString Incomparable{"Incomparable"};

// Z
template<>
struct DatatypeMapping<Incomparable> {
    using cpp_datatype = double;
};

template<>
inline capabilities::Default<Incomparable>::cpp_type capabilities::Default<Incomparable>::from_string(std::string_view s) {
    double value;
    auto const parse_res = std::from_chars(s.data(), s.data() + s.size(), value);

    if (parse_res.ptr != s.data() + s.size()) {
        throw std::runtime_error("XSD Parsing Error");
    } else {
        return value;
    }
}

} // rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

using Incomparable = registry::LiteralDatatypeImpl<registry::Incomparable>;

} // rdf4cpp::datatypes::xsd



TEST_SUITE("comparisons") {
    using namespace datatypes::xsd;

    TEST_CASE("Ordering") {
        IRI iri1 = IRI{"http://www.example.org/test2"};
        IRI iri2 = IRI{"http://www.example.org/test1"};
        IRI iri3 = IRI{"http://www.example.org/oest"};
        Literal lit1 = Literal::make_typed_from_value<String>("testlit1");
        Literal lit2 = Literal::make_typed_from_value<String>("testlit2");
        CHECK(iri2.order_lt(iri1));
        CHECK(iri3.order_lt(iri1));
        CHECK(iri3.order_lt(iri2));
        CHECK(iri1.order_lt(lit1));
        CHECK(lit1.order_lt(lit2));
    }

    TEST_CASE("filter compare tests") {
        SUBCASE("nulls") {
            CHECK(Literal{} <=> Literal{} == std::partial_ordering::unordered);
            CHECK(Literal{} <=> Literal::make_typed_from_value<Int>(1) == std::partial_ordering::unordered);
            CHECK(Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.0)) <=> Literal{} == std::partial_ordering::unordered);
        }

        SUBCASE("inconvertibility") {
            CHECK(Literal::make_typed_from_value<String>("hello") <=> Literal::make_typed_from_value<Int>(5) == std::partial_ordering::unordered);
            CHECK(Literal::make_typed_from_value<Float>(1.f) <=> Literal::make_typed_from_value<String>("world") == std::partial_ordering::unordered);
        }

        SUBCASE("incomparability") {
            CHECK(Literal::make_typed_from_value<Incomparable>(1) <=> Literal::make_typed_from_value<Incomparable>(2) == std::partial_ordering::unordered);
            CHECK(Literal::make_typed_from_value<Incomparable>(1) <=> Literal::make_typed_from_value<Incomparable>(1) == std::partial_ordering::equivalent); // exactly the same object so still equal
            CHECK(Literal::make_typed_from_value<Int>(1) <=> Literal::make_typed_from_value<Incomparable>(1) == std::partial_ordering::unordered);
            CHECK(Literal::make_typed_from_value<Incomparable>(1) <=> Literal::make_typed_from_value<Int>(1) == std::partial_ordering::unordered);
        }

        SUBCASE("conversion") {
            CHECK(Literal::make_typed_from_value<Int>(1) <=> Literal::make_typed_from_value<Integer>(10) == std::partial_ordering::less);
            CHECK(Literal::make_typed_from_value<Integer>(0) <=> Literal::make_typed_from_value<Float>(1.2f) == std::partial_ordering::less);
            CHECK(Literal::make_typed_from_value<Float>(1.f) <=> Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.0)) == std::partial_ordering::equivalent);
        }
    }

    TEST_CASE("order by compare tests") {
        // Incomparable <=> Incomparable
        SUBCASE("incomparability") {
            CHECK(Literal::make_typed_from_value<Incomparable>(5).order(Literal::make_typed_from_value<Incomparable>(10)) == std::weak_ordering::greater);

            // reason: float has fixed id and any fixed id type is always less than a dynamic one
            CHECK(Literal::make_typed_from_value<Float>(10.f).order(Literal::make_typed_from_value<Incomparable>(1)) == std::weak_ordering::less);

            // reason: decimal has fixed id and any fixed id type is always less than a dynamic one
            CHECK(Literal::make_typed_from_value<Incomparable>(1).order(Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(10.0))) == std::weak_ordering::greater);
        }

        SUBCASE("nulls") {
            CHECK(Literal{}.order(Literal{}) == std::partial_ordering::equivalent);

            // null <=> other (expecting null < any other Literal)
            CHECK(Literal{}.order(Literal::make_typed_from_value<Int>(1)) == std::weak_ordering::less);
            CHECK(Literal::make_typed_from_value<String>("123").order(Literal{}) == std::weak_ordering::greater);
        }

        SUBCASE("test type ordering extensions") {
            // expected: string < float < decimal < integer < int

            CHECK(Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.0)).order(Literal::make_typed_from_value<Float>(1)) == std::weak_ordering::greater);
            CHECK(Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.0)).order(Literal::make_typed_from_value<Int>(1)) == std::weak_ordering::less);
            CHECK(Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.0)).order(Literal::make_typed_from_value<Integer>(1)) == std::weak_ordering::less);
            CHECK(Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.0)).order(Literal::make_typed_from_value<String>("hello")) == std::weak_ordering::greater);

            CHECK(Literal::make_typed_from_value<Float>(1.f).order(Literal::make_typed_from_value<Int>(1)) == std::weak_ordering::less);
            CHECK(Literal::make_typed_from_value<Float>(1.f).order(Literal::make_typed_from_value<Integer>(1)) == std::weak_ordering::less);
            CHECK(Literal::make_typed_from_value<Float>(1.f).order(Literal::make_typed_from_value<String>("hello")) == std::weak_ordering::greater);

            CHECK(Literal::make_typed_from_value<Int>(1).order(Literal::make_typed_from_value<Integer>(1)) == std::weak_ordering::greater);
            CHECK(Literal::make_typed_from_value<Int>(1).order(Literal::make_typed_from_value<String>("hello")) == std::weak_ordering::greater);
        }

        SUBCASE("test ordering extensions ignored when not equal") {
            CHECK(Literal::make_typed_from_value<Float>(2.f).order(Literal::make_typed_from_value<Integer>(1)) == std::weak_ordering::greater);
        }
    }

    TEST_CASE("std::set") {
        Literal const lit = Literal::make_typed_from_value<Int>(5);
        Literal const lit2 = Literal::make_typed_from_value<Int>(5); // duplicate to check if it's filtered out
        Literal const lit3 = Literal::make_typed_from_value<Integer>(10);
        Literal const lit4 = Literal::make_typed_from_value<String>("hello world");
        IRI const iri{"https://random.com/some/iri"};
        IRI const null_iri{};
        Node const node{};
        BlankNode const blank_node{"some_random_id"};
        query::Variable const variable{"a_variable"};

        std::set<Node> const s{
                lit, iri, node, lit2, blank_node, null_iri, lit3, variable, lit4};

        std::vector<Node> const v{s.begin(), s.end()};

        // - lexical order of types, and null smallest
        // - null Node has type BNode
        // - literal comparison
        std::vector<Node> const expected{node, blank_node, iri, lit4, lit, lit3, variable};

        CHECK(v == expected);
    }

    TEST_CASE("std::unordered_set") {
        Literal const lit = Literal::make_typed_from_value<Int>(5);
        Literal const lit2 = Literal::make_typed_from_value<Int>(5); // duplicate to check if it's filtered out
        Literal const lit3 = Literal::make_typed_from_value<Integer>(10);
        Literal const lit4 = Literal::make_typed_from_value<String>("hello world");
        IRI const iri{"https://random.com/some/iri"};
        IRI const null_iri{};
        Node const node{};
        BlankNode const blank_node{"some_random_id"};
        query::Variable const variable{"a_variable"};

        std::unordered_set<Node> s{
                iri, null_iri, lit, lit2, lit3, lit4, node, blank_node, variable};

        CHECK(s.size() == 7);
        CHECK(s.contains(lit));
        CHECK(s.contains(lit2));
        CHECK(s.contains(lit3));
        CHECK(s.contains(lit4));
        CHECK(s.contains(iri));
        CHECK(s.contains(null_iri));
        CHECK(s.contains(node));
        CHECK(s.contains(blank_node));
        CHECK(s.contains(variable));
    }
}

TEST_CASE("effective boolean value") {
    Node const iri = IRI{"http://hello.com"};
    Node const bnode = BlankNode{"asd"};
    Node const var = query::Variable{"x"};
    Node const falsy_lit = Literal::make_typed_from_value<datatypes::xsd::Float>(0.f);
    Node const truthy_lit = Literal::make_typed_from_value<datatypes::xsd::Integer>(100);
    Node const null_lit = Literal{};
    Node const null_bnode = BlankNode{};

    CHECK(iri.ebv() == TriBool::Err);
    CHECK(bnode.ebv() == TriBool::Err);
    CHECK(var.ebv() == TriBool::Err);
    CHECK(falsy_lit.ebv() == TriBool::False);
    CHECK(truthy_lit.ebv() == TriBool::True);
    CHECK(null_lit.ebv() == TriBool::Err);
    CHECK(null_bnode.ebv() == TriBool::Err);
}

TEST_CASE("IRI UUID") {
    IRI uuid = IRI::make_uuid();
    IRI uuid2 = IRI::make_uuid();

    CHECK(uuid != uuid2);  // note: non-deterministic but should basically never fail
    CHECK(regex::Regex{"^urn:uuid:[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"}.regex_match(uuid.identifier()));
}

TEST_CASE("IRI fetch or serialize") {
    auto const x = IRI::make("https://example.com#some-iri");
    auto const expected = x.identifier();

    std::string buf;
    writer::StringWriter w{buf};

    std::string_view fetched;
    auto const res = x.fetch_or_serialize_identifier(fetched, w);
    REQUIRE_EQ(res, FetchOrSerializeResult::Fetched);
    CHECK_EQ(fetched, expected);
}

template<typename T>
struct get_find_values {};

template<>
struct get_find_values<IRI> {
    static constexpr std::string_view t = "http://foo/bar";
    static constexpr std::string_view v = "http://foo/bar/x";
};
template<>
struct get_find_values<BlankNode> {
    static constexpr std::string_view t = "bl1aaaa";
    static constexpr std::string_view v = "bl2aaaa";
};

TEST_CASE_TEMPLATE("IRI/BlankNode::find", T, IRI, BlankNode) {
    static constexpr std::string_view t = get_find_values<T>::t;

    CHECK(T::find(t) == T{});
    T i{t};
    CHECK(T::find(t) == i);
    CHECK(T::find(t).backend_handle() == i.backend_handle());
    CHECK(T::find(get_find_values<T>::v) == T{});
}

TEST_CASE("node equality shortcut") {
    auto s = storage::reference_node_storage::SyncReferenceNodeStorage{};

    CHECK(IRI::make("https://ex") == IRI::make("https://ex"));
    CHECK(IRI::make("https://ex") != IRI::make("https://ex2"));
    CHECK(IRI::make("https://ex") == IRI::make("https://ex", s));
    CHECK(IRI::make("https://ex") != IRI::make("https://ex2", s));
    CHECK(Literal::make_typed_from_value<datatypes::xsd::Int>(5) == Literal::make_typed_from_value<datatypes::xsd::UnsignedInt>(5));
    CHECK(Literal::make_typed_from_value<datatypes::xsd::Int>(5) != Literal::make_typed_from_value<datatypes::xsd::Int>(6));
    CHECK(Literal::make_typed_from_value<datatypes::xsd::Int>(5) == Literal::make_typed_from_value<datatypes::xsd::UnsignedInt>(5, s));
    CHECK(Literal::make_typed_from_value<datatypes::xsd::Int>(5) != Literal::make_typed_from_value<datatypes::xsd::Int>(6, s));
}

TEST_CASE("node format") {
    auto iri = IRI::make("https://ex.com");
    CHECK(std::format("abc{}def", static_cast<const Node&>(iri)) == "abc<https://ex.com>def");
    CHECK(std::format("abc{}def", iri) == "abc<https://ex.com>def");
    CHECK(std::format("abc{}def", Literal::make_typed_from_value<rdf4cpp::datatypes::xsd::Int>(42)) == "abc\"42\"^^<http://www.w3.org/2001/XMLSchema#int>def");
    CHECK(std::format("abc{}def", BlankNode::make("blank")) == "abc_:blankdef");
    CHECK(std::format("abc{}def", query::Variable::make_named("x")) == "abc?xdef");
}

TEST_CASE("IRI validity") {
    IRI i{};
    CHECK_THROWS_AS(i = IRI::make("something not iri"), InvalidNode);
    CHECK_THROWS_AS(i = IRI("something not iri"), InvalidNode);
    CHECK(IRI::make_unchecked("something not iri").identifier() == "something not iri");
    CHECK(IRI::make("https://this.is/a/valid/iri").identifier() == "https://this.is/a/valid/iri");
    CHECK(i == IRI());
}

TEST_CASE("BlankNode validity") {
    BlankNode n{};
    CHECK_THROWS_AS(n = BlankNode::make("-may_not_be_first"), InvalidNode);
    CHECK_THROWS_AS(n = BlankNode("may_not_be_last."), InvalidNode);
    CHECK(BlankNode::make_unchecked("may_not_be_last.").identifier() == "may_not_be_last.");
    CHECK(BlankNode::make("a_middle.is_allowed").identifier() == "a_middle.is_allowed");
    CHECK(BlankNode::make("012_numbers_too567").identifier() == "012_numbers_too567");
    CHECK(BlankNode::make("\U0001f34cthrow_some_unicode_at_it\U0001f34c").identifier() == "\U0001f34cthrow_some_unicode_at_it\U0001f34c");
    CHECK(n == BlankNode{});
}

TEST_CASE_TEMPLATE("NodeStorage erase IRI", T, reference_node_storage::SyncReferenceNodeStorage, reference_node_storage::UnsyncReferenceNodeStorage) {
    static constexpr std::string_view example = "https://example.com";
    T ns{};

    // normal erase
    IRI i = IRI::make(example);
    CHECK(i.try_get_in_node_storage(ns) == IRI());
    CHECK(i.to_node_storage(ns) != IRI());
    CHECK(i.try_get_in_node_storage(ns) != IRI());
    CHECK(ns.erase_iri(i.try_get_in_node_storage(ns).backend_handle().id()));
    CHECK(i.try_get_in_node_storage(ns) == IRI());

    // try erase predefined
    CHECK(IRI::find(rdf4cpp::datatypes::xsd::Int::identifier, ns) != IRI());
    CHECK(!ns.erase_iri(IRI::find(rdf4cpp::datatypes::xsd::Int::identifier, ns).backend_handle().id()));
    CHECK(IRI::find(rdf4cpp::datatypes::xsd::Int::identifier, ns) != IRI());
}

TEST_CASE("null nodes") {
    Node n1{};
    Literal n2{};
    BlankNode n3{};
    IRI n4{};
    query::Variable n5{};

    CHECK_EQ(n1, n2);
    CHECK_EQ(n2, n3);
    CHECK_EQ(n3, n4);
    CHECK_EQ(n4, n5);
    CHECK_EQ(n5, n1);
    CHECK_EQ(n1.backend_handle(), n2.backend_handle());
    CHECK_EQ(n2.backend_handle(), n3.backend_handle());
    CHECK_EQ(n3.backend_handle(), n4.backend_handle());
    CHECK_EQ(n4.backend_handle(), n5.backend_handle());
    CHECK_EQ(n5.backend_handle(), n1.backend_handle());

    auto const run_checks = [](Node node) {
        // is_* checks are accessed via Node::is_*
        CHECK(node.null());
        CHECK_FALSE(node.is_blank_node());
        CHECK_FALSE(node.is_literal());
        CHECK_FALSE(node.is_iri());
        CHECK_FALSE(node.is_variable());
    };

    run_checks(n1);
    run_checks(n2);
    run_checks(n3);
    run_checks(n4);
    run_checks(n5);
}

TEST_CASE("variable inlining") {
    auto const check_node_storage_transfer = [](query::Variable const &var) {
        storage::reference_node_storage::UnsyncReferenceNodeStorage ns;

        auto again = [&]() {
            if (var.is_anonymous()) {
                return query::Variable::find_anonymous(var.name(), ns);
            } else {
                return query::Variable::find_named(var.name(), ns);
            }
        }();
        CHECK_EQ(var.backend_handle().id(), again.backend_handle().id());

        auto again2 = var.try_get_in_node_storage(ns);
        CHECK_EQ(var.backend_handle().id(), again2.backend_handle().id());

        auto again3 = var.to_node_storage(ns);
        CHECK_EQ(var.backend_handle().id(), again3.backend_handle().id());
    };

    auto const check_fetch_or_serialize = [&](query::Variable const &var) {
        auto const expected = var.name();

        std::string buf;
        writer::StringWriter w{buf};

        std::string_view fetched;
        auto res = var.fetch_or_serialize_name(fetched, w);
        auto const expected_res = var.is_inlined() ? FetchOrSerializeResult::Serialized : FetchOrSerializeResult::Fetched;
        CHECK_EQ(res, expected_res);

        switch (res) {
            case FetchOrSerializeResult::Fetched: {
                CHECK_EQ(fetched, expected);
                break;
            }
            case FetchOrSerializeResult::Serialized: {
                w.finalize();
                CHECK_EQ(buf, expected);
                break;
            }
            default: {
                FAIL("fetch_or_serialize failed");
                break;
            }
        }
    };

    auto v1 = query::Variable::make_named("abcde");
    CHECK(v1.is_inlined());
    CHECK_FALSE(v1.is_anonymous());
    CHECK_EQ(v1.name(), "abcde");
    check_node_storage_transfer(v1);
    check_fetch_or_serialize(v1);

    auto v2 = query::Variable::make_named("fghij");
    CHECK(v2.is_inlined());
    CHECK_FALSE(v2.is_anonymous());
    CHECK_EQ(v2.name(), "fghij");
    CHECK_EQ(v1.name().size(), v2.name().size());
    CHECK_EQ(v1.order(v2), std::strong_ordering::less);
    check_node_storage_transfer(v2);
    check_fetch_or_serialize(v2);

    auto v3 = query::Variable::make_anonymous(v1.name());
    CHECK(v3.is_inlined());
    CHECK(v3.is_anonymous());
    CHECK_EQ(v3.name(), "abcde");
    CHECK_EQ(v3.name(), v1.name());
    CHECK_NE(v3, v1);
    CHECK_EQ(v3.order(v1), std::strong_ordering::greater);
    check_node_storage_transfer(v3);
    check_fetch_or_serialize(v3);

    auto v4 = query::Variable::make_named("abcdef");
    CHECK_FALSE(v4.is_inlined());
    CHECK_FALSE(v4.is_anonymous());
    CHECK_EQ(v4.name(), "abcdef");
    CHECK_EQ(v4.order(v1), std::strong_ordering::greater);
    check_fetch_or_serialize(v4);

    auto v5 = query::Variable::make_anonymous("fghijk");
    CHECK_FALSE(v5.is_inlined());
    CHECK(v5.is_anonymous());
    CHECK_EQ(v5.name(), "fghijk");
    CHECK_EQ(v5.order(v4), std::strong_ordering::greater);
    check_fetch_or_serialize(v5);

    auto v6 = query::Variable::make_named("a");
    CHECK(v6.is_inlined());
    CHECK_FALSE(v6.is_anonymous());
    CHECK_EQ(v6.name(), "a");
    check_fetch_or_serialize(v6);

    auto v7 = query::Variable::make_anonymous("a");
    CHECK(v7.is_inlined());
    CHECK(v7.is_anonymous());
    CHECK_EQ(v7.name(), "a");
    CHECK_EQ(v7.order(v6), std::strong_ordering::greater);
    check_fetch_or_serialize(v7);

    auto v8 = query::Variable::make_named("aaaaaa");
    CHECK_FALSE(v8.is_inlined());
    CHECK_FALSE(v8.is_anonymous());
    CHECK_GT(v8.name().size(), v1.name().size());
    CHECK_EQ(v8.order(v1), std::strong_ordering::less);
    check_fetch_or_serialize(v8);
}

TEST_CASE("bnode inlining") {
    auto const check_node_storage_transfer = [](BlankNode const &bnode) {
        storage::reference_node_storage::UnsyncReferenceNodeStorage ns;

        auto again = BlankNode::find(bnode.identifier(), ns);
        CHECK_EQ(bnode.backend_handle().id(), again.backend_handle().id());

        auto again2 = bnode.try_get_in_node_storage(ns);
        CHECK_EQ(bnode.backend_handle().id(), again2.backend_handle().id());

        auto again3 = bnode.to_node_storage(ns);
        CHECK_EQ(bnode.backend_handle().id(), again3.backend_handle().id());
    };

    auto const check_fetch_or_serialize = [&](BlankNode const &var) {
        auto const expected = var.identifier();

        std::string buf;
        writer::StringWriter w{buf};

        std::string_view fetched;
        auto const res = var.fetch_or_serialize_identifier(fetched, w);
        auto const expected_res = var.is_inlined() ? FetchOrSerializeResult::Serialized : FetchOrSerializeResult::Fetched;
        CHECK_EQ(res, expected_res);

        switch (res) {
            case FetchOrSerializeResult::Fetched: {
                CHECK_EQ(fetched, expected);
                break;
            }
            case FetchOrSerializeResult::Serialized: {
                w.finalize();
                CHECK_EQ(buf, expected);
                break;
            }
            default: {
                FAIL("fetch_or_serialize failed");
                break;
            }
        }
    };

    auto v1 = BlankNode::make("abcdef");
    CHECK(v1.is_inlined());
    CHECK_EQ(v1.identifier(), "abcdef");
    check_node_storage_transfer(v1);
    check_fetch_or_serialize(v1);

    auto v2 = BlankNode::make("fghijk");
    CHECK(v2.is_inlined());
    CHECK_EQ(v2.identifier(), "fghijk");
    CHECK_EQ(v1.identifier().size(), v2.identifier().size());
    CHECK_EQ(v1.order(v2), std::strong_ordering::less);
    check_node_storage_transfer(v2);
    check_fetch_or_serialize(v2);

    auto v3 = BlankNode::make("abcdefg");
    CHECK_FALSE(v3.is_inlined());
    CHECK_EQ(v3.identifier(), "abcdefg");
    CHECK_EQ(v3.order(v1), std::strong_ordering::greater);
    check_fetch_or_serialize(v3);

    auto v4 = BlankNode::make("a");
    CHECK(v4.is_inlined());
    CHECK_EQ(v4.identifier(), "a");
    CHECK_EQ(v4.order(v1), std::strong_ordering::less);
    check_fetch_or_serialize(v4);

    auto v5 = BlankNode::make("aaaaaaa");
    CHECK_FALSE(v5.is_inlined());
    CHECK_GT(v5.identifier().size(), v1.identifier().size());
    CHECK_EQ(v5.order(v1), std::strong_ordering::less);
    check_fetch_or_serialize(v5);
}
