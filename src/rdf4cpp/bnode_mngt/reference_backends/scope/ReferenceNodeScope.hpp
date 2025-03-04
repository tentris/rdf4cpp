#ifndef RDF4CPP_RDF_BNODEMANAGEMENT_REFERENCEBACKENDS_SCOPE_REFERENCESCOPEBASE_HPP
#define RDF4CPP_RDF_BNODEMANAGEMENT_REFERENCEBACKENDS_SCOPE_REFERENCESCOPEBASE_HPP

#include <rdf4cpp/bnode_mngt/NodeScope.hpp>
#include <rdf4cpp/storage/identifier/NodeBackendHandle.hpp>

#include <dice/sparse-map/sparse_map.hpp>
#include <dice/hash.hpp>
#include <dice/template-library/static_string.hpp>

#include <rdf4cpp/bnode_mngt/reference_backends/generator/RandomIdGenerator.hpp>
#include <shared_mutex>

namespace rdf4cpp::bnode_mngt {

template<NodeGenerator G = RandomIdGenerator>
struct ReferenceNodeScope {
private:
    G gen_;

    std::shared_mutex mutable mutex_; // protects label_to_handle_ and handle_to_label_
    dice::sparse_map::sparse_map<dice::template_library::static_string,
                                 storage::identifier::NodeBackendHandle,
                                 dice::hash::DiceHashwyhash<std::string_view>,
                                 std::equal_to<>>
            label_to_handle_;

    dice::sparse_map::sparse_map<storage::identifier::NodeBackendHandle,
                                 std::string_view>
            handle_to_label_;

public:
    template<typename ...GenArgs>
    ReferenceNodeScope(GenArgs &&...gen_args) noexcept(std::is_nothrow_constructible_v<G, decltype(std::forward<GenArgs>(gen_args))...>)
        : gen_{std::forward<GenArgs>(gen_args)...} {
    }

    [[nodiscard]] G const &generator() const noexcept {
        return gen_;
    }

    [[nodiscard]] Node try_get_node(std::string_view label) const noexcept {
        std::shared_lock lock{mutex_};
        if (auto it = label_to_handle_.find(label); it != label_to_handle_.end()) {
            return Node{it->second};
        }

        return Node{};
    }

    [[nodiscard]] std::optional<std::string_view> try_get_label(Node handle) const noexcept {
        std::shared_lock lock{mutex_};
        if (auto it = handle_to_label_.find(handle.backend_handle()); it != handle_to_label_.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    [[nodiscard]] Node generate_node(storage::DynNodeStoragePtr node_storage = storage::default_node_storage) {
        return gen_.generate(node_storage);
    }

    [[nodiscard]] Node get_or_generate_node(std::string_view label, storage::DynNodeStoragePtr node_storage = storage::default_node_storage) {
        if (auto const node = try_get_node(label); !node.null()) {
            return node;
        }

        auto const node = generate_node(node_storage);

        std::unique_lock lock{mutex_};
        auto [it, inserted] = label_to_handle_.emplace(label, node.backend_handle());
        if (!inserted) {
            throw std::logic_error{"Node already labeled"};
        }

        handle_to_label_.emplace(it->second, it->first);
        return node;
    }

    /**
     * Clear the mappings from this scope
     * @note does not delete nodes from node storage
     */
    void clear() noexcept {
        std::unique_lock lock{mutex_};
        label_to_handle_.clear();
        handle_to_label_.clear();
    }
};
static_assert(NodeScope<ReferenceNodeScope<>>);

}  //namespace rdf4cpp::bnode_mngt

#endif  //RDF4CPP_RDF_BNODEMANAGEMENT_REFERENCEBACKENDS_SCOPE_REFERENCESCOPEBASE_HPP
