#ifndef RDF4CPP_REFERENCENODESCOPEMANAGER_HPP
#define RDF4CPP_REFERENCENODESCOPEMANAGER_HPP

#include <rdf4cpp/bnode_mngt/NodeScopeManager.hpp>
#include <rdf4cpp/bnode_mngt/reference_backends/scope/ReferenceNodeScope.hpp>
#include <dice/sparse-map/sparse_map.hpp>
#include <rdf4cpp/Assert.hpp>

#include <dice/hash.hpp>

namespace rdf4cpp::bnode_mngt {

template<NodeScope S = ReferenceNodeScope<>> requires std::is_default_constructible_v<S>
struct MergeNodeScopeManager {
    using node_scope_storage_type = dice::sparse_map::sparse_map<
            std::string,
            std::unique_ptr<S>,
            dice::hash::DiceHashwyhash<std::string_view>,
            std::equal_to<>>;

    node_scope_storage_type scopes;

    S &scope(std::string_view name) noexcept {
        if (auto it = scopes.find(name); it != scopes.end()) {
          return *it->second;
        }

        auto [it, inserted] = scopes.emplace(name, std::make_unique<S>());
        RDF4CPP_ASSERT(inserted);
        return *it->second;
    }
};
static_assert(NodeScopeManager<MergeNodeScopeManager<>>);

} // namespace rdf4cpp::bnode_mngt

#endif  //RDF4CPP_REFERENCENODESCOPEMANAGER_HPP
