#include "Duration.hpp"

#include <ranges>
#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_duration>::cpp_type capabilities::Default<xsd_duration>::from_string(std::string_view s) {
    using namespace registry::util;
    bool negative = false;
    if (!s.empty() && s[0] == '-') {
        negative = true;
        s = s.substr(1);
    }
    if (s.empty() || s[0] != 'P') {
        throw InvalidNode{std::format("{} parsing error: duration missing P", identifier)};
    }
    s = s.substr(1);
    auto p = s.find('T');
    auto date = s.substr(0, p);
    auto time = p == std::string::npos ? std::string_view{""} : s.substr(p);
    auto years = parse_duration_fragment<std::chrono::years, uint64_t, 'Y', identifier>(date);
    auto months = parse_duration_fragment<std::chrono::months, uint64_t, 'M', identifier>(date);
    auto days = parse_duration_fragment<std::chrono::days, uint64_t, 'D', identifier>(date);
    if (!time.empty()) {
        if (time[0] != 'T') {
            throw InvalidNode{std::format("{} parsing error: duration missing T", identifier)};
        }
        time = time.substr(1);
    }
    auto hours = parse_duration_fragment<std::chrono::hours, uint64_t, 'H', identifier>(time);
    auto minutes = parse_duration_fragment<std::chrono::minutes, uint64_t, 'M', identifier>(time);
    auto seconds = parse_duration_nanoseconds<identifier>(time);

    std::chrono::months m{};
    if (years.has_value()) {
        m += *years;
    }
    if (months.has_value()) {
        m += *months;
    }
    std::chrono::nanoseconds ms{};
    if (days.has_value()) {
        ms += *days;
    }
    if (!date.empty()) {
        throw InvalidNode{std::format("{} parsing error: found {}, expected empty", identifier, date)};
    }
    if (!time.empty()) {
        throw InvalidNode{std::format("{} parsing error: found {}, expected empty", identifier, time)};
    }
    if (!years.has_value() && !months.has_value() && !days.has_value() && !hours.has_value() && !minutes.has_value() && !seconds.has_value()) {
        throw InvalidNode{std::format("{} parsing error: duration without any fields", identifier)};
    }
    if (hours.has_value()) {
        ms += *hours;
    }
    if (minutes.has_value()) {
        ms += *minutes;
    }
    if (seconds.has_value()) {
        ms += *seconds;
    }
    if (negative) {
        m = -m;
        ms = -ms;
    }
    return std::make_pair(m, ms);
}

template<>
bool capabilities::Default<xsd_duration>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    if (value.first.count() == 0 && value.second.count() == 0) {
        return writer::write_str("PT0S", writer);
    }
    //-,P,years,months, days,T,hours,minutes,seconds
    std::array<char, 1 + 1 + registry::util::chrono_max_canonical_string_chars::years + 1 + registry::util::chrono_max_canonical_string_chars::months + 1 +
                             registry::util::chrono_max_canonical_string_chars::days + 1 + 1 +
                             registry::util::chrono_max_canonical_string_chars::hours + 1 +
                             registry::util::chrono_max_canonical_string_chars::minutes + 1 +
                             registry::util::chrono_max_canonical_string_chars::seconds + 1>
            buff;
    char* it = buff.data();
    std::chrono::months m_rem = value.first;
    std::chrono::nanoseconds ms_rem = value.second;
    if (m_rem.count() < 0) {
        *(it++) = '-';
        m_rem = -m_rem;
        ms_rem = -ms_rem;
    }
    *(it++) = 'P';
    auto years = std::chrono::floor<std::chrono::years>(m_rem);
    if (years.count() != 0) {
        it = std::format_to(it, "{}", years.count());
        *(it++) = 'Y';
    }
    m_rem -= years;
    if (m_rem.count() != 0) {
        it = std::format_to(it, "{}", m_rem.count());
        *(it++) = 'M';
    }
    auto days = std::chrono::floor<std::chrono::days>(ms_rem);
    if (days.count() != 0) {
        it = std::format_to(it, "{}", days.count());
        *(it++) = 'D';
    }
    ms_rem -= days;
    if (ms_rem.count() != 0) {
        *(it++) = 'T';
        auto hours = std::chrono::floor<std::chrono::hours>(ms_rem);
        if (hours.count() != 0) {
            it = std::format_to(it, "{}", hours.count());
            *(it++) = 'H';
        }
        ms_rem -= hours;
        auto minutes = std::chrono::floor<std::chrono::minutes>(ms_rem);
        if (minutes.count() != 0) {
            it = std::format_to(it, "{}", minutes.count());
            *(it++) = 'M';
        }
        ms_rem -= minutes;
        if (ms_rem.count() != 0) {
            it = std::format_to(it, "{:%S}", ms_rem);
            it = util::canonical_seconds_remove_empty_millis(it);
            *(it++) = 'S';
        }
    }
    size_t const len = it - buff.data();
    assert(len <= buff.size());
    return writer::write_str(std::string_view(buff.data(), len), writer);
}

