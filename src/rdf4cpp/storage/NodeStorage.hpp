#ifndef RDF4CPP_STORAGE_NODESTORAGEVTABLE_HPP
#define RDF4CPP_STORAGE_NODESTORAGEVTABLE_HPP

#include <concepts>

#include <rdf4cpp/storage/identifier/NodeBackendID.hpp>
#include <rdf4cpp/storage/view/BNodeBackendView.hpp>
#include <rdf4cpp/storage/view/LiteralBackendView.hpp>
#include <rdf4cpp/storage/view/VariableBackendView.hpp>
#include <rdf4cpp/storage/view/IRIBackendView.hpp>

namespace rdf4cpp::storage {

template<typename NS>
concept NodeStorage = requires (NS ns_mut,
                                NS const ns,
                                identifier::LiteralType const lit_type,
                                view::BNodeBackendView const &bnode_view,
                                view::LiteralBackendView const &lit_view,
                                view::VariableBackendView const &var_view,
                                view::IRIBackendView const &iri_view,
                                identifier::NodeBackendID const node_id) {
    /**
     * Backend for NodeStorage::has_specialized_storage_for(datatype)
     * @param datatype datatype of specialized storage to check for
     * @return whether this implementation has specialized storage for the given datatype
     */
    { ns.has_specialized_storage_for(lit_type) } -> std::convertible_to<bool>;

    /**
      * Backend for NodeStorage::find_or_make_id(view::BNodeBackendView const &)
      * @param view Describes requested node. Can be expected to be valid.
      * @return identifier::NodeBackendID identifying a BlankNode stored in the implementation
      */
    { ns_mut.find_or_make_id(bnode_view) } -> std::same_as<identifier::NodeBackendID>;

    /**
      * Backend for NodeStorage::find_or_make_id(view::IRIBackendView const &)
      * @param view Describes requested node. Can be expected to be valid.
      * @return identifier::NodeBackendID identifying a IRI stored in the implementation
      */
    { ns_mut.find_or_make_id(iri_view) } -> std::same_as<identifier::NodeBackendID>;

    /**
      * Backend for NodeStorage::find_or_make_id(view::LiteralBackendView const &)
      * @param view Describes requested node. Can be expected to be valid.
      * @return identifier::NodeBackendID identifying a BlankNode stored in the implementation
      */
    { ns_mut.find_or_make_id(lit_view) } -> std::same_as<identifier::NodeBackendID>;

    /**
      * Backend for NodeStorage::find_or_make_id(view::VariableBackendView const &)
      * @param view Describes requested node. Can be expected to be valid.
      * @return identifier::NodeBackendID identifying a Variable stored in the implementation
      */
    { ns_mut.find_or_make_id(var_view) } -> std::same_as<identifier::NodeBackendID>;

    /**
      * Backend for NodeStorage::find_id(view::BNodeBackendView const &) const
      * @param view Describes requested node. Can be expected to be valid.
      * @return identifier::NodeBackendID identifying a BlankNode stored at the implementation of INodeStorageBackend or a null() identifier::NodeID if there is no such BlankNode stored.
      */
    { ns.find_id(bnode_view) } -> std::same_as<identifier::NodeBackendID>;

    /**
      * Backend for NodeStorage::find_id(view::IRIBackendView const &) const
      * @param view Describes requested node. Can be expected to be valid.
      * @return identifier::NodeBackendID identifying a IRI stored at the implementation of INodeStorageBackend or a null() identifier::NodeID if there is no such IRI stored.
      */
    { ns.find_id(iri_view) } -> std::same_as<identifier::NodeBackendID>;

    /**
      * Backend for NodeStorage::find_id(view::LiteralBackendView const &) const
      * @param view Describes requested node. Can be expected to be valid.
      * @return identifier::NodeBackendID identifying a Literal stored in the implementation or a null() identifier::NodeID if there is no such Literal stored.
      */
    { ns.find_id(lit_view) } -> std::same_as<identifier::NodeBackendID>;

    /**
      * Backend for NodeStorage::find_id(view::VariableBackendView const &) const
      * @param view Describes requested node. Can be expected to be valid.
      * @return identifier::NodeBackendID identifying a Variable stored in the implementation or a null() identifier::NodeID if there is no such Variable stored.
      */
    { ns.find_id(var_view) } -> std::same_as<identifier::NodeBackendID>;

    /**
     * Throws if no Node for the given identifier::NodeBackendID exists.
     * @param id identifier of the requested Node
     * @return view::IRIBackendView describing the requested Node
     */
    { ns.find_iri_backend(node_id) } -> std::convertible_to<view::IRIBackendView>;

    /**
     * Throws if no Node for the given identifier::NodeBackendID exists.
     * @param id identifier of the requested Node
     * @return view::LiteralBackendView describing the requested Node
     */
    { ns.find_literal_backend(node_id) } -> std::convertible_to<view::LiteralBackendView>;

    /**
     * Throws if no Node for the given identifier::NodeBackendID exists.
     * @param id identifier of the requested Node
     * @return view::BNodeBackendView describing the requested Node
     */
    { ns.find_bnode_backend(node_id) } -> std::convertible_to<view::BNodeBackendView>;

    /**
     * Throws if no Node for the given identifier::NodeBackendID exists.
     * @param id identifier of the requested Node
     * @return view::VariableBackendView describing the requested Node
     */
    { ns.find_variable_backend(node_id) } -> std::convertible_to<view::VariableBackendView>;
};

/**
 * A VTable for a NodeStorage
 */
struct NodeStorageVTable {
    bool (*has_specialized_storage_for)(void const *self, identifier::LiteralType literal_type) noexcept;

