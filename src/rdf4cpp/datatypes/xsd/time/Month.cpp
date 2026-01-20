#include "Month.hpp"

#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_gMonth>::cpp_type capabilities::Default<xsd_gMonth>::from_string(std::string_view s) {
    using namespace registry::util;
    if (!s.starts_with("--")) {
        throw InvalidNode{std::format("{} parsing error: missing gMonth prefix", identifier)};
    }

    s.remove_prefix(2);

    auto tz = rdf4cpp::Timezone::parse_optional(s, identifier);
    auto month = parse_date_time_fragment<std::chrono::month, unsigned int, '\0', identifier>(s);
    if (!month.ok()) {
        throw InvalidNode(std::format("{} parsing error: {:%m} is invalid", identifier, month));
    }

    return std::make_pair(month, tz);
}

template<>
bool capabilities::Default<xsd_gMonth>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    //--,month,tz
    std::array<char, 2 + registry::util::chrono_max_canonical_string_chars::month + Timezone::max_canonical_string_chars> buff;
    char *it = std::format_to(buff.data(), "--{:%m}", value.first);
    if (value.second.has_value()) {
        it = value.second->to_canonical_string(it);
    }
    size_t const len = it - buff.data();
    RDF4CPP_ASSERT(len <= buff.size());
    return writer::write_str(std::string_view(buff.data(), len), writer);
}

template<>
std::partial_ordering capabilities::Comparable<xsd_gMonth>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    auto month_to_tp = [](std::chrono::month m) noexcept -> TimePoint {
        return rdf4cpp::util::construct_timepoint(YearMonthDay{rdf4cpp::util::time_point_replacement_date.year(), m, std::chrono::last}, rdf4cpp::util::time_point_replacement_time_of_day);
    };
    return registry::util::compare_time_points(month_to_tp(lhs.first), lhs.second, month_to_tp(rhs.first), rhs.second);
}

using IHelp = registry::util::InliningHelper<uint8_t>;
static_assert(registry::util::number_of_bits(12u) == 4);
static_assert(sizeof(std::chrono::month) == 1);
static_assert(sizeof(IHelp) * 8 < storage::identifier::LiteralID::width);

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_gMonth>::try_into_inlined(cpp_type const &value) noexcept {
    IHelp i{static_cast<uint8_t>(static_cast<unsigned int>(value.first)), value.second};
    return util::pack<storage::identifier::LiteralID>(i);
}

template<>
capabilities::Inlineable<xsd_gMonth>::cpp_type capabilities::Inlineable<xsd_gMonth>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto i = util::unpack<IHelp>(inlined);
    return std::make_pair(std::chrono::month{i.time_value}, i.decode_tz());
}

template<>
template<>
capabilities::Promotable<xsd_gMonth>::promoted_cpp_type<0> capabilities::Promotable<xsd_gMonth>::promote<0>(cpp_type const &value) noexcept {
    return std::make_pair(YearMonthDay{rdf4cpp::util::time_point_replacement_date.year(), value.first, std::chrono::last}, value.second);
}

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gMonth>::cpp_type, DynamicError> capabilities::Promotable<xsd_gMonth>::demote<0>(promoted_cpp_type<0> const &value) noexcept {
    return std::make_pair(value.first.month(), value.second);
}
#endif

template struct LiteralDatatypeImpl<xsd_gMonth,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable,
                                    capabilities::Promotable>;

}  // namespace rdf4cpp::datatypes::registry
