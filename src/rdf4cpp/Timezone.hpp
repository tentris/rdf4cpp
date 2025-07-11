#ifndef RDF4CPP_TIMEZONE_HPP
#define RDF4CPP_TIMEZONE_HPP

#include <chrono>
#include <format>
#include <string_view>

#include <dice/hash.hpp>

#include <rdf4cpp/datatypes/rdf.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>
#include <rdf4cpp/util/CheckedInt.hpp>

namespace rdf4cpp {
    struct Timezone {
        // heavily inspired by https://howardhinnant.github.io/date/tz.html#Examples

        std::chrono::minutes offset = std::chrono::minutes{0};

        static constexpr char const *begin_tokens = "Z+-";

        constexpr Timezone() = default;

        inline explicit Timezone(std::chrono::time_zone const *tz, std::chrono::time_point<std::chrono::system_clock> n = std::chrono::system_clock::now())
            : offset(std::chrono::duration_cast<std::chrono::minutes>(tz->get_info(n).offset)) {
        }

        constexpr explicit Timezone(std::chrono::hours h) noexcept
            : offset(h) {
        }

        constexpr explicit Timezone(std::chrono::minutes h) noexcept
            : offset(h) {
        }

        constexpr auto operator<=>(Timezone const &) const noexcept = default;

        static constexpr Timezone parse(std::string_view v, std::string_view dt) {
            Timezone tz{};
            if (v == "Z") {
                return tz;
            }
            bool negative = false;
            if (v[0] == '-') {
                negative = true;
            }
            v = v.substr(1);
            auto sep = v.find(':');
            if (sep == std::string::npos) {
                throw InvalidNode{std::format("{} parsing error: timezone expected :", dt)};
            }
            std::chrono::hours const h{datatypes::registry::util::from_chars<int32_t, "timezone">(v.substr(0, sep))};
            tz.offset = std::chrono::minutes{datatypes::registry::util::from_chars<int32_t, "timezone">(v.substr(sep + 1))} + std::chrono::minutes{h};
            if (negative) {
                tz.offset *= -1;
            }
            if (tz.offset.count() < -840 || tz.offset.count() > 840) {
                throw InvalidNode{std::format("{} parsing error: timezone offset too big", dt)};
            }
            return tz;
        }

        static constexpr std::optional<Timezone> parse_optional(std::string_view &s, std::string_view dt) {
            auto p = s.find_first_of(begin_tokens, 1);
            if (p == 0 || p == std::string::npos)
                return std::nullopt;
            auto pre = s.substr(0, p);
            auto tz = parse(s.substr(p), dt);
            s = pre;
            return tz;
        }

        // sign, hours, :, minutes
        static constexpr size_t max_canonical_string_chars = 1 + (std::numeric_limits<int64_t>::digits10 + 1) + 1 + 2;
        template<std::output_iterator<char> T>
        T to_canonical_string(T o) const noexcept {
            if (offset == std::chrono::minutes{0}) {
                *o = 'Z';
                ++o;
                return o;
            }
            auto h = std::chrono::floor<std::chrono::hours>(std::chrono::abs(offset));
            auto m = std::chrono::abs(offset) - h;
            return std::format_to(o, "{}{:02}:{:02}", offset >= std::chrono::minutes{0} ? '+' : '-', h.count(), m.count());
        }
        [[nodiscard]] std::string to_canonical_string() const noexcept {
            std::string buf{};
            buf.reserve(max_canonical_string_chars);
            to_canonical_string(std::back_inserter(buf));
            return buf;
        }

        [[nodiscard]] std::chrono::time_zone const *get_tz(std::chrono::time_point<std::chrono::system_clock> n = std::chrono::system_clock::now()) const {
            for (auto const &tz : std::chrono::get_tzdb().zones) {
                if (tz.get_info(n).offset == std::chrono::seconds(offset)) {
                    return &tz;
                }
            }
            return nullptr;
        }

        template<typename Duration>
        [[nodiscard]] auto to_sys(std::chrono::local_time<Duration> const &tp) const noexcept {
            return std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>{(tp - offset).time_since_epoch()};
        }

        template<typename Duration>
        [[nodiscard]] auto to_local(std::chrono::sys_time<Duration> const &tp) const noexcept {
            return std::chrono::local_time<std::common_type_t<Duration, std::chrono::seconds>>{(tp + offset).time_since_epoch()};
        }