struct __attribute__((__packed__)) InlinedDurationHelper {
    static constexpr std::size_t width = storage::identifier::LiteralID::width;
    static constexpr int seconds_width = 25;
    static constexpr int months_with = width - seconds_width - 1;

    static constexpr int64_t seconds_mask = ~int64_t{0} << seconds_width;
    static constexpr int64_t months_mask = ~int64_t{0} << months_with;

    unsigned int sign : 1;
    unsigned int seconds : seconds_width;
    unsigned int months : months_with;

    InlinedDurationHelper(unsigned int s, unsigned int sec, unsigned int mo) noexcept : sign(s), seconds(sec), months(mo) {
    }

private:
    [[maybe_unused]] unsigned int padding : 64 - width = 0;  // to make sure the rest of the int64 is 0
};

static_assert(registry::util::number_of_bits<unsigned int>((std::chrono::years{1} + std::chrono::seconds{0}).count()) == 25);

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_duration>::try_into_inlined(cpp_type const &value) noexcept {
    auto ms = value.second;
    auto months = value.first;
    unsigned int sign = 0;
    if (months < std::chrono::months{0}) {
        sign = 1;
        ms = -ms;
        months = -months;
    }
    auto sec = std::chrono::floor<std::chrono::seconds>(ms);
    if ((ms - sec) != std::chrono::milliseconds{0})
        return std::nullopt;
    if (sec.count() & InlinedDurationHelper::seconds_mask)
        return std::nullopt;
    if (months.count() & InlinedDurationHelper::months_mask)
        return std::nullopt;
    return util::pack<storage::identifier::LiteralID>(InlinedDurationHelper{
            sign, static_cast<unsigned int>(sec.count()), static_cast<unsigned int>(months.count())});
}

template<>
capabilities::Inlineable<xsd_duration>::cpp_type capabilities::Inlineable<xsd_duration>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto i = util::unpack<InlinedDurationHelper>(inlined);
    std::chrono::seconds sec{i.seconds};
    std::chrono::months months{i.months};
    if (i.sign == 1) {
        sec = -sec;
        months = -months;
    }
    return std::make_pair(months, std::chrono::milliseconds{sec});
}

template<>
std::partial_ordering capabilities::Comparable<xsd_duration>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    static constexpr std::array<rdf4cpp::TimePoint, 4> to_compare{
            *rdf4cpp::util::construct_timepoint(YearMonthDay{Year{1696}, std::chrono::month{9}, std::chrono::day{1}}, std::chrono::milliseconds{0}),
            *rdf4cpp::util::construct_timepoint(YearMonthDay{Year{1697}, std::chrono::month{2}, std::chrono::day{1}}, std::chrono::milliseconds{0}),
            *rdf4cpp::util::construct_timepoint(YearMonthDay{Year{1903}, std::chrono::month{3}, std::chrono::day{1}}, std::chrono::milliseconds{0}),
            *rdf4cpp::util::construct_timepoint(YearMonthDay{Year{1903}, std::chrono::month{7}, std::chrono::day{1}}, std::chrono::milliseconds{0}),
    };
    auto cmp = [lhs, rhs](const rdf4cpp::TimePoint& tp) noexcept {
        try {
            auto l = registry::util::add_duration_to_date_time(tp, lhs);
            auto r = registry::util::add_duration_to_date_time(tp, rhs);
            return registry::util::compare_time_points(l, std::nullopt, r, std::nullopt);
        } catch (...) {
            return std::partial_ordering::unordered;
        }
    };
    std::partial_ordering o = cmp(to_compare[0]);
    for (unsigned int i = 1; i < to_compare.size(); ++i) {
        if (o != cmp(to_compare[i]))
            return std::partial_ordering::unordered;
    }
    return o;
}

