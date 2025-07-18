#ifndef RDF4CPP_RDF_LANGSTRING_HPP
#define RDF4CPP_RDF_LANGSTRING_HPP

#include <bit>
#include <optional>
#include <string_view>
#include <vector>

#include <dice/hash.hpp>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::datatypes::registry {
struct LangStringRepr {
    std::string_view lexical_form;
    std::string_view language_tag;

    auto operator<=>(LangStringRepr const &other) const noexcept = default;
};

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<rdf_lang_string> {
    using cpp_datatype = LangStringRepr;
};

template<>
inline capabilities::Default<rdf_lang_string>::cpp_type capabilities::Default<rdf_lang_string>::from_string(std::string_view) {
    // dummy implementation, actual implementation in Literal
    RDF4CPP_UNREACHABLE;
}

template<>
inline bool capabilities::Default<rdf_lang_string>::serialize_canonical_string(cpp_type const &, writer::BufWriterParts) noexcept {
    // dummy implementation, actual implementation in Literal
    RDF4CPP_UNREACHABLE;
}

template<>
inline std::optional<storage::identifier::LiteralID> capabilities::Inlineable<rdf_lang_string>::try_into_inlined(cpp_type const &) noexcept {
    // dummy implementation, actual implementation in Literal
    RDF4CPP_UNREACHABLE;
}

template<>
inline capabilities::Inlineable<rdf_lang_string>::cpp_type capabilities::Inlineable<rdf_lang_string>::from_inlined(storage::identifier::LiteralID) noexcept {
    // dummy implementation, actual implementation in Literal
    RDF4CPP_UNREACHABLE;
}
#endif

extern template struct LiteralDatatypeImpl<rdf_lang_string,
                                           capabilities::Comparable,
                                           capabilities::FixedId>;

}  // namespace rdf4cpp::datatypes::registry


namespace rdf4cpp::datatypes::rdf {

struct LangString : registry::LiteralDatatypeImpl<registry::rdf_lang_string,
                                                  registry::capabilities::Comparable,
                                                  registry::capabilities::FixedId,
                                                  registry::capabilities::Inlineable> {};

}  // namespace rdf4cpp::datatypes::rdf

namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline rdf::LangString const rdf_lang_string_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#ifndef DOXYGEN_PARSER
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::datatypes::registry::LangStringRepr> {
    static size_t dice_hash(rdf4cpp::datatypes::registry::LangStringRepr const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::tie(x.lexical_form, x.language_tag));
    }
};

template<>
struct std::hash<rdf4cpp::datatypes::registry::LangStringRepr> {
    size_t operator()(rdf4cpp::datatypes::registry::LangStringRepr const &x) const noexcept {
        return dice::hash::dice_hash_templates<dice::hash::Policies::wyhash>::dice_hash(x);
    }
};
#endif

#endif  //RDF4CPP_RDF_LANGSTRING_HPP