        template<typename Duration>
        [[nodiscard]] std::chrono::sys_info get_info(std::chrono::sys_time<Duration> const &) const noexcept {
            return std::chrono::sys_info{
                    std::chrono::sys_seconds{std::chrono::seconds{0L}},
                    std::chrono::sys_seconds{std::chrono::seconds{std::numeric_limits<int64_t>::max()}},
                    offset,
                    std::chrono::minutes{0},
                    to_canonical_string()};
        }

        Timezone const *operator->() const noexcept {
            return this;
        }

        static constexpr Timezone max_value() noexcept {
            return Timezone{std::chrono::hours{14}};
        };
        static constexpr Timezone min_value() noexcept {
            return Timezone{std::chrono::hours{-14}};
        };
    };

    namespace util {
        /**
     * turns any duration to its CheckedIntegral counterpart.
     * @tparam R
     * @param v
     * @return
     */
        template<typename R, typename I>
        constexpr std::chrono::duration<rdf4cpp::util::CheckedIntegral<I>, R> to_checked(std::chrono::duration<I, R> v) noexcept {
            return std::chrono::duration<rdf4cpp::util::CheckedIntegral<I>, R>{v.count()};
        }
        /**
     * turns any CheckedIntegral duration back to its integer based duration.
     * @note undefined behavior, if v is invalid
     * @tparam R
     * @param v
     * @return
     */
        template<typename R, typename I>
        constexpr std::optional<std::chrono::duration<I, R>> from_checked(std::chrono::duration<rdf4cpp::util::CheckedIntegral<I>, R> v) noexcept {
            if (v.count().is_invalid()) {
                return std::nullopt;
            }
            return std::chrono::duration<I, R>{v.count().get_value()};
        }
        /**
     * turns any time_point to its CheckedIntegral counterpart.
     * @tparam C
     * @tparam R
     * @param v
     * @return
     */
        template<typename C, typename R, typename I>
        constexpr std::chrono::time_point<C, std::chrono::duration<rdf4cpp::util::CheckedIntegral<I>, R>> to_checked(std::chrono::time_point<C, std::chrono::duration<I, R>> v) noexcept {
            return std::chrono::time_point<C, std::chrono::duration<rdf4cpp::util::CheckedIntegral<I>, R>>{to_checked(v.time_since_epoch())};
        }
        /**
     * turns any CheckedIntegral time_point back to its integer based time_point.
     * @note undefined behavior, if v is invalid
     * @tparam C
     * @tparam R
     * @param v
     * @return
     */
        template<typename C, typename R, typename I>
        constexpr std::optional<std::chrono::time_point<C, std::chrono::duration<I, R>>> from_checked(std::chrono::time_point<C, std::chrono::duration<rdf4cpp::util::CheckedIntegral<I>, R>> v) noexcept {
            if (v.time_since_epoch().count().is_invalid()) {
                return std::nullopt;
            }
            return std::chrono::time_point<C, std::chrono::duration<I, R>>{*from_checked(v.time_since_epoch())};
        }
    }  // namespace util


    using OptionalTimezone = std::optional<Timezone>;

    using Month = std::chrono::month;
    using Day = std::chrono::day;

    /**
 * Like std::chrono::year, except it has a greater range.
 * adapted from https://howardhinnant.github.io/date_algorithms.html
 */
    struct Year {
    private:
        int64_t value_;

    public:
        explicit constexpr Year(int64_t y = 0) noexcept : value_{y} {
        }

        constexpr explicit operator int64_t() const noexcept {
            return value_;
        }

        [[nodiscard]] constexpr bool is_leap() const noexcept(noexcept(value_ % 100)) {
            return value_ % 4 == 0 && (value_ % 100 != 0 || value_ % 400 == 0);
        }

        constexpr auto operator<=>(Year const &) const noexcept = default;

        friend constexpr Year operator+(Year const &y, std::chrono::years d) noexcept {
            return Year{y.value_ + d.count()};
        }
        friend constexpr Year operator+(std::chrono::years d, Year const &y) noexcept {
            return Year{y.value_ + d.count()};
        }
        [[nodiscard]] constexpr std::optional<Year> add_checked(std::chrono::years d) const noexcept {
            int64_t r;
            if (util::detail::add_checked<util::detail::OverflowMode::Checked>(value_, d.count(), r)) {
                return std::nullopt;
            }
            return Year{r};
        }

