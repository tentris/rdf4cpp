#ifndef RDF4CPP_XSD_DOUBLE_HPP
#define RDF4CPP_XSD_DOUBLE_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_double> {
    using cpp_datatype = double;
};

template<>
capabilities::Default<xsd_double>::cpp_type capabilities::Default<xsd_double>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_double>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
bool capabilities::Default<xsd_double>::serialize_simplified_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
bool capabilities::Logical<xsd_double>::effective_boolean_value(cpp_type const &value) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_double>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_double>::abs(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_double>::round_result_cpp_type, DynamicError> capabilities::Numeric<xsd_double>::round(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_double>::floor_result_cpp_type, DynamicError> capabilities::Numeric<xsd_double>::floor(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_double>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<xsd_double>::ceil(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Default<xsd_double>::cpp_type, DynamicError> capabilities::Numeric<xsd_double>::from_multiplicity(uint64_t multiplicity) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_double>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_double>::cpp_type capabilities::Inlineable<xsd_double>::from_inlined(storage::identifier::LiteralID inlined) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_double,
                                           capabilities::Logical,
                                           capabilities::Numeric,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry


namespace rdf4cpp::datatypes::xsd {

struct Double : registry::LiteralDatatypeImpl<registry::xsd_double,
                                              registry::capabilities::Logical,
                                              registry::capabilities::Numeric,
                                              registry::capabilities::Comparable,
                                              registry::capabilities::FixedId,
                                              registry::capabilities::Inlineable> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::Double const xsd_double_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_XSD_DOUBLE_HPP