template<>
nonstd::expected<capabilities::Duration<xsd_duration>::cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_add(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    auto mon = rdf4cpp::util::to_checked(lhs.first) + rdf4cpp::util::to_checked(rhs.first);
    auto nanos = rdf4cpp::util::to_checked(lhs.second) + rdf4cpp::util::to_checked(rhs.second);
    if (mon.count().is_invalid() || nanos.count().is_invalid()) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
    if (mon < std::chrono::months{0} && nanos > std::chrono::nanoseconds{0}) {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }
    if (mon > std::chrono::months{0} && nanos < std::chrono::nanoseconds{0}) {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }
    return std::make_pair(*rdf4cpp::util::from_checked(mon), *rdf4cpp::util::from_checked(nanos));
}

template<>
nonstd::expected<capabilities::Duration<xsd_duration>::cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    auto mon = rdf4cpp::util::to_checked(lhs.first) - rdf4cpp::util::to_checked(rhs.first);
    auto nanos = rdf4cpp::util::to_checked(lhs.second) - rdf4cpp::util::to_checked(rhs.second);
    if (mon.count().is_invalid() || nanos.count().is_invalid()) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
    if (mon < std::chrono::months{0} && nanos > std::chrono::nanoseconds{0}) {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }
    if (mon > std::chrono::months{0} && nanos < std::chrono::nanoseconds{0}) {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }
    return std::make_pair(*rdf4cpp::util::from_checked(mon), *rdf4cpp::util::from_checked(nanos));
}

template<>
nonstd::expected<capabilities::Duration<xsd_duration>::duration_div_result_cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_div([[maybe_unused]] cpp_type const &lhs, [[maybe_unused]] cpp_type const &rhs) noexcept {
    return nonstd::make_unexpected(DynamicError::Unsupported);
}

template<>
nonstd::expected<capabilities::Duration<xsd_duration>::cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_scalar_mul(cpp_type const &dur, duration_scalar_cpp_type const &scalar) noexcept {
    auto mon = std::round(static_cast<double>(dur.first.count()) * scalar);
    int64_t m64;
    if (rdf4cpp::util::detail::cast_checked<rdf4cpp::util::detail::OverflowMode::Checked>(mon, m64)) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
    auto nanos = std::round(static_cast<double>(dur.second.count()) * scalar);
    int64_t n64;
    if (rdf4cpp::util::detail::cast_checked<rdf4cpp::util::detail::OverflowMode::Checked>(nanos, n64)) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }

    return std::make_pair(std::chrono::months{m64}, std::chrono::nanoseconds{n64});
}

template<>
nonstd::expected<capabilities::Duration<xsd_duration>::cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_scalar_div(cpp_type const &dur, duration_scalar_cpp_type const &scalar) noexcept {
    if (scalar == 0.0) {
        return nonstd::make_unexpected(DynamicError::DivideByZero);
    }

    auto mon = std::round(static_cast<double>(dur.first.count()) / scalar);
    int64_t m64;
    if (rdf4cpp::util::detail::cast_checked<rdf4cpp::util::detail::OverflowMode::Checked>(mon, m64)) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
    auto nanos = std::round(static_cast<double>(dur.second.count()) / scalar);
    int64_t n64;
    if (rdf4cpp::util::detail::cast_checked<rdf4cpp::util::detail::OverflowMode::Checked>(nanos, n64)) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }

    return std::make_pair(std::chrono::months{m64}, std::chrono::nanoseconds{n64});
}

#endif

template struct LiteralDatatypeImpl<xsd_duration,
                                    capabilities::Duration,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;
}  // namespace rdf4cpp::datatypes::registry