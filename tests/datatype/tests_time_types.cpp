#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <format>
#include <rdf4cpp.hpp>
#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>
#include <rdf4cpp/storage/reference_node_storage/SyncReferenceNodeStorage.hpp>
#include <rdf4cpp/util/CheckedInt.hpp>

namespace doctest {
    template<> struct StringMaker<rdf4cpp::ZonedTime> {
        static String convert(const rdf4cpp::ZonedTime& value) {
            auto s = std::format("{}", value);
            return String{s.c_str(), static_cast<String::size_type>(s.size())};
        }
    };
}

template<typename Datatype>
void basic_test(typename Datatype::cpp_type a, std::string_view b, std::partial_ordering res, bool skip_string_comp = false) {
    using namespace rdf4cpp;
    // better output on failure than CHECK(a <=> b == res)
    if (res == std::partial_ordering::equivalent) {
        const auto lit_a = Literal::make_typed_from_value<Datatype>(a);
        const auto lit_b = Literal::make_typed<Datatype>(b);
        if (!skip_string_comp)
            CHECK(lit_a.lexical_form() == b);
        CHECK(lit_a == lit_b);
        CHECK(a == lit_b.template value<Datatype>());
    } else if (res == std::partial_ordering::less) {
        CHECK(Literal::make_typed_from_value<Datatype>(a) < Literal::make_typed<Datatype>(b));
    }
    else if (res == std::partial_ordering::greater) {
        CHECK(Literal::make_typed_from_value<Datatype>(a) > Literal::make_typed<Datatype>(b));
    }
    else if (res == std::partial_ordering::unordered) {
        CHECK((Literal::make_typed_from_value<Datatype>(a) <=> Literal::make_typed<Datatype>(b)) == res);
    }
}
template<typename Datatype>
void basic_test(std::string_view a, std::string_view b, std::partial_ordering res) {
    using namespace rdf4cpp;
    // better output on failure than CHECK(a <=> b == res)
    if (res == std::partial_ordering::equivalent) {
        const auto lit_a = Literal::make_typed<Datatype>(a);
        const auto lit_b = Literal::make_typed<Datatype>(b);
        CHECK(lit_a == lit_b);
    } else if (res == std::partial_ordering::less) {
        CHECK(Literal::make_typed<Datatype>(a) < Literal::make_typed<Datatype>(b));
    }
    else if (res == std::partial_ordering::greater) {
        CHECK(Literal::make_typed<Datatype>(a) > Literal::make_typed<Datatype>(b));
    }
    else if (res == std::partial_ordering::unordered) {
        CHECK((Literal::make_typed<Datatype>(a) <=> Literal::make_typed<Datatype>(b)) == res);
    }
}

TEST_CASE("timezone") {
    using namespace rdf4cpp::datatypes::registry;
    rdf4cpp::Timezone const tz_60{std::chrono::minutes{60}};
    rdf4cpp::Timezone const tz_0{};
    auto d = std::chrono::sys_days{std::chrono::day{1} / 2 / 2042} + std::chrono::hours{5} + std::chrono::minutes{30} + std::chrono::seconds{15};
    std::chrono::zoned_time d_in0{tz_0, d};
    std::chrono::zoned_time d_in60{tz_60, d_in0};
    CHECK(std::format("{:%Y-%m-%d-%H-%M-%S-%Z}", d_in60) == "2042-02-01-06-30-15-+01:00");
    CHECK(std::format("{:%Y-%m-%d-%H-%M-%S-%Z}", d_in0) == "2042-02-01-05-30-15-Z");

    auto l = std::chrono::local_days{std::chrono::day{1} / 2 / 2042} + std::chrono::hours{5} + std::chrono::minutes{30} + std::chrono::seconds{15};
    d_in60 = std::chrono::zoned_time(tz_60, l);
    d_in0 = std::chrono::zoned_time(tz_0, d_in60);
    CHECK(std::format("{:%Y-%m-%d-%H-%M-%S-%Z}", d_in60) == "2042-02-01-05-30-15-+01:00");
    CHECK(std::format("{:%Y-%m-%d-%H-%M-%S-%Z}", d_in0) == "2042-02-01-04-30-15-Z");
}

TEST_CASE("year api") {
    using y = rdf4cpp::Year;
    using ys = std::chrono::years;
    CHECK(y(5) + ys(5) == y(10));
    CHECK(ys(5) + y(5) == y(10));

    CHECK(y(5) - ys(5) == y(0));
    CHECK(y(5) - y(5) == ys(0));

    CHECK((y(5) += ys(5)) == y(10));
    CHECK((y(5) -= ys(5)) == y(0));

    CHECK(++y(5) == y(6));
    CHECK(y(5)++ == y(5));
    CHECK(--y(5) == y(4));
    CHECK(y(5)-- == y(5));

    CHECK(static_cast<int64_t>(y::max()) == std::numeric_limits<int64_t>::max());
    CHECK(static_cast<int64_t>(y::min()) == std::numeric_limits<int64_t>::min());
}

TEST_CASE("YearMonth api") {
    auto ym = [](int y, unsigned int m) {
        return rdf4cpp::YearMonth{rdf4cpp::Year{y}, std::chrono::month{m}};
    };
    using ms = std::chrono::months;
    using ys = std::chrono::years;
    CHECK(ym(5, 6) + ys(5) == ym(10, 6));
    CHECK(ym(5, 6) - ys(5) == ym(0, 6));
    CHECK((ym(5, 6) += ys(5)) == ym(10, 6));

    CHECK(ys(5) + ym(5, 6) == ym(10, 6));

    CHECK(ym(5, 6) + ms(5) == ym(5, 11));
    CHECK(ym(5, 12) + ms(2) == ym(6, 2));

    CHECK(ym(5, 6) - ms(5) == ym(5, 1));
    CHECK(ym(5, 6) - ms(8) == ym(4, 10));
    CHECK((ym(5, 6) -= ms(8)) == ym(4, 10));

    CHECK(ym(5, 5) - ym(4, 4) == ms(13));
}

TEST_CASE("date api") {
    auto ym = [](int y, unsigned int m) {
        return rdf4cpp::YearMonthDay{rdf4cpp::Year{y}, std::chrono::month{m}, std::chrono::day{1}};
    };
    using ms = std::chrono::months;
    using ys = std::chrono::years;
    CHECK(ym(5, 6) + ys(5) == ym(10, 6));
    CHECK(ym(5, 6) - ys(5) == ym(0, 6));
    CHECK((ym(5, 6) += ys(5)) == ym(10, 6));

    CHECK(ys(5) + ym(5, 6) == ym(10, 6));

    CHECK(ym(5, 6) + ms(5) == ym(5, 11));
    CHECK(ym(5, 12) + ms(2) == ym(6, 2));

    CHECK(ym(5, 6) - ms(5) == ym(5, 1));
    CHECK(ym(5, 6) - ms(8) == ym(4, 10));
    CHECK((ym(5, 6) -= ms(8)) == ym(4, 10));
}

