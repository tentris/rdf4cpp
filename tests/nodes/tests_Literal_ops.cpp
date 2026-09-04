#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

#include <charconv>
#include <limits>
#include <vector>

#include <dice/template-library/sandbox.hpp>

#include <rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.hpp>

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

TEST_SUITE("deferred numeric ops") {
    /**
     * checks that all deferred ops on lhs and rhs produce exactly what the corresponding Literal ops produce
     */
    void check_matches_eager(Literal const &lhs, Literal const &rhs) {
        auto const check = [](Literal const &expected, DeferredValue &&res) {
            auto const got = materialize_deferred(std::move(res));

            if (expected.null()) {
                CHECK(got.null());
            } else {
                REQUIRE_FALSE(got.null());
                CHECK(got.order_eq(expected));  // compares datatype and value, unlike ==
            }
        };

        check(lhs.add(rhs), numeric_add_deferred(make_deferred_from_literal(lhs), make_deferred_from_literal(rhs)));
        check(lhs.sub(rhs), numeric_sub_deferred(make_deferred_from_literal(lhs), make_deferred_from_literal(rhs)));
        check(lhs.mul(rhs), numeric_mul_deferred(make_deferred_from_literal(lhs), make_deferred_from_literal(rhs)));
        check(lhs.div(rhs), numeric_div_deferred(make_deferred_from_literal(lhs), make_deferred_from_literal(rhs)));
    }

    dice::template_library::SubProcessResult make_typed_from_value_sandboxed(std::any value, IRI const &datatype) {
        return DICE_SANDBOX {
            return Literal::make_typed_from_value(std::move(value), datatype).null() ? 1 : 0;
        };
    }

    template<datatypes::LiteralDatatype T>
    void check_make_typed_from_value(typename T::cpp_type const &value) {
        auto const expected = Literal::make_typed_from_value<T>(value);
        CHECK(Literal::make_typed_from_value(std::any{value}, expected.datatype()).order_eq(expected));
    }

    TEST_CASE("matches the eager ops") {
        using namespace datatypes;

        SUBCASE("same datatype") {
            check_matches_eager(Literal::make_typed_from_value<xsd::Integer>(6), Literal::make_typed_from_value<xsd::Integer>(7));
            check_matches_eager(Literal::make_typed_from_value<xsd::Double>(1.5), Literal::make_typed_from_value<xsd::Double>(0.25));
            check_matches_eager(Literal::make_typed_from_value<xsd::Float>(2.f), Literal::make_typed_from_value<xsd::Float>(4.f));
            check_matches_eager(Literal::make_typed_from_value<xsd::Decimal>(BigDecimal{2.0}), Literal::make_typed_from_value<xsd::Decimal>(BigDecimal{0.5}));
        }

        SUBCASE("promotion and subtype substitution") {
            check_matches_eager(Literal::make_typed_from_value<xsd::Integer>(100), Literal::make_typed_from_value<xsd::Float>(1.f));
            check_matches_eager(Literal::make_typed_from_value<xsd::Float>(42.f), Literal::make_typed_from_value<xsd::Decimal>(BigDecimal{120.0}));
            check_matches_eager(Literal::make_typed_from_value<xsd::Int>(8), Literal::make_typed_from_value<xsd::Integer>(9));
            check_matches_eager(Literal::make_typed_from_value<xsd::UnsignedByte>(3), Literal::make_typed_from_value<xsd::Double>(2.));
            // both operands are stub-numeric, so the result datatype is that of neither of them
            check_matches_eager(Literal::make_typed_from_value<xsd::Int>(8), Literal::make_typed_from_value<xsd::Int>(9));
        }

        SUBCASE("dynamic datatypes") {
            check_matches_eager(Literal::make_typed_from_value<xsd::Z>(2.0), Literal::make_typed_from_value<xsd::Z>(4.0));
            check_matches_eager(Literal::make_typed_from_value<xsd::A>(6), Literal::make_typed_from_value<xsd::C>(7));
            check_matches_eager(Literal::make_typed_from_value<xsd::B2>(6), Literal::make_typed_from_value<xsd::C>(7));
            // the result datatype (Z) is that of neither operand, so it has to be constructed
            check_matches_eager(Literal::make_typed_from_value<xsd::B>(1), Literal::make_typed_from_value<xsd::Y>(1.f));
        }

        SUBCASE("errors") {
            check_matches_eager(Literal::make_typed_from_value<xsd::Integer>(1), Literal::make_typed_from_value<xsd::Integer>(0));
            check_matches_eager(Literal::make_typed_from_value<xsd::Float>(1.f), Literal::make_typed_from_value<xsd::Boolean>(false));
            check_matches_eager(Literal::make_typed_from_value<xsd::Integer>(1), Literal::make_simple("abc"));
            check_matches_eager(Literal::make_typed_from_value<xsd::Integer>(1), Literal::make_typed("123", IRI{"http://mydatatype.com#int"}));
        }
    }

    TEST_CASE("the null-value propagates") {
        auto const one = Literal::make_typed_from_value<datatypes::xsd::Integer>(1);
        auto const zero = Literal::make_typed_from_value<datatypes::xsd::Integer>(0);

        CHECK(materialize_deferred(DeferredValue{}).null());
        CHECK(numeric_add_deferred(DeferredValue{}, make_deferred_from_literal(one)).second.null());
        CHECK(numeric_add_deferred(make_deferred_from_literal(one), DeferredValue{}).second.null());

        auto const div_by_zero = numeric_div_deferred(make_deferred_from_literal(one), make_deferred_from_literal(zero));
        CHECK(div_by_zero.second.null());
        CHECK(numeric_add_deferred(div_by_zero, make_deferred_from_literal(one)).second.null());
    }

    TEST_CASE("folding does not store the intermediate results") {
        using namespace datatypes;

        // 1/3 makes every intermediate sum need all 16 significant digits, i.e. uninlineable,
        // so that the eager fold actually has to store them
        std::vector<Literal> const summands{Literal::make_typed_from_value<xsd::Int>(3),
                                            Literal::make_typed_from_value<xsd::Integer>(4),
                                            Literal::make_typed_from_value<xsd::Double>(1.0 / 3.0),
                                            Literal::make_typed_from_value<xsd::Double>(1.5),
                                            Literal::make_typed_from_value<xsd::Float>(2.f)};

        storage::reference_node_storage::UnsyncReferenceNodeStorage eager_storage{};
        storage::reference_node_storage::UnsyncReferenceNodeStorage deferred_storage{};
        auto const empty_size = deferred_storage.size();

        auto acc = make_deferred_from_value<xsd::Integer>(0);
        auto eager = Literal::make_typed_from_value<xsd::Integer>(0, eager_storage);

        for (auto const &summand : summands) {
            acc = numeric_add_deferred(acc, make_deferred_from_literal(summand));
            eager = eager.add(summand, eager_storage);
        }

        CHECK(materialize_deferred(std::move(acc), deferred_storage).order_eq(eager));
        CHECK_EQ(deferred_storage.size(), empty_size + 1);  // only the result
        CHECK_GT(eager_storage.size(), deferred_storage.size());
    }

    TEST_CASE("make_deferred_from_value") {
        using namespace datatypes;

        // the value is converted to the cpp_type by the compiler, so it cannot disagree with the datatype
        auto const check = [](DeferredValue const &got, Literal const &expected) {
            CHECK(materialize_deferred(got).order_eq(expected));
        };

        check(make_deferred_from_value<xsd::Integer>(42), Literal::make_typed_from_value<xsd::Integer>(42));
        check(make_deferred_from_value<xsd::Double>(1.5), Literal::make_typed_from_value<xsd::Double>(1.5));
        check(make_deferred_from_value<xsd::Decimal>(BigDecimal{1.5}), Literal::make_typed_from_value<xsd::Decimal>(BigDecimal{1.5}));
        check(make_deferred_from_value<xsd::Z>(1.0), Literal::make_typed_from_value<xsd::Z>(1.0));  // dynamic datatype
    }

    TEST_CASE("make_typed_from_value") {
        using namespace datatypes;

        check_make_typed_from_value<xsd::Integer>(42);
        check_make_typed_from_value<xsd::Double>(1.5);
        check_make_typed_from_value<xsd::Decimal>(BigDecimal{1.5});
        check_make_typed_from_value<xsd::Boolean>(true);
        check_make_typed_from_value<xsd::Int>(7);
        check_make_typed_from_value<xsd::String>("abc");
        check_make_typed_from_value<rdf::LangString>({"abc", "en"});
        check_make_typed_from_value<xsd::Z>(1.0);

        CHECK(Literal::make_typed_from_value(std::any{xsd::Integer::cpp_type{1}}, IRI{}).null());
        CHECK(Literal::make_typed_from_value(std::any{1}, IRI{"http://mydatatype.com#int"}).null());
    }

    TEST_CASE("make_typed_from_value with a value that does not match its datatype") {
        using namespace datatypes;
        using dice::template_library::SubProcessResult;

        // a matching value constructs a Literal, so the checks below are not vacuous
        CHECK_EQ(make_typed_from_value_sandboxed(std::any{xsd::Integer::cpp_type{1}}, IRI::datatype<xsd::Integer>()), SubProcessResult::ExitSuccess);

        // violating the precondition is undefined behaviour; in practice the noexcept registry functions
        // std::any_cast the value, so it terminates instead of yielding a Literal holding the wrong type
        CHECK_EQ(make_typed_from_value_sandboxed(std::any{1.0}, IRI::datatype<xsd::Integer>()), SubProcessResult::Aborted);
        CHECK_EQ(make_typed_from_value_sandboxed(std::any{1}, IRI::datatype<xsd::Double>()), SubProcessResult::Aborted);
        CHECK_EQ(make_typed_from_value_sandboxed(std::any{1.0}, IRI::datatype<xsd::Decimal>()), SubProcessResult::Aborted);
        CHECK_EQ(make_typed_from_value_sandboxed(std::any{1}, IRI::datatype<xsd::Boolean>()), SubProcessResult::Aborted);
        CHECK_EQ(make_typed_from_value_sandboxed(std::any{}, IRI::datatype<xsd::Integer>()), SubProcessResult::Aborted);
        CHECK_EQ(make_typed_from_value_sandboxed(std::any{1}, IRI::datatype<xsd::Z>()), SubProcessResult::Aborted);
        // a stub-numeric datatype does not share its cpp_type with its impl supertype
        CHECK_EQ(make_typed_from_value_sandboxed(std::any{xsd::Integer::cpp_type{1}}, IRI::datatype<xsd::Int>()), SubProcessResult::Aborted);

        // the string like datatypes are cast in make_typed_from_value itself, which does report the mismatch
        CHECK_THROWS_AS((void) Literal::make_typed_from_value(std::any{1}, IRI::datatype<xsd::String>()), std::bad_any_cast);
        CHECK_THROWS_AS((void) Literal::make_typed_from_value(std::any{1}, IRI::datatype<rdf::LangString>()), std::bad_any_cast);
    }

    TEST_CASE("materializing into another node storage") {
        storage::reference_node_storage::UnsyncReferenceNodeStorage node_storage{};

        auto const lhs = Literal::make_typed_from_value<datatypes::xsd::B>(1);
        auto const rhs = Literal::make_typed_from_value<datatypes::xsd::Y>(1.f);

        auto const sum = materialize_deferred(numeric_add_deferred(make_deferred_from_literal(lhs), make_deferred_from_literal(rhs)), node_storage);

        CHECK_EQ(sum.backend_handle().storage(), storage::DynNodeStoragePtr{node_storage});
        CHECK(sum.order_eq(lhs.add(rhs, node_storage)));
    }

    TEST_CASE("the result datatype lives in the given node storage") {
        using namespace datatypes;

        storage::reference_node_storage::UnsyncReferenceNodeStorage node_storage{};

        // B + Y -> Z, so the result datatype is that of neither operand and has to be created
        auto const lhs = make_deferred_from_literal(Literal::make_typed_from_value<xsd::B>(1));
        auto const rhs = make_deferred_from_literal(Literal::make_typed_from_value<xsd::Y>(1.f));
        CHECK_EQ(numeric_add_deferred(lhs, rhs, node_storage).second.backend_handle().storage(),
                 storage::DynNodeStoragePtr{node_storage});

        // this also holds if the result datatype is that of an operand
        auto const same = make_deferred_from_literal(Literal::make_typed_from_value<xsd::Z>(2.0));
        CHECK_EQ(numeric_add_deferred(same, same).second.backend_handle().storage(),
                 storage::default_node_storage);
        CHECK_EQ(numeric_add_deferred(same, same, node_storage).second.backend_handle().storage(),
                 storage::DynNodeStoragePtr{node_storage});
    }
}
