#include "MonthDay.hpp"

#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_gMonthDay>::cpp_type capabilities::Default<xsd_gMonthDay>::from_string(std::string_view s) {
    using namespace registry::util;
    if (!s.starts_with("--")) {
        throw InvalidNode{std::format("{} parsing error: gMonth prefix missing", identifier)};
    }

    s.remove_prefix(2);

    auto month = parse_date_time_fragment<std::chrono::month, unsigned int, '-', identifier>(s);
    auto tz = rdf4cpp::Timezone::parse_optional(s, identifier);
    auto day = parse_date_time_fragment<std::chrono::day, unsigned int, '\0', identifier>(s);
    auto date = month / day;
    if (!date.ok()) {
        throw InvalidNode(std::format("{} parsing error: {:%m-%d} is invalid", identifier, date));
    }

    return std::make_pair(date, tz);
}

template<>
bool capabilities::Default<xsd_gMonthDay>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    //--, month, -, day, tz
    std::array<char, 2 + registry::util::chrono_max_canonical_string_chars::month + 1 + registry::util::chrono_max_canonical_string_chars::day +
                             Timezone::max_canonical_string_chars>
            buff;
    char *it = std::format_to(buff.data(), "--{:%m-%d}", value.first);
    if (value.second.has_value()) {
        it = value.second->to_canonical_string(it);
    }
    size_t const len = it - buff.data();
    assert(len <= buff.size());
    return writer::write_str(std::string_view(buff.data(), len), writer);
}

struct __attribute__((__packed__)) InliningHelperMonthDay {
    uint8_t month, day;
};
using IHelp = registry::util::InliningHelper<InliningHelperMonthDay>;
static_assert(sizeof(std::chrono::month_day) == 2);
static_assert(sizeof(IHelp) * 8 < storage::identifier::LiteralID::width);

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_gMonthDay>::try_into_inlined(cpp_type const &value) noexcept {
    IHelp i{InliningHelperMonthDay{static_cast<uint8_t>(static_cast<unsigned int>(value.first.month())), static_cast<uint8_t>(static_cast<unsigned int>(value.first.day()))}, value.second};
    return util::pack<storage::identifier::LiteralID>(i);
}

template<>
capabilities::Inlineable<xsd_gMonthDay>::cpp_type capabilities::Inlineable<xsd_gMonthDay>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto i = util::unpack<IHelp>(inlined);
    return std::make_pair(std::chrono::month{i.time_value.month} / std::chrono::day{i.time_value.day}, i.decode_tz());
}

template<>
std::partial_ordering capabilities::Comparable<xsd_gMonthDay>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    auto md_to_tp = [](std::chrono::month_day md) noexcept -> TimePoint {
        return rdf4cpp::util::construct_timepoint(YearMonthDay{rdf4cpp::util::time_point_replacement_date.year(), md.month(), md.day()}, rdf4cpp::util::time_point_replacement_time_of_day);
    };
    return registry::util::compare_time_points(md_to_tp(lhs.first), lhs.second, md_to_tp(rhs.first), rhs.second);
}

template<>
template<>
capabilities::Promotable<xsd_gMonthDay>::promoted_cpp_type<0> capabilities::Promotable<xsd_gMonthDay>::promote<0>(cpp_type const &value) noexcept {
    return std::make_pair(YearMonthDay{rdf4cpp::util::time_point_replacement_date.year(), value.first.month(), value.first.day()}, value.second);
}

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gMonthDay>::cpp_type, DynamicError> capabilities::Promotable<xsd_gMonthDay>::demote<0>(promoted_cpp_type<0> const &value) noexcept {
    return std::make_pair(value.first.month() / value.first.day(), value.second);
}
#endif

template struct LiteralDatatypeImpl<xsd_gMonthDay,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable,
                                    capabilities::Promotable>;

}  // namespace rdf4cpp::datatypes::registry