TEST_CASE("date") {
    for (int y = -32767; y <= 32767; ++y) { // limits of std::chrono::year
        for (unsigned int m = 1; m <= 12; ++m) {
            for (unsigned int d = 1; d <= static_cast<unsigned int>((std::chrono::year{y} / m / std::chrono::last).day()); ++d) {
                std::chrono::year_month_day const base = std::chrono::year{y} / m / d;
                auto base_sd = static_cast<std::chrono::sys_days>(base);
                rdf4cpp::YearMonthDay date{base};
                CHECK(date == rdf4cpp::YearMonthDay{rdf4cpp::Year(y), std::chrono::month(m), std::chrono::day(d)});
                CHECK(date.to_time_point() == base_sd);
                CHECK(date == rdf4cpp::YearMonthDay{base_sd});
                CHECK(date.ok());
            }
            std::chrono::year_month_day const base_last = std::chrono::year{y} / m / std::chrono::last;
            CHECK(rdf4cpp::YearMonthDay{rdf4cpp::Year(y), std::chrono::month(m), std::chrono::last} == rdf4cpp::YearMonthDay{base_last});
            CHECK(!rdf4cpp::YearMonthDay{rdf4cpp::Year(y), std::chrono::month(m), std::chrono::day{static_cast<unsigned int>(base_last.day()) + 1}}.ok());
            CHECK(!rdf4cpp::YearMonthDay{rdf4cpp::Year(y), std::chrono::month(m), std::chrono::day{0}}.ok());
        }
        CHECK(!rdf4cpp::YearMonthDay{rdf4cpp::Year(y), std::chrono::month(0), std::chrono::day{1}}.ok());
        CHECK(!rdf4cpp::YearMonthDay{rdf4cpp::Year(y), std::chrono::month(13), std::chrono::day{1}}.ok());
    }
}

TEST_CASE("date normalization") {
    auto ct = [](int64_t y, unsigned int m, unsigned int d) {
        return rdf4cpp::YearMonthDay{rdf4cpp::Year{y}, std::chrono::month{m}, std::chrono::day{d}};
    };
    using namespace rdf4cpp::datatypes::registry::util;
    CHECK(normalize(ct(1000, 0, 1)) == ct(999, 12, 1));
    CHECK(normalize(ct(1000, 13, 1)) == ct(1001, 1, 1));
    CHECK(normalize(ct(1000, 24, 1)) == ct(1001, 12, 1));
    CHECK(normalize(ct(1000, 24, 0)) == ct(1001, 11, 30));
    CHECK(normalize(ct(1000, 1, 35)) == ct(1000, 2, 4));
}

TEST_CASE("precision") {
    auto ms = std::chrono::microseconds::max();
    auto ys = std::chrono::floor<std::chrono::years>(ms);
    CHECK(ys > std::chrono::years{10000});
    CHECK(ys > std::chrono::years{static_cast<int>(std::chrono::year::max())});

    rdf4cpp::DurationNano::rep end_of_universe{"100000000000000"};
    CHECK(end_of_universe < std::numeric_limits<int64_t>::max());
    using checked_years = std::chrono::duration<rdf4cpp::DurationNano::rep, std::chrono::years::period>;
    using checked_nanos = std::chrono::duration<rdf4cpp::DurationNano::rep, std::chrono::nanoseconds::period>;
    auto nanos = static_cast<checked_nanos>(checked_years{end_of_universe});  // throws, if out of range
    CHECK(nanos.count() > end_of_universe);
}

TEST_CASE("datatype gYear") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::GYear::identifier) == "http://www.w3.org/2001/XMLSchema#gYear");

    rdf4cpp::OptionalTimezone const tz = std::nullopt;
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{500}, tz), "0500", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{1500}, tz), "1500", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{-500}, tz), "-0500", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{-1}, tz), "-0001", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{0}, tz), "0000", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{42500}, tz), "42500", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{500}, tz), "0501", std::partial_ordering::less);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{500}, Timezone{std::chrono::hours{1}}), "0500+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{500}, Timezone{std::chrono::minutes{-65}}), "0500-01:05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{500}, Timezone{std::chrono::hours{-14}}), "0500-14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(Year{500}, Timezone{std::chrono::hours{14}}), "0500+14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>("0500-1:00", "0500Z", std::partial_ordering::greater);
    basic_test<datatypes::xsd::GYear>("0500+1:00", "0500Z", std::partial_ordering::less);
    CHECK(Literal::make_typed<datatypes::xsd::GYear>("05").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GYear>("12").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::GYear>("12+14:00").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::GYear>("12-14:00").is_inlined());
    Literal n{};
    CHECK_THROWS_WITH_AS(n = Literal::make_typed<datatypes::xsd::GYear>("abc"), "http://www.w3.org/2001/XMLSchema#gYear parsing error: found a, invalid for datatype", InvalidNode);
    CHECK(n.null()); // turn off unused and nodiscard ignored warnings
}

TEST_CASE("datatype gMonth") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::GMonth::identifier) == "http://www.w3.org/2001/XMLSchema#gMonth");

    rdf4cpp::OptionalTimezone const tz = std::nullopt;
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::May, tz), "--05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::April, tz), "--05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::May, Timezone{std::chrono::hours{1}}), "--05+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::May, Timezone{std::chrono::minutes{-65}}), "--05-01:05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::May, Timezone{std::chrono::hours{-14}}), "--05-14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::May, Timezone{std::chrono::hours{14}}), "--05+14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::January, Timezone{std::chrono::hours{-14}}), "--01-14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::December, Timezone{std::chrono::hours{14}}), "--12+14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonth>("--05+1:00", "--05Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::GMonth>("--05-1:00", "--05Z", std::partial_ordering::greater);
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--05").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--12").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--12+14:00").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--12-14:00").is_inlined());
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::GMonth>("--00"), "http://www.w3.org/2001/XMLSchema#gMonth parsing error: 00 is invalid", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::GMonth>("00"), "http://www.w3.org/2001/XMLSchema#gMonth parsing error: missing gMonth prefix", InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::GMonth>("--13"), InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings
}

TEST_CASE("datatype gDay") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::GDay::identifier) == "http://www.w3.org/2001/XMLSchema#gDay");

    rdf4cpp::OptionalTimezone const tz = std::nullopt;
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{5}, tz), "---05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{4}, tz), "---05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{5}, Timezone{std::chrono::hours{1}}), "---05+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{5}, Timezone{std::chrono::minutes{-65}}), "---05-01:05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{1}, Timezone{std::chrono::hours{-14}}), "---01-14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{31}, Timezone{std::chrono::hours{14}}), "---31+14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GDay>("---05+1:00", "---05Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::GDay>("---05-1:00", "---05Z", std::partial_ordering::greater);
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---05").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---31").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---31+14:00").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---31-14:00").is_inlined());
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::GDay>("---00"), "http://www.w3.org/2001/XMLSchema#gDay parsing error: 00 is invalid", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::GDay>("-00"), "http://www.w3.org/2001/XMLSchema#gDay parsing error: missing gDay prexfix", InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::GDay>("---32"), InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings
}

