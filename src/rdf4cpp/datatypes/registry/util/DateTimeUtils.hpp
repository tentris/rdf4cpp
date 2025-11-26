#ifndef RDF4CPP_DATETIMEUTILS_HPP
#define RDF4CPP_DATETIMEUTILS_HPP

#include <chrono>
#include <format>
#include <string_view>

#include <dice/hash.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>
#include <rdf4cpp/util/CheckedInt.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/Assert.hpp>

/**
 * @file
 * various date/time utilities
 * @note this header is not intended to be included by end users. But if something in here is useful for some edge cases, feel free to do so anyway.
 */

namespace rdf4cpp::datatypes::registry::util {
using CheckedMilliseconds = std::chrono::duration<rdf4cpp::util::CheckedIntegral<int64_t>, std::milli>;
using CheckedMonths = std::chrono::duration<rdf4cpp::util::CheckedIntegral<int64_t>, std::ratio<2629746>>;
static_assert(std::same_as<std::chrono::months::period, CheckedMonths::period>);
using CheckedTimePoint = std::chrono::time_point<std::chrono::local_t, CheckedMilliseconds>;
using CheckedZonedTime = std::chrono::zoned_time<CheckedMilliseconds, rdf4cpp::Timezone>;
using CheckedTimePointSys = std::chrono::time_point<std::chrono::system_clock, CheckedMilliseconds>;

/**
 * turns any duration to its CheckedIntegral counterpart.
 * @tparam R
 * @param v
 * @return
 */
template<typename R>
std::chrono::duration<rdf4cpp::util::CheckedIntegral<int64_t>, R> to_checked(std::chrono::duration<int64_t, R> v) noexcept {
    return std::chrono::duration<rdf4cpp::util::CheckedIntegral<int64_t>, R>{v.count()};
}
/**
 * turns any CheckedIntegral duration back to its integer based duration.
 * @note undefined behavior, if v is invalid
 * @tparam R
 * @param v
 * @return
 */
template<typename R>
std::chrono::duration<int64_t, R> from_checked(std::chrono::duration<rdf4cpp::util::CheckedIntegral<int64_t>, R> v) noexcept {
    RDF4CPP_ASSERT(!v.count().is_invalid());
    return std::chrono::duration<int64_t, R>{v.count().get_value()};
}
/**
 * turns any time_point to its CheckedIntegral counterpart.
 * @tparam C
 * @tparam R
 * @param v
 * @return
 */
template<typename C, typename R>
std::chrono::time_point<C, std::chrono::duration<rdf4cpp::util::CheckedIntegral<int64_t>, R>> to_checked(std::chrono::time_point<C, std::chrono::duration<int64_t, R>> v) noexcept {
    return std::chrono::time_point<C, std::chrono::duration<rdf4cpp::util::CheckedIntegral<int64_t>, R>>{to_checked(v.time_since_epoch())};
}
/**
 * turns any CheckedIntegral time_point back to its integer based time_point.
 * @note undefined behavior, if v is invalid
 * @tparam C
 * @tparam R
 * @param v
 * @return
 */
template<typename C, typename R>
std::chrono::time_point<C, std::chrono::duration<int64_t, R>> from_checked(std::chrono::time_point<C, std::chrono::duration<rdf4cpp::util::CheckedIntegral<int64_t>, R>> v) noexcept {
    return std::chrono::time_point<C, std::chrono::duration<int64_t, R>>{from_checked(v.time_since_epoch())};
}

/**
 * checks if a double fits into a specified integer type.
 * @tparam I
 * @param d
 * @return
 */
template<std::integral I, typename J>
bool fits_into(J d) {
    if constexpr (std::is_floating_point_v<J>) {
        if (std::isnan(d) || std::isinf(d)) {
            return false;
        }
    }
    if (d >= static_cast<J>(std::numeric_limits<I>::max())) {
        return false;
    }
    if (d <= static_cast<J>(std::numeric_limits<I>::min())) {
        return false;
    }
    return true;
}

/**
 * checks if a boost::multiprecision::number fits into a specified integer type.
 * @tparam I
 * @tparam B
 * @tparam e
 * @param n
 * @return
 */
template<std::integral I, typename B, boost::multiprecision::expression_template_option e>
constexpr bool fits_into(boost::multiprecision::number<B, e> const &n) {
    return n <= std::numeric_limits<I>::max() && n >= std::numeric_limits<I>::min();
}

template<typename ResultType, typename ParsingType, char Separator, ConstexprString datatype>
ResultType parse_date_time_fragment(std::string_view &s) {
    std::string_view res_s = s;
    if constexpr (Separator != '\0') {
        auto p = s.find(Separator, 1);
        if (p == std::string::npos)
            throw InvalidNode(std::format("{} parse error: missing {}", datatype, Separator));
        res_s = s.substr(0, p);
        s = s.substr(p + 1);
    }
    return ResultType{from_chars<ParsingType, datatype>(res_s)};
}

template<typename ResultType, typename ParsingType, char Separator, ConstexprString datatype>
std::optional<ResultType> parse_duration_fragment(std::string_view &s) {
    if (s.empty())
        return std::nullopt;
    std::string_view res_s = s;
    auto p = s.find(Separator);
    if (p == std::string::npos)
        return std::nullopt;
    res_s = s.substr(0, p);
    s = s.substr(p + 1);
    return ResultType{from_chars<ParsingType, datatype>(res_s)};
}

template<ConstexprString datatype>
inline std::chrono::nanoseconds parse_nanoseconds(std::string_view s) {
    auto p = s.find('.');
    std::chrono::nanoseconds ms{};
    if (p != std::string::npos) {
        auto milli_s = s.substr(p + 1, 9);
        ms = std::chrono::nanoseconds{from_chars<unsigned int, datatype>(milli_s)};
        for (size_t i = milli_s.length(); i < 9; ++i) {
            ms *= 10;
        }
        s = s.substr(0, p);
    }
    std::chrono::seconds sec{from_chars<unsigned int, datatype>(s)};
    return sec + ms;
}

template<ConstexprString datatype>
inline std::optional<std::chrono::nanoseconds> parse_duration_nanoseconds(std::string_view &s) {
    if (s.empty()) {
        return std::nullopt;
    }
    std::string_view res_s = s;
    auto p = s.find('S');
    if (p == std::string::npos) {
        return std::nullopt;
    }
    res_s = s.substr(0, p);
    s = s.substr(p + 1);
    return parse_nanoseconds<datatype>(res_s);
}

inline char *canonical_seconds_remove_empty_millis(char *it) {
    for (size_t m = 0; m<9; ++m) {
        if (*(it - 1) != '0')
            return it;
        --it;
    }
    RDF4CPP_ASSERT(*(it - 1) == '.');
    --it;
    return it;
}

inline TimePoint add_duration_to_date_time(TimePoint const &tp, std::pair<std::chrono::months, std::chrono::nanoseconds> d) {
    // only gets smaller, no overflow possible
    auto [ymd, time] = rdf4cpp::util::deconstruct_timepoint(tp);

    boost::multiprecision::checked_int128_t checked_m = static_cast<unsigned int>(ymd.month());
    checked_m += boost::multiprecision::checked_int128_t{static_cast<int64_t>(ymd.year())} * 12;
    checked_m += d.first.count();

    auto const checked_y = (checked_m -1 ) / 12;

    checked_m = abs(checked_m - 1);

    auto const y = Year{static_cast<int64_t>(checked_y)};
    auto const m = std::chrono::month{static_cast<unsigned>(static_cast<int64_t>(checked_m % 12 + 1))};

    ymd = YearMonthDay{y, m, ymd.day()};
    if (!ymd.ok()) {
        ymd = YearMonthDay{ymd.year(), ymd.month(), std::chrono::last};
    }

    TimePoint date = ymd.to_time_point_local();
    date += time;
    date += d.second;

    rdf4cpp::util::deconstruct_timepoint(date); // check if it still fits into year, throws if not

    return date;
}

inline nonstd::expected<std::chrono::nanoseconds, DynamicError> timepoint_sub(std::pair<TimePoint, OptionalTimezone> const &lhs, std::pair<TimePoint, OptionalTimezone> const &rhs) noexcept {
    try {
        ZonedTime const this_tp{lhs.second.has_value() ? *lhs.second : Timezone{},
                                lhs.first};

        ZonedTime const other_tp{rhs.second.has_value() ? *rhs.second : Timezone{},
                                 rhs.first};

        auto d = this_tp.get_sys_time() - other_tp.get_sys_time();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(d);
    } catch (std::overflow_error const &) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
}

inline std::partial_ordering compare_time_points(const rdf4cpp::TimePoint& a, std::optional<rdf4cpp::Timezone> atz,
                                                        const rdf4cpp::TimePoint& b, std::optional<rdf4cpp::Timezone> btz) noexcept {
    auto apply_timezone = [](const rdf4cpp::TimePoint& t, rdf4cpp::Timezone tz) noexcept -> std::optional<rdf4cpp::TimePointSys> {
        try {
            return rdf4cpp::ZonedTime{tz, t}.get_sys_time();
        } catch (const std::overflow_error&) {
            return std::nullopt;
        }
    };

    auto const cmp = [](const auto& a, const auto& b) noexcept -> std::partial_ordering {
        if (a == b)
            return std::partial_ordering::equivalent;
        else if (a < b)
            return std::partial_ordering::less;
        else if (a > b)
            return std::partial_ordering::greater;
        return std::partial_ordering::unordered;
    };
    auto const cmp_opt = [cmp](const std::optional<rdf4cpp::TimePointSys>& a, const std::optional<rdf4cpp::TimePointSys>& b) noexcept -> std::partial_ordering {
        if (!a.has_value() || !b.has_value())
            return std::partial_ordering::unordered;
        return cmp(*a, *b);
    };

    if (!atz.has_value()) {
        atz = rdf4cpp::util::time_point_replacement_timezone;
    }
    if (!btz.has_value()) {
        btz = rdf4cpp::util::time_point_replacement_timezone;
    }

    auto a_sys = apply_timezone(a, *atz);
    return cmp_opt(a_sys, apply_timezone(b, *btz));
}

inline std::partial_ordering compare_time_points(std::pair<rdf4cpp::TimePoint, rdf4cpp::OptionalTimezone> const &lhs,
                                                 std::pair<rdf4cpp::TimePoint, rdf4cpp::OptionalTimezone> const &rhs) {
    return compare_time_points(lhs.first, lhs.second, rhs.first, rhs.second);
}

template<std::unsigned_integral T>
constexpr T number_of_bits(T x) noexcept {
    return x < 2 ? x : 1 + number_of_bits(x >> 1);
}
template<typename TimeType>
    requires(sizeof(TimeType) <= 2)
struct __attribute__((__packed__)) InliningHelper {
    uint16_t tz_offset;
    TimeType time_value;

