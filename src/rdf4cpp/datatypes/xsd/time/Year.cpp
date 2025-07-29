#include "Year.hpp"

#include <rdf4cpp/datatypes/registry/util/DateTimeUtils.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_gYear>::cpp_type capabilities::Default<xsd_gYear>::from_string(std::string_view s) {
    auto tz = rdf4cpp::Timezone::parse_optional(s, identifier);
    auto year = registry::util::parse_date_time_fragment<rdf4cpp::Year, int64_t, '\0', identifier>(s);
    return std::make_pair(year, tz);
}

template<>
bool capabilities::Default<xsd_gYear>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    std::array<char, registry::util::chrono_max_canonical_string_chars::year + Timezone::max_canonical_string_chars> buff;
    char *it = std::format_to(buff.data(), "{}", value.first);
    if (value.second.has_value()) {
        it = value.second->to_canonical_string(it);
    }
    size_t const len = it - buff.data();
    RDF4CPP_ASSERT(len <= buff.size());
    return writer::write_str(std::string_view(buff.data(), len), writer);
}

template<>
std::partial_ordering capabilities::Comparable<xsd_gYear>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    try {
        auto year_to_tp = [](Year const &t) -> rdf4cpp::TimePoint {
            return rdf4cpp::util::construct_timepoint(YearMonthDay{t, rdf4cpp::util::time_point_replacement_date.month(), rdf4cpp::util::time_point_replacement_date.day()}, rdf4cpp::util::time_point_replacement_time_of_day);
        };
        return registry::util::compare_time_points(year_to_tp(lhs.first), lhs.second, year_to_tp(rhs.first), rhs.second);
    } catch (std::overflow_error const &) {
        return std::partial_ordering::unordered;
    }
}

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_gYear>::try_into_inlined(cpp_type const &value) noexcept {
    if (value.second.has_value()) [[unlikely]] {
        return std::nullopt;
    }
    return util::try_pack_integral<storage::identifier::LiteralID>(static_cast<int64_t>(value.first));
}

template<>
capabilities::Inlineable<xsd_gYear>::cpp_type capabilities::Inlineable<xsd_gYear>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto i = util::unpack_integral<int64_t>(inlined);
    return {Year{i}, std::nullopt};
}

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gYear>::promoted_cpp_type<0>, DynamicError>
capabilities::Promotable<xsd_gYear>::promote<0>([[maybe_unused]] cpp_type const &value) noexcept {
    return nonstd::make_unexpected(DynamicError::Unsupported);
}

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gYear>::cpp_type, DynamicError>
capabilities::Promotable<xsd_gYear>::demote<0>(promoted_cpp_type<0> const &value) noexcept {
    return std::make_pair(value.first.year(), value.second);
}
#endif

template struct LiteralDatatypeImpl<xsd_gYear,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable,
                                    capabilities::Promotable>;

}
