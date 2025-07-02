#ifndef RDF4CPP_XSD_HEXBINARY_HPP
#define RDF4CPP_XSD_HEXBINARY_HPP

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>

#include <dice/hash.hpp>

#include <cstddef>
#include <vector>

namespace rdf4cpp::datatypes::registry {

/**
 * Represents a decoded xsd:hexBinary value as a byte sequence
 * where each byte stores 2 half-octets.
 *
 * The bytes are stored in order most significant to least significant (i.e. index 0 is most significant).
 */
struct HexBinaryRepr {
    std::vector<std::byte> bytes;

    /**
     * Constructs a HexBinaryRepr from an hexadecimal-encoded string
     * @param hex_encoded encoded hexadecimal string
     * @return decoded hexadecimal value
     */
    [[nodiscard]] static HexBinaryRepr from_encoded(std::string_view hex_encoded);

    /**
     * Encodes this hexadecimal value into its string representation
     * @return encoded string representation
     */
    [[nodiscard]] std::string to_encoded() const noexcept;

    /**
     * Serializes what to_encoded (with a HexBinaryRepr of the same bytes) would output to the given writer.
     *
     * @param bytes bytes to serialize
     * @param writer writer
     * @return if serilization succeeded
     */
    static bool serialize(std::span<std::byte const> bytes, writer::BufWriterParts writer) noexcept;

    /**
     * Serializes what to_encoded (with a HexBinaryRepr of the same bytes) would output to the given writer
     * except that the order of bytes will be reversed and the output will be lowercase (like is requried for hashes)
     *
     * @param bytes bytes to serialize
     * @param writer writer
     * @return if serilization succeeded
     */
    static bool serialize_hash(std::span<std::byte const> bytes, writer::BufWriterParts writer) noexcept;

    /**
     * Same as other overload HexBinaryRepr::serialize, except uses bytes of *this
     */
    bool serialize(writer::BufWriterParts writer) const noexcept;
    /**
     * Same as other overload HexBinaryRepr::serialize_hash, except uses bytes of *this
     */
    bool serialize_hash(writer::BufWriterParts writer) const noexcept;

    /**
     * @param n index of half-octet / hex digit to extract
     * @return the n-th half-octet of this byte sequence
     * @note the most significant half-octet has index 0
     * @warning the provided index n must be less than n_half_octets() otherwise the behaviour is undefined
     */
    [[nodiscard]] std::byte half_octet(size_t n) const noexcept;

    /**
     * @return the number of half-octets / hex digits represented by this byte sequence
     */
    [[nodiscard]] size_t n_half_octets() const noexcept;

    /**
     * @param n index of the byte to extract
     * @return the n-th byte of this sequence
     * @note the bytes are stored in order most significant to least significant (i.e. index 0 is most significant).
     * @warning the provided index n must be less than n_bytes() otherwise the behaviour is undefined
     */
    [[nodiscard]] std::byte byte(size_t n) const noexcept;

    /**
     * @return the number of bytes in this encoded hexadecimal value
     */
    [[nodiscard]] size_t n_bytes() const noexcept;

    std::strong_ordering operator<=>(HexBinaryRepr const &) const noexcept = default;
};

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_hex_binary> {
    using cpp_datatype = HexBinaryRepr;
};

template<>
capabilities::Default<xsd_hex_binary>::cpp_type capabilities::Default<xsd_hex_binary>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_hex_binary>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_hex_binary,
                                           capabilities::FixedId>;

} // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

struct HexBinary : registry::LiteralDatatypeImpl<registry::xsd_hex_binary,
                                                 registry::capabilities::FixedId,
                                                 registry::capabilities::Comparable> {};

} // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::HexBinary const xsd_hex_binary_instance;

} // namespace rdf4cpp::datatypes::registry::instantiation_detail

#ifndef DOXYGEN_PARSER
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::datatypes::registry::HexBinaryRepr> {
    static size_t dice_hash(rdf4cpp::datatypes::registry::HexBinaryRepr const &x) noexcept {
        return Policy::hash_bytes(reinterpret_cast<char const *>(x.bytes.data()), x.bytes.size());
    }
};

template<>
struct std::hash<rdf4cpp::datatypes::registry::HexBinaryRepr> {
    size_t operator()(rdf4cpp::datatypes::registry::HexBinaryRepr const &value) const noexcept {
        return dice::hash::dice_hash_templates<dice::hash::Policies::wyhash>::dice_hash(value);
    }
};
#endif

#endif // RDF4CPP_XSD_HEXBINARY_HPP
