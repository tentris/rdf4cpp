#include "DateTime.hpp"

#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_dateTime>::cpp_type capabilities::Default<xsd_dateTime>::from_string(std::string_view s) {
    using namespace datatypes::registry::util;
    auto year = parse_date_time_fragment<Year, int64_t, '-', identifier>(s);
    auto month = parse_date_time_fragment<std::chrono::month, unsigned int, '-', identifier>(s);
    auto day = parse_date_time_fragment<std::chrono::day, unsigned int, 'T', identifier>(s);
    auto hours = parse_date_time_fragment<std::chrono::hours, unsigned int, ':', identifier>(s);
    auto minutes = parse_date_time_fragment<std::chrono::minutes, unsigned int, ':', identifier>(s);
    auto tz = rdf4cpp::Timezone::parse_optional(s, identifier);
    std::chrono::nanoseconds ms = parse_nanoseconds<identifier>(s);
    auto date = YearMonthDay{year, month, day};
    if (registry::relaxed_parsing_mode && !date.ok()) {
        date = normalize(date);
    }
    if (!date.ok()) {
        throw InvalidNode(std::format("{} parsing error: {} is invalid", identifier, date));
    }
    if (!registry::relaxed_parsing_mode) {
        if (minutes < std::chrono::minutes(0) || minutes > std::chrono::hours(1)) {
            throw InvalidNode(std::format("{} parsing error: minutes out of range", identifier));
        }
        if (hours < std::chrono::hours(0) || hours > std::chrono::days(1)) {
            throw InvalidNode(std::format("{} parsing error: hours out of range", identifier));
        }
        if (ms < std::chrono::seconds(0) || ms > std::chrono::minutes(1)) {
            throw InvalidNode(std::format("{} parsing error: seconds out of range", identifier));
        }
    }
    auto time = hours + minutes + ms;
    if (!registry::relaxed_parsing_mode) {
        if (time == std::chrono::hours{24}) {
            date = YearMonthDay{date.to_time_point_local() + std::chrono::days{1}};
            if (!date.ok()) {
                throw InvalidNode(std::format("{} parsing error: {} is invalid", identifier, date));
            }
            time = std::chrono::hours{0};
        } else if (time > std::chrono::hours{24}) {
            throw InvalidNode(std::format("{} parsing error: invalid time of day", identifier));
        }
    }

    return std::make_pair(rdf4cpp::util::construct_timepoint(date, time), tz);
}

template<>
bool capabilities::Default<xsd_dateTime>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    //year,-,month,-,day, T, hours,:,min,:,sec, tz
    std::array<char, registry::util::chrono_max_canonical_string_chars::year + 1 +
                             registry::util::chrono_max_canonical_string_chars::month + 1 +
                             registry::util::chrono_max_canonical_string_chars::day + 1 +
                             registry::util::chrono_max_canonical_string_chars::hours + 1 +
                             registry::util::chrono_max_canonical_string_chars::minutes + 1 +
                             registry::util::chrono_max_canonical_string_chars::seconds + Timezone::max_canonical_string_chars>
            buff;
    auto [date, time] = rdf4cpp::util::deconstruct_timepoint(value.first);
    char *it = std::format_to(buff.data(), "{}T{:%H:%M:%S}", date, std::chrono::hh_mm_ss{std::chrono::duration_cast<std::chrono::nanoseconds>(time)});
    it = util::canonical_seconds_remove_empty_millis(it);
    if (value.second.has_value()) {
        it = value.second->to_canonical_string(it);
    }
    size_t const len = it - buff.data();
    RDF4CPP_ASSERT(len <= buff.size());
    return writer::write_str(std::string_view(buff.data(), len), writer);
}

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_dateTime>::try_into_inlined(cpp_type const &value) noexcept {
    if (value.second.has_value())
        return std::nullopt;
    auto tp_sec = std::chrono::floor<std::chrono::duration<boost::multiprecision::checked_int128_t, std::chrono::seconds::period>>(value.first);
    if ((value.first - tp_sec).count() != 0)
        return std::nullopt;
    if (!util::fits_into<int64_t>(tp_sec.time_since_epoch().count())) {
        return std::nullopt;
    }
    auto s = static_cast<int64_t>(tp_sec.time_since_epoch().count());
    return util::try_pack_integral<storage::identifier::LiteralID>(s);
}

template<>
capabilities::Inlineable<xsd_dateTime>::cpp_type capabilities::Inlineable<xsd_dateTime>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    return std::make_pair(rdf4cpp::TimePoint{std::chrono::seconds{util::unpack_integral<int64_t>(inlined)}}, std::nullopt);
}

template<>
std::partial_ordering capabilities::Comparable<xsd_dateTime>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    return rdf4cpp::datatypes::registry::util::compare_time_points(lhs.first, lhs.second, rhs.first, rhs.second);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_dateTime>::timepoint_sub_result_cpp_type, DynamicError>
capabilities::Timepoint<xsd_dateTime>::timepoint_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    return util::timepoint_sub(lhs, rhs);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_dateTime>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_dateTime>::timepoint_duration_add(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept {
    try {
        auto ret_tp = util::add_duration_to_date_time(tp.first, dur);
        return std::make_pair(ret_tp, tp.second);
    } catch (std::overflow_error const &) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_dateTime>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_dateTime>::timepoint_duration_sub(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept {
    try {
        auto ret_tp = util::add_duration_to_date_time(tp.first, std::make_pair(-dur.first, -dur.second));
        return std::make_pair(ret_tp, tp.second);
    } catch (std::overflow_error const &) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
}

#endif

template struct LiteralDatatypeImpl<xsd_dateTime,
                                    capabilities::Timepoint,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;
}  // namespace rdf4cpp::datatypes::registry