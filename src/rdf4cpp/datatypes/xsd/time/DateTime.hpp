#ifndef RDF4CPP_DATETIME_HPP
#define RDF4CPP_DATETIME_HPP

#include <chrono>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <dice/hash.hpp>
#include <rdf4cpp/datatypes/xsd/time/DayTimeDuration.hpp>
#include <rdf4cpp/datatypes/xsd/time/Duration.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_dateTime> {
    using cpp_datatype = std::pair<rdf4cpp::TimePoint, rdf4cpp::OptionalTimezone>;
};
template<>
struct DatatypeTimepointDurationOperandMapping<xsd_dateTime> {
    using duration_type = xsd::Duration;
};
template<>
struct DatatypeTimepointSubResultMapping<xsd_dateTime> {
    using op_result = xsd::DayTimeDuration;
};

template<>
capabilities::Default<xsd_dateTime>::cpp_type capabilities::Default<xsd_dateTime>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_dateTime>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_dateTime>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_dateTime>::cpp_type capabilities::Inlineable<xsd_dateTime>::from_inlined(storage::identifier::LiteralID inlined) noexcept;

template<>
std::partial_ordering capabilities::Comparable<xsd_dateTime>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Timepoint<xsd_dateTime>::timepoint_sub_result_cpp_type, DynamicError>
capabilities::Timepoint<xsd_dateTime>::timepoint_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::expected<capabilities::Timepoint<xsd_dateTime>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_dateTime>::timepoint_duration_add(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept;

template<>
std::expected<capabilities::Timepoint<xsd_dateTime>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_dateTime>::timepoint_duration_sub(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept;

#endif

extern template struct LiteralDatatypeImpl<xsd_dateTime,
                                           capabilities::Timepoint,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

struct DateTime : registry::LiteralDatatypeImpl<registry::xsd_dateTime,
                                                registry::capabilities::Timepoint,
                                                registry::capabilities::Comparable,
                                                registry::capabilities::FixedId,
                                                registry::capabilities::Inlineable> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::DateTime const xsd_DateTime_instance;

}  // namespace rdf4cpp::datatypes::registry::instantiation_detail

#ifndef DOXYGEN_PARSER
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::TimePoint> {
    static size_t dice_hash(rdf4cpp::TimePoint const &x) noexcept {
        auto tp = x.time_since_epoch().count();
        return dice::hash::dice_hash_templates<Policy>::dice_hash(tp);
    }
};
#endif

#endif  //RDF4CPP_DATETIME_HPP