        constexpr Year operator+=(std::chrono::years d) noexcept {
            *this = *this + d;
            return *this;
        }

        friend constexpr Year operator-(Year const &y, std::chrono::years d) noexcept {
            return Year{y.value_ - d.count()};
        }
        friend constexpr std::chrono::years operator-(Year const &a, Year const &b) noexcept {
            return std::chrono::years{a.value_ - b.value_};
        }
        [[nodiscard]] constexpr std::optional<Year> sub_checked(std::chrono::years d) const noexcept {
            int64_t r;
            if (util::detail::sub_checked<util::detail::OverflowMode::Checked>(value_, d.count(), r)) {
                return std::nullopt;
            }
            return Year{r};
        }
        [[nodiscard]] constexpr std::optional<std::chrono::years> sub_checked(Year o) const noexcept {
            int64_t r;
            if (util::detail::sub_checked<util::detail::OverflowMode::Checked>(this->value_, o.value_, r)) {
                return std::nullopt;
            }
            return std::chrono::years{r};
        }

        constexpr Year operator-=(std::chrono::years d) noexcept {
            *this = *this - d;
            return *this;
        }

        constexpr Year &operator++() noexcept {
            *this += std::chrono::years{1};
            return *this;
        }
        constexpr Year operator++(int) noexcept {
            Year r = *this;
            ++(*this);
            return r;
        }

        constexpr Year &operator--() noexcept {
            *this -= std::chrono::years{1};
            return *this;
        }
        constexpr Year operator--(int) noexcept {
            Year r = *this;
            --(*this);
            return r;
        }

        static constexpr Year max() noexcept {
            return Year{std::numeric_limits<int64_t>::max()};
        }
        static constexpr Year min() noexcept {
            return Year{std::numeric_limits<int64_t>::min()};
        }
    };

    struct YearMonth {
    private:
        Year year_ = Year{0};
        Month month_ = Month{1};

        // returns nullopt iff overflow
        static constexpr std::optional<YearMonth> create_normalized(util::CheckedIntegral<int64_t> y, util::CheckedIntegral<int64_t> mo) noexcept {
            mo -= 1;
            y += mo / 12;
            mo %= 12;
            if (mo < 0) {  // fix result of % being in [-11,11]
                y -= 1;
                mo += 12;
            }
            mo += 1;
            auto um = mo.checked_cast<unsigned int>();
            if (y.is_invalid() || um.is_invalid()) {
                return std::nullopt;
            }
            return YearMonth{Year{y.get_value()}, std::chrono::month{um.get_value()}};
        }

    public:
        constexpr YearMonth() noexcept = default;

        constexpr YearMonth(Year y, std::chrono::month m) noexcept : year_{y}, month_{m} {
        }

        [[nodiscard]] constexpr Year year() const noexcept {
            return year_;
        }

        [[nodiscard]] constexpr Month month() const noexcept {
            return month_;
        }

        constexpr auto operator<=>(YearMonth const &) const noexcept = default;

        [[nodiscard]] constexpr bool ok() const noexcept {
            return month_.ok();
        }

        friend constexpr YearMonth operator+(YearMonth const &ym, std::chrono::years d) noexcept {
            return YearMonth{ym.year_ + d, ym.month_};
        }
        friend constexpr YearMonth operator+(std::chrono::years d, YearMonth const &ym) noexcept {
            return YearMonth{ym.year_ + d, ym.month_};
        }
        [[nodiscard]] constexpr std::optional<YearMonth> add_checked(std::chrono::years s) const noexcept {
            auto y = year_.add_checked(s);
            if (!y.has_value()) {
                return std::nullopt;
            }
            return YearMonth{*y, month_};
        }

        constexpr YearMonth &operator+=(std::chrono::years d) noexcept {
            *this = *this + d;
            return *this;
        }
        
        friend constexpr YearMonth operator+(YearMonth const &ym, std::chrono::months d) {
            return create_normalized(static_cast<int64_t>(ym.year_), static_cast<unsigned int>(ym.month_) + d.count()).value();
        }
        friend constexpr YearMonth operator+(std::chrono::months d, YearMonth const &ym) {
            return create_normalized(static_cast<int64_t>(ym.year_), static_cast<unsigned int>(ym.month_) + d.count()).value();
        }
        [[nodiscard]] constexpr std::optional<YearMonth> add_checked(std::chrono::months d) const noexcept {
            return create_normalized(static_cast<int64_t>(year_), util::CheckedIntegral<int64_t>(static_cast<unsigned int>(month_)) + d.count());
        }

