#ifndef RDF4CPP_XSD_LONG_HPP
#define RDF4CPP_XSD_LONG_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/datatypes/xsd/integers/signed/Integer.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>

#include <cstdint>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_long> {
    using cpp_datatype = int64_t;
};

template<>
struct DatatypeSupertypeMapping<xsd_long> {
    using supertype = xsd::Integer;
};

template<>
struct DatatypeNumericStubMapping<xsd_long> {
    using numeric_impl_type = xsd::Integer;
};

template<>
capabilities::Default<xsd_long>::cpp_type capabilities::Default<xsd_long>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_long>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
bool capabilities::Logical<xsd_long>::effective_boolean_value(cpp_type const &value) noexcept;

template<>
template<>
std::expected<capabilities::Default<xsd_long>::cpp_type, DynamicError> capabilities::Subtype<xsd_long>::from_supertype<0>(super_cpp_type<0> const &value) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_long,
                                           capabilities::Logical,
                                           capabilities::NumericStub,
                                           capabilities::Subtype,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry


namespace rdf4cpp::datatypes::xsd {

struct Long : registry::LiteralDatatypeImpl<registry::xsd_long,
                                            registry::capabilities::Logical,
                                            registry::capabilities::NumericStub,
                                            registry::capabilities::Subtype,
                                            registry::capabilities::Comparable,
                                            registry::capabilities::FixedId,
                                            registry::capabilities::Inlineable> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::Long const xsd_long_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_XSD_LONG_HPP
