#ifndef RDF4CPP_XSD_NONPOSITIVEINTEGER_HPP
#define RDF4CPP_XSD_NONPOSITIVEINTEGER_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/datatypes/xsd/integers/signed/Integer.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>

#include <rdf4cpp/util/Int128.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_non_positive_integer> {
    using cpp_datatype = rdf4cpp::util::Int128;
};

template<>
struct DatatypeSupertypeMapping<xsd_non_positive_integer> {
    using supertype = xsd::Integer;
};

template<>
struct DatatypeNumericStubMapping<xsd_non_positive_integer> {
    using numeric_impl_type = xsd::Integer;
};

template<>
capabilities::Default<xsd_non_positive_integer>::cpp_type capabilities::Default<xsd_non_positive_integer>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_non_positive_integer>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
bool capabilities::Logical<xsd_non_positive_integer>::effective_boolean_value(cpp_type const &value) noexcept;

template<>
std::partial_ordering capabilities::Comparable<xsd_non_positive_integer>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
template<>
nonstd::expected<capabilities::Default<xsd_non_positive_integer>::cpp_type, DynamicError> capabilities::Subtype<xsd_non_positive_integer>::from_supertype<0>(super_cpp_type<0> const &value) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_non_positive_integer>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_non_positive_integer>::cpp_type capabilities::Inlineable<xsd_non_positive_integer>::from_inlined(storage::identifier::LiteralID inlined) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_non_positive_integer,
                                           capabilities::Logical,
                                           capabilities::NumericStub,
                                           capabilities::Subtype,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry


namespace rdf4cpp::datatypes::xsd {

struct NonPositiveInteger : registry::LiteralDatatypeImpl<registry::xsd_non_positive_integer,
                                                          registry::capabilities::Logical,
                                                          registry::capabilities::NumericStub,
                                                          registry::capabilities::Subtype,
                                                          registry::capabilities::Comparable,
                                                          registry::capabilities::FixedId,
                                                          registry::capabilities::Inlineable> {};

} // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::NonPositiveInteger const xsd_non_positive_integer_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_XSD_NONPOSITIVEINTEGER_HPP