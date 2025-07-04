#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

#include <charconv>
#include <limits>

using namespace rdf4cpp;

TEST_CASE("Literal - logical ops") {
    SUBCASE("Literal - logical ops - results") {
        SUBCASE("Literal - logical ops - bool results - and") {
            {
                auto const lhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);

                auto const res = lhs && rhs;
                auto const res2 = rhs && lhs;
                CHECK(res == Literal::make_typed_from_value<datatypes::xsd::Boolean>(false));
                CHECK(res2 == Literal::make_typed_from_value<datatypes::xsd::Boolean>(false));
            }
            {
                auto const lhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);

                auto const res = lhs && rhs;
                auto const res2 = rhs && lhs;
                CHECK(res == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
                CHECK(res2 == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
            }
            {
                auto const lhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);

                auto const res = lhs && rhs;
                auto const res2 = rhs && lhs;
                CHECK(res == Literal::make_typed_from_value<datatypes::xsd::Boolean>(false));
                CHECK(res2 == Literal::make_typed_from_value<datatypes::xsd::Boolean>(false));
            }
            {
                auto const lhs = Literal{};
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);

                auto const res = lhs && rhs;
                auto const res2 = rhs && lhs;
                CHECK(res == Literal::make_typed_from_value<datatypes::xsd::Boolean>(false));
                CHECK(res2 == Literal::make_typed_from_value<datatypes::xsd::Boolean>(false));
            }
            {
                auto const lhs = Literal{};
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);

                auto const res = lhs && rhs;
                auto const res2 = rhs && lhs;
                CHECK(res.null());
                CHECK(res2.null());
            }
            {
                auto const lhs = Literal{};
                auto const rhs = Literal{};

                auto const res = lhs && rhs;
                auto const res2 = rhs && lhs;
                CHECK(res.null());
                CHECK(res2.null());
            }
        }

        SUBCASE("Literal - logical ops - bool results - or") {
            {
                auto const lhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);

                auto const res = lhs || rhs;
                auto const res2 = rhs || lhs;
                CHECK(res == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
                CHECK(res2 == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
            }
            {
                auto const lhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);

                auto const res = lhs || rhs;
                auto const res2 = rhs || lhs;
                CHECK(res == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
                CHECK(res2 == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
            }
            {
                auto const lhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);

                auto const res = lhs || rhs;
                auto const res2 = rhs || lhs;
                CHECK(res == Literal::make_typed_from_value<datatypes::xsd::Boolean>(false));
                CHECK(res2 == Literal::make_typed_from_value<datatypes::xsd::Boolean>(false));
            }
            {
                auto const lhs = Literal{};
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);

                auto const res = lhs || rhs;
                auto const res2 = rhs || lhs;
                CHECK(res.null());
                CHECK(res2.null());
            }
            {
                auto const lhs = Literal{};
                auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);

                auto const res = lhs || rhs;
                auto const res2 = rhs || lhs;
                CHECK(res == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
                CHECK(res2 == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
            }
            {
                auto const lhs = Literal{};
                auto const rhs = Literal{};

                auto const res = lhs || rhs;
                auto const res2 = rhs || lhs;
                CHECK(res.null());
                CHECK(res2.null());
            }
        }

        SUBCASE("Literal - logical ops - bool results - not") {
            {
                auto const op = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);
                CHECK(!op == Literal::make_typed_from_value<datatypes::xsd::Boolean>(false));
            }
            {
                auto const op = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);
                CHECK(!op == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
            }
            {
                auto const op = Literal{};
                CHECK((!op).null());
            }
        }
    }
}

#define GENERATE_BINOP_TESTCASE(lhs_type, lhs, op, rhs_type, rhs, expected_type, expected) \
    SUBCASE(#lhs_type " " #op " " #rhs_type) {                                             \
        auto const lhs_lit = Literal::make_typed_from_value<datatypes::xsd::lhs_type>(lhs);                 \
        auto const rhs_lit = Literal::make_typed_from_value<datatypes::xsd::rhs_type>(rhs);                 \
                                                                                           \
        auto const expected_lit = Literal::make_typed_from_value<datatypes::xsd::expected_type>(expected);  \
        CHECK(lhs_lit op rhs_lit == expected_lit);                                         \
    }

#define GENERATE_UNOP_TESTCASE(type, value, op, expected_type, expected)                  \
    SUBCASE(#op " type") {                                                                \
        auto const value_lit = Literal::make_typed_from_value<datatypes::xsd::type>(value);                \
                                                                                          \
        auto const expected_lit = Literal::make_typed_from_value<datatypes::xsd::expected_type>(expected); \
        CHECK(op value_lit == expected_lit);                                              \
    }

