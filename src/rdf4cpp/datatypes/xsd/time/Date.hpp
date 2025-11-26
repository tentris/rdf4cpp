#ifndef RDF4CPP_DATE_HPP
#define RDF4CPP_DATE_HPP

#include <chrono>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <rdf4cpp/datatypes/xsd/time/DateTime.hpp>
#include <dice/hash.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_date> {
    using cpp_datatype = std::pair<YearMonthDay, rdf4cpp::OptionalTimezone>;
};
template<>
struct DatatypePromotionMapping<xsd_date> {
    using promoted = xsd::DateTime;
};
template<>
struct DatatypeTimepointDurationOperandMapping<xsd_date> {
    using duration_type = xsd::Duration;
};
template<>
struct DatatypeTimepointSubResultMapping<xsd_date> {
    using op_result = xsd::DayTimeDuration;
};


template<>
capabilities::Default<xsd_date>::cpp_type capabilities::Default<xsd_date>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_date>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;


template<>
std::partial_ordering capabilities::Comparable<xsd_date>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_date>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_date>::cpp_type capabilities::Inlineable<xsd_date>::from_inlined(storage::identifier::LiteralID inlined) noexcept;

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_date>::promoted_cpp_type<0>, DynamicError>
capabilities::Promotable<xsd_date>::promote<0>(cpp_type const &value) noexcept;

template<>
template<>
nonstd::expected<capabilities::Promotable<xsd_date>::cpp_type, DynamicError> capabilities::Promotable<xsd_date>::demote<0>(promoted_cpp_type<0> const &value) noexcept;

template<>
nonstd::expected<capabilities::Timepoint<xsd_date>::timepoint_sub_result_cpp_type, DynamicError>
capabilities::Timepoint<xsd_date>::timepoint_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
nonstd::expected<capabilities::Timepoint<xsd_date>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_date>::timepoint_duration_add(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept;

template<>
nonstd::expected<capabilities::Timepoint<xsd_date>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_date>::timepoint_duration_sub(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept;

#endif

extern template struct LiteralDatatypeImpl<xsd_date,
                                           capabilities::Timepoint,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable,
                                           capabilities::Promotable>;

}  // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

struct Date : registry::LiteralDatatypeImpl<registry::xsd_date,
                                            registry::capabilities::Timepoint,
                                            registry::capabilities::Comparable,
                                            registry::capabilities::FixedId,
                                            registry::capabilities::Inlineable,
                                            registry::capabilities::Promotable> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::Date const xsd_Date_instance;

}  // namespace rdf4cpp::datatypes::registry::instantiation_detail

#ifndef DOXYGEN_PARSER
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, std::chrono::year_month_day> {
    static size_t dice_hash(std::chrono::year_month_day const &x) noexcept {
        auto year = static_cast<int>(x.year());
        auto month = static_cast<unsigned int>(x.month());
        auto day = static_cast<unsigned int>(x.day());
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::tie(year, month, day));
    }
};
#endif

#endif  //RDF4CPP_DATE_HPP
