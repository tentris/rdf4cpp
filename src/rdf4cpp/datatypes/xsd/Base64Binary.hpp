#ifndef RDF4CPP_XSD_BASE64BINARY_HPP
#define RDF4CPP_XSD_BASE64BINARY_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>

#include <dice/hash.hpp>

#include <cstddef>
#include <vector>

namespace rdf4cpp::datatypes::registry {

/**
 * Represents a decoded base64 value as a sequence of bytes.
 * Bytes are stored in the same order as the encoded hextets.
 */
struct Base64BinaryRepr {
    std::vector<std::byte> bytes;

    /**
     * Constructs a Base64BinaryRepr from an encoded base64 value string
     * @param base64encoded encoded base64 value
     * @return decoded base64 value
     */
    [[nodiscard]] static Base64BinaryRepr from_encoded(std::string_view base64encoded);

    /**
     * Encodes this base64 value into its string representation
     * @return encoded string representation
     */
    [[nodiscard]] std::string to_encoded() const noexcept;

    static bool serialize(std::span<std::byte const> bytes, writer::BufWriterParts writer) noexcept;

    bool serialize(writer::BufWriterParts writer) const noexcept;

    /**
     * @param n the index of the hextet to extract
     * @return the n-th hextet in this decoded base64 value
     * @warning the provided index n must be less than n_hextets() otherwise the behaviour is undefined
     */
    [[nodiscard]] std::byte hextet(size_t n) const noexcept;

    /**
     * @return the number of hextets in this decoded base64 value
     * @note count includes padding hextets
     */
    [[nodiscard]] size_t n_hextets() const noexcept;

    /**
     * @param n the index of the byte to extract
     * @return the n-th byte in this decoded base64 value
     * @warning the provided index n must be less than n_bytes() otherwise the behaviour is undefined
     */
     [[nodiscard]] std::byte byte(size_t n) const noexcept;

     /**
      * @return the number of bytes in this decoded base64 value
      */
     [[nodiscard]] size_t n_bytes() const noexcept;

    std::strong_ordering operator<=>(Base64BinaryRepr const &) const noexcept = default;
};

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_base64_binary> {
    using cpp_datatype = Base64BinaryRepr;
};

template<>
capabilities::Default<xsd_base64_binary>::cpp_type capabilities::Default<xsd_base64_binary>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_base64_binary>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_base64_binary,
                                           capabilities::FixedId>;

} // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

struct Base64Binary : registry::LiteralDatatypeImpl<registry::xsd_base64_binary,
                                                    registry::capabilities::FixedId,
                                                    registry::capabilities::Comparable> {};

} // namespace rdf4cpp::datatypes::xsd

namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::Base64Binary const xsd_base64_binary_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#ifndef DOXYGEN_PARSER
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::datatypes::registry::Base64BinaryRepr> {
    static size_t dice_hash(rdf4cpp::datatypes::registry::Base64BinaryRepr const &x) noexcept {
        return Policy::hash_bytes(reinterpret_cast<char const *>(x.bytes.data()), x.bytes.size());
    }
};

template<>
struct std::hash<rdf4cpp::datatypes::registry::Base64BinaryRepr> {
    size_t operator()(rdf4cpp::datatypes::registry::Base64BinaryRepr const &x) const noexcept {
        return dice::hash::dice_hash_templates<::dice::hash::Policies::wyhash>::dice_hash(x);
    }
};
#endif

#endif //RDF4CPP_XSD_BASE64BINARY_HPP
