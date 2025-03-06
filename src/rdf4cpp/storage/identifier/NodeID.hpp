#ifndef RDF4CPP_NODEID_HPP
#define RDF4CPP_NODEID_HPP

#include <rdf4cpp/datatypes/xsd.hpp>
#include <rdf4cpp/datatypes/rdf.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/storage/identifier/RDFNodeType.hpp>

#include <cassert>
#include <compare>
#include <utility>
#include <string_view>

namespace rdf4cpp::storage::identifier {
/**
 * NodeID is an 48 bit identifier for a Node given a NodeManager. If the Node is a Literal, The 48 bits consist of a LiteralID (42 bits) and a LiteralType (6 bits).
 */
struct __attribute__((__packed__)) NodeID {
    using underlying_type = uint64_t;
    static constexpr size_t width = 48;

    static NodeID const min_bnode_id;
    static NodeID const min_iri_id;
    static NodeID const min_variable_id;
    static LiteralID const min_literal_id;

private:
    struct __attribute__((__packed__)) literal_id_parts {
        LiteralID::underlying_type id_: LiteralID::width;
        LiteralType::underlying_type type_: LiteralType::width;
    };

    static_assert(sizeof(literal_id_parts) == 6);

    union __attribute__((__packed__)) {
        underlying_type underlying_: width;
        literal_id_parts literal_parts_;
    };

public:
    constexpr NodeID() = default;

    /**
     * Constructs a LiteralID from a single unsigned integer.
     * @param value literal ID. MUST be smaller than 2^48. Bounds are not checked.
     */
    explicit constexpr NodeID(underlying_type const underlying) noexcept : underlying_{underlying} {
        assert(underlying < (1UL << 48));
    }

    constexpr NodeID(LiteralID const literal_id, LiteralType const literal_type) noexcept
        : literal_parts_{literal_id.to_underlying(), literal_type.to_underlying()} {
    }

    /**
     * Get LiteralID. This method does not check if the NodeID actually represents a literal.
     * @return
     */
    [[nodiscard]] constexpr LiteralID literal_id() const noexcept {
        return LiteralID{literal_parts_.id_};
    }

    /**
     * Get LiteralType. This method does not check if the NodeID actually represents a literal.
     * @return
     */
    [[nodiscard]] constexpr LiteralType literal_type() const noexcept {
        return LiteralType{literal_parts_.type_};
    }

    [[nodiscard]] constexpr underlying_type to_underlying() const noexcept {
        return underlying_;
    }

    [[nodiscard]] constexpr bool null() const noexcept {
        return underlying_ == 0;
    }

    explicit operator underlying_type() const noexcept {
        return underlying_;
    }

    constexpr std::strong_ordering operator<=>(NodeID const &other) const noexcept {
        return underlying_ <=> other.underlying_;
    }

    constexpr bool operator==(NodeID const &other) const noexcept {
        return underlying_ == other.underlying_;
    }
};

static_assert(sizeof(NodeID) == 6);

inline constexpr NodeID NodeID::min_bnode_id{1};
inline constexpr NodeID NodeID::min_iri_id{datatypes::registry::min_dynamic_datatype_id};
inline constexpr NodeID NodeID::min_variable_id{1};
inline constexpr LiteralID NodeID::min_literal_id{1};

inline std::ostream &operator<<(std::ostream &os, NodeID id) {
    os << "NodeID { .underlying = " << id.to_underlying() << " }";
    return os;
}

}  // namespace rdf4cpp::storage::identifier

#ifndef DOXYGEN_PARSER
template<>
struct std::hash<rdf4cpp::storage::identifier::NodeID> {
    size_t operator()(rdf4cpp::storage::identifier::NodeID const id) const noexcept {
        return std::hash<uint64_t>{}(id.to_underlying());
    }
};

template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::storage::identifier::NodeID> {
    static size_t dice_hash(rdf4cpp::storage::identifier::NodeID const id) noexcept {
        return dice_hash_templates<Policy>::dice_hash(id.to_underlying());
    }
};
#endif

#endif  //RDF4CPP_NODEID_HPP
