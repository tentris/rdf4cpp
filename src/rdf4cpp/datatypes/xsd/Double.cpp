#include "Double.hpp"
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

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

// both layouts below declare their fields from least to most significant bit
// (see https://en.wikipedia.org/wiki/Double-precision_floating-point_format)
static_assert(std::endian::native == std::endian::little, "This does not work on big endian");

struct DoubleLayout {
    uint64_t mantissa : 52; // fraction, the leading 1 is implicit
    uint64_t exponent : 11; // biased by 1023, 0 means zero/subnormal
    uint64_t sign : 1;

    static_assert(52 + 11 + 1 == 64);
};
static_assert(sizeof(DoubleLayout) == sizeof(double));

// A DoubleLayout squeezed into the bits of a LiteralID. Lossless only for doubles that need at most
// 44 significant bits and whose magnitude is in [2^-31, 2^32) (plus zero and subnormals);
// everything else cannot be inlined.
struct __attribute__((packed)) CompressedDoubleLayout {
    uint64_t mantissa : 43; // least significant bits dropped
    uint64_t exponent : 6;  // re-biased into a window around 1.0, see exponent_offset
    uint64_t sign : 1;      // kept as is

    static_assert(43 + 6 + 1 == storage::identifier::LiteralID::width);

    // number of least significant mantissa bits that do not fit; they must be unused
    static constexpr uint64_t dropped_mantissa_bits = 52 - 43;

    // exponent 0 (zero and subnormals) is compressed to 0, the biased exponents
    // [exponent_offset + 1, exponent_offset + 63] (i.e. unbiased [-31, 31]) to themselves minus exponent_offset
    static constexpr uint64_t exponent_offset = 1023 - 32;

    static std::optional<CompressedDoubleLayout> try_compress(DoubleLayout const dbl) noexcept {
        if ((dbl.mantissa & ((1UL << dropped_mantissa_bits) - 1)) != 0) {
            return std::nullopt; // needs more precision than the shortened mantissa provides
        }

        if (dbl.exponent != 0 && (dbl.exponent <= exponent_offset || dbl.exponent > exponent_offset + 63)) {
            return std::nullopt; // magnitude outside the window (this also excludes inf and nan)
        }

        return CompressedDoubleLayout{.mantissa = dbl.mantissa >> dropped_mantissa_bits,
                                      .exponent = dbl.exponent == 0 ? 0 : dbl.exponent - exponent_offset,
                                      .sign = dbl.sign};
    }
};
static_assert(sizeof(CompressedDoubleLayout) == sizeof(storage::identifier::LiteralID));

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_double>::try_into_inlined(cpp_type const &value) noexcept {
    auto const packed = std::bit_cast<DoubleLayout>(value);
    auto const shortened = CompressedDoubleLayout::try_compress(packed);

    if (!shortened.has_value()) {
        return std::nullopt;
    }

    return util::pack<storage::identifier::LiteralID>(*shortened);
}

template<>
capabilities::Inlineable<xsd_double>::cpp_type capabilities::Inlineable<xsd_double>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto const shortened = util::unpack<CompressedDoubleLayout>(inlined);
    return std::bit_cast<cpp_type>(DoubleLayout{.mantissa = shortened.mantissa << CompressedDoubleLayout::dropped_mantissa_bits,
                                                .exponent = shortened.exponent == 0
                                                                    ? 0
                                                                    : shortened.exponent + CompressedDoubleLayout::exponent_offset,
                                                .sign = shortened.sign});
}
#endif

template struct LiteralDatatypeImpl<xsd_double,
                                    capabilities::Logical,
                                    capabilities::Numeric,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry
