#include "NonNegativeInteger.hpp"

#include <stdexcept>

#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_non_negative_integer>::cpp_type capabilities::Default<xsd_non_negative_integer>::from_string(std::string_view s) {
    cpp_type ret = Default<xsd_integer>::from_string(s);

    if (ret < 0) {
        throw InvalidNode{std::format("{} parsing error: found negative value", identifier)};
    }

    return ret;
}

template<>
bool capabilities::Default<xsd_non_negative_integer>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    return Default<xsd_integer>::serialize_canonical_string(value, writer);
}

template<>
bool capabilities::Logical<xsd_non_negative_integer>::effective_boolean_value(cpp_type const &value) noexcept {
    return value != 0;
}

template<>
std::partial_ordering capabilities::Comparable<xsd_non_negative_integer>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    return lhs <=> rhs;
}

template<>
template<>
std::expected<capabilities::Default<xsd_non_negative_integer>::cpp_type, DynamicError> capabilities::Subtype<xsd_non_negative_integer>::from_supertype<0>(super_cpp_type<0> const &value) noexcept {
    if (value < 0) {
        return std::unexpected(DynamicError::InvalidValueForCast);
    }

    return value;
}

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_non_negative_integer>::try_into_inlined(cpp_type const &value) noexcept {
    if (dice::template_library::holds_alternative<cpp_type::big_type>(value)) {
        return std::nullopt;
    }
    auto const v = dice::template_library::get<cpp_type::small_type>(value);
    if (v >= (uint64_t{1} << storage::identifier::LiteralID::width)) {
        return std::nullopt;
    }

    return util::try_pack_integral<storage::identifier::LiteralID>(static_cast<uint64_t>(v));
}

template<>
capabilities::Inlineable<xsd_non_negative_integer>::cpp_type capabilities::Inlineable<xsd_non_negative_integer>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    return cpp_type{cpp_type::small_type{util::unpack_integral<uint64_t>(inlined)}};
}
#endif

template struct LiteralDatatypeImpl<xsd_non_negative_integer,
                                    capabilities::Logical,
                                    capabilities::NumericStub,
                                    capabilities::Comparable,
                                    capabilities::Subtype,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry
