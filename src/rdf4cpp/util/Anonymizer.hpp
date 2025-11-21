#ifndef RDF4CPP_ANONYMIZER_HPP
#define RDF4CPP_ANONYMIZER_HPP

#include <rdf4cpp/Quad.hpp>
#include <rdf4cpp/Statement.hpp>
#include <rdf4cpp/storage/NodeStorage.hpp>

#include <dice/sparse-map/sparse_map.hpp>

#include <random>
#include <span>

namespace rdf4cpp::util {

/**
 * A utility for anonymizing nodes and graphs.
 * When used via Graph::anonymize or Dataset::anonymize will keep the graph/dataset structure intact while
 * removing all other information.
 */
struct Anonymizer {
private:
    std::mt19937_64 rng_;
    std::uniform_int_distribution<size_t> dist_;

    storage::DynNodeStoragePtr node_storage_;
    dice::sparse_map::sparse_map<storage::identifier::NodeBackendHandle, storage::identifier::NodeBackendID> lookup_;

    void gen_random_id(std::span<char> output);

public:
    /**
     * Initialize the anonymizer
     * @param node_storage node storage where to anonymizer will place its nodes
     * @param seed optional seed for the random generator, if the seed is not specified std::random_device will be used to seed the random generator
     */
    explicit Anonymizer(storage::DynNodeStoragePtr node_storage = storage::default_node_storage, std::optional<uint64_t> seed = std::nullopt);

    /**
     * @return the node storage where this anonymizer places its nodes
     */
    [[nodiscard]] storage::DynNodeStoragePtr node_storage() const noexcept;

    /**
     * Anonymize a node by replacing sensitive data with randomly generated data.
     * Passing in the same node yield the same anonymized node as last time, this is to ensure that
     * the graph structure stays intact.
     *
     * Any node passed in gets turned into an IRI matching the following regex: <http://example.com/[a-zA-Z]{16}>
     *      except for the null node and the default graph IRI which both stay as they are.
     *
     * @param non_anon non-anonymized node to anonymize
     * @return anonymized node
     */
    [[nodiscard]] IRI anonymize(Node const &non_anon);

        /**
     * Anonymize a statement by anonymizing each component (subject, predicate and object)
     */
    [[nodiscard]] Statement anonymize(Statement const &non_anon);

    /**
     * Anonymize a quad by anonymizing each component (graph, subject, predicate and object)
     */
    [[nodiscard]] Quad anonymize(Quad const &non_anon);
};

} // namespace rdf4cpp::util

#endif // RDF4CPP_ANONYMIZER_HPP
