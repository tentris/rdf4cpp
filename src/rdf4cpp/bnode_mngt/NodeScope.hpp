#ifndef RDF4CPP_NODESCOPE_HPP
#define RDF4CPP_NODESCOPE_HPP

#include <concepts>
#include <string_view>
#include <rdf4cpp/bnode_mngt/NodeGenerator.hpp>
#include <rdf4cpp/storage/NodeStorage.hpp>

namespace rdf4cpp::bnode_mngt {

template<typename S>
concept NodeScope = requires (S &s, std::string_view label, storage::DynNodeStoragePtr node_storage) {
    /**
     * Retrieve the node for the given label
     * or generate a new one based on the label.
     */
    { s.get_or_generate_node(label, node_storage) } -> std::convertible_to<Node>;
};

struct DynNodeScopePtr {
private:
    struct VTable {
        Node (*get_or_generate_node)(void *self, std::string_view label, storage::DynNodeStoragePtr node_storage);

        template<NodeScope S>
        static VTable const *make() {
            static constexpr VTable vtable{
                .get_or_generate_node = [](void *self, std::string_view label, storage::DynNodeStoragePtr node_storage) -> Node {
                    return static_cast<S *>(self)->get_or_generate_node(label, node_storage);
                }};

            return &vtable;
        }
    };

    void *instance_;
    VTable const *vtable_;

public:
    DynNodeScopePtr() noexcept : DynNodeScopePtr{nullptr} {
    }

    DynNodeScopePtr(std::nullptr_t) noexcept : instance_{nullptr}, vtable_{nullptr} {
    }

    template<NodeScope S> requires (!std::is_same_v<S, DynNodeScopePtr>)
    DynNodeScopePtr(S &scope) noexcept : instance_{&scope}, vtable_{VTable::make<S>()} {
    }

    template<NodeScope S>
    DynNodeScopePtr(S *scope) noexcept : instance_{scope}, vtable_{VTable::make<S>()} {
    }

    [[nodiscard]] Node get_or_generate_node(std::string_view label, storage::DynNodeStoragePtr node_storage) const {
        return vtable_->get_or_generate_node(instance_, label, node_storage);
    }

    bool operator==(DynNodeScopePtr const &other) const noexcept = default;

    friend bool operator==(DynNodeScopePtr const &self, std::nullptr_t) noexcept {
        return self.instance_ == nullptr;
    }
};
static_assert(NodeScope<DynNodeScopePtr>);

}  //namespace rdf4cpp::bnode_mngt

#endif  //RDF4CPP_NODESCOPE_HPP
