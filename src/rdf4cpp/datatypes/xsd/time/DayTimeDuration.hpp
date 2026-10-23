#ifndef RDF4CPP_DAYTIMEDURATION_HPP
#define RDF4CPP_DAYTIMEDURATION_HPP

#include <chrono>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <rdf4cpp/datatypes/xsd/time/Duration.hpp>
#include <rdf4cpp/datatypes/xsd/Decimal.hpp>
#include <rdf4cpp/datatypes/xsd/Double.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_dayTimeDuration> {
    using cpp_datatype = std::chrono::nanoseconds;
};
template<>
struct DatatypeSupertypeMapping<xsd_dayTimeDuration> {
    using supertype = xsd::Duration;
};
template<>
struct DatatypeDurationScalarMapping<xsd_dayTimeDuration> {
    using scalar_type = xsd::Double;
};
template<>
struct DatatypeDurationDivResultMapping<xsd_dayTimeDuration> {
    using op_result = xsd::Decimal;
};


template<>
capabilities::Default<xsd_dayTimeDuration>::cpp_type capabilities::Default<xsd_dayTimeDuration>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_dayTimeDuration>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_dayTimeDuration>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_dayTimeDuration>::cpp_type capabilities::Inlineable<xsd_dayTimeDuration>::from_inlined(storage::identifier::LiteralID inlined) noexcept;

template<>
std::partial_ordering capabilities::Comparable<xsd_dayTimeDuration>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
template<>
capabilities::Subtype<xsd_dayTimeDuration>::super_cpp_type<0> capabilities::Subtype<xsd_dayTimeDuration>::into_supertype<0>(cpp_type const &value) noexcept;

template<>
template<>
std::expected<capabilities::Subtype<xsd_dayTimeDuration>::cpp_type, DynamicError> capabilities::Subtype<xsd_dayTimeDuration>::from_supertype<0>(super_cpp_type<0> const &value) noexcept;

template<>
template<>
std::expected<capabilities::Subtype<xsd_dayTimeDuration>::cpp_type, DynamicError>
capabilities::Subtype<xsd_dayTimeDuration>::from_supertype<0>(super_cpp_type<0> const &value) noexcept;

template<>
std::expected<capabilities::Duration<xsd_dayTimeDuration>::cpp_type, DynamicError>
capabilities::Duration<xsd_dayTimeDuration>::duration_add(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_dayTimeDuration>::cpp_type, DynamicError>
capabilities::Duration<xsd_dayTimeDuration>::duration_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_dayTimeDuration>::duration_div_result_cpp_type, DynamicError>
capabilities::Duration<xsd_dayTimeDuration>::duration_div(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_dayTimeDuration>::cpp_type, DynamicError>
capabilities::Duration<xsd_dayTimeDuration>::duration_scalar_mul(cpp_type const &dur, duration_scalar_cpp_type const &scalar) noexcept;

template<>
std::expected<capabilities::Duration<xsd_dayTimeDuration>::cpp_type, DynamicError>
capabilities::Duration<xsd_dayTimeDuration>::duration_scalar_div(cpp_type const &dur, duration_scalar_cpp_type const &scalar) noexcept;

#endif

extern template struct LiteralDatatypeImpl<xsd_dayTimeDuration,
                                           capabilities::Duration,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable,
                                           capabilities::Subtype>;

}  // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

struct DayTimeDuration : registry::LiteralDatatypeImpl<registry::xsd_dayTimeDuration,
                                                       registry::capabilities::Duration,
                                                       registry::capabilities::Comparable,
                                                       registry::capabilities::FixedId,
                                                       registry::capabilities::Inlineable,
                                                       registry::capabilities::Subtype> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::DayTimeDuration const xsd_DaysTimeDuration_instance;

}  // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_DAYTIMEDURATION_HPP
