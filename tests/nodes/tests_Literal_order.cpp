#define DOCTEST_CONFIG_IMPLEMENT

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <dice/hash.hpp>
#include <doctest/doctest.h>
#include <rdf4cpp.hpp>
#include <rdf4cpp/storage/reference_node_storage/SyncReferenceNodeStorage.hpp>
#include <rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.hpp>

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
        default_node_storage = nullptr;
    }

    {
        reference_node_storage::UnsyncReferenceNodeStorage unsyncns{};
        default_node_storage = unsyncns;
        auto ret = doctest::Context{argc, argv}.run();
        default_node_storage = nullptr;
        return ret;
    }
}

constexpr std::string_view xsd_prefix = "http://www.w3.org/2001/XMLSchema#";

Literal make_typed_literal(std::string_view lexical_form, std::string_view datatype_suffix) {
    return Literal::make_typed(lexical_form, IRI{std::string{xsd_prefix} + std::string{datatype_suffix}});
}

/**
 * Two literals of the same datatype whose *values* compare equivalent but which are
 * distinct terms (distinct backend handles). These are exactly the pairs that reach the
 * `equivalent` branch of Literal::compare_impl and therefore depend on the alternative
 * (lexical form) ordering being populated.
 */
struct EquivalentPair {
    char const *datatype;
    char const *smaller;  ///< lexical form that must order *before* greater
    char const *greater;
};

/**
 * Every same-datatype, distinct-term, value-equivalent pair rdf4cpp can construct.
 * Signed zeros for the floating point types, and timezone variants for every
 * timezone-carrying temporal type. The expected order is the ordering of the
 * canonical lexical forms, which is what Literal::order falls back to.
 */
constexpr EquivalentPair equivalent_pairs[] = {
        // floating point signed zeros: canonical forms are "-0E0" and "0.0E0", and '-' < '0'
        {"double", "-0.0", "0.0"},
        {"float", "-0.0", "0.0"},
        // timezone variants: same instant, different lexical form
        {"dateTime", "2000-01-01T00:00:00Z", "2000-01-01T01:00:00+01:00"},
        {"dateTime", "1999-12-31T23:00:00-01:00", "2000-01-01T00:00:00Z"},
        {"dateTime", "2000-01-01T00:00:00", "2000-01-01T00:00:00Z"},
        {"dateTimeStamp", "2000-01-01T00:00:00Z", "2000-01-01T05:00:00+05:00"},
        {"time", "12:00:00Z", "13:00:00+01:00"},
        {"time", "12:00:00", "12:00:00Z"},
        {"date", "2000-01-01", "2000-01-01Z"},
        {"gYear", "2000", "2000Z"},
        {"gMonth", "--01", "--01Z"},
        {"gDay", "---01", "---01Z"},
        {"gYearMonth", "2000-01", "2000-01Z"},
        {"gMonthDay", "--01-01", "--01-01Z"},
};

struct DatatypeGroup {
    char const *name;
    std::vector<Node> terms;
};

std::vector<Node> nodes_dedup(std::vector<Node> nodes) {
    std::vector<Node> unique_nodes;
    for (auto const &node : nodes) {
        auto const already_present = [&node](Node const &seen) {
            return seen.backend_handle() == node.backend_handle();
        };

        if (std::ranges::none_of(unique_nodes, already_present)) {
            unique_nodes.push_back(node);
        }
    }
    return unique_nodes;
}

