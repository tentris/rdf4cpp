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
    Literal sum{};
    if (lits.empty()) {
        sum = Literal::make_typed_from_value<datatypes::xsd::Integer>(0);
    } else {
        sum = lits.front();
        for (Literal const &lit : lits.subspan(1)) {
            sum += lit;
        }
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
        CHECK_FALSE(sum.poisoned());

        SUBCASE("single value, non-numeric") {
            sum.add("spherical cow"_xsd_string);
        }

        SUBCASE("start with non-numeric") {
            sum.add("not a number"_xsd_string);
            CHECK(sum.poisoned());
            sum.add(1.0_xsd_double);
        }

        SUBCASE("start with numeric") {
            sum.add(1.0_xsd_double);
            CHECK_FALSE(sum.poisoned());
            sum.add("not a number"_xsd_string);
        }

        CHECK(sum.poisoned());
        CHECK(sum.value().null());
    }

    SUBCASE("null literal") {
        CompensatedSum sum;
        CHECK_FALSE(sum.poisoned());
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Integer>(1));
        CHECK_FALSE(sum.poisoned());

        SUBCASE("Literal") {
            sum.add(Literal{});
        }

        SUBCASE("DeferredLiteral") {
            sum.add(DeferredLiteral{});
        }

        CHECK(sum.poisoned());
        CHECK(sum.value().null());
    }

    SUBCASE("once poisoned it stays poisoned") {
        CompensatedSum sum;
        CHECK_FALSE(sum.poisoned());
        sum.add(Literal{});
        CHECK(sum.poisoned());
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Integer>(1));
        CHECK(sum.poisoned());
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
        sum.add(lit, 3);
    }

    // only the datatype IRI may have been added, never any of the 100 partial sums or the doubled terms
    CHECK_LE(ns.size() - size_before, 1);

    auto const result = sum.value();
    CHECK_EQ(result.value<datatypes::xsd::Decimal>(), BigDecimal<>{"3.0"});
}

TEST_CASE("multiplicity") {
    auto const with_multiplicity = [](Literal const &lit, uint64_t n) {
        CompensatedSum sum;
        sum.add(lit, n);
        return sum.value();
    };

    SUBCASE("equals adding the value n times") {
        for (uint64_t const n : {2, 3, 7, 10, 100, 1000}) {
            CAPTURE(n);
            auto const lit = Literal::make_typed_from_value<datatypes::xsd::Double>(0.1);
            CHECK_EQ(with_multiplicity(lit, n), compensated_sum(std::vector<Literal>(n, lit)));
        }

        CHECK_EQ(with_multiplicity(Literal::make_typed_from_value<datatypes::xsd::Float>(0.1f), 10).value<datatypes::xsd::Float>(), 1.0f);
    }

    SUBCASE("the rounding of the product does not leak") {
        // 0.1 * 3 rounds to 0.30000000000000004, ten of those would drift to 3.0000000000000004
        CompensatedSum sum;
        for (int i = 0; i < 10; ++i) {
            sum.add(Literal::make_typed_from_value<datatypes::xsd::Double>(0.1), 3);
        }

        CHECK_EQ(sum.value().value<datatypes::xsd::Double>(), 3.0);
    }

    SUBCASE("a multiplied term dwarfed by the running sum") {
        CompensatedSum sum;
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Double>(1e16));
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Double>(1.0), 3);
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Double>(-1e16));

        CHECK_EQ(sum.value().value<datatypes::xsd::Double>(), 3.0);
    }

    SUBCASE("exact datatypes") {
        CHECK_EQ(with_multiplicity(Literal::make_typed_from_value<datatypes::xsd::Integer>(7), 6).value<datatypes::xsd::Integer>(), 42);
        CHECK_EQ(with_multiplicity(Literal::make_typed_from_value<datatypes::xsd::Decimal>(BigDecimal<>{"0.01"}), 100).value<datatypes::xsd::Decimal>(), BigDecimal<>{"1.0"});

        // no common type with xsd:integer, so the multiplicity must never become one
        CHECK_EQ(with_multiplicity(Literal::make_typed_from_value<datatypes::owl::Rational>(boost::multiprecision::cpp_rational{1, 3}), 3).value<datatypes::owl::Rational>(), 1);
        CHECK_EQ(with_multiplicity(Literal::make_typed_from_value<datatypes::owl::Real>(boost::multiprecision::cpp_bin_float_quad{0.1}), 10).value<datatypes::owl::Real>(), 1);
    }

    SUBCASE("xsd:int is a numeric stub, its arithmetic is exact xsd:integer") {
        auto const result = with_multiplicity(Literal::make_typed_from_value<datatypes::xsd::Int>(1), 10);

        CHECK_EQ(result.datatype(), IRI{datatypes::xsd::Integer::identifier});
        CHECK_EQ(result.value<datatypes::xsd::Integer>(), 10);
    }

    SUBCASE("datatype promotion") {
        CompensatedSum sum;
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Integer>(1), 2);
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Double>(0.5), 2);

        CHECK_EQ(sum.value(), Literal::make_typed_from_value<datatypes::xsd::Double>(3.0));
    }

    SUBCASE("zero adds nothing") {
        CompensatedSum sum;
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Integer>(5), 0);
        CHECK_EQ(sum.value(), Literal::make_typed_from_value<datatypes::xsd::Integer>(0));

        sum.add(Literal::make_typed_from_value<datatypes::xsd::Integer>(1));
        sum.add(Literal::make_typed_from_value<datatypes::xsd::Integer>(5), 0);
        CHECK_EQ(sum.value(), Literal::make_typed_from_value<datatypes::xsd::Integer>(1));
    }

    SUBCASE("non-numeric value poisons") {
        CHECK(with_multiplicity("spherical cow"_xsd_string, 2).null());
    }

    SUBCASE("overflow and NaN") {
        CHECK(std::isinf(with_multiplicity(Literal::make_typed_from_value<datatypes::xsd::Double>(std::numeric_limits<double>::max()), 2).value<datatypes::xsd::Double>()));
        CHECK(std::isnan(with_multiplicity(Literal::make_typed_from_value<datatypes::xsd::Double>(std::numeric_limits<double>::quiet_NaN()), 3).value<datatypes::xsd::Double>()));
    }
}

