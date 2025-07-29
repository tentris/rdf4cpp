#ifndef RDF4CPP_NODEBACKENDID_HPP
#define RDF4CPP_NODEBACKENDID_HPP

#include <rdf4cpp/storage/identifier/NodeID.hpp>
#include <rdf4cpp/storage/identifier/RDFNodeType.hpp>

namespace rdf4cpp::storage::identifier {

struct alignas(uint64_t) NodeBackendID {
    using underlying_type = uint64_t;
    static constexpr size_t width = 64;
    static constexpr size_t tagging_bits_width = 13;

    static std::pair<NodeBackendID, std::string_view> const default_graph_iri;
    static std::pair<NodeBackendID, std::string_view> const xsd_string_iri;
    static std::pair<NodeBackendID, std::string_view> const rdf_langstring_iri;

private:
    struct __attribute__((__packed__)) id_parts {
        NodeID node_id_;
        RDFNodeType node_type_: 2;
        uint8_t inlined_: 1;
        uint16_t free_tagging_bits_: tagging_bits_width;

        static_assert(NodeID::width + 2 + 1 + tagging_bits_width == width);
    };

    union __attribute__((packed)) {
        underlying_type underlying_: width;
        id_parts parts_;
    };

public:
    constexpr NodeBackendID() noexcept = default;

    explicit constexpr NodeBackendID(underlying_type const underlying_id) noexcept : underlying_{underlying_id} {
    }

    /**
     * Constructor
     * @param node_id NodeID to be used. NodeID() indicates a null() NodeBackendHandle.
     * @param node_type RDFNodeType to be used
     * @param inlined inlined bit
     * @param tagging_bits tagging bits (must be all zero for usage with the backend)
     */
    constexpr NodeBackendID(NodeID const node_id,
                  RDFNodeType const node_type,
                  bool const inlined = false,
                  uint16_t const tagging_bits = 0) noexcept : parts_{node_id, node_type, inlined, tagging_bits} {
        RDF4CPP_ASSERT(tagging_bits < (1 << tagging_bits_width));
    }

    /**
     * Get the RDFNodeType
     * @return RDFNodeType
     */
    [[nodiscard]] constexpr RDFNodeType type() const noexcept {
        return parts_.node_type_;
    }

    /**
     * Check if the NodeBackendHandle identifies a IRI.
     * @return
     */
    [[nodiscard]] constexpr bool is_iri() const noexcept {
        return parts_.node_type_ == RDFNodeType::IRI;
    }

    /**
     * Check if the NodeBackendHandle identifies a Literal.
     * @return
     */
    [[nodiscard]] constexpr bool is_literal() const noexcept {
        return parts_.node_type_ == RDFNodeType::Literal;
    }

    /**
     * Check if the NodeBackendHandle identifies a BlankNode.
     * @return
     */
    [[nodiscard]] constexpr bool is_blank_node() const noexcept {
        return parts_.node_type_ == RDFNodeType::BNode;
    }

    /**
     * Check if the NodeBackendHandle identifies a Variable.
     * @return
     */
    [[nodiscard]] constexpr bool is_variable() const noexcept {
        return parts_.node_type_ == RDFNodeType::Variable;
    }

    /**
     * If true, the NodeBackendHandle identifies no Node.
     * @return
     */
    [[nodiscard]] constexpr bool null() const noexcept {
        return node_id().null();
    }

    /**
     * Get the NodeID
     * @return
     */
    [[nodiscard]] constexpr NodeID node_id() const noexcept {
        return parts_.node_id_;
    }

    /**
     * @return Whether the LiteralID is an inlined value or a normal node storage ID
     */
    [[nodiscard]] constexpr bool is_inlined() const noexcept {
        return parts_.inlined_;
    }

    /**
     * Get the free tagging bits
     * @return
     */
    [[nodiscard]] constexpr uint16_t free_tagging_bits() const noexcept {
        return parts_.free_tagging_bits_;
    }

    /**
     * Set the free tagging bits. Must be reset to 0 before using comparison functions or raw() const
     * @param new_value
     */
    constexpr void set_free_tagging_bits(uint16_t new_value) noexcept {
        RDF4CPP_ASSERT(new_value < (1 << tagging_bits_width));
        parts_.free_tagging_bits_ = new_value;
    }

