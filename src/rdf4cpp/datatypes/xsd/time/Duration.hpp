#ifndef RDF4CPP_DURATION_HPP
#define RDF4CPP_DURATION_HPP

#include <chrono>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <dice/hash.hpp>
#include <rdf4cpp/datatypes/xsd/Decimal.hpp>
#include <rdf4cpp/datatypes/xsd/Double.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_duration> {
    using cpp_datatype = std::pair<std::chrono::months, std::chrono::nanoseconds>;
};
template<>
struct DatatypeDurationScalarMapping<xsd_duration> {
    using scalar_type = xsd::Double;
};
template<>
struct DatatypeDurationDivResultMapping<xsd_duration> {
    using op_result = xsd::Decimal;
};

template<>
capabilities::Default<xsd_duration>::cpp_type capabilities::Default<xsd_duration>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_duration>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_duration>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_duration>::cpp_type capabilities::Inlineable<xsd_duration>::from_inlined(storage::identifier::LiteralID inlined) noexcept;

template<>
std::partial_ordering capabilities::Comparable<xsd_duration>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_duration>::cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_add(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_duration>::cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_duration>::duration_div_result_cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_div(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Duration<xsd_duration>::cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_scalar_mul(cpp_type const &dur, duration_scalar_cpp_type const &scalar) noexcept;

template<>
std::expected<capabilities::Duration<xsd_duration>::cpp_type, DynamicError>
capabilities::Duration<xsd_duration>::duration_scalar_div(cpp_type const &dur, duration_scalar_cpp_type const &scalar) noexcept;

#endif

extern template struct LiteralDatatypeImpl<xsd_duration,
                                           capabilities::Duration,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

struct Duration : registry::LiteralDatatypeImpl<registry::xsd_duration,
                                                registry::capabilities::Duration,
                                                registry::capabilities::Comparable,
                                                registry::capabilities::FixedId,
                                                registry::capabilities::Inlineable> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::Duration const xsd_Duration_instance;

}  // namespace rdf4cpp::datatypes::registry::instantiation_detail

#ifndef DOXYGEN_PARSER
template<typename Policy, typename A, typename B>
struct dice::hash::dice_hash_overload<Policy, std::chrono::duration<A, B>> {
    static size_t dice_hash(std::chrono::duration<A, B> const &x) noexcept {
        auto m = x.count();
        return dice::hash::dice_hash_templates<Policy>::dice_hash(m);
    }
};
#endif

#endif  //RDF4CPP_DURATION_HPP
