#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <random>

#include <rdf4cpp/BigDecimal.hpp>
#include <rdf4cpp/datatypes/registry/util/ConstexprString.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

using Dec = rdf4cpp::Decimal128;
using DecI = rdf4cpp::util::BigDecimal<int32_t, uint32_t>;
using RoundingMode = rdf4cpp::util::RoundingMode;

namespace doctest {
    template<> struct StringMaker<__int128> {
        static String convert(const __int128& value) {
            std::stringstream s{};
            ::operator<<(s, value);
            return doctest::String{s.view().data(), static_cast<unsigned int>(s.view().size())};
        }
    };
}
static_assert(!rdf4cpp::util::detail::BoostNumber<__int128>);
static_assert(rdf4cpp::util::detail::BoostNumber<boost::multiprecision::checked_cpp_int>);
static_assert(!rdf4cpp::util::detail::BoostNumber<int>);

TEST_CASE_TEMPLATE("checked arithmetic signed", T, int32_t, int64_t, __int128, boost::multiprecision::checked_int256_t) {
    using namespace rdf4cpp::util::detail;

    SUBCASE("add") {
        auto eq = [](auto a, auto b, auto r) {
            T re;
            CHECK(add_checked<OverflowMode::Checked>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
            CHECK(add_checked<OverflowMode::UndefinedBehavior>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
        };
        eq(5, 5, 10);
        eq(5, -5, 0);
        [[maybe_unused]] T re = 0;
        CHECK(add_checked<OverflowMode::Checked>(std::numeric_limits<T>::max(), T{1}, re) == true);
    }
    SUBCASE("sub") {
        auto eq = [](auto a, auto b, auto r) {
            T re;
            CHECK(sub_checked<OverflowMode::Checked>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
            CHECK(sub_checked<OverflowMode::UndefinedBehavior>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
        };
        eq(5, -5, 10);
        eq(5, 5, 0);
        [[maybe_unused]] T re = 0;
        CHECK(sub_checked<OverflowMode::Checked>(std::numeric_limits<T>::max(), T{-1}, re) == true);
    }
    SUBCASE("mul") {
        auto eq = [](auto a, auto b, auto r) {
            T re;
            CHECK(mul_checked<OverflowMode::Checked>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
            CHECK(mul_checked<OverflowMode::UndefinedBehavior>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
        };
        eq(5, 5, 25);
        eq(5, -5, -25);
        [[maybe_unused]] T re = 0;
        CHECK(mul_checked<OverflowMode::Checked>(std::numeric_limits<T>::max(), T{2}, re) == true);
    }
    SUBCASE("pow") {
        auto eq = [](auto a, unsigned int b, auto r) {
            T re;
            CHECK(pow_checked<OverflowMode::Checked>(T{a}, b, re) == false);
            CHECK(re == T{r});
            CHECK(pow_checked<OverflowMode::UndefinedBehavior>(T{a}, b, re) == false);
            CHECK(re == T{r});
        };
        eq(5, 3, 25*5);
        eq(5, 2, 25);
        [[maybe_unused]] T re = 0;
        CHECK(pow_checked<OverflowMode::Checked>(std::numeric_limits<T>::max(), 2, re) == true);
    }
}
TEST_CASE_TEMPLATE("checked arithmetic unsigned", T, uint32_t, uint64_t, unsigned __int128, boost::multiprecision::checked_uint256_t) {
    using namespace rdf4cpp::util::detail;

    SUBCASE("add") {
        auto eq = [](auto a, auto b, auto r) {
            T re;
            CHECK(add_checked<OverflowMode::Checked>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
            CHECK(add_checked<OverflowMode::UndefinedBehavior>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
        };
        eq(5u, 5u, 10u);
        eq(5u, 10u, 15u);
        [[maybe_unused]] T re = 0;
        CHECK(add_checked<OverflowMode::Checked>(std::numeric_limits<T>::max(), T{1}, re) == true);
    }
    SUBCASE("sub") {
        auto eq = [](auto a, auto b, auto r) {
            T re;
            CHECK(sub_checked<OverflowMode::Checked>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
            CHECK(sub_checked<OverflowMode::UndefinedBehavior>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
        };
        eq(5u, 3u, 2u);
        eq(5u, 5u, 0u);
        [[maybe_unused]] T re = 0;
        CHECK(sub_checked<OverflowMode::Checked>(T{0}, std::numeric_limits<T>::max(), re) == true);
    }
    SUBCASE("mul") {
        auto eq = [](auto a, auto b, auto r) {
            T re;
            CHECK(mul_checked<OverflowMode::Checked>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
            CHECK(mul_checked<OverflowMode::UndefinedBehavior>(T{a}, T{b}, re) == false);
            CHECK(re == T{r});
        };
        eq(5u, 5u, 25u);
        eq(5u, 15u, 5u*15u);
        [[maybe_unused]] T re = 0;
        CHECK(mul_checked<OverflowMode::Checked>(std::numeric_limits<T>::max(), T{2}, re) == true);
    }
    SUBCASE("pow") {
        auto eq = [](auto a, unsigned int b, auto r) {
            T re;
            CHECK(pow_checked<OverflowMode::Checked>(T{a}, b, re) == false);
            CHECK(re == T{r});
            CHECK(pow_checked<OverflowMode::UndefinedBehavior>(T{a}, b, re) == false);
            CHECK(re == T{r});
        };
        eq(5u, 3u, 25u*5u);
        eq(5u, 2u, 25u);
        [[maybe_unused]] T re = 0;
        CHECK(pow_checked<OverflowMode::Checked>(std::numeric_limits<T>::max(), 2, re) == true);
    }
}
TEST_CASE_TEMPLATE("checked casting", T, uint32_t, uint64_t, unsigned __int128, boost::multiprecision::checked_uint256_t,
                   int32_t, int64_t, __int128, boost::multiprecision::checked_int256_t ) {
    using namespace rdf4cpp::util::detail;

    static constexpr bool sign = std::numeric_limits<T>::is_signed;
    [[maybe_unused]] uint8_t u8 = 0;
    [[maybe_unused]] int8_t i8 = 0;
    [[maybe_unused]] T t{};
    CHECK(cast_checked<OverflowMode::Checked>(T{5}, i8) == false);
    CHECK(i8 == 5);
    CHECK(cast_checked<OverflowMode::Checked>(T{5}, u8) == false);
    CHECK(u8 == 5);
    if constexpr (sign) {
        CHECK(cast_checked<OverflowMode::Checked>(T{-5}, i8) == false);
        CHECK(i8 == -5);
        CHECK(cast_checked<OverflowMode::Checked>(T{-5}, u8) == true);
        CHECK(cast_checked<OverflowMode::Checked>(std::numeric_limits<T>::min(), i8) == true);
        if constexpr (IntegralExt<T>) {
            CHECK(cast_checked<OverflowMode::Checked>(std::numeric_limits<typename MakeUnsigned<T>::t>::max(), t) == true);
        }
        else {
            CHECK(cast_checked<OverflowMode::Checked>(std::numeric_limits<typename MakeUnsigned<T>::t>::max(), t) == false);
            CHECK(t == std::numeric_limits<T>::max());
        }
    }
    CHECK(cast_checked<OverflowMode::Checked>(std::numeric_limits<T>::max(), u8) == true);
    CHECK(cast_checked<OverflowMode::Checked>(std::numeric_limits<T>::max(), i8) == true);
    CHECK(cast_checked<OverflowMode::Checked>(std::numeric_limits<uint8_t>::max(), t) == false);
    CHECK(t == std::numeric_limits<uint8_t>::max());
}

constexpr __int128 Make128(int64_t h, int64_t l) {
    constexpr __int128 p = []() {
        __int128 r = 1;
        for (int i = 0; i < std::numeric_limits<int64_t>::digits10; ++i) {
            r *= 10;
        }
        return r;
    }();
    return static_cast<__int128>(h) * p + l;
}

TEST_CASE("int128 to_chars") {
    auto tos = [](__int128 x) {
        return rdf4cpp::writer::StringWriter::oneshot([x](rdf4cpp::writer::StringWriter& w) {
            return rdf4cpp::util::to_chars_canonical(x, w);
        });
    };
    CHECK(tos(0) == "0");
    CHECK(tos(std::numeric_limits<__int128>::max()) == "170141183460469231731687303715884105727");
    CHECK(tos(std::numeric_limits<__int128>::min()) == "-170141183460469231731687303715884105728");
    CHECK(tos(std::numeric_limits<__int128>::min()+1) == "-170141183460469231731687303715884105727");
    CHECK(Make128(5000, 5) > std::numeric_limits<uint64_t>::max());
    CHECK(tos(Make128(1, 5)) == "1000000000000000005");
    CHECK(tos(Make128(5001, 5)) == "5001000000000000000005");
    CHECK(tos(Make128(5000, 5)) == "5000000000000000000005");
    CHECK(tos(Make128(5000, 500000)) == "5000000000000000500000");
    CHECK(tos(Make128(-1, -5)) == "-1000000000000000005");
    CHECK(tos(Make128(-5001, -5)) == "-5001000000000000000005");
    CHECK(tos(Make128(-5000, -5)) == "-5000000000000000000005");
    CHECK(tos(Make128(-5000, -500000)) == "-5000000000000000500000");
}

TEST_CASE("int128 from_chars") {
    static constexpr rdf4cpp::datatypes::registry::util::ConstexprString s{"test"};
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("0") == 0);
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("170141183460469231731687303715884105727") == std::numeric_limits<__int128>::max());
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("-170141183460469231731687303715884105728") == std::numeric_limits<__int128>::min());
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("-170141183460469231731687303715884105727") == std::numeric_limits<__int128>::min()+1);
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("1000000000000000005") == Make128(1, 5));
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("5000000000000000000005") == Make128(5000, 5));
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("5001000000000000000005") == Make128(5001, 5));
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("5000000000000000500000") == Make128(5000, 500000));
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("-5001000000000000000005") == Make128(-5001, -5));
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("-5000000000000000000005") == Make128(-5000, -5));
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("-1000000000000000005") == Make128(-1, -5));
    CHECK(rdf4cpp::datatypes::registry::util::from_chars<__int128, s>("-5000000000000000500000") == Make128(-5000, -500000));

    std::random_device rd{};
    std::default_random_engine r{std::uniform_int_distribution<size_t>{}(rd)};
    std::uniform_int_distribution<int64_t> d{};
    std::array<char, std::numeric_limits<__int128>::digits10 + 2> buff;
    for (int i = 0; i < 100000; ++i) {
        __int128 const c = static_cast<__int128>(d(r)) << 64 | d(r);
        auto char_res = rdf4cpp::util::to_chars(buff.data(), buff.data() + buff.size(), c);
        CHECK(char_res.ec == std::errc{});
        auto len = char_res.ptr - buff.data();
        CHECK(len >= 0);
        CHECK(len < buff.size());
        CHECK(c == rdf4cpp::datatypes::registry::util::from_chars<__int128, s>(std::string_view{buff.data(), static_cast<size_t>(len)}));
    }
}

