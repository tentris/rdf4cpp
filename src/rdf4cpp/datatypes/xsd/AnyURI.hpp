#ifndef RDF4CPP_XSD_ANYURI_HPP
#define RDF4CPP_XSD_ANYURI_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>

#include <dice/hash.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
    template<>
    struct DatatypeMapping<xsd_any_uri> {
        using cpp_datatype = std::string;
    };

    template<>
    capabilities::Default<xsd_any_uri>::cpp_type capabilities::Default<xsd_any_uri>::from_string(std::string_view s);

    template<>
    bool capabilities::Default<xsd_any_uri>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

    template<>
    bool capabilities::Default<xsd_any_uri>::serialize_simplified_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

    template<>
    bool capabilities::Logical<xsd_any_uri>::effective_boolean_value(cpp_type const &value) noexcept;

#endif

    extern template struct LiteralDatatypeImpl<xsd_any_uri,
                                               capabilities::Logical,
                                               capabilities::Comparable,
                                               capabilities::FixedId>;

}  // namespace rdf4cpp::datatypes::registry


namespace rdf4cpp::datatypes::xsd {

    struct AnyURI : registry::LiteralDatatypeImpl<registry::xsd_any_uri,
                                                   registry::capabilities::Logical,
                                                   registry::capabilities::Comparable,
                                                   registry::capabilities::FixedId> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

    [[maybe_unused]] inline xsd::AnyURI const xsd_any_uri_instance;

}  // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_XSD_ANYURI_HPP