        constexpr YearMonth &operator+=(std::chrono::months d) noexcept {
            *this = *this + d;
            return *this;
        }

        friend constexpr YearMonth operator-(YearMonth const &ym, std::chrono::years d) noexcept {
            return {ym.year_ - d, ym.month_};
        }
        [[nodiscard]] constexpr std::optional<YearMonth> sub_checked(std::chrono::years s) const noexcept {
            auto y = year_.sub_checked(s);
            if (!y.has_value()) {
                return std::nullopt;
            }
            return YearMonth{*y, month_};
        }
        friend constexpr YearMonth operator-(YearMonth const &ym, std::chrono::months d) {
            return create_normalized(static_cast<int64_t>(ym.year_), static_cast<unsigned int>(ym.month_) - d.count()).value();
        }
        [[nodiscard]] constexpr std::optional<YearMonth> sub_checked(std::chrono::months d) const noexcept {
            return create_normalized(static_cast<int64_t>(year_), util::CheckedIntegral<int64_t>(static_cast<unsigned int>(month_)) - d.count());
        }

        friend constexpr std::chrono::months operator-(YearMonth const &a, YearMonth const &b) noexcept {
            return (a.year_ - b.year_) + (a.month_ - b.month_);
        }
        [[nodiscard]] constexpr std::optional<std::chrono::months> sub_checked(YearMonth const &o) const noexcept {
            auto y = this->year_.sub_checked(o.year_);
            if (!y.has_value()) {
                return std::nullopt;
            }
            int64_t m;
            if (util::detail::sub_checked<util::detail::OverflowMode::Checked, int64_t>(static_cast<unsigned int>(this->month_), static_cast<unsigned int>(o.month_), m)) {
                return std::nullopt;
            }
            auto r = util::to_checked(*y) + std::chrono::months(m);
            return util::from_checked(r);
        }

        constexpr YearMonth &operator-=(std::chrono::years d) noexcept {
            *this = *this - d;
            return *this;
        }
        constexpr YearMonth &operator-=(std::chrono::months d) noexcept {
            *this = *this - d;
            return *this;
        }
    };

    struct YearMonthDay {
        template<typename P>
        using time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<P, std::chrono::days::period>>;
        template<typename P>
        using time_point_local = std::chrono::time_point<std::chrono::local_t, std::chrono::duration<P, std::chrono::days::period>>;

    private:
        // adapted from https://howardhinnant.github.io/date_algorithms.html
        Year year_ = Year{0};
        Month month_ = Month{1};
        Day day_ = Day{1};

