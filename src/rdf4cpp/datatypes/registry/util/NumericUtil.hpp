#ifndef RDF4CPP_DATATYPES_NUMERICUTIL_HPP
#define RDF4CPP_DATATYPES_NUMERICUTIL_HPP

#include <cstddef>
#include <concepts>

namespace rdf4cpp::datatypes::registry::util {

/**
 * equivalent to static_cast<size_t>(1 + std::log10(value))
 * only exists because the above is not a constexpr in clang
 */
template<typename T>
constexpr size_t log10ceil(T const value) noexcept {
    if (value < 10) {
        return 1;
    }
    return 1 + log10ceil(value / 10);
}

/**
 * @return true iff the value if the integral can fit into the floating point value without precision loss
 */
template<std::floating_point F, std::integral I>
constexpr bool fits_in_floating(I const value) noexcept {
    constexpr auto available_mantissa_bits = std::numeric_limits<F>::digits;
    auto const required_mantissa_bits = std::bit_width(value) - std::countr_zero(value);

    return required_mantissa_bits <= available_mantissa_bits;
}

/**
 * Like std::round, except that it rounds all ties towards +inf
 * In accordance to https://www.w3.org/TR/xpath-functions/#func-round
 */
template<std::floating_point F>
[[nodiscard]] F xsd_round(F const value) noexcept {
    // std::round already rounds every non-tie and every positive tie correctly.
    // Only a negative tie is rounded in the wrong direction (not towards +inf as xsd requires).
    // We need to invert the direction if this happens.
    F const rounded = std::round(value);

    // We are in a tie and rounding away from zero (wrong direction) means: value - rounded == +0.5.
    // Important: value - rounded is exact (https://en.wikipedia.org/wiki/Sterbenz_lemma)
    if (bool const tie_wrong_direction = value - rounded == F{0.5}; tie_wrong_direction) {
        return std::copysign(rounded + 1, value);
    }

    return rounded;
}

} // namespace rdf4cpp::datatypes::registry::util

#endif // RDF4CPP_DATATYPES_NUMERICUTIL_HPP
