#ifndef RDF4CPP_YEARMONTHDURATION_HPP
#define RDF4CPP_YEARMONTHDURATION_HPP

#include <chrono>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <rdf4cpp/datatypes/xsd/time/Duration.hpp>
#include <rdf4cpp/datatypes/xsd/Double.hpp>
#include <rdf4cpp/datatypes/xsd/Decimal.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_yearMonthDuration> {
    using cpp_datatype = std::chrono::months;
};
template<>
struct DatatypeSupertypeMapping<xsd_yearMonthDuration> {
    using supertype = xsd::Duration;
};
template<>
struct DatatypeDurationScalarMapping<xsd_yearMonthDuration> {
    using scalar_type = xsd::Double;
};
template<>
struct DatatypeDurationDivResultMapping<xsd_yearMonthDuration> {
    using op_result = xsd::Decimal;
};

template<>
capabilities::Default<xsd_yearMonthDuration>::cpp_type capabilities::Default<xsd_yearMonthDuration>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_yearMonthDuration>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_yearMonthDuration>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_yearMonthDuration>::cpp_type capabilities::Inlineable<xsd_yearMonthDuration>::from_inlined(storage::identifier::LiteralID inlined) noexcept;

template<>
std::partial_ordering capabilities::Comparable<xsd_yearMonthDuration>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
template<>
capabilities::Subtype<xsd_yearMonthDuration>::super_cpp_type<0> capabilities::Subtype<xsd_yearMonthDuration>::into_supertype<0>(cpp_type const &value) noexcept;

template<>
template<>
std::expected<capabilities::Subtype<xsd_yearMonthDuration>::cpp_type, DynamicError>
capabilities::Subtype<xsd_yearMonthDuration>::from_supertype<0>(super_cpp_type<0> const &value) noexcept;

template<>
std::expected<capabilities::Duration<xsd_yearMonthDuration>::cpp_type, DynamicError>
capabilities::Duration<xsd_yearMonthDuration>::duration_add(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_yearMonthDuration>::cpp_type, DynamicError>
capabilities::Duration<xsd_yearMonthDuration>::duration_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_yearMonthDuration>::duration_div_result_cpp_type, DynamicError>
capabilities::Duration<xsd_yearMonthDuration>::duration_div(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_yearMonthDuration>::cpp_type, DynamicError>
capabilities::Duration<xsd_yearMonthDuration>::duration_scalar_mul(cpp_type const &dur, duration_scalar_cpp_type const &scalar) noexcept;

template<>
std::expected<capabilities::Duration<xsd_yearMonthDuration>::cpp_type, DynamicError>
capabilities::Duration<xsd_yearMonthDuration>::duration_scalar_div(cpp_type const &dur, duration_scalar_cpp_type const &scalar) noexcept;

#endif

extern template struct LiteralDatatypeImpl<xsd_yearMonthDuration,
                                           capabilities::Duration,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable,
                                           capabilities::Subtype>;

}  // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

struct YearMonthDuration : registry::LiteralDatatypeImpl<registry::xsd_yearMonthDuration,
                                                         registry::capabilities::Duration,
                                                         registry::capabilities::Comparable,
                                                         registry::capabilities::FixedId,
                                                         registry::capabilities::Inlineable,
                                                         registry::capabilities::Subtype> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::YearMonthDuration const xsd_YearMonthDuration_instance;

}  // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_YEARMONTHDURATION_HPP
