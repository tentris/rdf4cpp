#ifndef RDF4CPP_MONTH_HPP
#define RDF4CPP_MONTH_HPP

#include <chrono>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <rdf4cpp/datatypes/xsd/time/Date.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_gMonth> {
    using cpp_datatype = std::pair<std::chrono::month, rdf4cpp::OptionalTimezone>;
};
template<>
struct DatatypePromotionMapping<xsd_gMonth> {
    using promoted = xsd::Date;
};


template<>
capabilities::Default<xsd_gMonth>::cpp_type capabilities::Default<xsd_gMonth>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_gMonth>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;



template<>
std::partial_ordering capabilities::Comparable<xsd_gMonth>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_gMonth>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_gMonth>::cpp_type capabilities::Inlineable<xsd_gMonth>::from_inlined(storage::identifier::LiteralID inlined) noexcept;

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gMonth>::promoted_cpp_type<0>, DynamicError>
capabilities::Promotable<xsd_gMonth>::promote<0>(cpp_type const &value) noexcept;

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_gMonth>::cpp_type, DynamicError> capabilities::Promotable<xsd_gMonth>::demote<0>(promoted_cpp_type<0> const &value) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_gMonth,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable,
                                           capabilities::Promotable>;

}

namespace rdf4cpp::datatypes::xsd {

struct GMonth : registry::LiteralDatatypeImpl<registry::xsd_gMonth,
                                              registry::capabilities::Comparable,
                                              registry::capabilities::FixedId,
                                              registry::capabilities::Inlineable,
                                              registry::capabilities::Promotable> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::GMonth const xsd_gMonth_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#ifndef DOXYGEN_PARSER
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, std::chrono::month> {
    static size_t dice_hash(std::chrono::month const &x) noexcept {
        auto month = static_cast<unsigned int>(x);
        return dice::hash::dice_hash_templates<Policy>::dice_hash(month);
    }
};
#endif

#endif  //RDF4CPP_MONTH_HPP
