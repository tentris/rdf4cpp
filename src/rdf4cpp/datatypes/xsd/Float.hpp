#ifndef RDF4CPP_XSD_FLOAT_HPP
#define RDF4CPP_XSD_FLOAT_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/datatypes/xsd/Double.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_float> {
    using cpp_datatype = float;
};

template<>
struct DatatypePromotionMapping<xsd_float> {
    using promoted = xsd::Double;
};

template<>
capabilities::Default<xsd_float>::cpp_type capabilities::Default<xsd_float>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_float>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
bool capabilities::Default<xsd_float>::serialize_simplified_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
bool capabilities::Logical<xsd_float>::effective_boolean_value(cpp_type const &value) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_float>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_float>::abs(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_float>::round_result_cpp_type, DynamicError> capabilities::Numeric<xsd_float>::round(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_float>::floor_result_cpp_type, DynamicError> capabilities::Numeric<xsd_float>::floor(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_float>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<xsd_float>::ceil(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Default<xsd_float>::cpp_type, DynamicError> capabilities::Numeric<xsd_float>::from_multiplicity(uint64_t multiplicity) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_float>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_float>::cpp_type capabilities::Inlineable<xsd_float>::from_inlined(storage::identifier::LiteralID inlined) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_float,
                                           capabilities::Logical,
                                           capabilities::Numeric,
                                           capabilities::Comparable,
                                           capabilities::Promotable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry


namespace rdf4cpp::datatypes::xsd {

struct Float : registry::LiteralDatatypeImpl<registry::xsd_float,
                                             registry::capabilities::Logical,
                                             registry::capabilities::Numeric,
                                             registry::capabilities::Comparable,
                                             registry::capabilities::Promotable,
                                             registry::capabilities::FixedId,
                                             registry::capabilities::Inlineable> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::Float const xsd_float_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_XSD_FLOAT_HPP
