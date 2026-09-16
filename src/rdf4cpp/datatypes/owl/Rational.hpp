#ifndef RDF4CPP_OWL_RATIONAL_HPP
#define RDF4CPP_OWL_RATIONAL_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/datatypes/xsd/integers/signed/Integer.hpp>

#include <boost/multiprecision/cpp_int.hpp>

namespace rdf4cpp::datatypes::registry {

inline constexpr util::ConstexprString owl_rational{"http://www.w3.org/2002/07/owl#rational"};

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<owl_rational> {
    using cpp_datatype = boost::multiprecision::cpp_rational;
};

template<>
capabilities::Default<owl_rational>::cpp_type capabilities::Default<owl_rational>::from_string(std::string_view s);

template<>
bool capabilities::Default<owl_rational>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
bool capabilities::Logical<owl_rational>::effective_boolean_value(cpp_type const &value) noexcept;

template<>
std::partial_ordering capabilities::Comparable<owl_rational>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
nonstd::expected<capabilities::Numeric<owl_rational>::abs_result_cpp_type, DynamicError> capabilities::Numeric<owl_rational>::abs(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<owl_rational>::round_result_cpp_type, DynamicError> capabilities::Numeric<owl_rational>::round(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<owl_rational>::floor_result_cpp_type, DynamicError> capabilities::Numeric<owl_rational>::floor(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<owl_rational>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<owl_rational>::ceil(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Default<owl_rational>::cpp_type, DynamicError> capabilities::Numeric<owl_rational>::from_multiplicity(uint64_t multiplicity) noexcept;

#endif

extern template struct LiteralDatatypeImpl<owl_rational,
                                           capabilities::Logical,
                                           capabilities::Numeric,
                                           capabilities::Comparable>;

} // rdf4cpp::datatypes::registry


namespace rdf4cpp::datatypes::owl {

struct Rational : registry::LiteralDatatypeImpl<registry::owl_rational,
                                                registry::capabilities::Logical,
                                                registry::capabilities::Numeric,
                                                registry::capabilities::Comparable> {};

} // rdf4cpp::datatypes::owl


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline owl::Rational const owl_rational_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_OWL_RATIONAL_HPP
