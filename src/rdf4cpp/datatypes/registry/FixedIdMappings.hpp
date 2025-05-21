#ifndef RDF4CPP_REGISTRY_FIXED_ID_MAPPINGS_HPP
#define RDF4CPP_REGISTRY_FIXED_ID_MAPPINGS_HPP

#include <algorithm>
#include <string_view>

#include <rdf4cpp/datatypes/registry/util/ConstexprString.hpp>
#include <rdf4cpp/datatypes/registry/util/StaticFlatMap.hpp>
#include <rdf4cpp/storage/identifier/LiteralType.hpp>

namespace rdf4cpp::datatypes::registry {

using storage::identifier::LiteralType;
using storage::identifier::LiteralTypeTag;

inline constexpr util::ConstexprString default_graph_iri{""};
inline constexpr util::ConstexprString rdf_type{"http://www.w3.org/1999/02/22-rdf-syntax-ns#type"};
inline constexpr util::ConstexprString xsd_string{"http://www.w3.org/2001/XMLSchema#string"};
inline constexpr util::ConstexprString rdf_lang_string{"http://www.w3.org/1999/02/22-rdf-syntax-ns#langString"};
inline constexpr util::ConstexprString xsd_boolean{"http://www.w3.org/2001/XMLSchema#boolean"};

inline constexpr util::ConstexprString xsd_base64_binary{"http://www.w3.org/2001/XMLSchema#base64Binary"};
inline constexpr util::ConstexprString xsd_hex_binary{"http://www.w3.org/2001/XMLSchema#hexBinary"};
inline constexpr util::ConstexprString xsd_any_uri{"http://www.w3.org/2001/XMLSchema#anyURI"};

inline constexpr util::ConstexprString xsd_decimal{"http://www.w3.org/2001/XMLSchema#decimal"};
inline constexpr util::ConstexprString xsd_float{"http://www.w3.org/2001/XMLSchema#float"};
inline constexpr util::ConstexprString xsd_double{"http://www.w3.org/2001/XMLSchema#double"};

inline constexpr util::ConstexprString xsd_byte{"http://www.w3.org/2001/XMLSchema#byte"};
inline constexpr util::ConstexprString xsd_short{"http://www.w3.org/2001/XMLSchema#short"};
inline constexpr util::ConstexprString xsd_int{"http://www.w3.org/2001/XMLSchema#int"};
inline constexpr util::ConstexprString xsd_long{"http://www.w3.org/2001/XMLSchema#long"};
inline constexpr util::ConstexprString xsd_integer{"http://www.w3.org/2001/XMLSchema#integer"};

inline constexpr util::ConstexprString xsd_unsigned_byte{"http://www.w3.org/2001/XMLSchema#unsignedByte"};
inline constexpr util::ConstexprString xsd_unsigned_short{"http://www.w3.org/2001/XMLSchema#unsignedShort"};
inline constexpr util::ConstexprString xsd_unsigned_int{"http://www.w3.org/2001/XMLSchema#unsignedInt"};
inline constexpr util::ConstexprString xsd_unsigned_long{"http://www.w3.org/2001/XMLSchema#unsignedLong"};
inline constexpr util::ConstexprString xsd_non_negative_integer{"http://www.w3.org/2001/XMLSchema#nonNegativeInteger"};
inline constexpr util::ConstexprString xsd_positive_integer{"http://www.w3.org/2001/XMLSchema#positiveInteger"};
inline constexpr util::ConstexprString xsd_non_positive_integer{"http://www.w3.org/2001/XMLSchema#nonPositiveInteger"};
inline constexpr util::ConstexprString xsd_negative_integer{"http://www.w3.org/2001/XMLSchema#negativeInteger"};

inline constexpr util::ConstexprString xsd_gYear{"http://www.w3.org/2001/XMLSchema#gYear"};
inline constexpr util::ConstexprString xsd_gMonth{"http://www.w3.org/2001/XMLSchema#gMonth"};
inline constexpr util::ConstexprString xsd_gDay{"http://www.w3.org/2001/XMLSchema#gDay"};
inline constexpr util::ConstexprString xsd_gYearMonth{"http://www.w3.org/2001/XMLSchema#gYearMonth"};
inline constexpr util::ConstexprString xsd_gMonthDay{"http://www.w3.org/2001/XMLSchema#gMonthDay"};
inline constexpr util::ConstexprString xsd_date{"http://www.w3.org/2001/XMLSchema#date"};
inline constexpr util::ConstexprString xsd_time{"http://www.w3.org/2001/XMLSchema#time"};
inline constexpr util::ConstexprString xsd_dateTime{"http://www.w3.org/2001/XMLSchema#dateTime"};
inline constexpr util::ConstexprString xsd_dateTimeStamp{"http://www.w3.org/2001/XMLSchema#dateTimeStamp"};
inline constexpr util::ConstexprString xsd_duration{"http://www.w3.org/2001/XMLSchema#duration"};
inline constexpr util::ConstexprString xsd_dayTimeDuration{"http://www.w3.org/2001/XMLSchema#dayTimeDuration"};
inline constexpr util::ConstexprString xsd_yearMonthDuration{"http://www.w3.org/2001/XMLSchema#yearMonthDuration"};

using ReservedDatatypeIdEntry = std::pair<std::string_view, LiteralType>;

/**
 * All datatypes that have their own LiteralTypes in the node storage and their own fixed id
 * in the registry are listed here.
 *
 * TODO fix issue https://github.com/rdf4cpp/rdf4cpp/issues/243
 */
inline constexpr util::StaticFlatMap reserved_datatype_ids{
        // ReservedDatatypeIdEntry{reserved for dynamic types, LiteralType::from_parts(LiteralTypeTag::Default, 0)},
        ReservedDatatypeIdEntry{default_graph_iri, LiteralType::from_parts(LiteralTypeTag::Default, 1)},
        ReservedDatatypeIdEntry{rdf_type,          LiteralType::from_parts(LiteralTypeTag::Default, 2)},
        ReservedDatatypeIdEntry{xsd_string,        LiteralType::from_parts(LiteralTypeTag::Default, 3)},
        ReservedDatatypeIdEntry{rdf_lang_string,   LiteralType::from_parts(LiteralTypeTag::Default, 4)},
        ReservedDatatypeIdEntry{xsd_boolean,       LiteralType::from_parts(LiteralTypeTag::Default, 5)},
        ReservedDatatypeIdEntry{xsd_base64_binary, LiteralType::from_parts(LiteralTypeTag::Default, 6)},
        ReservedDatatypeIdEntry{xsd_hex_binary,    LiteralType::from_parts(LiteralTypeTag::Default, 7)},
        ReservedDatatypeIdEntry{xsd_gYear,         LiteralType::from_parts(LiteralTypeTag::Default, 8)},
        ReservedDatatypeIdEntry{xsd_gMonth,        LiteralType::from_parts(LiteralTypeTag::Default, 9)},
        ReservedDatatypeIdEntry{xsd_gDay,          LiteralType::from_parts(LiteralTypeTag::Default, 10)},
        ReservedDatatypeIdEntry{xsd_gYearMonth,    LiteralType::from_parts(LiteralTypeTag::Default, 11)},
        ReservedDatatypeIdEntry{xsd_gMonthDay,     LiteralType::from_parts(LiteralTypeTag::Default, 12)},
        ReservedDatatypeIdEntry{xsd_any_uri,     LiteralType::from_parts(LiteralTypeTag::Default, 13)},

        ReservedDatatypeIdEntry{xsd_date,          LiteralType::from_parts(LiteralTypeTag::Timepoint, 0)},
        ReservedDatatypeIdEntry{xsd_time,          LiteralType::from_parts(LiteralTypeTag::Timepoint, 1)},
        ReservedDatatypeIdEntry{xsd_dateTime,      LiteralType::from_parts(LiteralTypeTag::Timepoint, 2)},
        ReservedDatatypeIdEntry{xsd_dateTimeStamp, LiteralType::from_parts(LiteralTypeTag::Timepoint, 3)},

        ReservedDatatypeIdEntry{xsd_duration,          LiteralType::from_parts(LiteralTypeTag::Duration, 0)},
        ReservedDatatypeIdEntry{xsd_dayTimeDuration,   LiteralType::from_parts(LiteralTypeTag::Duration, 1)},
        ReservedDatatypeIdEntry{xsd_yearMonthDuration, LiteralType::from_parts(LiteralTypeTag::Duration, 2)},

        ReservedDatatypeIdEntry{xsd_float,   LiteralType::from_parts(LiteralTypeTag::Numeric, 0)},
        ReservedDatatypeIdEntry{xsd_double,  LiteralType::from_parts(LiteralTypeTag::Numeric, 1)},
        ReservedDatatypeIdEntry{xsd_decimal, LiteralType::from_parts(LiteralTypeTag::Numeric, 2)},

        ReservedDatatypeIdEntry{xsd_integer,              LiteralType::from_parts(LiteralTypeTag::Numeric, 3)},
        ReservedDatatypeIdEntry{xsd_non_positive_integer, LiteralType::from_parts(LiteralTypeTag::Numeric, 4)},
        ReservedDatatypeIdEntry{xsd_long,                 LiteralType::from_parts(LiteralTypeTag::Numeric, 5)},
        ReservedDatatypeIdEntry{xsd_non_negative_integer, LiteralType::from_parts(LiteralTypeTag::Numeric, 6)},
        ReservedDatatypeIdEntry{xsd_negative_integer,     LiteralType::from_parts(LiteralTypeTag::Numeric, 7)},
        ReservedDatatypeIdEntry{xsd_int,                  LiteralType::from_parts(LiteralTypeTag::Numeric, 8)},
        ReservedDatatypeIdEntry{xsd_unsigned_long,        LiteralType::from_parts(LiteralTypeTag::Numeric, 9)},
        ReservedDatatypeIdEntry{xsd_positive_integer,     LiteralType::from_parts(LiteralTypeTag::Numeric, 10)},
        ReservedDatatypeIdEntry{xsd_short,                LiteralType::from_parts(LiteralTypeTag::Numeric, 11)},
        ReservedDatatypeIdEntry{xsd_unsigned_int,         LiteralType::from_parts(LiteralTypeTag::Numeric, 12)},
        ReservedDatatypeIdEntry{xsd_byte,                 LiteralType::from_parts(LiteralTypeTag::Numeric, 13)},
        ReservedDatatypeIdEntry{xsd_unsigned_short,       LiteralType::from_parts(LiteralTypeTag::Numeric, 14)},
        ReservedDatatypeIdEntry{xsd_unsigned_byte,        LiteralType::from_parts(LiteralTypeTag::Numeric, 15)}
};

/**
 * the first id that can be used for dynamic datatypes
 * this is used for two things:
 *      - determine the first iri id that can be dynamically assigned
 *      - determine the size of the static (constant-time-lookup) section of the registry
 */
inline constexpr uint64_t min_dynamic_datatype_id =
#ifndef DOXYGEN_PARSER
        reserved_datatype_ids.size() == 0
                ? 1
                : 1 + std::ranges::max_element(reserved_datatype_ids, {},
                                               [](auto const &entry) {
                                                   return entry.second.to_underlying();
                                               })
                                  ->second.to_underlying();
#else
        48;
#endif

} // rdf4cpp::datatypes::registry

#endif  //RDF4CPP_REGISTRY_FIXED_ID_MAPPINGS_HPP