std::vector<DatatypeGroup> make_datatype_groups() {
    auto make_typed_group = [](char const *datatype, std::vector<char const *> const &lexical_forms) {
        DatatypeGroup group{datatype, {}};
        for (auto const *lexical_form : lexical_forms) {
            group.terms.emplace_back(make_typed_literal(lexical_form, datatype));
        }
        return group;
    };

    std::vector<DatatypeGroup> groups;

    groups.push_back(make_typed_group("double", {"-0.0", "0.0", "1.0", "-1.5", "NaN", "INF", "-INF"}));
    groups.push_back(make_typed_group("float", {"-0.0", "0.0", "1.0", "NaN", "-INF"}));
    groups.push_back(make_typed_group("decimal", {"0.0", "1.0", "-1.5"}));
    groups.push_back(make_typed_group("integer", {"0", "1", "-1", "99999999999999999999999999"}));
    groups.push_back(make_typed_group("int", {"0", "1", "-2147483648", "2147483647"}));
    groups.push_back(make_typed_group("boolean", {"true", "false"}));
    // the escaped forms interleave with the plain ones in byte order, so an ordering that
    // grouped by "needs escaping" instead of by lexical form would break the total order
    groups.push_back(make_typed_group("string", {"", "abc", "ABC", "abd", "zzz",
                                                 "\n", "\"quoted\"", "a\"b", "a\\b", "z\rz"}));
    groups.push_back(make_typed_group("anyURI", {"http://example.org/", "http://example.org/a",
                                                 "http://example.org/b", "https://example.org/a",
                                                 "urn:isbn:0451450523"}));
    groups.push_back(make_typed_group("dateTime", {"2000-01-01T00:00:00Z", "2000-01-01T01:00:00+01:00",
                                                   "1999-12-31T23:00:00-01:00", "2000-01-01T00:00:00",
                                                   "2000-01-01T00:00:01Z", "1999-01-01T00:00:00Z"}));
    groups.push_back(make_typed_group("dateTimeStamp", {"2000-01-01T00:00:00Z", "2000-01-01T05:00:00+05:00"}));
    groups.push_back(make_typed_group("time", {"12:00:00Z", "13:00:00+01:00", "12:00:00", "00:00:00Z"}));
    groups.push_back(make_typed_group("date", {"2000-01-01Z", "2000-01-01", "2000-01-02Z"}));
    groups.push_back(make_typed_group("gYear", {"2000Z", "2000", "1999Z"}));
    groups.push_back(make_typed_group("gMonth", {"--01Z", "--01", "--12Z"}));
    groups.push_back(make_typed_group("gDay", {"---01Z", "---01", "---15Z"}));
    groups.push_back(make_typed_group("gYearMonth", {"2000-01Z", "2000-01", "2000-02Z"}));
    groups.push_back(make_typed_group("gMonthDay", {"--01-01Z", "--01-01"}));
    groups.push_back(make_typed_group("dayTimeDuration", {"P1D", "PT1H", "-P1D"}));
    groups.push_back(make_typed_group("yearMonthDuration", {"P1Y", "P1M", "-P1Y"}));
    groups.push_back(make_typed_group("hexBinary", {"0F", "FF"}));
    groups.push_back(make_typed_group("base64Binary", {"AA==", "AQ=="}));

    // no compare_fptr at all: ordering is the lexical form only
    DatatypeGroup unregistered{"unregistered datatype", {}};
    for (auto const *lexical_form : {"abc", "abd", "zzz"}) {
        unregistered.terms.emplace_back(Literal::make_typed(lexical_form, IRI{"http://example.org/unregistered"}));
    }
    groups.push_back(std::move(unregistered));

    groups.push_back({"simple literals", {Literal::make_simple("abc"), Literal::make_simple("abd")}});
    groups.push_back({"language tagged literals",
                      {Literal::make_lang_tagged("abc", "en"), Literal::make_lang_tagged("abc", "de"),
                       Literal::make_lang_tagged("abd", "en")}});
    groups.push_back({"non-literal terms",
                      {IRI{"http://a.com"}, IRI{"http://b.com"}, BlankNode{"a"}, BlankNode{"b"},
                       query::Variable{"x"}, query::Variable{"y"}, Node{}, Literal{}, IRI{}, BlankNode{}}});

    return groups;
}

std::vector<Node> make_unique_nodes_from_dt_groups() {
    std::vector<Node> nodes;
    for (auto const &group : make_datatype_groups()) {
        nodes.insert(nodes.end(), group.terms.begin(), group.terms.end());
    }
    for (auto const *lexical_form : {"1", "-1"}) {
        nodes.emplace_back(make_typed_literal(lexical_form, "long"));
    }
    nodes.emplace_back(Literal::make_typed("abc", IRI{"http://example.org/unregistered2"}));
    return nodes_dedup(std::move(nodes));
}