    static constexpr int tz_shift = rdf4cpp::Timezone::max_value().offset.count() + 1;
    static_assert(number_of_bits(static_cast<unsigned int>(rdf4cpp::Timezone::max_value().offset.count() + tz_shift)) == 11);

    static constexpr uint16_t encode_tz(rdf4cpp::OptionalTimezone tz) noexcept {
        if (tz.has_value())
            return static_cast<uint16_t>(tz->offset.count() + tz_shift);
        else
            return 0;
    }

    constexpr InliningHelper(TimeType t, rdf4cpp::OptionalTimezone tz) noexcept : tz_offset(encode_tz(tz)), time_value(t) {
    }

    [[nodiscard]] constexpr rdf4cpp::OptionalTimezone decode_tz() const noexcept {
        if (tz_offset == 0)
            return std::nullopt;
        else
            return rdf4cpp::Timezone{std::chrono::minutes{static_cast<int>(tz_offset) - tz_shift}};
    }
};
struct __attribute__((__packed__)) InliningHelperPacked {
    static constexpr std::size_t width = storage::identifier::LiteralID::width;
    static constexpr std::size_t tv_width = width - 11;

    uint16_t tz_offset : 11;
    uint32_t time_value : tv_width;

private:
    [[maybe_unused]] uint32_t padding : 64 - width = 0;  // to make sure the rest of the int64 is 0

public:
    static constexpr int tz_shift = rdf4cpp::Timezone::max_value().offset.count() + 1;
    static_assert(number_of_bits(static_cast<unsigned int>(rdf4cpp::Timezone::max_value().offset.count() + tz_shift)) == 11);

