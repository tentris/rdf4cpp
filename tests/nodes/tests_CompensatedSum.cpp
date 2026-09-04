#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp.hpp>
#include <rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.hpp>

#include <bit>
#include <cmath>

using namespace rdf4cpp;

namespace {

// the plain fold this is meant to improve on. Seeded with the first value, just like CompensatedSum:
// a "0"^^xsd:integer seed would make every owl:rational and owl:real fold null.
Literal naive_sum(std::span<Literal const> lits) {
    if (lits.empty()) {
        return Literal::make_typed_from_value<datatypes::xsd::Integer>(0);
    }

    Literal sum = lits.front();
    for (Literal const &lit : lits.subspan(1)) {
        sum += lit;
    }
    return sum;
}

Literal compensated_sum(std::span<Literal const> lits) {
    CompensatedSum sum;
    for (Literal const &lit : lits) {
        sum.add(lit);
    }
    return sum.value();
}

template<datatypes::LiteralDatatype T>
std::vector<Literal> repeat(typename T::cpp_type const &value, size_t n) {
    return std::vector<Literal>(n, Literal::make_typed_from_value<T>(value));
}

}  // namespace

TEST_CASE("empty sum is zero") {
    CompensatedSum sum;
    CHECK_EQ(sum.value(), Literal::make_typed_from_value<datatypes::xsd::Integer>(0));
    CHECK_EQ(sum.value().datatype(), IRI{datatypes::xsd::Integer::identifier});
}

TEST_CASE("compensation recovers what the naive fold loses") {
    SUBCASE("ten times 0.1") {
        auto const lits = repeat<datatypes::xsd::Double>(0.1, 10);

        CHECK_EQ(naive_sum(lits).value<datatypes::xsd::Double>(), 0.9999999999999999);
        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Double>(), 1.0);
    }

    SUBCASE("hundred times 0.01") {
        auto const lits = repeat<datatypes::xsd::Double>(0.01, 100);

        CHECK_EQ(naive_sum(lits).value<datatypes::xsd::Double>(), 1.0000000000000007);
        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Double>(), 1.0);
    }

    SUBCASE("the exact result inlines, the drifted one does not") {
        auto const lits = repeat<datatypes::xsd::Double>(0.01, 100);

        CHECK_FALSE(naive_sum(lits).is_inlined());
        CHECK(compensated_sum(lits).is_inlined());
    }

    SUBCASE("xsd:float") {
        auto const lits = repeat<datatypes::xsd::Float>(0.1f, 10);

        CHECK_EQ(naive_sum(lits).value<datatypes::xsd::Float>(), 1.0000001f);
        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Float>(), 1.0f);
    }
}

TEST_CASE("exact datatypes match the naive fold") {
    SUBCASE("xsd:integer") {
        std::vector<Literal> lits;
        for (int i = 1; i <= 100; ++i) {
            lits.push_back(Literal::make_typed_from_value<datatypes::xsd::Integer>(i));
        }

        CHECK_EQ(compensated_sum(lits), naive_sum(lits));
        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Integer>(), 5050);
    }

    SUBCASE("xsd:decimal") {
        auto const lits = repeat<datatypes::xsd::Decimal>(BigDecimal<>{"0.01"}, 100);

        CHECK_EQ(compensated_sum(lits), naive_sum(lits));
        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Decimal>(), BigDecimal<>{"1.0"});
    }

    SUBCASE("owl:rational") {
        auto const lits = repeat<datatypes::owl::Rational>(boost::multiprecision::cpp_rational{1, 3}, 3);

        CHECK_EQ(compensated_sum(lits), naive_sum(lits));
        CHECK_EQ(compensated_sum(lits).value<datatypes::owl::Rational>(), 1);
    }

    SUBCASE("xsd:int is a numeric stub, its arithmetic is exact xsd:integer") {
        auto const lits = repeat<datatypes::xsd::Int>(1, 10);

        CHECK_EQ(compensated_sum(lits), naive_sum(lits));
        CHECK_EQ(compensated_sum(lits).datatype(), IRI{datatypes::xsd::Integer::identifier});
    }
}

TEST_CASE("owl:real is inexact, so it takes the compensated path") {
    auto const lits = repeat<datatypes::owl::Real>(boost::multiprecision::cpp_bin_float_quad{0.1}, 10);

    // the drift is not observable in the result: owl:real has no serializer of its own, so its
    // canonical form is whatever operator<< writes at the default precision of 6 significant digits
    CHECK_EQ(compensated_sum(lits), naive_sum(lits));
    CHECK_EQ(compensated_sum(lits).value<datatypes::owl::Real>(), 1);
}