    /**
     * Retrieve underlying 64 bit data
     * @return
     */
    [[nodiscard]] underlying_type to_underlying() const noexcept {
        return underlying_;
    }

    template<std::unsigned_integral I> requires (sizeof(I) >= sizeof(underlying_type))
    explicit constexpr operator I() const noexcept {
        return static_cast<I>(underlying_);
    }

    constexpr std::strong_ordering operator<=>(NodeBackendID const &other) const noexcept {
        return underlying_ <=> other.underlying_;
    }

    constexpr bool operator==(NodeBackendID const &other) const noexcept {
        return underlying_ == other.underlying_;
    }
};

static_assert(sizeof(NodeBackendID) == sizeof(uint64_t));
static_assert(alignof(NodeBackendID) == alignof(uint64_t));
static_assert(std::is_same_v<decltype(static_cast<uintptr_t>(NodeBackendID{})), uintptr_t>, "NodeBackendID must be convertible to uintptr_t");
static_assert(std::is_same_v<decltype(static_cast<uint64_t>(NodeBackendID{})), uint64_t>, "NodeBackendID must be convertible to uint64_t");


inline std::ostream &operator<<(std::ostream &os, NodeBackendID id) {
    if (id.is_literal()) {
        os << "{ .node_id = { .literal_id = " << id.node_id().literal_id() << ", .literal_type = " << id.node_id().literal_type()
           << " }, .type = " << id.type() << ", .is_inlined = " << std::boolalpha << id.is_inlined() << ", .free_tagging_bits = " << id.free_tagging_bits() << " }";
    } else {
        os << "{ .node_id = " << id.node_id() << ", .type = " << id.type() << ", .is_inlined = " << std::boolalpha << id.is_inlined() << ", .free_tagging_bits = " << id.free_tagging_bits() << " }";
    }

    return os;
}

/**
 * Convert a NodeId for an IRI
 * to a LiteralType.
 *
 * SAFETY: caller must ensure the node id actually refers to an IRI
 *
 * @param id IRI NodeId
 * @return the LiteralType associated with that IRI
 */
constexpr LiteralType iri_node_id_to_literal_type(NodeBackendID const id) noexcept {
    RDF4CPP_ASSERT(id.is_iri());
    auto const value = id.node_id().to_underlying();

    // all ids values below min_dynamic_datatype_id (except for the null id) are reserved for fixed datatype IRIs
    return value < datatypes::registry::min_dynamic_datatype_id && value != 0
                   ? static_cast<LiteralType>(value)
                   : LiteralType::other();
}

/**
 * Convert a LiteralType to the corresponding IRI NodeID.
 *
 * @param datatype fixed dataype
 * @return NodeID of the IRI associated with the given datatype
 */
constexpr NodeBackendID literal_type_to_iri_node_id(LiteralType const datatype) {
    RDF4CPP_ASSERT(datatype.is_fixed());
    return NodeBackendID{NodeID{datatype.to_underlying()}, RDFNodeType::IRI};
}

inline constexpr std::pair<NodeBackendID, std::string_view> NodeBackendID::default_graph_iri{literal_type_to_iri_node_id(datatypes::registry::reserved_datatype_ids[datatypes::registry::default_graph_iri]),
                                                                                             datatypes::registry::default_graph_iri};

inline constexpr std::pair<NodeBackendID, std::string_view> NodeBackendID::xsd_string_iri{literal_type_to_iri_node_id(datatypes::xsd::String::fixed_id),
                                                                                          datatypes::xsd::String::identifier};

inline constexpr std::pair<NodeBackendID, std::string_view> NodeBackendID::rdf_langstring_iri{literal_type_to_iri_node_id(datatypes::rdf::LangString::fixed_id),
                                                                                              datatypes::rdf::LangString::identifier};

} // namespace rdf4cpp::storage::identifier

template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::storage::identifier::NodeBackendID> {
    static inline size_t dice_hash(rdf4cpp::storage::identifier::NodeBackendID const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(x.to_underlying());
    }
};

template<>
struct std::hash<rdf4cpp::storage::identifier::NodeBackendID> {
    inline size_t operator()(rdf4cpp::storage::identifier::NodeBackendID const &v) const noexcept {
        return ::dice::hash::dice_hash_templates<::dice::hash::Policies::wyhash>::dice_hash(v);
    }
};

#endif // RDF4CPP_NODEBACKENDID_HPP