TEST_CASE("datatype gYearMonth") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::GYearMonth::identifier) == "http://www.w3.org/2001/XMLSchema#gYearMonth");

    rdf4cpp::OptionalTimezone const tz = std::nullopt;
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(YearMonth{Year{2042}, std::chrono::month{5}}, tz), "2042-05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(YearMonth{Year{2042}, std::chrono::month{4}}, tz), "2042-05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(YearMonth{Year{2041}, std::chrono::month{6}}, tz), "2042-05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(YearMonth{Year{2042}, std::chrono::month{5}}, Timezone{std::chrono::hours{1}}), "2042-05+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(YearMonth{Year{2042}, std::chrono::month{5}}, Timezone{std::chrono::minutes{-65}}), "2042-05-01:05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(YearMonth{Year{2042}, std::chrono::month{5}}, Timezone{std::chrono::hours{-14}}), "2042-05-14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(YearMonth{Year{2042}, std::chrono::month{5}}, Timezone{std::chrono::hours{14}}), "2042-05+14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYearMonth>("2042-05+1:00", "2042-05Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::GYearMonth>("2042-05-1:00", "2042-05Z", std::partial_ordering::greater);
    CHECK(Literal::make_typed<datatypes::xsd::GYearMonth>("2042-05").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::GYearMonth>("32769-12").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GYearMonth>("-32767-1").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::GYearMonth>("-32767-1-14:00").is_inlined());
    CHECK(storage::reference_node_storage::SyncReferenceNodeStorage::has_specialized_storage_for(datatypes::xsd::GYearMonth::fixed_id));
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::GYearMonth>("01-00"), "http://www.w3.org/2001/XMLSchema#gYearMonth parsing error: 0001-00 is invalid", InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::GYearMonth>("32767-32"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::GYearMonth>("32768-30"), InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings
}

TEST_CASE("datatype gMonthDay") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::GMonthDay::identifier) == "http://www.w3.org/2001/XMLSchema#gMonthDay");

    rdf4cpp::OptionalTimezone const tz = std::nullopt;
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{5} / 5, tz), "--05-05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{5} / 4, tz), "--05-05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{3} / 6, tz), "--04-05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{5} / 5, Timezone{std::chrono::hours{1}}), "--05-05+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{5} / 5, Timezone{std::chrono::minutes{-65}}), "--05-05-01:05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{1} / 1, Timezone{std::chrono::hours{-14}}), "--01-01-14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{12} / 31, Timezone{std::chrono::hours{14}}), "--12-31+14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonthDay>("--05-05+1:00", "--05-05Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::GMonthDay>("--05-05-1:00", "--05-05Z", std::partial_ordering::greater);
    CHECK(Literal::make_typed<datatypes::xsd::GMonthDay>("--05-1").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GMonthDay>("--12-31").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GMonthDay>("--12-31+14:00").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::GMonthDay>("--12-31-14:00").is_inlined());
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::GMonthDay>("--01-00"), "http://www.w3.org/2001/XMLSchema#gMonthDay parsing error: 01-00 is invalid", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::GMonthDay>("01-00"), "http://www.w3.org/2001/XMLSchema#gMonthDay parsing error: gMonth prefix missing", InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::GMonthDay>("--00-01"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::GMonthDay>("--12-32"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::GMonthDay>("--13-30"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::GMonthDay>("--02-30"), InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings
}

TEST_CASE("datatype date") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::Date::identifier) == "http://www.w3.org/2001/XMLSchema#date");

    rdf4cpp::OptionalTimezone const tz = std::nullopt;
    basic_test<datatypes::xsd::Date>(std::make_pair(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, tz), "2042-05-01", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Date>(std::make_pair(YearMonthDay{Year{2042}, std::chrono::month{4}, std::chrono::day{1}}, tz), "2042-05-01", std::partial_ordering::less);
    basic_test<datatypes::xsd::Date>(std::make_pair(YearMonthDay{Year{2041}, std::chrono::month{6}, std::chrono::day{1}}, tz), "2042-05-01", std::partial_ordering::less);
    basic_test<datatypes::xsd::Date>(std::make_pair(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, Timezone{std::chrono::hours{1}}), "2042-05-01+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Date>(std::make_pair(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, Timezone{std::chrono::minutes{-65}}), "2042-05-01-01:05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Date>("2042-05-01+1:00", "2042-05-01Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::Date>("2042-05-01-1:00", "2042-05-01Z", std::partial_ordering::greater);
    CHECK(Literal::make_typed<datatypes::xsd::Date>("2042-05-1").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::Date>("2042-12-31").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::Date>("2042-12-31+14:00").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::Date>("2042-12-31-14:00").is_inlined());
    CHECK(storage::default_node_storage.has_specialized_storage_for(datatypes::xsd::Date::fixed_id));
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Date>("2042-01-00"), "http://www.w3.org/2001/XMLSchema#date parsing error: 2042-01-00 is invalid", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Date>("2042-00-01"), "http://www.w3.org/2001/XMLSchema#date parsing error: 2042-00-01 is invalid", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Date>("2042-12-32"), "http://www.w3.org/2001/XMLSchema#date parsing error: 2042-12-32 is invalid", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Date>("2042-13-30"), "http://www.w3.org/2001/XMLSchema#date parsing error: 2042-13-30 is invalid", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Date>("2042-02-30"), "http://www.w3.org/2001/XMLSchema#date parsing error: 2042-02-30 is invalid", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Date>("1937-0-0"), "http://www.w3.org/2001/XMLSchema#date parsing error: 1937-00-00 is invalid", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Date>("1937-1-1+18:00"), "http://www.w3.org/2001/XMLSchema#date parsing error: timezone offset too big", InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings
}

TEST_CASE("datatype time") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::Time::identifier) == "http://www.w3.org/2001/XMLSchema#time");

    rdf4cpp::OptionalTimezone const tz = std::nullopt;
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50}, tz), "00:50:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::hours{12} + std::chrono::minutes{34} + std::chrono::seconds{56} + std::chrono::milliseconds{789}, tz), "12:34:56.789", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50} + std::chrono::milliseconds{100}, tz), "00:50:00.1", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50} + std::chrono::microseconds{123450}, tz), "00:50:00.12345", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50} + std::chrono::nanoseconds{123456789}, tz), "00:50:00.123456789", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50} + std::chrono::nanoseconds{123456789}, tz), "00:50:00.1234567891011", std::partial_ordering::equivalent, true);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{42}, tz), "00:50:00", std::partial_ordering::less);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50}, Timezone{std::chrono::hours{1}}), "00:50:00+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50}, Timezone{std::chrono::minutes{-65}}), "00:50:00-01:05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::hours{0}, Timezone{std::chrono::hours{-14}}), "00:00:00-14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::hours{24}, Timezone{std::chrono::hours{14}}), "24:00:00+14:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>("00:50:00+1:00", "00:50:00Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::Time>("00:50:00-1:00", "00:50:00Z", std::partial_ordering::greater);
    CHECK(Literal::make_typed<datatypes::xsd::Time>("00:00:00.000").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::Time>("24:00:00.000").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::Time>("24:00:00.000+14:00").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::Time>("00:00:00.000-14:00").is_inlined());
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Time>("-10:00:00.000"), "http://www.w3.org/2001/XMLSchema#time parsing error: found -, invalid for datatype", InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::Time>("25:00:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::Time>("00:-1:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::Time>("00:70:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::Time>("00:00:-1.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::Time>("00:00:70.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::Time>("00:00:5.-100"), InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings
}

TEST_CASE("datatype dateTime") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::DateTime::identifier) == "http://www.w3.org/2001/XMLSchema#dateTime");

    rdf4cpp::OptionalTimezone const tz = std::nullopt;
    basic_test<datatypes::xsd::DateTime>(std::make_pair(rdf4cpp::util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50}), tz), "2042-05-01T00:50:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(rdf4cpp::util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::hours{12} + std::chrono::minutes{34} + std::chrono::seconds{56} + std::chrono::milliseconds{789}), tz), "2042-05-01T12:34:56.789", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(rdf4cpp::util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50} + std::chrono::milliseconds{100}), tz), "2042-05-01T00:50:00.1", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(rdf4cpp::util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50} + std::chrono::nanoseconds{123456789}), tz), "2042-05-01T00:50:00.12345678910", std::partial_ordering::equivalent, true);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(rdf4cpp::util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{42}), tz), "2042-05-01T00:50:00", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(rdf4cpp::util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50}), Timezone{std::chrono::hours{1}}), "2042-05-01T00:50:00+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(rdf4cpp::util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50}), Timezone{std::chrono::minutes{-65}}), "2042-05-01T00:50:00-01:05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08", "2042-05-05T13:40:08", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>("2041-05-05T13:40:08", "2042-05-05T13:40:08", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08", "2041-05-05T13:40:08", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTime>("2042-04-05T13:40:08", "2042-05-05T13:40:08", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08", "2042-04-05T13:40:08", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTime>("2042-05-04T13:40:08", "2042-05-05T13:40:08", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08", "2042-05-04T13:40:08", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T12:40:08", "2042-05-05T13:40:08", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08", "2042-05-05T12:40:08", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:39:08", "2042-05-05T13:40:08", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08", "2042-05-05T13:39:08", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:07", "2042-05-05T13:40:08", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08", "2042-05-05T13:40:07", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08.005", "2042-05-05T13:40:08.006", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08.006", "2042-05-05T13:40:08.005", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08Z", "2042-05-05T14:40:08+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08Z", "2042-05-05T12:30:08-01:10", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08Z", "2042-05-05T13:40:08", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08Z", "2043-05-05T13:40:08", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08Z", "2041-05-05T13:40:08", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08", "2043-05-05T13:40:08Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>("2042-05-05T13:40:08", "2041-05-05T13:40:08Z", std::partial_ordering::greater);
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("-32767-01-01T00:00:00.000").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("32767-12-31T23:59:59.000").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::DateTime>("-32767-01-01T00:00:00.000-14:00").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::DateTime>("32767-12-31T23:59:59.999+14:00").is_inlined());
    CHECK(storage::default_node_storage.has_specialized_storage_for(datatypes::xsd::DateTime::fixed_id));
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-05-05T-10:00:00.000"), "http://www.w3.org/2001/XMLSchema#dateTime parsing error: found -, invalid for datatype", InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-05-05T25:00:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-05-05T00:-1:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-05-05T00:70:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-05-05T00:00:-1.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-05-05T00:00:70.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-05-05T00:00:5.-100"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-01-00T00:00:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-00-01T00:00:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-12-32T00:00:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-13-30T00:00:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTime>("2042-02-30T00:00:00.000"), InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings
    auto m = std::format("{}-12-31T23:59:59", std::numeric_limits<int64_t>::max());
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>(m).lexical_form() == m);
    CHECK(static_cast<int64_t>(Literal::make_typed<datatypes::xsd::DateTime>(m).year().value()) == std::numeric_limits<int64_t>::max());
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-05-06T00:00:00.000") == Literal::make_typed<datatypes::xsd::DateTime>("2042-05-05T24:00:00"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-05-05T24:00:00.000").lexical_form() == "2042-05-06T00:00:00");

    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("-13798000000-01-01T00:00:00Z").lexical_form() == "-13798000000-01-01T00:00:00Z"); // check that age of universe is in range
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("100000000000000-01-01T00:00:00Z").lexical_form() == "100000000000000-01-01T00:00:00Z");
}