TEST_CASE("Literal - numeric ops") {
    // simple promotion test cases
    GENERATE_BINOP_TESTCASE(Float, 42.f, +, Decimal, rdf4cpp::BigDecimal{120.0}, Float, 162.f);
    GENERATE_BINOP_TESTCASE(Decimal, rdf4cpp::BigDecimal{2.f}, *, Float, 120.0, Float, 240.f);
    GENERATE_BINOP_TESTCASE(Integer, 100, -, Float, 1.f, Float, 99.f);
    GENERATE_BINOP_TESTCASE(Float, 100.f, /, Integer, 2, Float, 50.f);

    // simple subtype substitution test cases
    GENERATE_BINOP_TESTCASE(Integer, 6, +, Int, 7, Integer, 13);
    GENERATE_BINOP_TESTCASE(Int, 8, *, Integer, 9, Integer, 72);

    // operator assignment test cases
    GENERATE_BINOP_TESTCASE(Integer, 1, +, Integer , 2, Integer, 3);
    GENERATE_BINOP_TESTCASE(Integer, 12, -, Integer, 1, Integer, 11);
    GENERATE_BINOP_TESTCASE(Integer, 3, *, Integer, 6, Integer, 18);
    GENERATE_BINOP_TESTCASE(Integer, 12, /, Integer, 4, Decimal, rdf4cpp::BigDecimal{3.0});
    GENERATE_BINOP_TESTCASE(Int, 1, +, Integer, 3, Integer, 4);
    GENERATE_BINOP_TESTCASE(Int, 1, +, Decimal, rdf4cpp::BigDecimal{2}, Decimal, rdf4cpp::BigDecimal{3});
    GENERATE_UNOP_TESTCASE(Integer, 1, +, Integer, 1);
    GENERATE_UNOP_TESTCASE(Integer, 1, -, Integer, -1);

    // unary stub tests
    GENERATE_UNOP_TESTCASE(Int, 1, -, Integer, -1);
    GENERATE_UNOP_TESTCASE(Int, 5, +, Integer, 5);

    // binary stub tests
    GENERATE_BINOP_TESTCASE(Int, 1, +, Int, 5, Integer, 6);
    GENERATE_BINOP_TESTCASE(Int, 10, -, Int, 2, Integer, 8);
    GENERATE_BINOP_TESTCASE(Int, 1, /, Int, 5, Decimal, datatypes::xsd::Decimal::cpp_type{"1.0"} / datatypes::xsd::Decimal::cpp_type{"5.0"});

    auto const int_max = std::numeric_limits<datatypes::xsd::Int::cpp_type>::max();
    GENERATE_BINOP_TESTCASE(Int, int_max, *, Int, 2, Integer, static_cast<int64_t>(int_max) * 2);

    SUBCASE("boolean not add") {
        auto const lhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);
        auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);

        CHECK((lhs + rhs).null());
    }

    SUBCASE("add literal type mismatch") {
        auto const lhs = Literal::make_typed_from_value<datatypes::xsd::Float>(1.f);
        auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Boolean>(false);

        CHECK((lhs + rhs).null());
    }

    SUBCASE("op with null") {
        {
            auto const lhs = Literal::make_typed_from_value<datatypes::xsd::Float>(2.f);
            auto const rhs = Literal{};

            CHECK((lhs + rhs).null());
            CHECK((rhs + lhs).null());
        }
        {
            auto const lhs = Literal{};
            auto const rhs = Literal{};

            CHECK((lhs + rhs).null());
        }
        {
            auto const lit = Literal{};
            CHECK((-lit).null());
        }
    }

    SUBCASE("op with unknown") {
        using namespace shorthands;
        using namespace std::chrono_literals;

        auto const unknown = Literal::make_typed("123", IRI{"http://mydatatype.com#int"});

        auto const num = 5_xsd_int;
        auto const tp = Literal::make_typed_from_value<datatypes::xsd::Date>(std::make_pair(YearMonthDay{Year{2000}, std::chrono::January, 1d}, std::nullopt));
        auto const dur = Literal::make_typed_from_value<datatypes::xsd::DayTimeDuration>(100ns);

        auto check_null = [](Literal x) {
            CHECK(x.null());
        };

        auto check_err = [](TriBool b) {
            CHECK_EQ(b, TriBool::Err);
        };

        // simulate code paths
        check_null(num + unknown);
        check_null(unknown + num);
        check_null(tp + unknown); // tp + dur
        check_null(tp - unknown); // tp - dur / tp - tp
        check_null(dur + unknown); // dur + dur
        check_null(dur - unknown); // dur - dur
        check_null(dur / unknown); // dur / dur and dur / num
        check_null(dur * unknown); // dur * num

        // unknown with unknown
        check_null(unknown + unknown);
        check_null(unknown - unknown);
        check_null(unknown / unknown);
        check_null(unknown * unknown);
        check_null(unknown && unknown);
        check_null(unknown || unknown);
        check_null(+unknown);
        check_null(-unknown);
        check_null(!unknown);
        check_err(unknown.eq(unknown));
        check_err(unknown.ne(unknown));
        check_err(unknown.lt(unknown));
        check_err(unknown.le(unknown));
        check_err(unknown.gt(unknown));
        check_err(unknown.ge(unknown));
        CHECK(unknown.order_eq(unknown));
        CHECK_FALSE(unknown.order_ne(unknown));
        CHECK_FALSE(unknown.order_lt(unknown));
        CHECK(unknown.order_le(unknown));
        CHECK_FALSE(unknown.order_gt(unknown));
        CHECK(unknown.order_ge(unknown));
    }
}

