#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp/BigDecimal.hpp>

using Dec = rdf4cpp::BigDecimal<>;
using DecI = rdf4cpp::BigDecimal<int32_t, uint32_t>;
using RoundingMode = rdf4cpp::RoundingMode;

TEST_CASE("basics") {
    SUBCASE("ctor and compare") {
        static_assert(rdf4cpp::BigDecimalBaseType<uint32_t>);
        static_assert(rdf4cpp::BigDecimalBaseType<int32_t>);
        static_assert(rdf4cpp::BigDecimalBaseType<boost::multiprecision::cpp_int>);
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
    }
    /*SUBCASE("from double") {
        CHECK(Dec{50.0} == Dec{50, 0});
        CHECK(Dec{-50.5} == Dec{-505, 1});
        CHECK(Dec{500000.0} == Dec{500000, 0});
        CHECK(Dec{0.0009765625} == Dec{"0.0009765625"});
        CHECK(Dec{1.0} == Dec{1, 0});
        CHECK_EQ(static_cast<float>(Dec{1.0}), 1.0f);
    }*/
    SUBCASE("to double") {
        CHECK(boost::multiprecision::checked_int1024_t {std::numeric_limits<double>::max()} != boost::multiprecision::checked_int1024_t {});
        CHECK_EQ(static_cast<double>(Dec{50, 0}), 50.0);
        CHECK_EQ(static_cast<double>(Dec{500, 1}), 50.0);
        CHECK_EQ(static_cast<double>(Dec{static_cast<boost::multiprecision::checked_int128_t>(std::numeric_limits<double>::max()) * 100, 2}), std::numeric_limits<double>::max());
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
    SUBCASE("from cpp_int") {
        CHECK(Dec{boost::multiprecision::checked_int128_t{5}} == Dec{5, 0});
    }
    SUBCASE("to cpp_int") {
        CHECK(static_cast<boost::multiprecision::checked_int128_t>(Dec{5, 0}) == boost::multiprecision::cpp_int{5});
        CHECK(static_cast<boost::multiprecision::checked_int128_t>(Dec{59, 1}) == boost::multiprecision::cpp_int{5});
    }
}