TEST_CASE("datatype dateTimeStamp") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::DateTimeStamp::identifier) == "http://www.w3.org/2001/XMLSchema#dateTimeStamp");

    Timezone const tz{std::chrono::hours{0}};
    basic_test<datatypes::xsd::DateTimeStamp>(ZonedTime(tz, util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50})), "2042-05-01T00:50:00Z", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>(ZonedTime(tz, util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::hours{12} + std::chrono::minutes{34} + std::chrono::seconds{56} + std::chrono::milliseconds{789})), "2042-05-01T12:34:56.789Z", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>(ZonedTime(tz, util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50} + std::chrono::milliseconds{100})), "2042-05-01T00:50:00.1Z", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>(ZonedTime(tz, util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50} + std::chrono::nanoseconds{123456789})), "2042-05-01T00:50:00.12345678910Z", std::partial_ordering::equivalent, true);
    basic_test<datatypes::xsd::DateTimeStamp>(ZonedTime(tz, util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{42})), "2042-05-01T00:50:00Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTimeStamp>(ZonedTime(Timezone{std::chrono::hours{1}}, util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50})), "2042-05-01T00:50:00+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>(ZonedTime(Timezone{std::chrono::minutes{-65}}, util::construct_timepoint(YearMonthDay{Year{2042}, std::chrono::month{5}, std::chrono::day{1}}, std::chrono::minutes{50})), "2042-05-01T00:50:00-01:05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08Z", "2042-05-05T13:40:08Z", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>("2041-05-05T13:40:08Z", "2042-05-05T13:40:08Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08Z", "2041-05-05T13:40:08Z", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-04-05T13:40:08Z", "2042-05-05T13:40:08Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08Z", "2042-04-05T13:40:08Z", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-04T13:40:08Z", "2042-05-05T13:40:08Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08Z", "2042-05-04T13:40:08Z", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T12:40:08Z", "2042-05-05T13:40:08Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08Z", "2042-05-05T12:40:08Z", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:39:08Z", "2042-05-05T13:40:08Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08Z", "2042-05-05T13:39:08Z", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:07Z", "2042-05-05T13:40:08Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08Z", "2042-05-05T13:40:07Z", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08.005Z", "2042-05-05T13:40:08.006Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08.006Z", "2042-05-05T13:40:08.005Z", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08Z", "2042-05-05T14:40:08+01:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>("2042-05-05T13:40:08Z", "2042-05-05T12:30:08-01:10", std::partial_ordering::equivalent);
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("-32767-01-01T00:00:00.000Z").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("32767-12-31T23:59:59.000Z").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::DateTimeStamp>("-32767-01-01T00:00:00.000-14:00").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::DateTimeStamp>("32767-12-31T23:59:59.999+14:00").is_inlined());
    CHECK(storage::default_node_storage.has_specialized_storage_for(datatypes::xsd::DateTimeStamp::fixed_id));
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-05T-10:00:00.000Z"), "http://www.w3.org/2001/XMLSchema#dateTimeStamp parsing error: found -, invalid for datatype", InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-05T25:00:00.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-05T00:-1:00.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-05T00:70:00.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-05T00:00:-1.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-05T00:00:70.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-05T00:00:5.-100Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-01-00T00:00:00.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-00-01T00:00:00.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-12-32T00:00:00.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-13-30T00:00:00.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-02-30T00:00:00.000Z"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-02-24T00:00:00.000"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-02-24T00:00:00.000+20:00"), InvalidNode);
    CHECK_THROWS_AS(a = Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-02-24T00:00:00.000-20:00"), InvalidNode);
    CHECK(a.null());  // turn off unused and nodiscard ignored warnings
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-06T00:00:00.000Z") == Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-05T24:00:00Z"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-05-05T24:00:00.000Z").lexical_form() == "2042-05-06T00:00:00Z");
}

TEST_CASE("datatype duration") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::Duration::identifier) == "http://www.w3.org/2001/XMLSchema#duration");

    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::months{0}, std::chrono::minutes{0}), "PT0S", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::months{0}, std::chrono::minutes{1}), "PT1M", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::months{0}, std::chrono::seconds{1}), "PT01S", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::months{0}, std::chrono::hours{1}), "PT1H", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::months{0}, std::chrono::days{1}), "P1D", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::months{1}, std::chrono::minutes{0}), "P1M", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::years{1}, std::chrono::minutes{0}), "P1Y", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::months{1}, std::chrono::minutes{1}), "P1MT1M", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::months{-1}, std::chrono::minutes{-1}), "-P1MT1M", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Duration>(std::make_pair(std::chrono::months{14}, std::chrono::days{3}+std::chrono::hours{4}+std::chrono::minutes{5}+std::chrono::seconds{6}), "P1Y2M3DT4H5M06S", std::partial_ordering::equivalent);
    CHECK(Literal::make_typed<datatypes::xsd::Duration>("PT1M").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::Duration>("P1M").is_inlined());
    CHECK(Literal::make_typed<datatypes::xsd::Duration>("P365D").is_inlined());
    CHECK(!Literal::make_typed<datatypes::xsd::Duration>("P5000D").is_inlined());
    CHECK(storage::default_node_storage.has_specialized_storage_for(datatypes::xsd::Duration::fixed_id));

    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Duration>(""), "http://www.w3.org/2001/XMLSchema#duration parsing error: duration missing P", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Duration>("P"), "http://www.w3.org/2001/XMLSchema#duration parsing error: duration without any fields", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Duration>("PT"), "http://www.w3.org/2001/XMLSchema#duration parsing error: duration without any fields", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Duration>("P5M24Y"), "http://www.w3.org/2001/XMLSchema#duration parsing error: found M, invalid for datatype", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Duration>("P5YABC"), "http://www.w3.org/2001/XMLSchema#duration parsing error: found ABC, expected empty", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::Duration>("-P5Y-3D"), "http://www.w3.org/2001/XMLSchema#duration parsing error: found -, invalid for datatype", InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings

    basic_test<datatypes::xsd::Duration>("P1M", "P30D", std::partial_ordering::unordered);
    basic_test<datatypes::xsd::Duration>("P1M", "P2M", std::partial_ordering::less);
    basic_test<datatypes::xsd::Duration>("PT1M", "PT30S", std::partial_ordering::greater);
    basic_test<datatypes::xsd::Duration>("PT1M", "PT2M", std::partial_ordering::less);
}

