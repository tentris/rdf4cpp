#include "Date.hpp"

#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>
#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_date>::cpp_type capabilities::Default<xsd_date>::from_string(std::string_view s) {
    using namespace datatypes::registry::util;
    auto year = parse_date_time_fragment<Year, int64_t, '-', identifier>(s);
    auto month = parse_date_time_fragment<std::chrono::month, unsigned int, '-', identifier>(s);
    auto tz = rdf4cpp::Timezone::parse_optional(s, identifier);
    auto day = parse_date_time_fragment<std::chrono::day, unsigned int, '\0', identifier>(s);
    auto date = YearMonthDay{year, month, day};
    if (registry::relaxed_parsing_mode && !date.ok()) {
        auto const norm = normalize(date);
        if (!norm.has_value()) {
            throw InvalidNode(std::format("{} parsing error: failed to normalize", identifier, date));
        }
        date = *norm;
    }
    if (!date.ok()) {
        throw InvalidNode(std::format("{} parsing error: {} is invalid", identifier, date));
    }

    return std::make_pair(date, tz);
}

template<>
bool capabilities::Default<xsd_date>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    //year,-,month,-,day,tz
    std::array<char, registry::util::chrono_max_canonical_string_chars::year + 1 +
                             registry::util::chrono_max_canonical_string_chars::month + 1 +
                             registry::util::chrono_max_canonical_string_chars::day + Timezone::max_canonical_string_chars>
            buff;
    char *it = std::format_to(buff.data(), "{}", value.first);
    if (value.second.has_value()) {
        it = value.second->to_canonical_string(it);
    }
    size_t const len = it - buff.data();
    RDF4CPP_ASSERT(len <= buff.size());
    return writer::write_str(std::string_view(buff.data(), len), writer);
}

struct __attribute__((__packed__)) InliningHelperYearMonthDay {
    int16_t year;
    uint8_t month, day;
};
static_assert(sizeof(std::chrono::year_month_day) * 8 <= storage::identifier::LiteralID::width);

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_date>::try_into_inlined(cpp_type const &value) noexcept {
    if (value.second.has_value()) {
        return std::nullopt;
    }
    auto i = value.first.to_time_point().time_since_epoch().count();
    int64_t i64;
    if (rdf4cpp::util::detail::cast_checked<rdf4cpp::util::detail::OverflowMode::Checked>(i, i64)) [[unlikely]] {
        return std::nullopt;
    }
    return util::try_pack_integral<storage::identifier::LiteralID>(i64);
}

template<>
capabilities::Inlineable<xsd_date>::cpp_type capabilities::Inlineable<xsd_date>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto const i = util::unpack_integral<int64_t>(inlined);
    return capabilities::Inlineable<xsd_date>::cpp_type{YearMonthDay::time_point<int64_t>{YearMonthDay::time_point<int64_t>::duration{i}}, std::nullopt};
}

rdf4cpp::TimePoint date_to_tp(YearMonthDay const &d) noexcept {
    return rdf4cpp::util::construct_timepoint(d, rdf4cpp::util::time_point_replacement_time_of_day);
}

template<>
template<>
capabilities::Promotable<xsd_date>::promoted_cpp_type<0> capabilities::Promotable<xsd_date>::promote<0>(cpp_type const &value) noexcept {
    return std::make_pair(date_to_tp(value.first), value.second);
}

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_date>::cpp_type, DynamicError> capabilities::Promotable<xsd_date>::demote<0>(promoted_cpp_type<0> const &value) noexcept {
    return std::make_pair(YearMonthDay{std::chrono::floor<std::chrono::days>(value.first)}, value.second);
}

template<>
std::partial_ordering capabilities::Comparable<xsd_date>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    return rdf4cpp::datatypes::registry::util::compare_time_points(date_to_tp(lhs.first), lhs.second, date_to_tp(rhs.first), rhs.second);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_date>::timepoint_sub_result_cpp_type, DynamicError>
capabilities::Timepoint<xsd_date>::timepoint_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    auto const super_lhs = Promotable<xsd_date>::promote(lhs);
    auto const super_rhs = Promotable<xsd_date>::promote(rhs);
    return util::timepoint_sub(super_lhs, super_rhs);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_date>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_date>::timepoint_duration_add(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept {
    auto const super_tp = Promotable<xsd_date>::promote(tp);
    auto res_tp = util::add_duration_to_date_time(super_tp.first, dur);
    if (!res_tp.has_value()) {
        return nonstd::make_unexpected(res_tp.error());
    }

    auto deconstructed = rdf4cpp::util::deconstruct_timepoint(*res_tp);
    if (!deconstructed.has_value()) {
        return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
    }
    auto [date, _] = *deconstructed;
    return std::make_pair(date, super_tp.second);
}

template<>
nonstd::expected<capabilities::Timepoint<xsd_date>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_date>::timepoint_duration_sub(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept {
    auto const super_tp = Promotable<xsd_date>::promote(tp);

    auto const sdur_month = rdf4cpp::util::from_checked(-rdf4cpp::util::to_checked(dur.first));
    auto const sdur_nanos = rdf4cpp::util::from_checked(-rdf4cpp::util::to_checked((dur.second)));
    if (!sdur_month.has_value()) {
        return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
    }
    if (!sdur_nanos.has_value()) {
        return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
    }

    auto res_tp = util::add_duration_to_date_time(super_tp.first, std::make_pair(*sdur_month, *sdur_nanos));
    if (!res_tp.has_value()) {
        return nonstd::make_unexpected(res_tp.error());
    }

    auto deconstructed = rdf4cpp::util::deconstruct_timepoint(*res_tp);
    if (!deconstructed.has_value()) {
        return nonstd::make_unexpected(datatypes::DynamicError::OverOrUnderFlow);
    }
    auto [date, _] = *deconstructed;
    return std::make_pair(date, super_tp.second);
}

#endif

template struct LiteralDatatypeImpl<xsd_date,
                                    capabilities::Timepoint,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable,
                                    capabilities::Promotable>;
}  // namespace rdf4cpp::datatypes::registry
