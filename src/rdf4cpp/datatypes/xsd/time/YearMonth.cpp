#include "YearMonth.hpp"

#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_gYearMonth>::cpp_type capabilities::Default<xsd_gYearMonth>::from_string(std::string_view s) {
    using namespace registry::util;
    auto year = parse_date_time_fragment<Year, int64_t, '-', identifier>(s);
    auto tz = rdf4cpp::Timezone::parse_optional(s, identifier);
    auto month = parse_date_time_fragment<std::chrono::month, unsigned int, '\0', identifier>(s);
    auto date = YearMonth{year, month};
    if (!month.ok()) {
        throw InvalidNode(std::format("{} parsing error: {} is invalid", identifier, date));
    }

    return std::make_pair(date, tz);
}

template<>
bool capabilities::Default<xsd_gYearMonth>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    //year,-,month,tz
    std::array<char, registry::util::chrono_max_canonical_string_chars::year + 1 +
                             registry::util::chrono_max_canonical_string_chars::month + Timezone::max_canonical_string_chars>
            buff;
    char *it = std::format_to(buff.data(), "{}", value.first);
    if (value.second.has_value()) {
        it = value.second->to_canonical_string(it);
    }
    size_t const len = it - buff.data();
    assert(len <= buff.size());
    return writer::write_str(std::string_view(buff.data(), len), writer);
}

struct __attribute__((__packed__)) InliningHelperYearMonth {
    int16_t year;
    uint8_t month;
};
static_assert(sizeof(std::chrono::year_month) * 8 <= storage::identifier::LiteralID::width);

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_gYearMonth>::try_into_inlined(cpp_type const &value) noexcept {
    if (value.second.has_value()) [[unlikely]] {
        return std::nullopt;
    }
    auto const yearint = static_cast<int64_t>(value.first.year());
    int16_t yint16;
    if (rdf4cpp::util::detail::cast_checked<rdf4cpp::util::detail::OverflowMode::Checked>(yearint, yint16)) [[unlikely]] {
        return std::nullopt;
    }
    return util::pack<storage::identifier::LiteralID>(InliningHelperYearMonth{yint16,
                                                                              static_cast<uint8_t>(static_cast<unsigned int>(value.first.month()))});
}

template<>
capabilities::Inlineable<xsd_gYearMonth>::cpp_type capabilities::Inlineable<xsd_gYearMonth>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto i = util::unpack<InliningHelperYearMonth>(inlined);
    return std::make_pair(YearMonth{Year{i.year}, std::chrono::month{i.month}}, std::nullopt);
}

template<>
std::partial_ordering capabilities::Comparable<xsd_gYearMonth>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    auto ym_to_tp = [](YearMonth const &t) -> TimePoint {
        return rdf4cpp::util::construct_timepoint(YearMonthDay{t.year(), t.month(), std::chrono::last}, rdf4cpp::util::time_point_replacement_time_of_day);
    };
    return registry::util::compare_time_points(ym_to_tp(lhs.first), lhs.second, ym_to_tp(rhs.first), rhs.second);
}

template<>
template<>
capabilities::Promotable<xsd_gYearMonth>::promoted_cpp_type<0> capabilities::Promotable<xsd_gYearMonth>::promote<0>(cpp_type const &value) noexcept {
    return std::make_pair(YearMonthDay{value.first.year(), value.first.month(), std::chrono::last}, value.second);
}

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gYearMonth>::cpp_type, DynamicError> capabilities::Promotable<xsd_gYearMonth>::demote<0>(promoted_cpp_type<0> const &value) noexcept {
    return std::make_pair(YearMonth{value.first.year(), value.first.month()}, value.second);
}
#endif

template struct LiteralDatatypeImpl<xsd_gYearMonth,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable,
                                    capabilities::Promotable>;
}  // namespace rdf4cpp::datatypes::registry