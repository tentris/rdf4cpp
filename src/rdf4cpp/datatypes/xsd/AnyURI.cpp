#include "AnyURI.hpp"

#include <cmath>
#include <stdexcept>
#include <rdf4cpp/IRIView.hpp>

#include <rdf4cpp/InvalidNode.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_any_uri>::cpp_type capabilities::Default<xsd_any_uri>::from_string(std::string_view s) {
    auto e = IRIView{s}.quick_validate(true);
    if (e != IRIFactoryError::Ok) {
        throw InvalidNode(std::format("http://www.w3.org/2001/XMLSchema#anyURI parsing error: invalid IRI: {}", e));
    }
    return cpp_type{s};
}

template<>
bool capabilities::Default<xsd_any_uri>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    return writer::write_str(value, writer);
}

template<>
bool capabilities::Default<xsd_any_uri>::serialize_simplified_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    return writer::write_str(value, writer);
}

template<>
bool capabilities::Logical<xsd_any_uri>::effective_boolean_value(cpp_type const &value) noexcept {
    return !value.empty();
}

template<>
std::partial_ordering capabilities::Comparable<xsd_any_uri>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    return lhs <=> rhs;
}
#endif

template struct LiteralDatatypeImpl<xsd_any_uri,
                                    capabilities::Logical,
                                    capabilities::Comparable,
                                    capabilities::FixedId>;

}  // namespace rdf4cpp::datatypes::registry