// create fake hierarchy
namespace rdf4cpp::datatypes::registry {

constexpr static util::ConstexprString Z{"Z"};
constexpr static util::ConstexprString Y{"Y"};

constexpr static util::ConstexprString A{"A"};
constexpr static util::ConstexprString B{"B"};
constexpr static util::ConstexprString B2{"B2"};
constexpr static util::ConstexprString C{"C"};

} // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {
/**
 *       A   -->   Z
 *     |  |        |
 *     B  B2 -->   Y
 *     |
 *     C
 *
 *     Legend:
 *          A --> B : A is promotable to B
 *
 *          A
 *          |  : B is a subtype of A
 *          B
 */
using A = registry::LiteralDatatypeImpl<registry::A, registry::capabilities::Numeric, registry::capabilities::Comparable, registry::capabilities::Promotable>;

using B = registry::LiteralDatatypeImpl<registry::B, registry::capabilities::Numeric, registry::capabilities::Comparable, registry::capabilities::Subtype>;
using B2 = registry::LiteralDatatypeImpl<registry::B2, registry::capabilities::Numeric, registry::capabilities::Comparable, registry::capabilities::Subtype, registry::capabilities::Promotable>;

using C = registry::LiteralDatatypeImpl<registry::C, registry::capabilities::Numeric, registry::capabilities::Comparable, registry::capabilities::Subtype>;

using Z = registry::LiteralDatatypeImpl<registry::Z, registry::capabilities::Numeric, registry::capabilities::Comparable>;
using Y = registry::LiteralDatatypeImpl<registry::Y, registry::capabilities::Numeric, registry::capabilities::Comparable, registry::capabilities::Subtype>;
}  // namespace rdf4cpp::datatypes::xsd