        static constexpr std::chrono::day last_day_in_month(Year year, Month month) noexcept {
            assert(month.ok());
            constexpr unsigned char common[]{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            auto m = static_cast<unsigned int>(month);
            return std::chrono::day{m != 2 || !year.is_leap() ? common[m - 1] : 29u};
        }

    public:
        constexpr YearMonthDay() noexcept = default;

        constexpr explicit YearMonthDay(std::chrono::year_month_day ymd) noexcept
            : year_(static_cast<int>(ymd.year())), month_(ymd.month()), day_(ymd.day()) {
        }
        constexpr YearMonthDay(Year const &y, Month m, std::chrono::day d) noexcept
            : year_(y), month_(m), day_(d) {
        }
        constexpr YearMonthDay(Year const &y, Month m, std::chrono::last_spec) noexcept
            : year_(y), month_(m), day_(last_day_in_month(y, m)) {
        }
        constexpr YearMonthDay(YearMonth const &ym, Day d) noexcept
            : year_(ym.year()), month_(ym.month()), day_(d) {
        }
        constexpr YearMonthDay(YearMonth const &ym, std::chrono::last_spec) noexcept
            : YearMonthDay(ym.year(), ym.month(), std::chrono::last) {
        }
        template<typename P>
        static constexpr std::optional<YearMonthDay> from_time_point_checked(time_point<P> sd) noexcept {
            static_assert(std::numeric_limits<unsigned>::digits >= 18, "This algorithm has not been ported to a 16 bit unsigned integer");
            static_assert(std::numeric_limits<int64_t>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
            static_assert(std::numeric_limits<P>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
            rdf4cpp::util::CheckedIntegral<P> z = sd.time_since_epoch().count();
            z += 719468;
            rdf4cpp::util::CheckedIntegral<P> const era = (z >= 0 ? z : z - 146096) / 146097;
            rdf4cpp::util::CheckedIntegral<P> const doe = z - era * 146097;                                       // [0, 146096]
            rdf4cpp::util::CheckedIntegral<P> const yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
            rdf4cpp::util::CheckedIntegral<P> const y = yoe + era * 400;
            rdf4cpp::util::CheckedIntegral<P> const doy = doe - (yoe * 365 + yoe / 4 - yoe / 100);  // [0, 365]
            rdf4cpp::util::CheckedIntegral<P> const mp = (doy * 5 + 2) / 153;                       // [0, 11]
            rdf4cpp::util::CheckedIntegral<P> const d = doy - (mp * 153 + 2) / 5 + 1;               // [1, 31]
            rdf4cpp::util::CheckedIntegral<P> const m = mp < 10 ? mp + 3 : mp - 9;                  // [1, 12]
            auto const y_out = (y + (m <= 2)).template checked_cast<int64_t>();
            auto const m_out = m.template checked_cast<unsigned>();
            auto const d_out = d.template checked_cast<unsigned>();
            if (y_out.is_invalid() || m_out.is_invalid() || d_out.is_invalid()) {
                return std::nullopt;
            }
            return YearMonthDay{Year{y_out.get_value()}, std::chrono::month{m_out.get_value()}, std::chrono::day{d_out.get_value()}};
        }
        template<typename P>
        static constexpr std::optional<YearMonthDay> from_local_time_point_checked(time_point_local<P> sd) noexcept {
            return from_time_point_checked(time_point<P>(sd.time_since_epoch()));
        }
        template<typename P>
        constexpr explicit YearMonthDay(time_point<P> sd) : YearMonthDay(from_time_point_checked(sd).value()) {
        }
        template<typename P>
        constexpr explicit YearMonthDay(time_point_local<P> sd) : YearMonthDay(from_local_time_point_checked(sd).value()) {
        }

        [[nodiscard]] constexpr Year year() const noexcept {
            return year_;
        }

        [[nodiscard]] constexpr Month month() const noexcept {
            return month_;
        }

        [[nodiscard]] constexpr Day day() const noexcept {
            return day_;
        }

        // with year as int64 and timepoint as int128, guaranteed to not overflow
        [[nodiscard]] constexpr time_point<Int128> to_time_point() const {
            static_assert(std::numeric_limits<unsigned>::digits >= 18, "This algorithm has not been ported to a 16 bit unsigned integer");
            static_assert(std::numeric_limits<int64_t>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
            static_assert(std::numeric_limits<Int128>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
            Int128 y = static_cast<int64_t>(year_);
            auto m = static_cast<unsigned int>(month_);
            auto d = static_cast<unsigned int>(day_);
            y -= m <= 2;
            Int128 const era = (y >= 0 ? y : y - 399) / 400;
            auto const yoe = y - era * 400;                                    // [0, 399]
            auto const doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;  // [0, 365]
            auto const doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;            // [0, 146096]
            // note that the epoch of system_clock is specified as 00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970
            time_point<Int128> const r{typename time_point<Int128>::duration{era * 146097 + doe - 719468}};
            return r;
        }
        [[nodiscard]] constexpr time_point_local<Int128> to_time_point_local() const {
            auto const v = to_time_point();
            return time_point_local<Int128>{v.time_since_epoch()};
        }

        [[nodiscard]] constexpr bool ok() const noexcept {
            return month_.ok() && day_.ok() && day_ <= last_day_in_month(year_, month_);
        }

        constexpr auto operator<=>(YearMonthDay const &) const noexcept = default;

        friend constexpr YearMonthDay operator+(YearMonthDay const &ym, std::chrono::years d) noexcept {
            return {ym.year_ + d, ym.month_, ym.day_};
        }
        friend constexpr YearMonthDay operator+(std::chrono::years d, YearMonthDay const &ym) noexcept {
            return {ym.year_ + d, ym.month_, ym.day_};
        }
        [[nodiscard]] constexpr std::optional<YearMonthDay> add_checked(std::chrono::years d) const noexcept {
            auto y = year_.add_checked(d);
            if (!y.has_value()) {
                return std::nullopt;
            }
            return YearMonthDay{*y, month_, day_};
        }

        constexpr YearMonthDay &operator+=(std::chrono::years d) noexcept {
            *this = *this + d;
            return *this;
        }

        friend constexpr YearMonthDay operator+(YearMonthDay const &d, std::chrono::months m) {
            return YearMonthDay{YearMonth{d.year_, d.month_} + m, d.day_};
        }
        friend constexpr YearMonthDay operator+(std::chrono::months m, YearMonthDay const &d) {
            return YearMonthDay{YearMonth{d.year_, d.month_} + m, d.day_};
        }
        [[nodiscard]] constexpr std::optional<YearMonthDay> add_checked(std::chrono::months d) const noexcept {
            auto ym = YearMonth{year_, month_}.add_checked(d);
            if (!ym.has_value()) {
                return std::nullopt;
            }
            return YearMonthDay{*ym, day_};
        }

        constexpr YearMonthDay &operator+=(std::chrono::months d) noexcept {
            *this = *this + d;
            return *this;
        }

        friend constexpr YearMonthDay operator-(YearMonthDay const &ym, std::chrono::years d) noexcept {
            return {ym.year_ - d, ym.month_, ym.day_};
        }
        friend constexpr YearMonthDay operator-(YearMonthDay const &d, std::chrono::months m) {
            return YearMonthDay{YearMonth{d.year_, d.month_} - m, d.day_};
        }
        [[nodiscard]] constexpr std::optional<YearMonthDay> sub_checked(std::chrono::years d) const noexcept {
            auto y = year_.sub_checked(d);
            if (!y.has_value()) {
                return std::nullopt;
            }
            return YearMonthDay{*y, month_, day_};
        }
        [[nodiscard]] constexpr std::optional<YearMonthDay> sub_checked(std::chrono::months d) const noexcept {
            auto ym = YearMonth{year_, month_}.sub_checked(d);
            if (!ym.has_value()) {
                return std::nullopt;
            }
            return YearMonthDay{*ym, day_};
        }

        constexpr YearMonthDay &operator-=(std::chrono::years d) noexcept {
            *this = *this - d;
            return *this;
        }
        constexpr YearMonthDay &operator-=(std::chrono::months d) noexcept {
            *this = *this - d;
            return *this;
        }
    };

    using DurationNano = std::chrono::duration<Int128, std::chrono::nanoseconds::period>;
    using TimePoint = std::chrono::time_point<std::chrono::local_t, DurationNano>;
    // system_clock does not use leap seconds, as required by rdf (xsd)
    using TimePointSys = std::chrono::time_point<std::chrono::system_clock, DurationNano>;
    using ZonedTime = std::chrono::zoned_time<DurationNano, Timezone>;

    namespace util {
        using DurationNano_Checked = std::chrono::duration<util::CheckedIntegral<Int128>, std::chrono::nanoseconds::period>;
        using TimePoint_Checked = std::chrono::time_point<std::chrono::local_t, DurationNano_Checked>;
        using TimePointSys_Checked = std::chrono::time_point<std::chrono::system_clock, DurationNano_Checked>;
        using ZonedTime_Checked = std::chrono::zoned_time<DurationNano_Checked, Timezone>;

        // see https://www.w3.org/TR/xpath-functions/#comp.datetime
        inline constexpr YearMonthDay time_point_replacement_date{Year(1972), std::chrono::January, std::chrono::day{1}};
        inline constexpr DurationNano time_point_replacement_time_of_day{0};
        // implementation defined, not from standard
        inline constexpr Timezone time_point_replacement_timezone{std::chrono::minutes{0}};

        // with year as int64 and timepoint as int128, guaranteed to not overflow
        constexpr TimePoint construct_timepoint(YearMonthDay const &date, DurationNano const &time_of_day) noexcept {
            auto sd = date.to_time_point_local();
            auto ms = static_cast<TimePoint>(sd);
            ms += time_of_day;
            return ms;
        }

        // TODO check return values
        constexpr std::optional<std::pair<YearMonthDay, DurationNano>> deconstruct_timepoint(TimePoint const &tp) noexcept {
            auto const days = std::chrono::floor<std::chrono::duration<DurationNano_Checked::rep, std::chrono::days::period>>(to_checked(tp));
            if (days.time_since_epoch().count().is_invalid()) {
                return std::nullopt;
            }
            auto const h = tp - days;
            if (h.count().is_invalid()) {
                return std::nullopt;
            }
            auto ymd = YearMonthDay::from_local_time_point_checked(*from_checked(days));
            if (!ymd.has_value()) {
                return std::nullopt;
            }
            return std::pair<YearMonthDay, DurationNano>{*ymd, *from_checked(h)};
        }

    }  // namespace util

}  // namespace rdf4cpp

namespace std::chrono {
    template<>
    struct zoned_traits<::rdf4cpp::Timezone> {
        static ::rdf4cpp::Timezone default_zone() noexcept {
            return ::rdf4cpp::Timezone{};
        }
    };
}  // namespace std::chrono

#ifndef DOXYGEN_PARSER
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::Timezone> {
    static size_t dice_hash(rdf4cpp::Timezone const &x) noexcept {
        auto off = x.offset.count();
        return dice::hash::dice_hash_templates<Policy>::dice_hash(off);
    }
};
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::OptionalTimezone> {
    static size_t dice_hash(rdf4cpp::OptionalTimezone const &x) noexcept {
        auto off = x.has_value() ? x->offset.count() : std::chrono::minutes{std::chrono::hours{15}}.count();
        return dice::hash::dice_hash_templates<Policy>::dice_hash(off);
    }
};
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::Year> {
    static size_t dice_hash(rdf4cpp::Year const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(static_cast<int64_t>(x));
    }
};
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::YearMonthDay> {
    static size_t dice_hash(rdf4cpp::YearMonthDay const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::make_tuple(x.year(), x.month(), x.day()));
    }
};
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::YearMonth> {
    static size_t dice_hash(rdf4cpp::YearMonth const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::make_tuple(x.year(), x.month()));
    }
};

template<>
struct std::formatter<rdf4cpp::Year> : std::formatter<std::string_view> {
    inline auto format(rdf4cpp::Year const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{:0{}}", static_cast<int64_t>(p), static_cast<int64_t>(p) < 0 ? 5 : 4);
    }
};
template<>
struct std::formatter<rdf4cpp::YearMonthDay> : std::formatter<std::string_view> {
    inline auto format(rdf4cpp::YearMonthDay const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{}-{:%m}-{:%d}", p.year(), p.month(), p.day());
    }
};
template<>
struct std::formatter<rdf4cpp::YearMonth> : std::formatter<std::string_view> {
    inline auto format(rdf4cpp::YearMonth const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{}-{:%m}", p.year(), p.month());
    }
};
template<>
struct std::formatter<rdf4cpp::TimePoint> : std::formatter<std::string_view> {
    inline auto format(rdf4cpp::TimePoint const &p, format_context &ctx) const {
        auto const dc = rdf4cpp::util::deconstruct_timepoint(p);
        if (!dc.has_value()) {
            return std::format_to(ctx.out(), "{}", "invalid timepoint");
        }
        auto [date, time] = *dc;
        return std::format_to(ctx.out(), "{}T{:%H:%M:%S}", date, std::chrono::hh_mm_ss{std::chrono::duration_cast<std::chrono::nanoseconds>(time)});
    }
};
template<>
struct std::formatter<rdf4cpp::ZonedTime> : std::formatter<std::string_view> {
    inline auto format(rdf4cpp::ZonedTime const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{}{}", p.get_local_time(), p.get_time_zone().to_canonical_string());
    }
};

namespace rdf4cpp {
    inline std::ostream &operator<<(std::ostream &os, rdf4cpp::Year const &value) {
        std::format_to(std::ostream_iterator<char, char>{os}, "{}", value);
        return os;
    }
    inline std::ostream &operator<<(std::ostream &os, rdf4cpp::YearMonthDay const &value) {
        std::format_to(std::ostream_iterator<char, char>{os}, "{}", value);
        return os;
    }
    inline std::ostream &operator<<(std::ostream &os, rdf4cpp::YearMonth const &value) {
        std::format_to(std::ostream_iterator<char, char>{os}, "{}", value);
        return os;
    }
}  // namespace rdf4cpp
#endif

#endif  //RDF4CPP_TIMEZONE_HPP
