#ifndef RDF4CPP_XSD_STRING_HPP
#define RDF4CPP_XSD_STRING_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_string> {
    using cpp_datatype = std::string_view;
};

template<>
inline capabilities::Default<xsd_string>::cpp_type capabilities::Default<xsd_string>::from_string(std::string_view) {
    // dummy implementation, actual implementation in Literal
    RDF4CPP_UNREACHABLE;
}

template<>
inline bool capabilities::Default<xsd_string>::serialize_canonical_string(cpp_type const &, writer::BufWriterParts) noexcept {
    // dummy implementation, actual implementation in Literal
    RDF4CPP_UNREACHABLE;
}

template<>
bool capabilities::Logical<xsd_string>::effective_boolean_value(cpp_type const &value) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_string,
                                           capabilities::Logical,
                                           capabilities::Comparable,
                                           capabilities::FixedId>;

} // namespace rdf4cpp::datatypes::registry


namespace rdf4cpp::datatypes::xsd {

struct String : registry::LiteralDatatypeImpl<registry::xsd_string,
                                              registry::capabilities::Logical,
                                              registry::capabilities::Comparable,
                                              registry::capabilities::FixedId> {};

} // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::String const xsd_string_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_XSD_STRING_HPP
