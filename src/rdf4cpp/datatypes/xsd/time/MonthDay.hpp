#ifndef RDF4CPP_MONTHDAY_HPP
#define RDF4CPP_MONTHDAY_HPP

#include <chrono>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <rdf4cpp/datatypes/xsd/time/Date.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_gMonthDay> {
    using cpp_datatype = std::pair<std::chrono::month_day, rdf4cpp::OptionalTimezone>;
};
template<>
struct DatatypePromotionMapping<xsd_gMonthDay> {
    using promoted = xsd::Date;
};


template<>
capabilities::Default<xsd_gMonthDay>::cpp_type capabilities::Default<xsd_gMonthDay>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_gMonthDay>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;


template<>
std::partial_ordering capabilities::Comparable<xsd_gMonthDay>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_gMonthDay>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_gMonthDay>::cpp_type capabilities::Inlineable<xsd_gMonthDay>::from_inlined(storage::identifier::LiteralID inlined) noexcept;

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gMonthDay>::promoted_cpp_type<0>, DynamicError>
capabilities::Promotable<xsd_gMonthDay>::promote<0>(cpp_type const &value) noexcept;

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gMonthDay>::cpp_type, DynamicError> capabilities::Promotable<xsd_gMonthDay>::demote<0>(promoted_cpp_type<0> const &value) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_gMonthDay,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable,
                                           capabilities::Promotable>;

}  // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

struct GMonthDay : registry::LiteralDatatypeImpl<registry::xsd_gMonthDay,
                                                 registry::capabilities::Comparable,
                                                 registry::capabilities::FixedId,
                                                 registry::capabilities::Inlineable,
                                                 registry::capabilities::Promotable> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::GMonthDay const xsd_GMonthDay_instance;

}  // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_MONTHDAY_HPP
