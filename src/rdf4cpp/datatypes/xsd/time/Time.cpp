#include "Time.hpp"

#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_time>::cpp_type capabilities::Default<xsd_time>::from_string(std::string_view s) {
    using namespace registry::util;
    auto hours = parse_date_time_fragment<std::chrono::hours, unsigned int, ':', identifier>(s);
    auto minutes = parse_date_time_fragment<std::chrono::minutes, unsigned int, ':', identifier>(s);
    auto tz = rdf4cpp::Timezone::parse_optional(s, identifier);
    std::chrono::nanoseconds ms = parse_nanoseconds<identifier>(s);
    if (!registry::relaxed_parsing_mode) {
        if (minutes < std::chrono::minutes(0) || minutes > std::chrono::hours(1)) {
            throw InvalidNode{std::format("{} parsing error: minutes out of range", identifier)};
        }
        if (hours < std::chrono::hours(0) || hours > std::chrono::days(1)) {
            throw InvalidNode{std::format("{} parsing error: hours out of range", identifier)};
        }
        if (ms < std::chrono::seconds(0) || ms > std::chrono::minutes(1)) {
            throw InvalidNode{std::format("{} parsing error: seconds out of range", identifier)};
        }
    }
    auto time = hours + minutes + ms;
    if (time > std::chrono::hours{24}) {
        throw InvalidNode{std::format("{} parsing error: invalid time of day", identifier)};
    }

    return std::make_pair(time, tz);
}

template<>
bool capabilities::Default<xsd_time>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    //hours,:,min,:,sec,tz
    std::array<char, registry::util::chrono_max_canonical_string_chars::hours_unbound + 1 +
                             registry::util::chrono_max_canonical_string_chars::minutes + 1 +
                             registry::util::chrono_max_canonical_string_chars::seconds + Timezone::max_canonical_string_chars>
            buff;
    char *it = std::format_to(buff.data(), "{:%H:%M:%S}", std::chrono::hh_mm_ss(value.first));
    it = util::canonical_seconds_remove_empty_millis(it);
    if (value.second.has_value()) {
        it = value.second->to_canonical_string(it);
    }
    size_t const len = it - buff.data();
    assert(len <= buff.size());
    return writer::write_str(std::string_view(buff.data(), len), writer);
}

using IHelp = registry::util::InliningHelperPacked;
static_assert(registry::util::number_of_bits(static_cast<unsigned int>(std::chrono::seconds{std::chrono::hours{24}}.count())) == 17);

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_time>::try_into_inlined(cpp_type const &value) noexcept {
    if ((value.first - std::chrono::floor<std::chrono::seconds>(value.first)) != std::chrono::milliseconds(0))
        return std::nullopt;
    auto sec = std::chrono::floor<std::chrono::seconds>(value.first);
    if (sec < std::chrono::seconds(0) || sec > std::chrono::hours(24))
        return std::nullopt; // should never happen
    IHelp i{static_cast<uint32_t>(sec.count()), value.second};
    return util::pack<storage::identifier::LiteralID>(i);
}

template<>
capabilities::Inlineable<xsd_time>::cpp_type capabilities::Inlineable<xsd_time>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto i = util::unpack<IHelp>(inlined);
    return std::make_pair(std::chrono::seconds(i.time_value), i.decode_tz());
}

template<>
std::partial_ordering capabilities::Comparable<xsd_time>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    return registry::util::compare_time_points(rdf4cpp::util::construct_timepoint(rdf4cpp::util::time_point_replacement_date, lhs.first), lhs.second,
                                               rdf4cpp::util::construct_timepoint(rdf4cpp::util::time_point_replacement_date, rhs.first), rhs.second);
}

template<>
template<>
capabilities::Promotable<xsd_time>::promoted_cpp_type<0> capabilities::Promotable<xsd_time>::promote<0>(cpp_type const &value) noexcept {
    // time_point_replacement_date can not cause an overflow
    return std::make_pair(*rdf4cpp::util::construct_timepoint(rdf4cpp::util::time_point_replacement_date, value.first), value.second);
}

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_time>::cpp_type, DynamicError> capabilities::Promotable<xsd_time>::demote<0>(promoted_cpp_type<0> const &value) noexcept {
    return std::make_pair(std::chrono::duration_cast<std::chrono::nanoseconds>(value.first - std::chrono::floor<std::chrono::days>(value.first)), value.second);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_time>::timepoint_sub_result_cpp_type, DynamicError>
capabilities::Timepoint<xsd_time>::timepoint_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    auto const super_lhs = Promotable<xsd_time>::promote(lhs);
    auto const super_rhs = Promotable<xsd_time>::promote(rhs);
    return util::timepoint_sub(super_lhs, super_rhs);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_time>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_time>::timepoint_duration_add(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept {
    auto const super_tp = Promotable<xsd_time>::promote(tp);
    auto const super_dur = Subtype<timepoint_duration_operand_type::identifier>::into_supertype(dur);

    auto ret_tp = util::add_duration_to_date_time(super_tp.first, super_dur);
    if (!ret_tp.has_value()) {
        return nonstd::make_unexpected(ret_tp.error());
    }

    auto deconstructed = rdf4cpp::util::deconstruct_timepoint(*ret_tp);
    if (!deconstructed.has_value()) {
        return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
    }
    auto [_, time] = *deconstructed;
    return std::make_pair(std::chrono::duration_cast<std::chrono::nanoseconds>(time), super_tp.second);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_time>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_time>::timepoint_duration_sub(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept {
    auto const super_tp = Promotable<xsd_time>::promote(tp);
    auto const super_dur = Subtype<timepoint_duration_operand_type::identifier>::into_supertype(dur);

    auto const sdur_month = rdf4cpp::util::from_checked(-rdf4cpp::util::to_checked(super_dur.first));
    auto const sdur_nanos = rdf4cpp::util::from_checked(-rdf4cpp::util::to_checked((super_dur.second)));
    if (!sdur_month.has_value()) {
        return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
    }
    if (!sdur_nanos.has_value()) {
        return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
    }

    auto ret_tp = util::add_duration_to_date_time(super_tp.first, std::make_pair(*sdur_month, *sdur_nanos));
    if (!ret_tp.has_value()) {
        return nonstd::make_unexpected(ret_tp.error());
    }

    auto deconstructed = rdf4cpp::util::deconstruct_timepoint(*ret_tp);
    if (!deconstructed.has_value()) {
        return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
    }
    auto [_, time] = *deconstructed;
    return std::make_pair(std::chrono::duration_cast<std::chrono::nanoseconds>(time), super_tp.second);
}

#endif

template struct LiteralDatatypeImpl<xsd_time,
                                    capabilities::Timepoint,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable,
                                    capabilities::Promotable>;
}  // namespace rdf4cpp::datatypes::registry