TEST_SUITE("literal ordering") {

    TEST_CASE("value-equivalent distinct terms are separated by Literal::order") {
        // Literal::compare_impl computes the alternative (lexical form) ordering when value comparison is `equivalent`
        for (auto const &pair : equivalent_pairs) {
            CAPTURE(std::string_view{pair.datatype});
            CAPTURE(std::string_view{pair.smaller});
            CAPTURE(std::string_view{pair.greater});

            Literal const smaller = make_typed_literal(pair.smaller, pair.datatype);
            Literal const greater = make_typed_literal(pair.greater, pair.datatype);

            REQUIRE(!smaller.null());
            REQUIRE(!greater.null());

            // precondition: distinct terms whose values are equivalent
            REQUIRE(smaller.backend_handle() != greater.backend_handle());
            REQUIRE(smaller.compare(greater) == std::partial_ordering::equivalent);
            REQUIRE(smaller.eq(greater) == TriBool::True);

            // the ordering extension must still separate them, in both directions
            CHECK(smaller.order(greater) == std::strong_ordering::less);
            CHECK(greater.order(smaller) == std::strong_ordering::greater);

            CHECK_FALSE(smaller.order_eq(greater));
            CHECK_FALSE(greater.order_eq(smaller));
            CHECK(smaller.order_lt(greater));
            CHECK(greater.order_gt(smaller));

            // and the same through Node, which is what ORDER BY uses
            Node const smaller_node{smaller};
            Node const greater_node{greater};
            CHECK(smaller_node.order(greater_node) == std::strong_ordering::less);
            CHECK(greater_node.order(smaller_node) == std::strong_ordering::greater);
            CHECK_FALSE(smaller_node == greater_node);
            CHECK(smaller_node != greater_node);
        }
    }

    TEST_CASE("Node equality is consistent with the hash implementations") {
        auto const nodes = make_unique_nodes_from_dt_groups();

        auto check_equal_nodes_hash_equal = [&nodes](char const *hasher_name, auto const &hasher) {
            CAPTURE(std::string_view{hasher_name});
            for (auto const &lhs : nodes) {
                for (auto const &rhs : nodes) {
                    if (lhs == rhs) {
                        CAPTURE(writer::StringWriter::oneshot([&lhs](auto &sink) { return lhs.serialize(sink); }));
                        CAPTURE(writer::StringWriter::oneshot([&rhs](auto &sink) { return rhs.serialize(sink); }));
                        CHECK(hasher(lhs) == hasher(rhs));
                    }
                }
            }
        };
        check_equal_nodes_hash_equal("std::hash", std::hash<Node>{});
        check_equal_nodes_hash_equal("dice_hash wyhash", dice::hash::DiceHashwyhash<Node>{});
        check_equal_nodes_hash_equal("dice_hash xxh3", dice::hash::DiceHashxxh3<Node>{});
        check_equal_nodes_hash_equal("dice_hash rapidhash", dice::hash::DiceHashrapidhash<Node>{});
        check_equal_nodes_hash_equal("dice_hash Martinus", dice::hash::DiceHashMartinus<Node>{});
    }

    TEST_CASE("value-equivalent distinct terms stay distinct in hash and tree containers") {
        for (auto const &pair : equivalent_pairs) {
            CAPTURE(std::string_view{pair.datatype});

            Node const smaller = make_typed_literal(pair.smaller, pair.datatype);
            Node const greater = make_typed_literal(pair.greater, pair.datatype);

            std::unordered_set<Node> const hashed{smaller, greater};
            CHECK(hashed.size() == 2);
            CHECK(hashed.contains(smaller));
            CHECK(hashed.contains(greater));

            std::set<Node> const tree{greater, smaller};
            CHECK(tree.size() == 2);
            CHECK(*tree.begin() == smaller);
            CHECK(*std::next(tree.begin()) == greater);
        }
    }

    TEST_CASE("unordered comparisons fall back to the lexical form ordering") {
        SUBCASE("NaN against an ordered value") {
            Literal const nan = make_typed_literal("NaN", "double");
            Literal const one = make_typed_literal("1.0", "double");

            REQUIRE(nan.compare(one) == std::partial_ordering::unordered);
            // canonical forms "NaN" and "1.0E0": 'N' > '1'
            CHECK(nan.order(one) == std::strong_ordering::greater);
            CHECK(one.order(nan) == std::strong_ordering::less);
            CHECK_FALSE(nan.order_eq(one));
        }

        SUBCASE("unregistered datatype has no value comparison") {
            IRI const datatype{"http://example.org/unregistered"};
            Literal const abc = Literal::make_typed("abc", datatype);
            Literal const abd = Literal::make_typed("abd", datatype);

            REQUIRE(abc.compare(abd) == std::partial_ordering::unordered);
            CHECK(abc.order(abd) == std::strong_ordering::less);
            CHECK(abd.order(abc) == std::strong_ordering::greater);
        }

        SUBCASE("equal terms of an unregistered datatype stay equivalent") {
            IRI const datatype{"http://example.org/unregistered"};
            Literal const first = Literal::make_typed("abc", datatype);
            Literal const second = Literal::make_typed("abc", datatype);

            REQUIRE(first.backend_handle() == second.backend_handle());
            CHECK(first.order(second) == std::strong_ordering::equivalent);
            CHECK(first.order_eq(second));
        }
    }

    TEST_CASE("different datatypes fall back to the datatype ordering") {
        // xsd:int and xsd:integer hold the same value but are different datatypes
        Literal const int_literal = Literal::make_typed_from_value<datatypes::xsd::Int>(1);
        Literal const integer_literal = Literal::make_typed_from_value<datatypes::xsd::Integer>(1);

        REQUIRE(int_literal.compare(integer_literal) == std::partial_ordering::equivalent);
        CHECK_FALSE(int_literal.order_eq(integer_literal));
        CHECK(int_literal.order(integer_literal) == std::strong_ordering::greater);
        CHECK(integer_literal.order(int_literal) == std::strong_ordering::less);
    }

    TEST_CASE("null terms order before everything else") {
        Literal const null_literal{};

        CHECK(null_literal.order(null_literal) == std::strong_ordering::equivalent);
        CHECK(null_literal.order(make_typed_literal("1", "integer")) == std::strong_ordering::less);
        CHECK(make_typed_literal("1", "integer").order(null_literal) == std::strong_ordering::greater);
    }

    TEST_CASE("Node::order (ORDER BY) is a strict total order within a datatype") {
        for (auto const &group : make_datatype_groups()) {
            CAPTURE(std::string_view{group.name});
            auto const &terms = group.terms;
            size_t const count = terms.size();

            // reflexivity and antisymmetry
            for (size_t lhs = 0; lhs < count; ++lhs) {
                CHECK(terms[lhs].order(terms[lhs]) == std::strong_ordering::equivalent);

                for (size_t rhs = 0; rhs < count; ++rhs) {
                    auto const lhs_to_rhs = terms[lhs].order(terms[rhs]);
                    auto const rhs_to_lhs = terms[rhs].order(terms[lhs]);

                    if (lhs_to_rhs == std::strong_ordering::less) {
                        CHECK(rhs_to_lhs == std::strong_ordering::greater);
                    } else if (lhs_to_rhs == std::strong_ordering::greater) {
                        CHECK(rhs_to_lhs == std::strong_ordering::less);
                    } else {
                        CHECK(rhs_to_lhs == std::strong_ordering::equivalent);
                    }
                }
            }

            // matrix of the pairwise orderings (less: -1, equivalent: 0, greater: +1), indexed [lhs * count + rhs]
            // used to check transitivity of comparisons
            std::vector<int8_t> orderings(count * count);
            for (size_t lhs = 0; lhs < count; ++lhs) {
                for (size_t rhs = 0; rhs < count; ++rhs) {
                    auto const result = terms[lhs].order(terms[rhs]);
                    int8_t ordering_sign = 0;
                    if (result == std::strong_ordering::less) {
                        ordering_sign = -1;
                    } else if (result == std::strong_ordering::greater) {
                        ordering_sign = 1;
                    }
                    orderings[lhs * count + rhs] = ordering_sign;
                }
            }

            // transitivity of <, and of < composed with equivalence
            for (size_t first = 0; first < count; ++first) {
                for (size_t second = 0; second < count; ++second) {
                    if (orderings[first * count + second] != -1) {
                        continue;
                    }
                    for (size_t third = 0; third < count; ++third) {
                        if (orderings[second * count + third] == -1 || orderings[second * count + third] == 0) {
                            CAPTURE(first);
                            CAPTURE(second);
                            CAPTURE(third);
                            CHECK(orderings[first * count + third] == -1);
                        }
                    }
                }
            }

            // transitivity of equivalence
            for (size_t first = 0; first < count; ++first) {
                for (size_t second = 0; second < count; ++second) {
                    if (orderings[first * count + second] != 0) {
                        continue;
                    }
                    for (size_t third = 0; third < count; ++third) {
                        if (orderings[second * count + third] == 0) {
                            CAPTURE(first);
                            CAPTURE(second);
                            CAPTURE(third);
                            CHECK(orderings[first * count + third] == 0);
                        }
                    }
                }
            }
        }
    }

    TEST_CASE("order (ORDER BY) agrees with compare (FILTER) when the values are ordered") {
        auto const nodes = make_unique_nodes_from_dt_groups();

        for (auto const &lhs : nodes) {
            for (auto const &rhs : nodes) {
                if (!lhs.is_literal() || !rhs.is_literal()) {
                    continue;
                }

                auto const compare_result = lhs.as_literal().compare(rhs.as_literal());
                auto const order_result = lhs.order(rhs);

                if (compare_result == std::partial_ordering::less) {
                    CHECK(order_result == std::strong_ordering::less);
                } else if (compare_result == std::partial_ordering::greater) {
                    CHECK(order_result == std::strong_ordering::greater);
                }
            }
        }
    }

    TEST_CASE("sorting a datatype is independent of the input order") {
        for (auto const &group : make_datatype_groups()) {
            CAPTURE(std::string_view{group.name});

            auto sorted = [](std::vector<Node> terms) {
                std::ranges::sort(terms, [](Node const &lhs, Node const &rhs) { return lhs.order_lt(rhs); });
                return terms;
            };

            std::vector<Node> reversed = group.terms;
            std::ranges::reverse(reversed);

            auto const ascending = sorted(group.terms);
            auto const from_reversed = sorted(reversed);

            REQUIRE(ascending.size() == group.terms.size());
            CHECK(ascending == from_reversed);

            for (size_t index = 1; index < ascending.size(); ++index) {
                CHECK(ascending[index - 1].order_le(ascending[index]));
                CHECK_FALSE(ascending[index].order_lt(ascending[index - 1]));
            }
        }
    }

    TEST_CASE("value-equivalent distinct terms are not merged when deduplicating by order") {
        for (auto const &pair : equivalent_pairs) {
            CAPTURE(std::string_view{pair.datatype});

            std::vector<Node> terms{
                make_typed_literal(pair.greater, pair.datatype),
                make_typed_literal(pair.smaller, pair.datatype)
            };

            std::ranges::sort(terms, [](Node const &lhs, Node const &rhs) { return lhs.order_lt(rhs); });
            auto const duplicates = std::ranges::unique(terms, [](Node const &lhs, Node const &rhs) { return lhs.order_eq(rhs); });
            terms.erase(duplicates.begin(), duplicates.end());

            REQUIRE(terms.size() == 2);
            CHECK(terms[0] == Node{make_typed_literal(pair.smaller, pair.datatype)});
            CHECK(terms[1] == Node{make_typed_literal(pair.greater, pair.datatype)});
        }
    }

    TEST_CASE("ordering/comparing std:string and needs_escape") {
        struct EscapeData {
            char const *lexical_form;
            bool needs_escape;
        };

        // interleaved in byte order: '\n' 0x0A, '\r' 0x0D, '"' 0x22, '\\' 0x5C, 'A' 0x41, 'a' 0x61, 'z' 0x7A
        constexpr EscapeData test_data[] = {
                {"", false},
                {"\n", true},
                {"\"quoted\"", true},
                {"ABC", false},
                {"a\"b", true},
                {"a\\b", true},
                {"abc", false},
                {"abd", false},
                {"z\rz", true},
                {"zzz", false},
        };

        SUBCASE("xsd:string compares by lexical form regardless of escaping") {
            for (auto const &lhs_sample : test_data) {
                for (auto const &rhs_sample : test_data) {
                    CAPTURE(std::string_view{lhs_sample.lexical_form});
                    CAPTURE(std::string_view{rhs_sample.lexical_form});

                    Literal const lhs = make_typed_literal(lhs_sample.lexical_form, "string");
                    Literal const rhs = make_typed_literal(rhs_sample.lexical_form, "string");

                    // the expectation comes from the raw lexical forms, not from compare
                    auto const expected = std::string_view{lhs_sample.lexical_form} <=> std::string_view{rhs_sample.lexical_form};

                    CHECK(lhs.compare(rhs) == expected);
                    CHECK(Node{lhs}.order(Node{rhs}) == expected);
                }
            }
        }

        SUBCASE("rdf:langString compares by lexical form regardless of escaping") {
            for (auto const &lhs_sample : test_data) {
                for (auto const &rhs_sample : test_data) {
                    CAPTURE(std::string_view{lhs_sample.lexical_form});
                    CAPTURE(std::string_view{rhs_sample.lexical_form});

                    Literal const lhs = Literal::make_lang_tagged(lhs_sample.lexical_form, "en");
                    Literal const rhs = Literal::make_lang_tagged(rhs_sample.lexical_form, "en");

                    // same language tag, so the lexical forms decide
                    auto const expected = std::string_view{lhs_sample.lexical_form} <=> std::string_view{rhs_sample.lexical_form};

                    CHECK(lhs.compare(rhs) == expected);
                    CHECK(Node{lhs}.order(Node{rhs}) == expected);
                }
            }
        }
    }

    TEST_CASE("comparing xsd:anyURI with xsd:string") {
        // the cpp_type for anyURI used to be xsd:string
        // asan was triggering heap-use-after-free, when anyURI's cpp_type was xsd:string

        std::string const uri_value = "http://example.org/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        Literal const uri = Literal::make_typed(uri_value, IRI{"http://www.w3.org/2001/XMLSchema#anyURI"});

        Literal const str = make_typed_literal(std::string(uri_value.size(), 'h'), "string");

        REQUIRE(!uri.null());
        REQUIRE(!str.null());

        auto const expected = str <=> uri;
        REQUIRE(expected == std::strong_ordering::less);

        CHECK(str.compare(uri) == expected);
    }
}