TEST_CASE("datatype dayTimeDuration") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::DayTimeDuration::identifier) == "http://www.w3.org/2001/XMLSchema#dayTimeDuration");

    basic_test<datatypes::xsd::DayTimeDuration>(std::chrono::minutes{0}, "PT0S", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DayTimeDuration>(std::chrono::minutes{1}, "PT1M", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DayTimeDuration>(std::chrono::seconds{1}, "PT01S", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DayTimeDuration>(std::chrono::hours{1}, "PT1H", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DayTimeDuration>(std::chrono::days{1}, "P1D", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DayTimeDuration>(std::chrono::days{1}+std::chrono::hours{2}+std::chrono::minutes{3}+std::chrono::seconds{4}, "P1DT2H3M04S", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DayTimeDuration>(std::chrono::days{-1}, "-P1D", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DayTimeDuration>(std::chrono::nanoseconds::max(), "P106751DT23H47M16.854775807S", std::partial_ordering::equivalent);
    CHECK(Literal::make_typed<datatypes::xsd::DayTimeDuration>("P500DT42M").is_inlined());
    CHECK(storage::default_node_storage.has_specialized_storage_for(datatypes::xsd::DayTimeDuration::fixed_id));
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::DayTimeDuration>(""), "http://www.w3.org/2001/XMLSchema#dayTimeDuration parsing error: duration missing P", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::DayTimeDuration>("P"), "http://www.w3.org/2001/XMLSchema#dayTimeDuration parsing error: duration without any fields", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT"), "http://www.w3.org/2001/XMLSchema#dayTimeDuration parsing error: duration without any fields", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT5M24H"), "http://www.w3.org/2001/XMLSchema#dayTimeDuration parsing error: found M, invalid for datatype", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::DayTimeDuration>("P5DABC"), "http://www.w3.org/2001/XMLSchema#dayTimeDuration parsing error: found ABC, expected empty", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::DayTimeDuration>("P10Y"), "http://www.w3.org/2001/XMLSchema#dayTimeDuration parsing error: found 10Y, expected empty", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::DayTimeDuration>("P5M"), "http://www.w3.org/2001/XMLSchema#dayTimeDuration parsing error: found 5M, expected empty", InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings

    basic_test<datatypes::xsd::DayTimeDuration>("PT1M", "PT30S", std::partial_ordering::greater);
    basic_test<datatypes::xsd::DayTimeDuration>("PT1M", "PT2M", std::partial_ordering::less);
}

TEST_CASE("datatype yearMonthDuration") {
    using namespace rdf4cpp;

    CHECK(std::string(datatypes::xsd::YearMonthDuration::identifier) == "http://www.w3.org/2001/XMLSchema#yearMonthDuration");

    basic_test<datatypes::xsd::YearMonthDuration>(std::chrono::months{0}, "P0M", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::YearMonthDuration>(std::chrono::years{1}, "P1Y", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::YearMonthDuration>(std::chrono::years{1} + std::chrono::months{1}, "P1Y1M", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::YearMonthDuration>(std::chrono::years{-1} + std::chrono::months{-1}, "-P1Y1M", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::YearMonthDuration>(std::chrono::months::max(), "P768614336404564650Y7M", std::partial_ordering::equivalent);
    CHECK(Literal::make_typed<datatypes::xsd::YearMonthDuration>("P500Y30M").is_inlined());
    CHECK(storage::default_node_storage.has_specialized_storage_for(datatypes::xsd::YearMonthDuration::fixed_id));
    Literal a{};
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::YearMonthDuration>(""), "http://www.w3.org/2001/XMLSchema#yearMonthDuration parsing error: found empty string", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::YearMonthDuration>("P"), "http://www.w3.org/2001/XMLSchema#yearMonthDuration parsing error: duration without any fields", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::YearMonthDuration>("PT"), "http://www.w3.org/2001/XMLSchema#yearMonthDuration parsing error: found T, expected empty", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::YearMonthDuration>("P5M24Y"), "http://www.w3.org/2001/XMLSchema#yearMonthDuration parsing error: found M, invalid for datatype", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::YearMonthDuration>("P5YABC"), "http://www.w3.org/2001/XMLSchema#yearMonthDuration parsing error: found ABC, expected empty", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::YearMonthDuration>("PT10H"), "http://www.w3.org/2001/XMLSchema#yearMonthDuration parsing error: found T10H, expected empty", InvalidNode);
    CHECK_THROWS_WITH_AS(a = Literal::make_typed<datatypes::xsd::YearMonthDuration>("P5D"), "http://www.w3.org/2001/XMLSchema#yearMonthDuration parsing error: found 5D, expected empty", InvalidNode);
    CHECK(a.null()); // turn off unused and nodiscard ignored warnings

    basic_test<datatypes::xsd::YearMonthDuration>("P1M", "P2M", std::partial_ordering::less);
    basic_test<datatypes::xsd::YearMonthDuration>("P1Y", "P1M", std::partial_ordering::greater);
}

TEST_CASE("cross compare") {
    using namespace rdf4cpp;

    CHECK(Literal::make_typed<datatypes::xsd::Date>("2042-5-5") < Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T10:0:0"));
    CHECK(Literal::make_typed<datatypes::xsd::Date>("2042-5-5") > Literal::make_typed<datatypes::xsd::GDay>("---5"));
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---5") < Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T10:0:0"));
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--5") < Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T10:0:0"));
    CHECK(Literal::make_typed<datatypes::xsd::GYear>("2043") > Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T10:0:0"));
    CHECK(Literal::make_typed<datatypes::xsd::GYearMonth>("2043-5") > Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T10:0:0"));
    CHECK(Literal::make_typed<datatypes::xsd::GMonthDay>("--5-5") < Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T10:0:0"));
    CHECK(Literal::make_typed<datatypes::xsd::Time>("12:0:0") > Literal::make_typed<datatypes::xsd::DateTime>("1972-1-1T10:0:0"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("1972-12-31T12:0:0Z") > Literal::make_typed<datatypes::xsd::DateTime>("1972-12-31T10:0:0Z"));

    CHECK(Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y") < Literal::make_typed<datatypes::xsd::Duration>("P1YT1H"));
    CHECK(Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT1H") < Literal::make_typed<datatypes::xsd::Duration>("P1YT1H"));
}

TEST_CASE("Literal API") {
    using namespace rdf4cpp;

    CHECK(Literal::now().datatype_eq<datatypes::xsd::DateTime>());

    CHECK(Literal::make_typed<datatypes::xsd::Date>("2042-5-6").as_year() == Literal::make_typed<datatypes::xsd::Integer>("2042"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3").as_year() == Literal::make_typed<datatypes::xsd::Integer>("2042"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3Z").as_year() == Literal::make_typed<datatypes::xsd::Integer>("2042"));
    CHECK(Literal::make_typed<datatypes::xsd::GYear>("2042").as_year() == Literal::make_typed<datatypes::xsd::Integer>("2042"));
    CHECK(Literal::make_typed<datatypes::xsd::GYearMonth>("2042-5").as_year() == Literal::make_typed<datatypes::xsd::Integer>("2042"));
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---5").as_year().null());
    CHECK(Literal::make_simple("5").as_year().null());

    CHECK(Literal::make_typed<datatypes::xsd::Date>("2042-5-6").as_month() == Literal::make_typed<datatypes::xsd::Integer>("5"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3").as_month() == Literal::make_typed<datatypes::xsd::Integer>("5"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3Z").as_month() == Literal::make_typed<datatypes::xsd::Integer>("5"));
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--5").as_month() == Literal::make_typed<datatypes::xsd::Integer>("5"));
    CHECK(Literal::make_typed<datatypes::xsd::GMonthDay>("--5-6").as_month() == Literal::make_typed<datatypes::xsd::Integer>("5"));
    CHECK(Literal::make_typed<datatypes::xsd::GYearMonth>("2042-5").as_month() == Literal::make_typed<datatypes::xsd::Integer>("5"));
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---5").as_month().null());
    CHECK(Literal::make_simple("5").as_month().null());

    CHECK(Literal::make_typed<datatypes::xsd::Date>("2042-5-6").as_day() == Literal::make_typed<datatypes::xsd::Integer>("6"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3").as_day() == Literal::make_typed<datatypes::xsd::Integer>("6"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3Z").as_day() == Literal::make_typed<datatypes::xsd::Integer>("6"));
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---5").as_day() == Literal::make_typed<datatypes::xsd::Integer>("5"));
    CHECK(Literal::make_typed<datatypes::xsd::GMonthDay>("--5-6").as_day() == Literal::make_typed<datatypes::xsd::Integer>("6"));
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--5").as_day().null());
    CHECK(Literal::make_simple("5").as_day().null());

    CHECK(Literal::make_typed<datatypes::xsd::Time>("1:2:3").as_hours() == Literal::make_typed<datatypes::xsd::Integer>("1"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3+1:0").as_hours() == Literal::make_typed<datatypes::xsd::Integer>("1"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3+1:0").as_hours() == Literal::make_typed<datatypes::xsd::Integer>("1"));
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--5").as_hours().null());
    CHECK(Literal::make_simple("5").as_hours().null());

    CHECK(Literal::make_typed<datatypes::xsd::Time>("1:2:3").as_minutes() == Literal::make_typed<datatypes::xsd::Integer>("2"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3+1:0").as_minutes() == Literal::make_typed<datatypes::xsd::Integer>("2"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3+1:0").as_minutes() == Literal::make_typed<datatypes::xsd::Integer>("2"));
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--5").as_minutes().null());
    CHECK(Literal::make_simple("5").as_minutes().null());

    CHECK(Literal::make_typed<datatypes::xsd::Time>("1:2:3").as_seconds() == Literal::make_typed<datatypes::xsd::Decimal>("3"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3+1:0").as_seconds() == Literal::make_typed<datatypes::xsd::Decimal>("3"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3+1:0").as_seconds() == Literal::make_typed<datatypes::xsd::Decimal>("3"));
    CHECK(Literal::make_typed<datatypes::xsd::Time>("1:2:59.999").as_seconds() == Literal::make_typed<datatypes::xsd::Decimal>("59.999"));
    CHECK(Literal::make_typed<datatypes::xsd::GMonth>("--5").as_seconds().null());
    CHECK(Literal::make_simple("5").as_seconds().null());

    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3+1:0").as_timezone() == Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT1H"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3Z").as_timezone() == Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT0H"));
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---3+1:30").as_timezone() == Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT1H30M"));
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---3").as_timezone().null());
    CHECK(Literal::make_simple("5").as_timezone().null());

    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3+1:0").as_tz() == Literal::make_simple("+01:00"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3Z").as_tz() == Literal::make_simple("Z"));
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---3+1:30").as_tz() == Literal::make_simple("+01:30"));
    CHECK(Literal::make_typed<datatypes::xsd::GDay>("---3").as_tz() == Literal::make_simple(""));
    CHECK(Literal::make_simple("5").as_tz().null());
}

TEST_CASE("arithmetic") {
    using namespace rdf4cpp;

    CHECK((Literal::make_typed<datatypes::xsd::Date>("2042-5-6") - Literal::make_typed<datatypes::xsd::Date>("2042-5-5")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1D"));
    CHECK((Literal::make_typed<datatypes::xsd::Date>("2042-4-6") - Literal::make_typed<datatypes::xsd::Date>("2042-5-7")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("-P31D"));
    CHECK((Literal::make_typed<datatypes::xsd::Time>("1:2:3") - Literal::make_typed<datatypes::xsd::Time>("1:1:3")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT1M"));
    CHECK((Literal::make_typed<datatypes::xsd::Time>("1:2:3") - Literal::make_typed<datatypes::xsd::Time>("1:3:3")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("-PT1M"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") - Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T1:2:3")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1D"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3Z") - Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-7T1:2:3Z")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("-P1D"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3Z") - Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3+10:00")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT10H"));
    CHECK((Literal::make_typed_from_value<datatypes::xsd::DateTime>(std::make_pair(rdf4cpp::TimePoint{std::chrono::milliseconds::min()}, std::nullopt)) - Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T1:2:3")).null());

    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") + Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1S")) == Literal::make_typed<datatypes::xsd::DateTime>("2042-5-7T1:2:4"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3Z") + Literal::make_typed<datatypes::xsd::DayTimeDuration>("-P1DT1S")) == Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T1:2:2Z"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") + Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y2M")) == Literal::make_typed<datatypes::xsd::DateTime>("2043-7-6T1:2:3"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") + Literal::make_typed<datatypes::xsd::Duration>("P1Y2MT5S")) == Literal::make_typed<datatypes::xsd::DateTime>("2043-7-6T1:2:8"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-10-6T1:2:3") + Literal::make_typed<datatypes::xsd::Duration>("P4M")) == Literal::make_typed<datatypes::xsd::DateTime>("2043-2-6T1:2:3"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-2-6T1:2:3") + Literal::make_typed<datatypes::xsd::Duration>("-P4M")) == Literal::make_typed<datatypes::xsd::DateTime>("2041-10-6T1:2:3"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") + Literal::make_typed<datatypes::xsd::Duration>("P1Y14MT5S")) == Literal::make_typed<datatypes::xsd::DateTime>("2044-7-6T1:2:8"));
    CHECK((Literal::make_typed<datatypes::xsd::Date>("2042-5-6") + Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y")) == Literal::make_typed<datatypes::xsd::Date>("2043-5-6"));
    CHECK((Literal::make_typed<datatypes::xsd::Time>("1:2:3") + Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1S")) == Literal::make_typed<datatypes::xsd::Time>("1:2:4"));
    CHECK(!(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") + Literal::make_typed_from_value<datatypes::xsd::YearMonthDuration>(std::chrono::months::max())).null());
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>(std::format("{}-5-6T1:2:3", rdf4cpp::Year::max())) + Literal::make_typed_from_value<datatypes::xsd::YearMonthDuration>(std::chrono::months::max())).null());
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>(std::format("{}-5-6T1:2:3", rdf4cpp::Year::max())) + Literal::make_typed_from_value<datatypes::xsd::YearMonthDuration>(std::chrono::months{12})).null());
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>(std::format("{}-5-6T1:2:3", rdf4cpp::Year::max())) + Literal::make_typed_from_value<datatypes::xsd::DayTimeDuration>(std::chrono::years{1})).null());
    CHECK(!(Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") + Literal::make_typed_from_value<datatypes::xsd::DayTimeDuration>(std::chrono::nanoseconds::max())).null());

    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") - Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1S")) == Literal::make_typed<datatypes::xsd::DateTime>("2042-5-5T1:2:2"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTimeStamp>("2042-5-6T1:2:3Z") - Literal::make_typed<datatypes::xsd::DayTimeDuration>("-P1DT1S")) == Literal::make_typed<datatypes::xsd::DateTime>("2042-5-7T1:2:4Z"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") - Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y2M")) == Literal::make_typed<datatypes::xsd::DateTime>("2041-3-6T1:2:3"));
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>("2042-5-6T1:2:3") - Literal::make_typed<datatypes::xsd::Duration>("P1Y2MT3S")) == Literal::make_typed<datatypes::xsd::DateTime>("2041-3-6T1:2:0"));
    CHECK((Literal::make_typed<datatypes::xsd::Date>("2042-5-6") - Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y")) == Literal::make_typed<datatypes::xsd::Date>("2041-5-6"));
    CHECK((Literal::make_typed<datatypes::xsd::Time>("1:2:3") - Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1S")) == Literal::make_typed<datatypes::xsd::Time>("1:2:2"));
    CHECK(!(Literal::make_typed<datatypes::xsd::DateTime>("-2042-5-6T1:2:3") - Literal::make_typed_from_value<datatypes::xsd::YearMonthDuration>(std::chrono::months::max())).null());
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>(std::format("{}-5-6T1:2:3", Year::min())) - Literal::make_typed_from_value<datatypes::xsd::YearMonthDuration>(std::chrono::months::max())).null());
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>(std::format("{}-1-6T1:2:3", Year::min())) - Literal::make_typed_from_value<datatypes::xsd::YearMonthDuration>(std::chrono::months{12})).null());
    CHECK((Literal::make_typed<datatypes::xsd::DateTime>(std::format("{}-1-6T1:2:3", Year::min())) - Literal::make_typed_from_value<datatypes::xsd::DayTimeDuration>(std::chrono::years{1})).null());
    CHECK(!(Literal::make_typed<datatypes::xsd::DateTime>("-2042-5-6T1:2:3") - Literal::make_typed_from_value<datatypes::xsd::DayTimeDuration>(std::chrono::nanoseconds::max())).null());

    CHECK((Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1S") + Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1H")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("P2DT1H1S"));
    CHECK((Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y") + Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y2M")) == Literal::make_typed<datatypes::xsd::YearMonthDuration>("P2Y2M"));
    CHECK((Literal::make_typed_from_value<datatypes::xsd::DayTimeDuration>(std::chrono::nanoseconds::max()) + Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1H")).null());
    CHECK((Literal::make_typed_from_value<datatypes::xsd::YearMonthDuration>(std::chrono::months::max()) + Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y2M")).null());
    CHECK((Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1M") - Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1S")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT59S"));
    CHECK((Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y") - Literal::make_typed<datatypes::xsd::YearMonthDuration>("P2Y")) == Literal::make_typed<datatypes::xsd::YearMonthDuration>("-P1Y"));

    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") + Literal::make_typed<datatypes::xsd::Duration>("P2YT3M")) == Literal::make_typed<datatypes::xsd::Duration>("P3YT4M"));
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P2YT1M") + Literal::make_typed<datatypes::xsd::Duration>("-P1YT2M")).null());
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") - Literal::make_typed<datatypes::xsd::Duration>("-P2YT3M")) == Literal::make_typed<datatypes::xsd::Duration>("P3YT4M"));
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P2YT1M") - Literal::make_typed<datatypes::xsd::Duration>("P1YT2M")).null());
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") * Literal::make_typed<datatypes::xsd::Double>("2")) == Literal::make_typed<datatypes::xsd::Duration>("P2YT2M"));
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P2YT2M") * Literal::make_typed<datatypes::xsd::Double>("0.5")) == Literal::make_typed<datatypes::xsd::Duration>("P1YT1M"));
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P2YT2M") / Literal::make_typed<datatypes::xsd::Double>("2")) == Literal::make_typed<datatypes::xsd::Duration>("P1YT1M"));
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P2YT2M") / Literal::make_typed<datatypes::xsd::Double>("0")).null());
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P2YT1M") / Literal::make_typed<datatypes::xsd::Duration>("P1YT2M")).null());

    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") + Literal::make_typed<datatypes::xsd::YearMonthDuration>("P2Y")) == Literal::make_typed<datatypes::xsd::Duration>("P3YT1M"));
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") - Literal::make_typed<datatypes::xsd::YearMonthDuration>("-P2Y")) == Literal::make_typed<datatypes::xsd::Duration>("P3YT1M"));
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") + Literal::make_typed<datatypes::xsd::YearMonthDuration>("-P2Y")).null());
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") - Literal::make_typed<datatypes::xsd::YearMonthDuration>("P2Y")).null());

    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") + Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT2M")) == Literal::make_typed<datatypes::xsd::Duration>("P1YT3M"));
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") - Literal::make_typed<datatypes::xsd::DayTimeDuration>("-PT2M")) == Literal::make_typed<datatypes::xsd::Duration>("P1YT3M"));
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") + Literal::make_typed<datatypes::xsd::DayTimeDuration>("-PT2M")).null());
    CHECK((Literal::make_typed<datatypes::xsd::Duration>("P1YT1M") - Literal::make_typed<datatypes::xsd::DayTimeDuration>("PT2M")).null());

    CHECK((Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1S") * Literal::make_typed<datatypes::xsd::Double>("2")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("P2DT2S"));
    CHECK((Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y") * Literal::make_typed<datatypes::xsd::Double>("2")) == Literal::make_typed<datatypes::xsd::YearMonthDuration>("P2Y"));
    CHECK((Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1S") * Literal::make_typed_from_value<datatypes::xsd::Double>(std::numeric_limits<double>::infinity())).null());
    CHECK((Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y") * Literal::make_typed_from_value<datatypes::xsd::Double>(std::numeric_limits<double>::infinity())).null());
    CHECK((Literal::make_typed<datatypes::xsd::DayTimeDuration>("P2DT2S") / Literal::make_typed<datatypes::xsd::Double>("2")) == Literal::make_typed<datatypes::xsd::DayTimeDuration>("P1DT1S"));
    CHECK((Literal::make_typed<datatypes::xsd::YearMonthDuration>("P2Y") / Literal::make_typed<datatypes::xsd::Double>("2")) == Literal::make_typed<datatypes::xsd::YearMonthDuration>("P1Y"));
    CHECK((Literal::make_typed<datatypes::xsd::DayTimeDuration>("P2DT2S") / Literal::make_typed_from_value<datatypes::xsd::Double>(0.0)).null());
    CHECK((Literal::make_typed<datatypes::xsd::YearMonthDuration>("P2Y") / Literal::make_typed_from_value<datatypes::xsd::Double>(0.0)).null());
    CHECK((Literal::make_typed<datatypes::xsd::DayTimeDuration>("P4D") / Literal::make_typed<datatypes::xsd::DayTimeDuration>("P2D")) == Literal::make_typed<datatypes::xsd::Decimal>("2"));
    CHECK((Literal::make_typed<datatypes::xsd::YearMonthDuration>("P4Y") / Literal::make_typed<datatypes::xsd::YearMonthDuration>("P2Y")) == Literal::make_typed<datatypes::xsd::Decimal>("2"));
}

TEST_CASE("misc") {
    using namespace rdf4cpp;

    // https://github.com/w3c/rdf-tests/blob/main/sparql/sparql10/open-world/open-cmp-01.rq
    // https://github.com/w3c/rdf-tests/blob/main/sparql/sparql10/open-world/data-4.ttl
    // https://github.com/w3c/rdf-tests/blob/main/sparql/sparql10/open-world/open-cmp-01-result.srx
    {
        auto lit_date = Literal::make_typed<datatypes::xsd::Date>("2006-08-22");
        auto lit_dt = Literal::make_typed<datatypes::xsd::DateTime>("2006-08-23T09:00:00+01:00");
        CHECK(((lit_date < lit_dt) || (lit_date > lit_dt)));
        CHECK(!lit_date.as_ne(lit_dt).null());
    }


    // https://github.com/w3c/rdf-tests/blob/main/sparql/sparql10/open-world/date-3.rq
    // https://github.com/w3c/rdf-tests/blob/main/sparql/sparql10/open-world/data-3.ttl
    // https://github.com/w3c/rdf-tests/blob/main/sparql/sparql10/open-world/date-3-result.srx
    {
        auto query = Literal::make_typed<datatypes::xsd::Date>("2006-08-22");
        // not in the results, because of FILTER
        CHECK(Literal::make_typed<datatypes::xsd::DateTime>("2006-08-23T09:00:00+01:00") > query);
        CHECK(Literal::make_typed<datatypes::xsd::Date>("2006-08-23") > query);
        CHECK(Literal::make_typed<datatypes::xsd::Date>("2006-08-23Z") > query);
        CHECK(Literal::make_typed<datatypes::xsd::Date>("2006-08-23+00:00") > query);
        CHECK(Literal::make_typed<datatypes::xsd::Date>("2001-01-01") <= query);
        CHECK(Literal::make_typed<datatypes::xsd::Date>("2001-01-01Z") <= query);
    }
}
