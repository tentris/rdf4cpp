#ifndef RDF4CPP_YEAR_HPP
#define RDF4CPP_YEAR_HPP

#include <chrono>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <rdf4cpp/datatypes/xsd/time/Date.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_gYear> {
    using cpp_datatype = std::pair<rdf4cpp::Year, rdf4cpp::OptionalTimezone>;
};
template<>
struct DatatypePromotionMapping<xsd_gYear> {
    using promoted = xsd::Date;
};


template<>
capabilities::Default<xsd_gYear>::cpp_type capabilities::Default<xsd_gYear>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_gYear>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;



template<>
std::partial_ordering capabilities::Comparable<xsd_gYear>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_gYear>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_gYear>::cpp_type capabilities::Inlineable<xsd_gYear>::from_inlined(storage::identifier::LiteralID inlined) noexcept;

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gYear>::promoted_cpp_type<0>, DynamicError>
capabilities::Promotable<xsd_gYear>::promote<0>(cpp_type const &value) noexcept;

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gYear>::cpp_type, DynamicError> capabilities::Promotable<xsd_gYear>::demote<0>(promoted_cpp_type<0> const &value) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_gYear,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable,
                                           capabilities::Promotable>;

}

namespace rdf4cpp::datatypes::xsd {

struct GYear : registry::LiteralDatatypeImpl<registry::xsd_gYear,
                                             registry::capabilities::Comparable,
                                             registry::capabilities::FixedId,
                                             registry::capabilities::Inlineable,
                                             registry::capabilities::Promotable> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::GYear const xsd_gYear_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_YEAR_HPP