    identifier::NodeBackendID (*find_or_make_iri_id)(void *self, view::IRIBackendView const &view);
    identifier::NodeBackendID (*find_or_make_bnode_id)(void *self, view::BNodeBackendView const &view);
    identifier::NodeBackendID (*find_or_make_literal_id)(void *self, view::LiteralBackendView const &view);
    identifier::NodeBackendID (*find_or_make_variable_id)(void *self, view::VariableBackendView const &view);

    identifier::NodeBackendID (*find_iri_id)(void const *self, view::IRIBackendView const &view);
    identifier::NodeBackendID (*find_bnode_id)(void const *self, view::BNodeBackendView const &view);
    identifier::NodeBackendID (*find_literal_id)(void const *self, view::LiteralBackendView const &view);
    identifier::NodeBackendID (*find_variable_id)(void const *self, view::VariableBackendView const &view);

    view::IRIBackendView (*find_iri_backend)(void const *self, identifier::NodeBackendID id);
    view::BNodeBackendView (*find_bnode_backend)(void const *self, identifier::NodeBackendID id);
    view::LiteralBackendView (*find_literal_backend)(void const *self, identifier::NodeBackendID id);
    view::VariableBackendView (*find_variable_backend)(void const *self, identifier::NodeBackendID id);

    template<NodeStorage NS>
    static NodeStorageVTable const *get() noexcept {
        static constexpr NodeStorageVTable vtable{
            .has_specialized_storage_for = [](void const *self, identifier::LiteralType const lit_type) -> bool {
                return static_cast<NS const *>(self)->has_specialized_storage_for(lit_type);
            },
            .find_or_make_iri_id = [](void *self, view::IRIBackendView const &view) -> identifier::NodeBackendID {
                return static_cast<NS *>(self)->find_or_make_id(view);
            },
            .find_or_make_bnode_id = [](void *self, view::BNodeBackendView const &view) -> identifier::NodeBackendID {
                return static_cast<NS *>(self)->find_or_make_id(view);
            },
            .find_or_make_literal_id = [](void *self, view::LiteralBackendView const &view) -> identifier::NodeBackendID {
                return static_cast<NS *>(self)->find_or_make_id(view);
            },
            .find_or_make_variable_id = [](void *self, view::VariableBackendView const &view) -> identifier::NodeBackendID {
                return static_cast<NS *>(self)->find_or_make_id(view);
            },
            .find_iri_id = [](void const *self, view::IRIBackendView const &view) -> identifier::NodeBackendID {
                return static_cast<NS const *>(self)->find_id(view);
            },
            .find_bnode_id = [](void const *self, view::BNodeBackendView const &view) -> identifier::NodeBackendID {
                return static_cast<NS const *>(self)->find_id(view);
            },
            .find_literal_id = [](void const *self, view::LiteralBackendView const &view) -> identifier::NodeBackendID {
                return static_cast<NS const *>(self)->find_id(view);
            },
            .find_variable_id = [](void const *self, view::VariableBackendView const &view) -> identifier::NodeBackendID {
                return static_cast<NS const *>(self)->find_id(view);
            },
            .find_iri_backend = [](void const *self, identifier::NodeBackendID id) -> view::IRIBackendView {
                return static_cast<NS const *>(self)->find_iri_backend(id);
            },
            .find_bnode_backend = [](void const *self, identifier::NodeBackendID id) -> view::BNodeBackendView {
                return static_cast<NS const *>(self)->find_bnode_backend(id);
            },
            .find_literal_backend = [](void const *self, identifier::NodeBackendID id) -> view::LiteralBackendView {
                return static_cast<NS const *>(self)->find_literal_backend(id);
            },
            .find_variable_backend = [](void const *self, identifier::NodeBackendID id) -> view::VariableBackendView {
                return static_cast<NS const *>(self)->find_variable_backend(id);
            }};

        return &vtable;
    }
};

/**
 * Essentially a fat-pointer to any NodeStorage.
 * Consists of an instance-pointer and a vtable-pointer.
 *
 * This class fulfills the role of a NodeStorage interface (/ pure-virtual base class).
 *
 * Function documentation can be found on the NodeStorage concept.
 */
struct DynNodeStoragePtr {
private:
    void *backend_;
    NodeStorageVTable const *vtable_;

public:
    constexpr DynNodeStoragePtr() noexcept: DynNodeStoragePtr{nullptr} {
    }