namespace rdf4cpp::datatypes::registry {

// Z
template<>
struct DatatypeMapping<Z> {
    using cpp_datatype = double;
};

template<>
inline capabilities::Default<Z>::cpp_type capabilities::Default<Z>::from_string(std::string_view s) {
    double value;
    auto const parse_res = std::from_chars(s.data(), s.data() + s.size(), value);

    if (parse_res.ptr != s.data() + s.size()) {
        throw std::runtime_error("XSD Parsing Error");
    } else {
        return value;
    }
}


// Y
template<>
struct DatatypeMapping<Y> {
    using cpp_datatype = float;
};

template<>
struct DatatypeSupertypeMapping<Y> {
   using supertype = xsd::Z;
};

template<>
inline capabilities::Default<Y>::cpp_type capabilities::Default<Y>::from_string(std::string_view s) {
    cpp_type value;
    auto const parse_res = std::from_chars(s.data(), s.data() + s.size(), value);

    if (parse_res.ptr != s.data() + s.size()) {
        throw std::runtime_error("XSD Parsing Error");
    } else {
        return value;
    }
}


// A
template<>
struct DatatypeMapping<A> {
    using cpp_datatype = int64_t;
};

template<>
struct DatatypePromotionMapping<A> {
    using promoted = xsd::Z;
};

template<>
inline capabilities::Default<A>::cpp_type capabilities::Default<A>::from_string(std::string_view s) {
    cpp_type value;
    auto const parse_res = std::from_chars(s.data(), s.data() + s.size(), value);

    if (parse_res.ptr != s.data() + s.size()) {
        throw std::runtime_error("XSD Parsing Error");
    } else {
        return value;
    }
}


// B
template<>
struct DatatypeMapping<B> {
    using cpp_datatype = int32_t;
};

template<>
struct DatatypeSupertypeMapping<B> {
    using supertype = xsd::A;
};

template<>
inline capabilities::Default<B>::cpp_type capabilities::Default<B>::from_string(std::string_view s) {
    cpp_type value;
    auto const parse_res = std::from_chars(s.data(), s.data() + s.size(), value);

    if (parse_res.ptr != s.data() + s.size()) {
        throw std::runtime_error("XSD Parsing Error");
    } else {
        return value;
    }
}


// B2
template<>
struct DatatypeMapping<B2> {
    using cpp_datatype = uint32_t;
};

template<>
struct DatatypeSupertypeMapping<B2> {
    using supertype = xsd::A;
};

template<>
struct DatatypePromotionMapping<B2> {
    using promoted = xsd::Y;
};

template<>
inline capabilities::Default<B2>::cpp_type capabilities::Default<B2>::from_string(std::string_view s) {
    cpp_type value;
    auto const parse_res = std::from_chars(s.data(), s.data() + s.size(), value);

    if (parse_res.ptr != s.data() + s.size()) {
        throw std::runtime_error("XSD Parsing Error");
    } else {
        return value;
    }
}


// C
template<>
struct DatatypeMapping<C> {
    using cpp_datatype = int16_t;
};

template<>
struct DatatypeSupertypeMapping<C> {
    using supertype = xsd::B;
};

template<>
inline capabilities::Default<C>::cpp_type capabilities::Default<C>::from_string(std::string_view s) {
    cpp_type value;
    auto const parse_res = std::from_chars(s.data(), s.data() + s.size(), value);

    if (parse_res.ptr != s.data() + s.size()) {
        throw std::runtime_error("XSD Parsing Error");
    } else {
        return value;
    }
}

}  // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {
inline A const a_instance;
inline B const b_instance;
inline B2 const b2_instance;
inline C const c_instance;
inline Z const z_instance;
inline Y const y_instance;
} // namespace rdf4cpp::datatypes::xsd

#define GENERATE_HIERARCHY_TEST(lhs, rhs, expected) \
    GENERATE_BINOP_TESTCASE(lhs, 1, +, rhs, 1, expected, 2)

TEST_CASE("complex hierarchy order edge case") {
    // this conversion goes over A
    // which has to be instantiated even though it is never explicitly named in this test
    GENERATE_HIERARCHY_TEST(B, Y, Z);
}

TEST_CASE("complex hierarchy") {
    GENERATE_HIERARCHY_TEST(A, B, A);
    GENERATE_HIERARCHY_TEST(A, B2, A);
    GENERATE_HIERARCHY_TEST(A, C, A);
    GENERATE_HIERARCHY_TEST(A, Z, Z);
    GENERATE_HIERARCHY_TEST(A, Y, Z);
    GENERATE_HIERARCHY_TEST(B, B2, A);
    GENERATE_HIERARCHY_TEST(B, C, B);
    GENERATE_HIERARCHY_TEST(B, Z, Z);
    GENERATE_HIERARCHY_TEST(B, Y, Z);
    GENERATE_HIERARCHY_TEST(B2, C, A);
    GENERATE_HIERARCHY_TEST(B2, Z, Z);
    GENERATE_HIERARCHY_TEST(B2, Y, Y);
    GENERATE_HIERARCHY_TEST(C, Z, Z);
    GENERATE_HIERARCHY_TEST(C, Y, Z);
    GENERATE_HIERARCHY_TEST(Z, Y, Z);
}

TEST_CASE("compound assignment operators") {
    const auto ten = 10_xsd_int;
    const auto five = 5_xsd_int;
    const auto two = 2_xsd_int;

    auto curr = five;
    curr += five;
    CHECK(curr == ten);

    curr -= five;
    CHECK(curr == five);

    curr *= two;
    CHECK(curr == ten);

    curr /= two;
    CHECK(curr == five);

    const auto str = Literal::make_simple("some string");
    curr += str;
    CHECK(curr.null());
}

TEST_CASE("null ops") {
    auto const n = Literal::make_null();
    auto const l = Literal::make_typed_from_value<datatypes::xsd::Float>(2.f);

    CHECK((n + l).null());
    CHECK((l + n).null());
    CHECK((n - l).null());
    CHECK((l - n).null());
    CHECK((n * l).null());
    CHECK((l * n).null());
    CHECK((n / l).null());
    CHECK((l / n).null());
    CHECK((+n).null());
    CHECK((-n).null());
}