TEST_CASE("datatype promotion matches a fold of operator+") {
    SUBCASE("xsd:integer then xsd:double") {
        std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::xsd::Integer>(1),
                                        Literal::make_typed_from_value<datatypes::xsd::Double>(0.5)};

        CHECK_EQ(compensated_sum(lits).datatype(), naive_sum(lits).datatype());
        CHECK_EQ(compensated_sum(lits).datatype(), IRI{datatypes::xsd::Double::identifier});
        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Double>(), 1.5);
    }

    SUBCASE("xsd:double then xsd:integer, compensation already latched") {
        std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::xsd::Double>(0.5),
                                        Literal::make_typed_from_value<datatypes::xsd::Integer>(1)};

        CHECK_EQ(compensated_sum(lits).datatype(), IRI{datatypes::xsd::Double::identifier});
        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Double>(), 1.5);
    }

    SUBCASE("xsd:decimal narrows through xsd:float, as operator+ does") {
        std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::xsd::Decimal>(BigDecimal<>{"0.5"}),
                                        Literal::make_typed_from_value<datatypes::xsd::Float>(0.25f)};

        CHECK_EQ(compensated_sum(lits).datatype(), naive_sum(lits).datatype());
        CHECK_EQ(compensated_sum(lits).datatype(), IRI{datatypes::xsd::Float::identifier});
    }
}

TEST_CASE("errors poison the sum") {
    SUBCASE("non-numeric value") {
        CompensatedSum sum;
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Double>(1.0));
        sum.add(Literal::make_simple("not a number"));

        CHECK(sum.value().null());
    }

    SUBCASE("null literal") {
        CompensatedSum sum;
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Integer>(1));
        sum.add(Literal{});

        CHECK(sum.value().null());
    }

    SUBCASE("once poisoned it stays poisoned") {
        CompensatedSum sum;
        sum.add(Literal{});
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Integer>(1));

        CHECK(sum.value().null());
    }

    SUBCASE("owl:real and xsd:double have no common type") {
        std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::owl::Real>(boost::multiprecision::cpp_bin_float_quad{1}),
                                        Literal::make_typed_from_value<datatypes::xsd::Double>(1.0)};

        CHECK(naive_sum(lits).null());
        CHECK(compensated_sum(lits).null());
    }
}

TEST_CASE("NaN is a value, as it is for operator+") {
    std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::xsd::Double>(1.0),
                                    Literal::make_typed_from_value<datatypes::xsd::Double>(std::numeric_limits<double>::quiet_NaN())};

    CHECK(std::isnan(compensated_sum(lits).value<datatypes::xsd::Double>()));
}

TEST_CASE("intermediate results are not placed into the node storage") {
    storage::reference_node_storage::UnsyncReferenceNodeStorage ns;

    auto const lits = repeat<datatypes::xsd::Decimal>(BigDecimal<>{"0.01"}, 100);

    CompensatedSum sum{ns};
    auto const size_before = ns.size();

    for (Literal const &lit : lits) {
        sum.add(lit);
    }

    // only the datatype IRI may have been added, never any of the 100 partial sums
    CHECK_LE(ns.size() - size_before, 1);

    auto const result = sum.value();
    CHECK_EQ(result.value<datatypes::xsd::Decimal>(), BigDecimal<>{"1.0"});
}

// both document current behaviour, so that a follow-up has a target
TEST_CASE("known Kahan limitations") {
    SUBCASE("a value dwarfing the running sum") {
        // Kahan loses the compensation when |value| >> |sum|; Neumaier would return 1.0 here
        std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::xsd::Double>(1e16),
                                        Literal::make_typed_from_value<datatypes::xsd::Double>(1.0),
                                        Literal::make_typed_from_value<datatypes::xsd::Double>(-1e16)};

        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Double>(), 0.0);
    }

    SUBCASE("an infinite value") {
        // the correction term becomes inf - inf = NaN and poisons the total, where operator+ keeps
        // the infinity. No compensation scheme avoids this, it needs a finiteness guard.
        std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::xsd::Double>(1.0),
                                        Literal::make_typed_from_value<datatypes::xsd::Double>(std::numeric_limits<double>::infinity())};

        CHECK(std::isinf(naive_sum(lits).value<datatypes::xsd::Double>()));
        CHECK(std::isnan(compensated_sum(lits).value<datatypes::xsd::Double>()));
    }
}
