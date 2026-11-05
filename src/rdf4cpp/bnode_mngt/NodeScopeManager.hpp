#ifndef RDF4CPP_NODESCOPEMANAGER_HPP
#define RDF4CPP_NODESCOPEMANAGER_HPP

#include <string_view>
#include <rdf4cpp/bnode_mngt/NodeScope.hpp>

namespace rdf4cpp::bnode_mngt {

/**
 * Manages a set of scopes, by name.
 * I.e. you call subscope with a given name, if there is no scope
 * referring to that name a new one is created. Otherwise you get the previously created scope.
 *
 * You can think of this as a map<std::string, NodeScope>.
 */
template<typename M>
concept NodeScopeManager = requires (M &m, std::string_view scope_name) {
    /**
     * Get the scope for the given name.
     * The name might be a graph or filename.
     */
    { m.scope(scope_name) } -> NodeScope;
};

struct DynNodeScopeManagerPtr {
private:
    struct VTable {
        DynNodeScopePtr (*scope)(void *self, std::string_view);

        template<NodeScopeManager M>
        static VTable const *make() {
            static constexpr VTable vtable{
                .scope = [](void *self, std::string_view label) -> DynNodeScopePtr {
                    return static_cast<M *>(self)->scope(label);
                }};

            return &vtable;
        }
    };

    void *instance_;
    VTable const *vtable_;

public:
    DynNodeScopeManagerPtr() noexcept : DynNodeScopeManagerPtr{nullptr} {
    }

    DynNodeScopeManagerPtr(std::nullptr_t) noexcept : instance_{nullptr}, vtable_{nullptr} {
    }

    template<NodeScopeManager M> requires (!std::is_same_v<M, DynNodeScopeManagerPtr>)
    DynNodeScopeManagerPtr(M &manager) noexcept : instance_{&manager}, vtable_{VTable::make<M>()} {
    }

    template<NodeScopeManager M>
    DynNodeScopeManagerPtr(M *manager) noexcept : instance_{manager}, vtable_{VTable::make<M>()} {
    }

    [[nodiscard]] DynNodeScopePtr scope(std::string_view label) {
        return vtable_->scope(instance_, label);
    }

    bool operator==(DynNodeScopeManagerPtr const &other) const noexcept = default;

    friend bool operator==(DynNodeScopeManagerPtr const &self, std::nullptr_t) noexcept {
        return self.instance_ == nullptr;
    }
};
static_assert(NodeScopeManager<DynNodeScopeManagerPtr>);

} // namespace rdf4cpp::bnode_mngt

#endif  //RDF4CPP_NODESCOPEMANAGER_HPP
