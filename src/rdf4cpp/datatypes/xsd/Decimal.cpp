#include "Decimal.hpp"

#include <cmath>
#include <regex>
#include <sstream>
#include <stdexcept>

#include <rdf4cpp/Assert.hpp>
#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_decimal>::cpp_type capabilities::Default<xsd_decimal>::from_string(std::string_view s) {
    // BigDecimal handles the optional sign itself, stripping + here would accept "+-5"
    return cpp_type::parse(s, []<typename T>(std::string_view s) {
        return T(s);
    });
}

template<>
bool capabilities::Default<xsd_decimal>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    return dice::template_library::visit([&](auto const& r) {
        return r.serialize(writer);
    }, value);
}

template<>
bool capabilities::Default<xsd_decimal>::serialize_simplified_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    return dice::template_library::visit([&](auto const& r) {
        auto v = r;
        v.normalize();
        if (v.get_exponent() == 0) {
            return util::to_chars_canonical(v.get_unscaled_value(), writer);
        }
        return v.serialize(writer);
    }, value);
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::add_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::add(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#op.numeric
    // decimal needs overflow protection
    return lhs.apply_op(rhs, []<typename T>(const T& lhs, const T& rhs) {
        return lhs.add_checked(rhs);
    });
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::sub_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::sub(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#op.numeric
    // decimal needs overflow protection
    return lhs.apply_op(rhs, []<typename T>(const T& lhs, const T& rhs) {
        return lhs.sub_checked(rhs);
    });
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::div_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::div(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#func-numeric-divide
    // decimal needs error (and cpp_type is not integral) && overflow protection

    if (rhs == 0) {
        return std::unexpected(DynamicError::DivideByZero);
    }

    return lhs.apply_op(rhs, []<typename T>(const T& lhs, const T& rhs) {
        return lhs.div_checked(rhs, T::default_max_scale_increase, rdf4cpp::util::RoundingMode::Trunc);
    });
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::mul_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::mul(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#op.numeric
    // decimal needs overflow protection
    return lhs.apply_op(rhs, []<typename T>(const T& lhs, const T& rhs) {
        return lhs.mul_checked(rhs);
    });
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::pos(cpp_type const &operand) noexcept {
    return operand;
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::neg(cpp_type const &operand) noexcept {
    return mul(operand, cpp_type{cpp_type::first_type{-1}});
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::abs(cpp_type const &operand) noexcept {
    if (operand >= 0) {
        return operand;
    } else {
        return neg(operand);
    }
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::round_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::round(cpp_type const &operand) noexcept {
    return dice::template_library::visit([](auto const & o) {
        return cpp_type{o.round()};
    }, operand);
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::floor_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::floor(cpp_type const &operand) noexcept {
    return dice::template_library::visit([](auto const & o) {
        return cpp_type{o.floor()};
    }, operand);
}

template<>
std::expected<capabilities::Numeric<xsd_decimal>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::ceil(cpp_type const &operand) noexcept {
    return dice::template_library::visit([](auto const & o) {
        return cpp_type{o.ceil()};
    }, operand);
}

template<>
capabilities::Default<xsd_decimal>::cpp_type capabilities::Numeric<xsd_decimal>::zero_value() noexcept {
    return cpp_type{cpp_type::small_type{0}};
}

template<>
capabilities::Default<xsd_decimal>::cpp_type capabilities::Numeric<xsd_decimal>::one_value() noexcept {
    return cpp_type{cpp_type::small_type{1}};
}

template<>
std::expected<capabilities::Default<xsd_decimal>::cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::from_multiplicity(uint64_t multiplicity) noexcept {
    return cpp_type{cpp_type::small_type{multiplicity}}; // any 64bit integer is exactly representable in BigDecimal
}

template<>
bool capabilities::Logical<xsd_decimal>::effective_boolean_value(cpp_type const &value) noexcept {
    return value != 0;
}

template<>
std::partial_ordering capabilities::Comparable<xsd_decimal>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    return lhs <=> rhs;
}

struct __attribute__((__packed__)) InlinedDecimal {
    static constexpr size_t exponent_size = 10;
    static constexpr size_t unscaled_size = storage::identifier::LiteralID::width - 10;

    uint64_t unscaled_value : unscaled_size;
    uint32_t exponent : exponent_size;
    uint32_t padding : 64 - unscaled_size - exponent_size = 0;
};
static_assert(sizeof(InlinedDecimal) == sizeof(uint64_t));

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_decimal>::try_into_inlined(cpp_type const &value) noexcept {
    if (dice::template_library::holds_alternative<cpp_type::big_type>(value)) {
        return std::nullopt;
    }
    auto const v = dice::template_library::get<cpp_type::small_type>(value);

    auto big_unscaled_value = v.get_unscaled_value();
    auto exponent = v.get_exponent();

    cpp_type::small_type::normalize(big_unscaled_value, exponent);

    int64_t unscaled_value;
    if (rdf4cpp::util::detail::cast_checked<rdf4cpp::util::detail::OverflowMode::Checked>(big_unscaled_value, unscaled_value)) {
        // unscaled value > 64 bit, cannot fit
        return std::nullopt;
    }

    auto const unscaled_inlined = util::try_pack_integral<uint64_t, InlinedDecimal::unscaled_size>(unscaled_value);
    if (!unscaled_inlined.has_value()) {
        return std::nullopt;
    }

    auto const exponent_inlined = util::try_pack_integral<uint32_t, InlinedDecimal::exponent_size>(exponent);
    if (!exponent_inlined.has_value()) {
        return std::nullopt;
    }

    InlinedDecimal const inlined{*unscaled_inlined, *exponent_inlined};
    RDF4CPP_ASSERT(inlined.padding == 0);
    return util::pack<storage::identifier::LiteralID>(inlined);
}

template<>
capabilities::Inlineable<xsd_decimal>::cpp_type capabilities::Inlineable<xsd_decimal>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto const data = util::unpack<InlinedDecimal>(inlined);
    RDF4CPP_ASSERT(data.padding == 0);

    auto const unscaled_value = util::unpack_integral<int64_t, InlinedDecimal::unscaled_size>(data.unscaled_value);
    auto const exponent = util::unpack_integral<uint32_t, InlinedDecimal::exponent_size>(data.exponent);
    return cpp_type{cpp_type::small_type{unscaled_value, exponent}};
}

template<>
template<>
capabilities::Promotable<xsd_decimal>::promoted_cpp_type<0> capabilities::Promotable<xsd_decimal>::promote<0>(cpp_type const &value) noexcept {
    return dice::template_library::visit([](auto const &v) {
        return static_cast<promoted_cpp_type<0>>(v);
    }, value);
}

// BigDecimal(double) throws std::overflow_error for too large and std::invalid_argument for NaN/inf values
template<>
template<>
std::expected<capabilities::Promotable<xsd_decimal>::cpp_type, DynamicError> capabilities::Promotable<xsd_decimal>::demote<0>(promoted_cpp_type<0> const &value) noexcept {
    try {
        return cpp_type{static_cast<cpp_type::small_type>(value)};
    } catch (std::exception const &) {
        return std::unexpected{DynamicError::InvalidValueForCast};
    }
}

template<>
template<>
capabilities::Promotable<xsd_decimal>::promoted_cpp_type<1> capabilities::Promotable<xsd_decimal>::promote<1>(cpp_type const &value) noexcept {
    return dice::template_library::visit([](auto const &v) {
        return static_cast<promoted_cpp_type<1>>(v);
    }, value);
}

template<>
template<>
std::expected<capabilities::Promotable<xsd_decimal>::cpp_type, DynamicError> capabilities::Promotable<xsd_decimal>::demote<1>(promoted_cpp_type<1> const &value) noexcept {
    try {
        return cpp_type{static_cast<cpp_type::small_type>(value)};
    } catch (std::exception const &) {
        return std::unexpected{DynamicError::InvalidValueForCast};
    }
}

#endif

template struct LiteralDatatypeImpl<xsd_decimal,
                                    capabilities::Logical,
                                    capabilities::Numeric,
                                    capabilities::Comparable,
                                    capabilities::Promotable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry
