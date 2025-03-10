#ifndef RDF4CPP_NODEBACKENDHANDLE_HPP
#define RDF4CPP_NODEBACKENDHANDLE_HPP

#include <rdf4cpp/storage/NodeStorage.hpp>
#include <rdf4cpp/storage/identifier/NodeBackendID.hpp>
#include <rdf4cpp/storage/view/BNodeBackendView.hpp>
#include <rdf4cpp/storage/view/IRIBackendView.hpp>
#include <rdf4cpp/storage/view/LiteralBackendView.hpp>
#include <rdf4cpp/storage/view/VariableBackendView.hpp>

#include <compare>
#include <cstdint>

namespace rdf4cpp::storage::identifier {
/**
 * NodeBackendHandle is a a 64 bit wide type that is used to identify \ref Node "Nodes" stored at the backend.
 * It consists of a NodeID, a RDFNodeType, a NodeStorageID and free tagging bits.
 */
struct NodeBackendHandle {
private:
    NodeBackendID id_;
    DynNodeStoragePtr storage_;


public:
    NodeBackendHandle() noexcept = default;

    NodeBackendHandle(NodeBackendID id,
                      DynNodeStoragePtr storage) noexcept : id_{id},
                                                            storage_{storage} {
    }

    /**
     * Constructor
     * @param node_id NodeID to be used. NodeID() indicates a null() NodeBackendHandle.
     * @param node_type RDFNodeType to be used
     * @param inlined inlined bit
     * @param tagging_bits tagging bits (must be all zero for usage with the backend)
     */
    NodeBackendHandle(NodeID const node_id,
                      RDFNodeType const node_type,
                      DynNodeStoragePtr storage,
                      bool const inlined = false,
                      uint16_t const tagging_bits = 0) noexcept : id_{node_id, node_type, inlined, tagging_bits},
                                                                  storage_{storage} {
        assert(tagging_bits < (1 << 13));
    }

    /**
     * Get the RDFNodeType
     * @return RDFNodeType
     */
    [[nodiscard]] constexpr RDFNodeType type() const noexcept {
        return id_.type();
    }

    /**
     * Check if the NodeBackendHandle identifies a IRI.
     * @return
     */
    [[nodiscard]] constexpr TriBool is_iri() const noexcept {
        if (id_.null()) {
            return TriBool::Err;
        }
        return id_.is_iri();
    }

    /**
     * Check if the NodeBackendHandle identifies a Literal.
     * @return
     */
    [[nodiscard]] constexpr TriBool is_literal() const noexcept {
        if (id_.null()) {
            return TriBool::Err;
        }
        return id_.is_literal();
    }

    /**
     * Check if the NodeBackendHandle identifies a BlankNode.
     * @return
     */
    [[nodiscard]] constexpr TriBool is_blank_node() const noexcept {
        if (id_.null()) {
            return TriBool::Err;
        }
        return id_.is_blank_node();
    }

    /**
     * Check if the NodeBackendHandle identifies a Variable.
     * @return
     */
    [[nodiscard]] constexpr TriBool is_variable() const noexcept {
        if (id_.null()) {
            return TriBool::Err;
        }
        return id_.is_variable();
    }

    /**
     * If true, the NodeBackendHandle identifies no Node.
     * @return
     */
    [[nodiscard]] constexpr bool null() const noexcept {
        return id_.null();
    }

    /**
     * Get the NodeID
     * @return
     */
    [[nodiscard]] constexpr NodeID node_id() const noexcept {
        return id_.node_id();
    }

    /**
     * @return Whether the LiteralID is an inlined value or a normal node storage ID
     */
    [[nodiscard]] constexpr bool is_inlined() const noexcept {
        return id_.is_inlined();
    }

    /**
     * Get the free tagging bits
     * @return
     */
    [[nodiscard]] constexpr uint16_t free_tagging_bits() const noexcept {
        return id_.free_tagging_bits();
    }

    /**
     * Set the free tagging bits. Must be reset to 0 before using comparison functions or raw() const
     * @param new_value
     */
    constexpr void set_free_tagging_bits(uint16_t new_value) noexcept {
        id_.set_free_tagging_bits(new_value);
    }

    [[nodiscard]] NodeBackendID id() const noexcept {
        return id_;
    }

    [[nodiscard]] DynNodeStoragePtr storage() const noexcept {
        return storage_;
    }

    constexpr std::strong_ordering operator<=>(NodeBackendHandle const &other) const noexcept {
        return std::tie(id_, storage_) <=> std::tie(other.id_, other.storage_);
    }

    constexpr bool operator==(NodeBackendHandle const &other) const noexcept {
        return std::tie(id_, storage_) == std::tie(other.id_, other.storage_);
    }

    /**
     * Retrieve IRIBackendView with information details about the IRI identified by this NodeBackendHandle.
     * MUST only be called if this is not null and is_iri() const is true.
     * @return
     */
    [[nodiscard]] view::IRIBackendView iri_backend() const noexcept {
        assert(id_.is_iri());
        return storage_.find_iri_backend(id_);
    }

    /**
     * Retrieve LiteralBackendView with information details about the Literal identified by this NodeBackendHandle.
     * MUST only be called if this is not null and is_literal() const is true.
     * @return
     */
    [[nodiscard]] view::LiteralBackendView literal_backend() const noexcept {
        assert(id_.is_literal());
        return storage_.find_literal_backend(id_);
    }

    /**
     * Retrieve BNodeBackendView with information details about the BlankNode identified by this NodeBackendHandle.
     * MUST only be called if this is not null and is_bnode() const is true.
     * @return
     */
    [[nodiscard]] view::BNodeBackendView bnode_backend() const noexcept {
        assert(id_.is_blank_node());
        return storage_.find_bnode_backend(id_);
    }

    /**
     * Retrieve VariableBackendView with information details about the Variable identified by this NodeBackendHandle.
     * MUST only be called if this is not null and is_variable() const is true.
     * @return
     */
    [[nodiscard]] view::VariableBackendView variable_backend() const noexcept {
        assert(id_.is_variable());
        return storage_.find_variable_backend(id_);
    }
};

static_assert(sizeof(NodeBackendHandle) == 3 * sizeof(void *));

inline std::ostream &operator<<(std::ostream &os, NodeBackendHandle handle) {
    os << "NodeBackendHandle { .id = " << handle.id() << ", .node_storage = DynNodeStoragePtr { .backend = " << handle.storage().backend() << ", .vtable = " << handle.storage().vtable() << " } }";
    return os;
}

/**
 * Constructor to create an IRI handle for the datatype of a Literal with fixed id datatype
 * @param lit_handle handle of the literal
 * @return handle to the datatype of the literal
 */
[[nodiscard]] inline NodeBackendHandle datatype_iri_handle_for_fixed_lit_handle(NodeBackendHandle lit_handle) noexcept {
    assert(lit_handle.is_literal());
    assert(lit_handle.node_id().literal_type().is_fixed());
    return NodeBackendHandle{literal_type_to_iri_node_id(lit_handle.node_id().literal_type()),
                             lit_handle.storage()};
}

}  // namespace rdf4cpp::storage::identifier


template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::storage::identifier::NodeBackendHandle> {
    static inline size_t dice_hash(rdf4cpp::storage::identifier::NodeBackendHandle const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::make_tuple(x.id(), x.storage()));
    }
};

template<>
struct std::hash<rdf4cpp::storage::identifier::NodeBackendHandle> {
    inline size_t operator()(rdf4cpp::storage::identifier::NodeBackendHandle const &v) const noexcept {
        return ::dice::hash::dice_hash_templates<::dice::hash::Policies::wyhash>::dice_hash(v);
    }
};

#endif  //RDF4CPP_NODEBACKENDHANDLE_HPP