    constexpr DynNodeStoragePtr(std::nullptr_t) noexcept : backend_{nullptr},
                                                           vtable_{nullptr} {
    }

    template<NodeStorage NS> requires (!std::is_same_v<NS, DynNodeStoragePtr>)
    DynNodeStoragePtr(NS &ns) noexcept : backend_{&ns},
                                         vtable_{NodeStorageVTable::get<NS>()} {
    }

    template<NodeStorage NS> requires (!std::is_same_v<NS, DynNodeStoragePtr>)
    DynNodeStoragePtr(NS *ns) noexcept : backend_{ns},
                                         vtable_{NodeStorageVTable::get<NS>()} {
    }

    constexpr DynNodeStoragePtr(void *backend, NodeStorageVTable const *vtable) noexcept : backend_{backend},
                                                                                           vtable_{vtable} {
    }

    template<NodeStorage NS> requires (!std::is_same_v<NS, DynNodeStoragePtr>)
    DynNodeStoragePtr(std::unique_ptr<NS> const &ptr) noexcept : DynNodeStoragePtr{*ptr} {
    }

    [[nodiscard]] constexpr void *backend() const noexcept {
        return backend_;
    }

    [[nodiscard]] constexpr NodeStorageVTable const *vtable() const noexcept {
        return vtable_;
    }

    [[nodiscard]] bool has_specialized_storage_for(identifier::LiteralType literal_type) const {
        return vtable_->has_specialized_storage_for(backend_, literal_type);
    }

    [[nodiscard]] identifier::NodeBackendID find_or_make_id(view::IRIBackendView const &view) {
        return vtable_->find_or_make_iri_id(backend_, view);
    }

    [[nodiscard]] identifier::NodeBackendID find_or_make_id(view::BNodeBackendView const &view) {
        return vtable_->find_or_make_bnode_id(backend_, view);
    }

    [[nodiscard]] identifier::NodeBackendID find_or_make_id(view::LiteralBackendView const &view) {
        return vtable_->find_or_make_literal_id(backend_, view);
    }

    [[nodiscard]] identifier::NodeBackendID find_or_make_id(view::VariableBackendView const &view) {
        return vtable_->find_or_make_variable_id(backend_, view);
    }

    [[nodiscard]] identifier::NodeBackendID find_id(view::IRIBackendView const &view) const {
        return vtable_->find_iri_id(backend_, view);
    }

    [[nodiscard]] identifier::NodeBackendID find_id(view::BNodeBackendView const &view) const {
        return vtable_->find_bnode_id(backend_, view);
    }

    [[nodiscard]] identifier::NodeBackendID find_id(view::LiteralBackendView const &view) const {
        return vtable_->find_literal_id(backend_, view);
    }

    [[nodiscard]] identifier::NodeBackendID find_id(view::VariableBackendView const &view) const {
        return vtable_->find_variable_id(backend_, view);
    }

    [[nodiscard]] view::IRIBackendView find_iri_backend(identifier::NodeBackendID id) const {
        return vtable_->find_iri_backend(backend_, id);
    }

    [[nodiscard]] view::BNodeBackendView find_bnode_backend(identifier::NodeBackendID id) const {
        return vtable_->find_bnode_backend(backend_, id);
    }

    [[nodiscard]] view::LiteralBackendView find_literal_backend(identifier::NodeBackendID id) const {
        return vtable_->find_literal_backend(backend_, id);
    }

    [[nodiscard]] view::VariableBackendView find_variable_backend(identifier::NodeBackendID id) const {
        return vtable_->find_variable_backend(backend_, id);
    }

    std::strong_ordering operator<=>(DynNodeStoragePtr const &other) const noexcept {
        return backend_ <=> other.backend_;
    }

    bool operator==(DynNodeStoragePtr const &other) const noexcept {
        return backend_ == other.backend_;
    }
};

static_assert(NodeStorage<DynNodeStoragePtr>);

/**
 * Pointer to the default node-storage instance. By default it points to rdf4cpp::storage::reference_node_storage::default_instance.
 * This instance is used by default if you do not specify one when creating or modifying nodes.
 *
 * This pointer can be reassigned to point to any instance of your choosing, but be
 * aware that this pointer does not manage ownership, or keep the instance alive.
 * You need to do that yourself.
 */
extern DynNodeStoragePtr default_node_storage;

} // namespace rdf4cpp::storage

template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::storage::DynNodeStoragePtr> {
    static inline size_t dice_hash(rdf4cpp::storage::DynNodeStoragePtr const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(x.backend());
    }
};

template<>
struct std::hash<rdf4cpp::storage::DynNodeStoragePtr> {
    inline size_t operator()(rdf4cpp::storage::DynNodeStoragePtr const &v) const noexcept {
        return ::dice::hash::dice_hash_templates<::dice::hash::Policies::wyhash>::dice_hash(v);
    }
};

#endif // RDF4CPP_STORAGE_NODESTORAGEVTABLE_HPP