    static constexpr uint16_t encode_tz(rdf4cpp::OptionalTimezone tz) noexcept {
        if (tz.has_value())
            return static_cast<uint16_t>(tz->offset.count() + tz_shift);
        else
            return 0;
    }

    constexpr InliningHelperPacked(uint32_t t, rdf4cpp::OptionalTimezone tz) noexcept : tz_offset(encode_tz(tz)), time_value(t) {
    }

    [[nodiscard]] rdf4cpp::OptionalTimezone decode_tz() const noexcept {
        if (tz_offset == 0)
            return std::nullopt;
        else
            return rdf4cpp::Timezone{std::chrono::minutes{static_cast<int>(tz_offset) - tz_shift}};
    }
};

inline YearMonthDay normalize(YearMonthDay const &i) {
    // normalize
    // see https://en.cppreference.com/w/cpp/chrono/year_month_day/operator_days
    return YearMonthDay{(i + std::chrono::months{0}).to_time_point()};
}

template<std::integral I, I base = 10>
consteval I number_of_digits(I num) {
    if (num < 0) {
        return 1 + number_of_digits(-num);
    } else if (num < base) {
        return 1;
    } else {
        return 1 + number_of_digits(num / base);
    }
}
static_assert(number_of_digits(0)==1);
static_assert(number_of_digits(9)==1);
static_assert(number_of_digits(-1)==2);
static_assert(number_of_digits(10)==2);
static_assert(number_of_digits(std::numeric_limits<uint64_t>::max())==std::numeric_limits<uint64_t>::digits10+1);
namespace chrono_max_canonical_string_chars {
    inline constexpr size_t year = std::numeric_limits<uint64_t>::digits10 + 2; // +1 for the not fully representable digit, +1 for - sign
    //std::chrono::day is in [0, 255]
    inline constexpr size_t day = number_of_digits(255);
    //std::chrono::month is in [0, 255]
    inline constexpr size_t month = number_of_digits(255);
    //[0, 59.999...] (includes nanoseconds)
    inline constexpr size_t seconds = 2 + 1 + 9;
    //[0,59]
    inline constexpr size_t minutes = 2;
    //[0,24] (used if more than 24 hours get added to days)
    inline constexpr size_t hours = 2;
    //used if no days are serialized
    inline constexpr size_t hours_unbound = number_of_digits(std::chrono::floor<std::chrono::hours>(std::chrono::milliseconds::max()).count());
    static_assert(sizeof(std::chrono::hours::rep) <= sizeof(int64_t));
    static_assert(sizeof(std::chrono::milliseconds ::rep) <= sizeof(int64_t));
    //duration
    static constexpr size_t years = number_of_digits(std::chrono::floor<std::chrono::years>(std::chrono::months::max()).count());
    static_assert(sizeof(std::chrono::years::rep) <= sizeof(int64_t));
    static_assert(sizeof(std::chrono::months::rep) <= sizeof(int64_t));
    //duration
    inline constexpr size_t months = 2;
    //duration
    inline constexpr size_t days = number_of_digits(std::chrono::floor<std::chrono::days>(std::chrono::milliseconds::max()).count());;
    static_assert(sizeof(std::chrono::days::rep) <= sizeof(int64_t));
};

}  // namespace rdf4cpp::datatypes::registry::util

#endif  //RDF4CPP_DATETIMEUTILS_HPP
