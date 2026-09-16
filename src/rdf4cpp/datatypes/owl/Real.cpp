#include "Real.hpp"
#include <rdf4cpp/InvalidNode.hpp>

#include <boost/numeric/conversion/cast.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<owl_real>::cpp_type capabilities::Default<owl_real>::from_string(std::string_view s) {
    try {
        return cpp_type{s};
    } catch (std::runtime_error const &e) {
        throw InvalidNode{std::format("{} parsing error: {}", identifier, e.what())};
    }
}

template<>
bool capabilities::Logical<owl_real>::effective_boolean_value(cpp_type const &value) noexcept {
    return value != 0;
}

template<>
std::partial_ordering capabilities::Comparable<owl_real>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    if (lhs < rhs) {
        return std::partial_ordering::less;
    } else if (rhs < lhs) {
        return std::partial_ordering::greater;
    } else {
        return std::partial_ordering::equivalent;
    }
}

template<>
nonstd::expected<capabilities::Numeric<owl_real>::abs_result_cpp_type, DynamicError> capabilities::Numeric<owl_real>::abs(cpp_type const &operand) noexcept {
    return boost::multiprecision::abs(operand);
}

template<>
nonstd::expected<capabilities::Numeric<owl_real>::round_result_cpp_type, DynamicError> capabilities::Numeric<owl_real>::round(cpp_type const &operand) noexcept {
    return boost::multiprecision::round(operand);
}

template<>
nonstd::expected<capabilities::Numeric<owl_real>::floor_result_cpp_type, DynamicError> capabilities::Numeric<owl_real>::floor(cpp_type const &operand) noexcept {
    return boost::multiprecision::floor(operand);
}

template<>
nonstd::expected<capabilities::Numeric<owl_real>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<owl_real>::ceil(cpp_type const &operand) noexcept {
    return boost::multiprecision::ceil(operand);
}

template<>
nonstd::expected<capabilities::Default<owl_real>::cpp_type, DynamicError> capabilities::Numeric<owl_real>::from_multiplicity(uint64_t multiplicity) noexcept {
    return multiplicity; // any 64bit integer is exactly representable in a IEEE quad float (113 bit mantissa)
}

#endif

template struct LiteralDatatypeImpl<owl_real,
                                    capabilities::Logical,
                                    capabilities::Numeric,
                                    capabilities::Comparable>;

} // rdf4cpp::datatypes::registry