TEST_CASE("basics") {
    SUBCASE("ctor and compare") {
        static_assert(rdf4cpp::util::BigDecimalBaseType<uint32_t>);
        static_assert(rdf4cpp::util::BigDecimalBaseType<int32_t>);
        static_assert(rdf4cpp::util::BigDecimalBaseType<boost::multiprecision::cpp_int>);
        Dec d{500, 1};
        CHECK_GT(d, Dec{-500, 1});
        CHECK(Dec{-500, 1} < d);
        CHECK(Dec{50, 0} == d);
        CHECK_GT(d, Dec{40, 0});
        CHECK_LT(d, Dec{60, 0});
        CHECK(Dec{40, 0} < d);
        CHECK(Dec{60, 0} > d);
        CHECK_NE(d, Dec{60, 1});

        int32_t m = std::numeric_limits<int32_t>::max();
        CHECK_LT(DecI{m, 5}, DecI{m, 0});
        CHECK_GT(DecI{m, 5}, DecI{0, 0});
        CHECK_GT(DecI{m, 5}, DecI{m, 10});
    }
    SUBCASE("normalize") {
        Dec d{500, 1};
        d.normalize();
        CHECK_EQ(d, Dec{50, 0});
        CHECK_EQ(d.get_exponent(), 0);
        d = Dec{51, 1};
        d.normalize();
        CHECK_EQ(d, Dec{51, 1});
        CHECK_EQ(d.get_exponent(), 1);
    }
}

TEST_CASE("arithmetic") {
    int32_t m = std::numeric_limits<int32_t>::max();
    int32_t mi = std::numeric_limits<int32_t>::min();
    SUBCASE("unary -") {
        CHECK(-Dec{1, 0} == Dec{-1, 0});
        CHECK(-Dec{1, 5} == Dec{-1, 5});
        CHECK(-Dec{-1, 0} == Dec{1, 0});
        CHECK(-Dec{-1, 5} == Dec{1, 5});
        CHECK(!(DecI{mi, 5}.unary_minus_checked()).has_value());
    }
    SUBCASE("+") {
        CHECK((Dec{1, 0} + Dec{2, 0}) == Dec{3, 0});
        CHECK((Dec{100, 1} + Dec{200, 1}) == Dec{300, 1});
        CHECK((Dec{1, 0} + Dec{200, 2}) == Dec{300, 2});
        CHECK((Dec{1, 2} + Dec{2, 0}) == Dec{201, 2});
        CHECK((Dec{1, 0} + Dec{-2, 0}) == Dec{-1, 0});
        CHECK((Dec{1, 0} + Dec{-1, 0}) == Dec{0, 0});
        CHECK((Dec{-1, 0} + Dec{-2, 0}) == Dec{-3, 0});
        CHECK((Dec{2, 0} + Dec{-1, 0}) == Dec{1, 0});
        CHECK((Dec{-1, 0} + Dec{2, 0}) == Dec{1, 0});
        CHECK((Dec{-2, 0} + Dec{1, 0}) == Dec{-1, 0});
        CHECK(!DecI{m, 0}.mul_checked(DecI{m, 0}).has_value());
        Dec d{5, 0};
        d += Dec{5, 1};
        CHECK_EQ(d, Dec{55, 1});
    }
    SUBCASE("-") {
        CHECK((Dec{2, 0} - Dec{1, 0}) == Dec{1, 0});
        CHECK((Dec{200, 1} - Dec{100, 1}) == Dec{100, 1});
        CHECK((Dec{2, 0} - Dec{100, 2}) == Dec{100, 2});
        CHECK((Dec{1, 2} - Dec{-2, 0}) == Dec{201, 2});
        CHECK((Dec{1, 0} - Dec{2, 0}) == Dec{-1, 0});
        CHECK((Dec{1, 0} - Dec{1, 0}) == Dec{0, 0});
        CHECK((Dec{-1, 0} - Dec{2, 0}) == Dec{-3, 0});
        CHECK((Dec{2, 0} - Dec{-1, 0}) == Dec{3, 0});
        CHECK((Dec{-1, 0} - Dec{-2, 0}) == Dec{1, 0});
        CHECK((Dec{-2, 0} - Dec{-1, 0}) == Dec{-1, 0});
        auto v = DecI{-5, 0}.sub_checked(DecI(mi, 0));
        CHECK(v.has_value());
        CHECK_EQ(v.value(), DecI{-5 - mi});
        v = DecI{5, 0}.sub_checked(DecI(mi, 0));
        CHECK(!v.has_value());
        Dec d{5, 0};
        d -= Dec{5, 1};
        CHECK_EQ(d, Dec{45, 1});
    }
    SUBCASE("*") {
        CHECK((Dec{2, 0} * Dec{2, 0}) == Dec{4, 0});
        CHECK((Dec{20, 0} * Dec{20, 0}) == Dec{400, 0});
        CHECK((Dec{2, 1} * Dec{2, 0}) == Dec{4, 1});
        CHECK((Dec{2, 1} * Dec{2, 1}) == Dec{4, 2});
        CHECK((Dec{
                       -2,
                       1,
               } *
               Dec{2, 1}) == Dec{-4, 2});
        CHECK((Dec{2, 1} * Dec{-2, 1}) == Dec{-4, 2});
        CHECK((Dec{-2, 1} * Dec{-2, 1}) == Dec{4, 2});
        CHECK(!DecI{m, 100}.mul_checked(DecI{m, 0}).has_value());
        Dec d{5, 0};
        d *= Dec{5, 1};
        CHECK_EQ(d, Dec{25, 1});
    }
    SUBCASE("/") {
        CHECK((Dec{2, 0} / Dec{2, 0}) == Dec{1, 0});
        CHECK((Dec{-2, 0} / Dec{2, 0}) == Dec{-1, 0});
        CHECK((Dec{2, 0} / Dec{-2, 0}) == Dec{-1, 0});
        CHECK((Dec{-2, 0} / Dec{-2, 0}) == Dec{1, 0});
        CHECK((Dec{1, 0}.div(Dec{3, 0}, 2, RoundingMode::Floor)) == Dec{33, 2});
        CHECK((Dec{1, 0}.div(Dec{3, 0}, 2, RoundingMode::Ceil)) == Dec{34, 2});
        CHECK((Dec{1, 0}.div(Dec{3, 0}, 2, RoundingMode::Round)) == Dec{33, 2});
        CHECK((Dec{2, 0}.div(Dec{3, 0}, 2, RoundingMode::Round)) == Dec{67, 2});
        CHECK((Dec{1, 0}.div(Dec{-3, 0}, 2, RoundingMode::Floor)) == Dec{-34, 2});
        CHECK(!DecI{1, 0}.div_checked(DecI{3, 0}, 1000, RoundingMode::Floor).has_value());
        Dec d{5, 0};
        d /= Dec{2, 0};
        CHECK_EQ(d, Dec{25, 1});
    }
    SUBCASE("precision") {
        CHECK(((Dec{1, 0} + Dec{2, 0} + Dec{3, 0}) / Dec{3, 0}) == Dec{2, 0});
    }
    SUBCASE("round") {
        SUBCASE("floor") {
            CHECK(Dec{50, 1}.round(RoundingMode::Floor) == Dec{5, 0});
            CHECK(Dec{51, 1}.round(RoundingMode::Floor) == Dec{5, 0});
            CHECK(Dec{56, 1}.round(RoundingMode::Floor) == Dec{5, 0});
            CHECK(Dec{"-1.6"}.round(RoundingMode::Floor) == Dec{"-2.0"});
            CHECK(Dec{"-1.4"}.round(RoundingMode::Floor) == Dec{"-2.0"});
            CHECK(Dec{"-1.0"}.round(RoundingMode::Floor) == Dec{"-1.0"});
            CHECK(Dec{"0.0"}.round(RoundingMode::Floor) == Dec{"0.0"});
        }

        SUBCASE("ceil") {
            CHECK(Dec{50, 1}.round(RoundingMode::Ceil) == Dec{5, 0});
            CHECK(Dec{41, 1}.round(RoundingMode::Ceil) == Dec{5, 0});
            CHECK(Dec{47, 1}.round(RoundingMode::Ceil) == Dec{5, 0});
            CHECK(Dec{"-1.6"}.round(RoundingMode::Ceil) == Dec{"-1.0"});
            CHECK(Dec{"-1.0"}.round(RoundingMode::Ceil) == Dec{"-1.0"});
            CHECK(Dec{"0.0"}.round(RoundingMode::Ceil) == Dec{"0.0"});
            CHECK(Dec{"-1.4"}.round(RoundingMode::Ceil) == Dec{"-1.0"});
        }

        SUBCASE("round") {
            CHECK(Dec{50, 1}.round(RoundingMode::Round) == Dec{5, 0});
            CHECK(Dec{51, 1}.round(RoundingMode::Round) == Dec{5, 0});
            CHECK(Dec{48, 1}.round(RoundingMode::Round) == Dec{5, 0});
            CHECK(Dec{549, 2}.round(RoundingMode::Round) == Dec{5, 0});
            CHECK(Dec{450, 2}.round(RoundingMode::Round) == Dec{5, 0});
            CHECK(Dec{5, 0}.round(RoundingMode::Round) == Dec{5, 0});
            CHECK(round(Dec{450, 2}) == Dec{5, 0});
            CHECK(Dec{"-1.6"}.round(RoundingMode::Round) == Dec{"-2.0"});
            CHECK(Dec{"-1.5"}.round(RoundingMode::Round) == Dec{"-2.0"});
            CHECK(Dec{"-1.4"}.round(RoundingMode::Round) == Dec{"-1.0"});
            CHECK(Dec{"-1.0"}.round(RoundingMode::Round) == Dec{"-1.0"});
            CHECK(Dec{"0.0"}.round(RoundingMode::Round) == Dec{"0.0"});
        }

        SUBCASE("trunc") {
            CHECK(Dec{50, 1}.round(RoundingMode::Trunc) == Dec{5, 0});
            CHECK(Dec{51, 1}.round(RoundingMode::Trunc) == Dec{5, 0});
            CHECK(Dec{56, 1}.round(RoundingMode::Trunc) == Dec{5, 0});
            CHECK(Dec{"-1.6"}.round(RoundingMode::Trunc) == Dec{"-1.0"});
            CHECK(Dec{"-1.4"}.round(RoundingMode::Trunc) == Dec{"-1.0"});
            CHECK(Dec{"-1.0"}.round(RoundingMode::Trunc) == Dec{"-1.0"});
            CHECK(Dec{"0.0"}.round(RoundingMode::Trunc) == Dec{"0.0"});
        }
    }
    SUBCASE("abs") {
        CHECK(Dec{51, 1}.abs() == Dec{51, 1});
        CHECK(Dec{-51, 1}.abs() == Dec{51, 1});
        CHECK(abs(Dec{-51, 1}) == Dec{51, 1});
    }
    SUBCASE("pow") {
        CHECK(Dec{5, 0}.pow(5) == Dec{3125});
        CHECK(Dec{5, 1}.pow(5) == Dec{"0.03125"});
        CHECK(pow(Dec{5, 1}, 0) == Dec{1});
        CHECK(!DecI{500, 0}.pow_checked(m).has_value());
    }
    SUBCASE("hash") {
        CHECK_EQ(Dec{1, 0}.hash(), Dec{1, 0}.hash());
        CHECK_NE(Dec{1, 0}.hash(), Dec{2, 0}.hash());
        CHECK_NE(Dec{1, 0}.hash(), Dec{1, 1}.hash());
        CHECK_NE(Dec{1, 0}.hash(), Dec{-1, 0}.hash());
        CHECK_EQ(Dec{1, 0}.hash(), Dec{10, 1}.hash());
    }
}

