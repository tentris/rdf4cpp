#ifndef RDF4CPP_DATETIMEUTILS_HPP
#define RDF4CPP_DATETIMEUTILS_HPP

#include <chrono>
#include <format>
#include <string_view>

#include <dice/hash.hpp>
#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>
#include <rdf4cpp/util/CheckedInt.hpp>
#include <rdf4cpp/Assert.hpp>

/**
 * @file
 * various date/time utilities
 * @note this header is not intended to be included by end users. But if something in here is useful for some edge cases, feel free to do so anyway.
 */

namespace rdf4cpp::datatypes::registry::util {
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
        for (size_t m = 0; m < 9; ++m) {
            if (*(it - 1) != '0')
                return it;
            --it;
        }
        RDF4CPP_ASSERT(*(it - 1) == '.');
        --it;
        return it;
    }

    template<typename T>
    inline nonstd::expected<T, datatypes::DynamicError> optional_to_overflow(std::optional<T> const &o) {
        if (o.has_value()) {
            return *o;
        } else {
            return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
        }
    }

    inline nonstd::expected<TimePoint, datatypes::DynamicError> add_duration_to_date_time(TimePoint const &tp, std::pair<std::chrono::months, std::chrono::nanoseconds> d) noexcept {
        auto dec_tp = rdf4cpp::util::deconstruct_timepoint(tp);
        if (!dec_tp.has_value()) {
            return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
        }
        auto [ymd, time] = *dec_tp;

        auto ymd_c = ymd.add_checked(d.first);
        if (!ymd_c.has_value()) {
            return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
        }
        ymd = *ymd_c;
        if (!ymd.ok()) {
            ymd = YearMonthDay{ymd.year(), ymd.month(), std::chrono::last};
        }

        rdf4cpp::util::TimePoint_Checked date = ymd.to_time_point_local();
        date += time;
        date += d.second;

        auto date_uc = rdf4cpp::util::from_checked(date);
        if (!date_uc.has_value()) {
            return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
        }

        auto const deconstructed = rdf4cpp::util::deconstruct_timepoint(*date_uc);
        if (!deconstructed.has_value()) {
            return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
        }

        return *date_uc;
    }

    inline rdf4cpp::util::ZonedTime_Checked apply_tz_checked(TimePoint tp, OptionalTimezone tz) {
        return {tz.has_value() ? *tz : rdf4cpp::util::time_point_replacement_timezone, rdf4cpp::util::to_checked(tp)};
    }
    inline rdf4cpp::util::ZonedTime_Checked apply_tz_checked(std::pair<TimePoint, OptionalTimezone> const &tp) {
        return apply_tz_checked(tp.first, tp.second);
    }

    inline nonstd::expected<std::chrono::nanoseconds, DynamicError> timepoint_sub(std::pair<TimePoint, OptionalTimezone> const &lhs, std::pair<TimePoint, OptionalTimezone> const &rhs) noexcept {
        rdf4cpp::util::ZonedTime_Checked const this_tp = apply_tz_checked(lhs);
        rdf4cpp::util::ZonedTime_Checked const other_tp = apply_tz_checked(rhs);

        auto d = this_tp.get_sys_time() - other_tp.get_sys_time();
        return util::optional_to_overflow(rdf4cpp::util::from_checked(std::chrono::duration_cast<std::chrono::duration<rdf4cpp::util::CheckedIntegral<int64_t>, std::chrono::nanoseconds::period>>(d)));
    }

    inline static std::partial_ordering compare_time_points(rdf4cpp::TimePoint const &a, std::optional<rdf4cpp::Timezone> atz,
                                                            rdf4cpp::TimePoint const &b, std::optional<rdf4cpp::Timezone> btz) noexcept {
        auto const a_sys = apply_tz_checked(a, atz).get_sys_time();
        auto const b_sys = apply_tz_checked(b, btz).get_sys_time();
        if (a_sys.time_since_epoch().count().is_invalid() || b_sys.time_since_epoch().count().is_invalid()) {
            return std::partial_ordering::unordered;
        }
        return a_sys <=> b_sys;
    }
    template<typename T>
    requires std::same_as<T, nonstd::expected<TimePoint, datatypes::DynamicError>> || std::same_as<T, std::optional<TimePoint>>
    inline static std::partial_ordering compare_time_points(T const &a, std::optional<rdf4cpp::Timezone> atz,
                                                            T const &b, std::optional<rdf4cpp::Timezone> btz) noexcept {
        if (!a.has_value() || !b.has_value()) {
            return std::partial_ordering::unordered;
        }
        return compare_time_points(*a, atz, *b, btz);
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

    inline nonstd::expected<YearMonthDay, datatypes::DynamicError> normalize(YearMonthDay const &i) {
        // normalize
        // see https://en.cppreference.com/w/cpp/chrono/year_month_day/operator_days
        auto ym = i.add_checked(std::chrono::months{0});
        if (!ym.has_value()) {
            return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
        }
        return util::optional_to_overflow(YearMonthDay::from_time_point_checked(ym->to_time_point()));
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
    static_assert(number_of_digits(0) == 1);
    static_assert(number_of_digits(9) == 1);
    static_assert(number_of_digits(-1) == 2);
    static_assert(number_of_digits(10) == 2);
    static_assert(number_of_digits(std::numeric_limits<int64_t>::max()) == std::numeric_limits<int64_t>::digits10 + 1);
    namespace chrono_max_canonical_string_chars {
        inline constexpr size_t year = std::numeric_limits<int64_t>::digits10 + 2;  // +1 for the not fully representable digit, +1 for - sign
        //std::chrono::day is in [0, 255]
        inline constexpr size_t day = number_of_digits(255);
        //std::chrono::month is in [0, 255]
        inline constexpr size_t month = number_of_digits(255);
        // not including leading 0.
        inline constexpr size_t sub_seconds = number_of_digits(std::chrono::nanoseconds::period::den) - 1;
        static_assert(std::chrono::nanoseconds::period::num == 1);
        //[0, 59.999...] (includes nanoseconds)
        inline constexpr size_t seconds = number_of_digits(59) + 1 + sub_seconds;
        //[0,59]
        inline constexpr size_t minutes = number_of_digits(59);
        //[0,24]
        inline constexpr size_t hours = number_of_digits(24);
        //used if no days are serialized
        inline constexpr size_t hours_unbound = number_of_digits(std::chrono::floor<std::chrono::hours>(std::chrono::nanoseconds::max()).count());
        static_assert(sizeof(std::chrono::hours::rep) <= sizeof(int64_t));
        static_assert(sizeof(std::chrono::nanoseconds::rep) <= sizeof(int64_t));
        //duration
        static constexpr size_t years = number_of_digits(std::chrono::floor<std::chrono::years>(std::chrono::months::max()).count());
        static_assert(sizeof(std::chrono::years::rep) <= sizeof(int64_t));
        static_assert(sizeof(std::chrono::months::rep) <= sizeof(int64_t));
        //duration [1,12]
        inline constexpr size_t months = number_of_digits(12);
        //duration
        inline constexpr size_t days = number_of_digits(std::chrono::floor<std::chrono::days>(std::chrono::nanoseconds::max()).count());
        static_assert(sizeof(std::chrono::days::rep) <= sizeof(int64_t));
    };  // namespace chrono_max_canonical_string_chars

}  // namespace rdf4cpp::datatypes::registry::util

#endif  //RDF4CPP_DATETIMEUTILS_HPP
