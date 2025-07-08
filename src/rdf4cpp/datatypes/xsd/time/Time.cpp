#include "Time.hpp"

#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER

inline capabilities::Promotable<xsd_time>::promoted_cpp_type<0> time_to_tp(capabilities::Default<xsd_time>::cpp_type const &value) {
    return std::make_pair(rdf4cpp::util::construct_timepoint(rdf4cpp::util::time_point_replacement_date, value.first), value.second);
}

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
    return registry::util::compare_time_points(time_to_tp(lhs),
                                               time_to_tp(rhs));
}

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_time>::promoted_cpp_type<0>, DynamicError>
capabilities::Promotable<xsd_time>::promote<0>([[maybe_unused]] cpp_type const &value) noexcept {
    return nonstd::make_unexpected(DynamicError::Unsupported);
}

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_time>::cpp_type, DynamicError>
capabilities::Promotable<xsd_time>::demote<0>(promoted_cpp_type<0> const &value) noexcept {
    return std::make_pair(std::chrono::duration_cast<std::chrono::nanoseconds>(value.first - std::chrono::floor<std::chrono::days>(value.first)), value.second);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_time>::timepoint_sub_result_cpp_type, DynamicError>
capabilities::Timepoint<xsd_time>::timepoint_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    auto const lhs_tp = time_to_tp(lhs);
    auto const rhs_tp = time_to_tp(rhs);

    return util::timepoint_sub(lhs_tp, rhs_tp);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_time>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_time>::timepoint_duration_add(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept {
    auto const super_tp = time_to_tp(tp);
    auto const super_dur = Subtype<timepoint_duration_operand_type::identifier>::into_supertype(dur);

    auto ret_tp = util::add_duration_to_date_time(super_tp.first, super_dur);

    auto [_, time] = rdf4cpp::util::deconstruct_timepoint(ret_tp);
    return std::make_pair(std::chrono::duration_cast<std::chrono::nanoseconds>(time), super_tp.second);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_time>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_time>::timepoint_duration_sub(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept {
    auto const super_tp = time_to_tp(tp);
    auto const super_dur = Subtype<timepoint_duration_operand_type::identifier>::into_supertype(dur);

    auto ret_tp = util::add_duration_to_date_time(super_tp.first, std::make_pair(-super_dur.first, -super_dur.second));

    auto [_, time] = rdf4cpp::util::deconstruct_timepoint(ret_tp);
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