TEST_CASE("a value dwarfing the running sum") {
    // the correction is accumulated instead of applied to the next value, which is what Kahan
    // would lose here, in either order of magnitude
    SUBCASE("|value| << |sum|") {
        std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::xsd::Double>(1e16),
                                        Literal::make_typed_from_value<datatypes::xsd::Double>(1.0),
                                        Literal::make_typed_from_value<datatypes::xsd::Double>(-1e16)};

        CHECK_EQ(naive_sum(lits).value<datatypes::xsd::Double>(), 0.0);
        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Double>(), 1.0);
    }

    SUBCASE("|value| >> |sum|") {
        std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::xsd::Double>(1.0),
                                        Literal::make_typed_from_value<datatypes::xsd::Double>(1e16),
                                        Literal::make_typed_from_value<datatypes::xsd::Double>(-1e16)};

        CHECK_EQ(naive_sum(lits).value<datatypes::xsd::Double>(), 0.0);
        CHECK_EQ(compensated_sum(lits).value<datatypes::xsd::Double>(), 1.0);
    }
}

TEST_CASE("infinity is a value, as it is for operator+") {
    // an infinite total has no meaningful loss to compensate (it would be inf - inf = NaN),
    // so the compensation is left untouched while it is one
    auto const inf = Literal::make_typed_from_value<datatypes::xsd::Double>(std::numeric_limits<double>::infinity());
    auto const one = Literal::make_typed_from_value<datatypes::xsd::Double>(1.0);

    SUBCASE("as the first value") {
        std::vector<Literal> const lits{inf, one};

        CHECK(std::isinf(compensated_sum(lits).value<datatypes::xsd::Double>()));
    }

    SUBCASE("as a later value") {
        std::vector<Literal> const lits{one, inf};

        CHECK(std::isinf(compensated_sum(lits).value<datatypes::xsd::Double>()));
    }

    SUBCASE("reached by overflow") {
        auto const lits = repeat<datatypes::xsd::Double>(std::numeric_limits<double>::max(), 3);

        CHECK(std::isinf(compensated_sum(lits).value<datatypes::xsd::Double>()));
    }

    SUBCASE("owl:real, whose infinity is not std::isinf") {
        std::vector<Literal> const lits{Literal::make_typed_from_value<datatypes::owl::Real>(boost::multiprecision::cpp_bin_float_quad{1}),
                                        Literal::make_typed_from_value<datatypes::owl::Real>(std::numeric_limits<boost::multiprecision::cpp_bin_float_quad>::infinity())};

        CHECK(isinf(compensated_sum(lits).value<datatypes::owl::Real>()));
    }

    SUBCASE("both signs still cancel to NaN") {
        std::vector<Literal> const lits{one, inf, Literal::make_typed_from_value<datatypes::xsd::Double>(-std::numeric_limits<double>::infinity())};

        CHECK(std::isnan(naive_sum(lits).value<datatypes::xsd::Double>()));
        CHECK(std::isnan(compensated_sum(lits).value<datatypes::xsd::Double>()));
    }
}

TEST_CASE("non numeric literal nulls the sum") {
    CompensatedSum s;
    s.add(1_xsd_integer);
    s.add(2.0_xsd_double);

    CHECK_EQ(s.value(), 3.0_xsd_double);

    s.add("Hello"_xsd_string);
    CHECK(s.value().null());
}

TEST_CASE("compensation type changes") {
    CompensatedSum s;
    s.add(1.0_xsd_float);
    s.add(2.0_xsd_float);
    s.add(3.0_xsd_double);

    auto const res = s.value();
    CHECK(res.datatype_eq<datatypes::xsd::Double>());
    CHECK_EQ(res, 6.0_xsd_double);
}

TEST_CASE("near max") {
    auto const d_max = Literal::make_typed_from_value<datatypes::xsd::Double>(std::numeric_limits<double>::max());

    std::array const rows = {
        std::make_pair(d_max, 1),
        std::make_pair(-d_max, 2),
        std::make_pair(d_max, 1)
    };

    CompensatedSum s;
    for (auto const &row : rows) {
        s.add(row.first, row.second);
    }

    Literal manual_sum = 0_xsd_integer;
    for (auto const &row : rows) {
        for (int repeat = 0; repeat < row.second; ++repeat) {
            manual_sum += row.first;
        }
    }

    CHECK_EQ(s.value(), manual_sum);
}

TEST_CASE("poisoned") {
    CompensatedSum s;
    s.add(1_xsd_integer);
    CHECK_FALSE(s.poisoned());

    s.add("hello"_xsd_string);
    CHECK(s.poisoned());
    CHECK(s.value().null());
}
