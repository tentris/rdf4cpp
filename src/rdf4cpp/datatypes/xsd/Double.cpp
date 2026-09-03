#include "Double.hpp"
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

#include <dragonbox/dragonbox.h>

#include <cmath>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_double>::cpp_type capabilities::Default<xsd_double>::from_string(std::string_view s) {
    return util::from_chars<cpp_type, identifier>(s);
}

template<>
bool capabilities::Default<xsd_double>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    return util::to_chars_canonical(value, writer);
}

template<>
bool capabilities::Default<xsd_double>::serialize_simplified_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    return util::to_chars_simplified(value, writer);
}

template<>
bool capabilities::Logical<xsd_double>::effective_boolean_value(cpp_type const &value) noexcept {
    return !std::isnan(value) && value != 0.0;
}

template<>
nonstd::expected<capabilities::Numeric<xsd_double>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_double>::abs(cpp_type const &operand) noexcept {
    return std::abs(operand);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_double>::round_result_cpp_type, DynamicError> capabilities::Numeric<xsd_double>::round(cpp_type const &operand) noexcept {
    return std::round(operand);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_double>::floor_result_cpp_type, DynamicError> capabilities::Numeric<xsd_double>::floor(cpp_type const &operand) noexcept {
    return std::floor(operand);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_double>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<xsd_double>::ceil(cpp_type const &operand) noexcept {
    return std::ceil(operand);
}

// A double stored as its shortest round-tripping decimal, i.e. as significand * 10^exponent.
struct __attribute__((packed)) DecimalDoubleLayout {
    static constexpr size_t exponent_width = 6;
    static constexpr size_t significand_width = storage::identifier::LiteralID::width - exponent_width - 1;

    static constexpr int max_exponent = 22; // 10^n is exactly representable as a double up to n == 22
    static_assert(max_exponent < 1uz << (exponent_width - 1));

    static constexpr auto pow10 = [] {
        std::array<double, max_exponent + 1> powers{};
        std::ranges::generate(powers, [power = 1.0] mutable { return std::exchange(power, power * 10); });
        return powers;
    }();
    static_assert(pow10[max_exponent] == 1e22);  // repeated multiplication stays exact this far

    uint64_t significand : significand_width;  // the decimal digits as a binary integer
    int64_t exponent : exponent_width;         // power of ten (signed)
    uint64_t sign : 1;

    // explicit padding so that the bits above a LiteralID are guaranteed to be zero when packing
    [[maybe_unused]] uint64_t pad : 64 - storage::identifier::LiteralID::width = 0;

    [[nodiscard]] static constexpr bool can_fit(auto decimal) noexcept {
        return decimal.significand >> significand_width == 0
               && decimal.exponent >= -max_exponent
               && decimal.exponent <= max_exponent;
    }
};
static_assert(sizeof(DecimalDoubleLayout) == sizeof(uint64_t));

template<>
capabilities::Inlineable<xsd_double>::cpp_type capabilities::Inlineable<xsd_double>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto const decimal = util::unpack<DecimalDoubleLayout>(inlined);

    auto const significand = static_cast<cpp_type>(decimal.significand);

    // dragonbox promised that the nearest double to significand * 10^exponent is the original value, so
    // we only have to compute that nearest double: one IEEE operation on two exactly representable
    // operands (integer < 2^53, 10^n where n < max_exponent) rounds once and lands on it.
    // Multiplying by 10^-n would round twice, since that is not representable.
    auto const value = [&] {
        if (decimal.exponent < 0) {
            return significand / DecimalDoubleLayout::pow10[-decimal.exponent];
        }

        return significand * DecimalDoubleLayout::pow10[decimal.exponent];
    }();

    return decimal.sign ? -value : value;
}

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_double>::try_into_inlined(cpp_type const &value) noexcept {
    if (!std::isfinite(value)) {
        return std::nullopt;  // inf and nan have no decimal representation
    }

    DecimalDoubleLayout decimal{};
    if (value == 0) {
        // dragonbox does not support zero input
        decimal.sign = std::signbit(value);
    } else {
        // shortest representation that still round-trips
        auto const dec_float = jkj::dragonbox::to_decimal(value);

        if (!DecimalDoubleLayout::can_fit(dec_float)) {
            return std::nullopt;
        }

        decimal.significand = dec_float.significand;
        decimal.exponent = dec_float.exponent;
        decimal.sign = dec_float.is_negative;
    }

    return util::pack<storage::identifier::LiteralID>(decimal);
}
#endif

template struct LiteralDatatypeImpl<xsd_double,
                                    capabilities::Logical,
                                    capabilities::Numeric,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry
