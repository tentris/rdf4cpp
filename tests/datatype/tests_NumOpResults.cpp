#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

#include <cmath>
#include <limits>
#include <iostream>

using namespace rdf4cpp;
using namespace datatypes::xsd;
using datatypes::DynamicError;

// https://www.w3.org/TR/xpath-functions/#op.numeric
TEST_SUITE("numeric op results") {
    TEST_CASE("integral") {
        SUBCASE("integer") {
            Integer::cpp_type const zero{0};
            Integer::cpp_type const one{1};

            CHECK(Integer::div(one, zero) == nonstd::make_unexpected(DynamicError::DivideByZero));
        }

        SUBCASE("decimal") {
            // https://www.w3.org/TR/xpath-functions/#op.numeric

            Decimal::cpp_type const zero{0};
            Decimal::cpp_type const one{1};
            Decimal::cpp_type const three{3};
            Decimal::cpp_type const min{std::numeric_limits<Decimal::cpp_type>::min()};
            Decimal::cpp_type const max{std::numeric_limits<Decimal::cpp_type>::max()};
            Decimal::cpp_type const big_number{"100000000000"};

            // "If the number of digits in the mathematical result exceeds the number of digits
            // that the implementation retains for that operation, the result is truncated or rounded in an ·implementation-defined· manner."
            if (max != 0) {  // if max == 0 -> unlimited
                auto r = Decimal::div(one, three);
                CHECK(r.has_value());
                CHECK(r.value() > zero);
                CHECK(r.value() < one);
            }

            // "For xs:decimal operations, overflow behavior must raise a dynamic error [err:FOAR0002]."
            if (max != 0) {  // if max == 0 -> unlimited
                CHECK(Decimal::add(max, max) == nonstd::make_unexpected(DynamicError::OverOrUnderFlow));
                CHECK(Decimal::sub(max, -max) == nonstd::make_unexpected(DynamicError::OverOrUnderFlow));
                CHECK(Decimal::mul(max, big_number) == nonstd::make_unexpected(DynamicError::OverOrUnderFlow));
                CHECK(Decimal::div(max, one / big_number) == nonstd::make_unexpected(DynamicError::OverOrUnderFlow));
            }

            // "On underflow, 0.0 must be returned."
            if (min != 0) {  // if min == 0 -> unlimited
                CHECK(Decimal::div(one, max) == zero);
            }

            // https://www.w3.org/TR/xpath-functions/#func-numeric-divide
            // "A dynamic error is raised [err:FOAR0001] for xs:decimal and xs:integer operands, if the divisor is (positive or negative) zero."
            CHECK(Decimal::div(one, zero) == nonstd::make_unexpected(DynamicError::DivideByZero));
        }
    }

    // checking for required IEEE conformance, realistically all compilers will fulfill this
    TEST_CASE("floating") {
        Float::cpp_type const zero{0.f};
        Float::cpp_type const one{1.f};
        Float::cpp_type const two{2.f};
        Float::cpp_type const min{std::numeric_limits<Float::cpp_type>::min()};
        Float::cpp_type const max{std::numeric_limits<Float::cpp_type>::max()};
        Float::cpp_type const inf{std::numeric_limits<Float::cpp_type>::infinity()};
        Float::cpp_type const nan{std::numeric_limits<Float::cpp_type>::quiet_NaN()};

        auto const correct_overflow_res = [&](auto const value) {
            return value == max || value == inf;
        };

        auto const correct_underflow_res = [&](auto const value) {
            return value == -one || std::fpclassify(value) == FP_SUBNORMAL;
        };

        // https://www.w3.org/TR/xpath-functions/#func-numeric-add
        CHECK(correct_overflow_res(Float::add(max, one)));
        CHECK(Float::add(inf, one) == inf);
        CHECK(Float::add(-inf, one) == -inf);
        CHECK(Float::add(inf, inf) == inf);
        CHECK(Float::add(-inf, -inf) == -inf);
        CHECK(std::isnan(Float::add(-inf, inf).value()));
        CHECK(std::isnan(Float::add(inf, -inf).value()));
        CHECK(std::isnan(Float::add(nan, one).value()));

        // https://www.w3.org/TR/xpath-functions/#func-numeric-subtract
        CHECK(correct_underflow_res(Float::sub(min, one).value()));
        CHECK(Float::sub(one, inf) == -inf);
        CHECK(Float::sub(one, -inf) == inf);
        CHECK(std::isnan(Float::sub(inf, inf).value()));
        CHECK(std::isnan(Float::sub(-inf, -inf).value()));
        CHECK(Float::sub(inf, -inf) == inf);
        CHECK(Float::sub(-inf, inf) == -inf);
        CHECK(std::isnan(Float::sub(nan, one).value()));

        // https://www.w3.org/TR/xpath-functions/#func-numeric-multiply
        CHECK(correct_overflow_res(Float::mul(max, two)));
        CHECK(std::isnan(Float::mul(inf, zero).value()));
        CHECK(std::isnan(Float::mul(zero, inf).value()));
        CHECK(Float::mul(one, inf) == inf);
        CHECK(Float::mul(-inf, one) == -inf);
        CHECK(std::isnan(Float::mul(nan, one).value()));

        // https://www.w3.org/TR/xpath-functions/#func-numeric-divide
        CHECK(Float::div(one, zero) == inf);
        CHECK(Float::div(-one, zero) == -inf);
        CHECK(std::isnan(Float::div(zero, zero).value()));
        CHECK(std::isnan(Float::div(inf, inf).value()));
        CHECK(std::isnan(Float::div(-inf, -inf).value()));
        CHECK(std::isnan(Float::div(-inf, inf).value()));
        CHECK(std::isnan(Float::div(inf, -inf).value()));
        CHECK(std::isnan(Float::div(nan, one).value()));
    }
}
