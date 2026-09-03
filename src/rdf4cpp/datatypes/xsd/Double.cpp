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

struct DoubleLayout {
    uint64_t sign : 1;
    uint64_t exponent : 11;
    uint64_t mantissa : 52;

    static_assert(1 + 11 + 52 == 64);
};
static_assert(sizeof(DoubleLayout) == sizeof(double));

struct __attribute__((packed)) CompressedDoubleLayout {
    uint64_t sign : 1;
    uint64_t exponent : 6;
    uint64_t mantissa : 43;

    static_assert(1 + 6 + 43 == storage::identifier::LiteralID::width);

    static std::optional<CompressedDoubleLayout> try_compress(DoubleLayout const dbl) noexcept {
        return std::nullopt; // TODO
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

    return storage::identifier::LiteralID{util::pack<storage::identifier::LiteralID>(shortened)};
}

template<>
capabilities::Inlineable<xsd_double>::cpp_type capabilities::Inlineable<xsd_double>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto const shortened = util::unpack<CompressedDoubleLayout>(inlined);
    return std::bit_cast<cpp_type>(DoubleLayout{.sign = shortened.sign,
                                                .exponent = shortened.exponent,
                                                .mantissa = shortened.mantissa});
}
#endif

template struct LiteralDatatypeImpl<xsd_double,
                                    capabilities::Logical,
                                    capabilities::Numeric,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry
