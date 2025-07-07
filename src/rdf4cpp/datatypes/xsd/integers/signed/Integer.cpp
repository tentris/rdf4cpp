#include "Integer.hpp"

#include <stdexcept>

#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_integer>::cpp_type capabilities::Default<xsd_integer>::from_string(std::string_view s) {
    return util::from_chars<cpp_type, identifier>(s);
}

template<>
bool capabilities::Default<xsd_integer>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    return rdf4cpp::util::to_chars_canonical(value, writer);
}

template<>
bool capabilities::Logical<xsd_integer>::effective_boolean_value(cpp_type const &value) noexcept {
    return value != 0;
}

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::add_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::add(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#op.numeric
    // needs overflow protection
    cpp_type r;
    if (rdf4cpp::util::detail::add_checked<rdf4cpp::util::detail::OverflowMode::Checked>(lhs, rhs, r)) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
    return r;
}

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::sub_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::sub(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#op.numeric
    // needs overflow protection
    cpp_type r;
    if (rdf4cpp::util::detail::sub_checked<rdf4cpp::util::detail::OverflowMode::Checked>(lhs, rhs, r)) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
    return r;
}

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::div_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::div(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    if (rhs == 0) {
        return nonstd::make_unexpected(DynamicError::DivideByZero);
    }

    // https://www.w3.org/TR/xpath-functions/#func-numeric-divide
    // integer needs to return decimal on division
    return static_cast<div_result_cpp_type>(lhs) / static_cast<div_result_cpp_type>(rhs);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::mul_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::mul(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#op.numeric
    // decimal needs overflow protection
    cpp_type r;
    if (rdf4cpp::util::detail::mul_checked<rdf4cpp::util::detail::OverflowMode::Checked>(lhs, rhs, r)) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
    return r;
}

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::neg(cpp_type const &operand) noexcept {
    cpp_type r;
    if (rdf4cpp::util::detail::mul_checked<rdf4cpp::util::detail::OverflowMode::Checked>(operand, cpp_type{-1}, r)) {
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
    }
    return r;
}

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::abs(cpp_type const &operand) noexcept {
    if (operand >= 0) {
        return operand;
    } else {
        return neg(operand);
    }
}

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::round_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::round(cpp_type const &operand) noexcept {
    return operand;
}

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::floor_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::floor(cpp_type const &operand) noexcept {
    return operand;
}

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::ceil(cpp_type const &operand) noexcept {
    return operand;
}

template<>
std::partial_ordering capabilities::Comparable<xsd_integer>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    if (lhs < rhs) {
        return std::partial_ordering::less;
    } else if (rhs < lhs) {
        return std::partial_ordering::greater;
    } else {
        return std::partial_ordering::equivalent;
    }
}

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_integer>::try_into_inlined(cpp_type const &value) noexcept {
    if (auto const boundary = 1l << (storage::identifier::LiteralID::width - 1); value >= boundary || value < -boundary) [[unlikely]] {
        return std::nullopt;
    }

    return util::try_pack_integral<storage::identifier::LiteralID>(static_cast<int64_t>(value));
}

template<>
capabilities::Inlineable<xsd_integer>::cpp_type capabilities::Inlineable<xsd_integer>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    return cpp_type{util::unpack_integral<int64_t>(inlined)};
}
#endif

template struct LiteralDatatypeImpl<xsd_integer,
                                    capabilities::Logical,
                                    capabilities::Numeric,
                                    capabilities::Comparable,
                                    capabilities::Subtype,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry
