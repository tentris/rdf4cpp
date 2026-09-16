#include "Float.hpp"
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

#include <cmath>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_float>::cpp_type capabilities::Default<xsd_float>::from_string(std::string_view s) {
    return util::from_chars<cpp_type, identifier>(s);
}

template<>
bool capabilities::Default<xsd_float>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts const writer) noexcept {
    return util::to_chars_canonical(value, writer);
}

template<>
bool capabilities::Default<xsd_float>::serialize_simplified_string(cpp_type const &value, writer::BufWriterParts const writer) noexcept {
    return util::to_chars_simplified(value, writer);
}

template<>
bool capabilities::Logical<xsd_float>::effective_boolean_value(cpp_type const &value) noexcept {
    return !std::isnan(value) && value != 0.f;
}

template<>
nonstd::expected<capabilities::Numeric<xsd_float>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_float>::abs(cpp_type const &operand) noexcept {
    return std::abs(operand);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_float>::round_result_cpp_type, DynamicError> capabilities::Numeric<xsd_float>::round(cpp_type const &operand) noexcept {
    return std::round(operand);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_float>::floor_result_cpp_type, DynamicError> capabilities::Numeric<xsd_float>::floor(cpp_type const &operand) noexcept {
    return std::floor(operand);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_float>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<xsd_float>::ceil(cpp_type const &operand) noexcept {
    return std::ceil(operand);
}

template<>
nonstd::expected<capabilities::Default<xsd_float>::cpp_type, DynamicError> capabilities::Numeric<xsd_float>::from_multiplicity(uint64_t multiplicity) noexcept {
    constexpr auto available_mantissa_bits = std::numeric_limits<cpp_type>::digits;
    auto const required_mantissa_bits = std::bit_width(multiplicity) - std::countr_zero(multiplicity);

    if (required_mantissa_bits > available_mantissa_bits) [[unlikely]] {
        // doesn't fit
        return nonstd::make_unexpected(DynamicError::InvalidValueForCast);
    }

    return static_cast<cpp_type>(multiplicity);
}

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_float>::try_into_inlined(cpp_type const &value) noexcept {
    return util::pack<storage::identifier::LiteralID>(value);
}

template<>
capabilities::Inlineable<xsd_float>::cpp_type capabilities::Inlineable<xsd_float>::from_inlined(storage::identifier::LiteralID const inlined) noexcept {
    return util::unpack<cpp_type>(inlined);
}
#endif

template struct LiteralDatatypeImpl<xsd_float,
                                    capabilities::Logical,
                                    capabilities::Numeric,
                                    capabilities::Comparable,
                                    capabilities::Promotable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry
