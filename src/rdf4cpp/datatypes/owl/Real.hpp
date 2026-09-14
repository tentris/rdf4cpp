#ifndef RDF4CPP_OWL_REAL_HPP
#define RDF4CPP_OWL_REAL_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>

#include <boost/multiprecision/cpp_bin_float.hpp>

namespace rdf4cpp::datatypes::registry {

inline constexpr util::ConstexprString owl_real{"http://www.w3.org/2002/07/owl#real"};

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<owl_real> {
    using cpp_datatype = boost::multiprecision::cpp_bin_float_quad; // IEEE quad float
};

template<>
capabilities::Default<owl_real>::cpp_type capabilities::Default<owl_real>::from_string(std::string_view s);

template<>
bool capabilities::Logical<owl_real>::effective_boolean_value(cpp_type const &value) noexcept;

template<>
std::partial_ordering capabilities::Comparable<owl_real>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
nonstd::expected<capabilities::Numeric<owl_real>::abs_result_cpp_type, DynamicError> capabilities::Numeric<owl_real>::abs(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<owl_real>::round_result_cpp_type, DynamicError> capabilities::Numeric<owl_real>::round(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<owl_real>::floor_result_cpp_type, DynamicError> capabilities::Numeric<owl_real>::floor(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<owl_real>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<owl_real>::ceil(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Default<owl_real>::cpp_type, DynamicError> capabilities::Numeric<owl_real>::from_multiplicity(uint64_t multiplicity) noexcept;

#endif

extern template struct LiteralDatatypeImpl<owl_real,
                                           capabilities::Logical,
                                           capabilities::Numeric,
                                           capabilities::Comparable>;

} // rdf4cpp::datatypes::registry


namespace rdf4cpp::datatypes::owl {

struct Real : registry::LiteralDatatypeImpl<registry::owl_real,
                                            registry::capabilities::Logical,
                                            registry::capabilities::Numeric,
                                            registry::capabilities::Comparable> {};

} // rdf4cpp::datatypes::owl


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline owl::Real const owl_real_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_OWL_REAL_HPP