TEST_CASE("conversion") {
    int32_t mini = std::numeric_limits<int32_t>::min();
    SUBCASE("to std::string") {
        CHECK(static_cast<std::string>(Dec{51, 1}) == "5.1");
        CHECK(static_cast<std::string>(Dec{-9514, 2}) == "-95.14");
        CHECK(static_cast<std::string>(Dec{-9514, 0}) == "-9514.0");
        CHECK(static_cast<std::string>(Dec{-9514, 4}) == "-0.9514");
        CHECK(static_cast<std::string>(Dec{-9514, 6}) == "-0.009514");
        CHECK(static_cast<std::string>(Dec{500, 2}) == "5.0");
        CHECK(static_cast<std::string>(Dec{0, 5}) == "0.0");
        CHECK(static_cast<std::string>(Dec{100, 5}) == "0.001");
        CHECK(static_cast<std::string>(Dec{100, 0}) == "100.0");
        CHECK(static_cast<std::string>(DecI{mini, 0}) == std::to_string(mini) + ".0");
    }
    SUBCASE("writing") {
        std::stringstream str{};
        str << Dec{50, 1};
        CHECK_EQ(str.view(), "5.0");
        // uses string conversion, so no more tests here
        str << rdf4cpp::Int128{100};
        CHECK_EQ(str.view(), "5.0100");
    }
    SUBCASE("from double") {
        CHECK(Dec{50.0} == Dec{50, 0});
        CHECK(Dec{-50.5} == Dec{-505, 1});
        CHECK(Dec{5000000000000000000.0} == Dec{5000000000000000000, 0});
        CHECK(Dec{0.0009765625} == Dec{"0.0009765625"});
        CHECK(Dec{1.0} == Dec{1, 0});
        CHECK(Dec{1.2} != Dec{12, 1}); // 1.2 can not be exactly represented as double but can be as decimal
        CHECK_EQ(static_cast<float>(Dec{1.0}), 1.0f);
        CHECK_THROWS_AS([[maybe_unused]] auto _ = Dec(std::numeric_limits<double>::max()), std::overflow_error);
        CHECK(Dec(std::numeric_limits<double>::min()) > Dec(0, 0));
        CHECK(Dec(std::numeric_limits<double>::denorm_min()) > Dec(0, 0));
    }
    SUBCASE("to double") {
        CHECK_EQ(static_cast<double>(Dec{50, 0}), 50.0);
        CHECK_EQ(static_cast<double>(Dec{500, 1}), 50.0);
    }
    SUBCASE("to float") {
        CHECK_EQ(static_cast<float>(Dec{50, 0}), 50.0f);
        CHECK_EQ(static_cast<float>(Dec{500, 1}), 50.0f);
    }
    SUBCASE("from string") {
        CHECK(Dec{"5"} == Dec{5, 0});
        CHECK(Dec{"5."} == Dec{5, 0});
        CHECK(Dec{"5.1"} == Dec{51, 1});
        CHECK(Dec{"54.32"} == Dec{5432, 2});
        CHECK(Dec{".54"} == Dec{54, 2});
        CHECK(Dec{"0005.000"} == Dec{5000, 3});
        CHECK_EQ(Dec{"0005.000"}.get_exponent(), 3);
        CHECK(Dec{"-5"} == Dec{-5, 0});
        CHECK(Dec{"-5."} == Dec{-5, 0});
        CHECK(Dec{"-5.1"} == Dec{-51, 1});
        CHECK(Dec{"-54.32"} == Dec{-5432, 2});
        CHECK(Dec{"-.54"} == Dec{-54, 2});
        CHECK(Dec{"-0005.000"} == Dec{-5000, 3});
        CHECK_EQ(Dec{"-0005.000"}.get_exponent(), 3);
        CHECK(Dec{"0.0"} == Dec{"-0.0"});
        CHECK_THROWS_AS(Dec{"5.5.5"}, rdf4cpp::InvalidNode);
        CHECK_THROWS_AS(Dec{"5.5-5"}, rdf4cpp::InvalidNode);
        CHECK_THROWS_AS(Dec{"5.5+5"}, rdf4cpp::InvalidNode);
        // no e notation allowed by rdf (xml) standard
    }
    SUBCASE("from Int128") {
        CHECK(Dec{rdf4cpp::Int128{5}} == Dec{5, 0});
    }
    SUBCASE("to Int128") {
        CHECK(static_cast<rdf4cpp::Int128>(Dec{5, 0}) == rdf4cpp::Int128{5});
        CHECK(static_cast<rdf4cpp::Int128>(Dec{59, 1}) == rdf4cpp::Int128{5});
        if constexpr (std::numeric_limits<rdf4cpp::Int128>::is_bounded) {
            CHECK(static_cast<rdf4cpp::Int128>(Dec{std::numeric_limits<rdf4cpp::Int128>::max(), std::numeric_limits<rdf4cpp::Int128>::digits10}) ==
                  (std::same_as<rdf4cpp::Int128, boost::multiprecision::checked_int128_t> ? rdf4cpp::Int128{3} : rdf4cpp::Int128{1})
            );
        }
    }
}
