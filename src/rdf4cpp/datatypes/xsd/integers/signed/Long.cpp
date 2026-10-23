#include "Long.hpp"

#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_long>::cpp_type capabilities::Default<xsd_long>::from_string(std::string_view s) {
    return util::from_chars<cpp_type, identifier>(s);
}

template<>
bool capabilities::Default<xsd_long>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    return util::to_chars_canonical(value, writer);
}

template<>
bool capabilities::Logical<xsd_long>::effective_boolean_value(cpp_type const &value) noexcept {
    return value != 0;
}

template<>
template<>
std::expected<capabilities::Default<xsd_long>::cpp_type, DynamicError> capabilities::Subtype<xsd_long>::from_supertype<0>(super_cpp_type<0> const &value) noexcept {
    if (value > std::numeric_limits<cpp_type>::max() || value < std::numeric_limits<cpp_type>::min()) {
        return std::unexpected(DynamicError::InvalidValueForCast);
    }

    return static_cast<cpp_type>(value);
}
#endif

template struct LiteralDatatypeImpl<xsd_long,
                                    capabilities::Logical,
                                    capabilities::NumericStub,
                                    capabilities::Subtype,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry
