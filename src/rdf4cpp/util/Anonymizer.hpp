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
    std::uniform_int_distribution<uint64_t> dist_;

    storage::DynNodeStoragePtr node_storage_;
    dice::sparse_map::sparse_map<storage::identifier::NodeBackendHandle, storage::identifier::NodeBackendID> lookup_;

    void gen_random_id(std::span<char> output);

    template<datatypes::registry::util::ConstexprString base>
    struct MkAnon {
    private:
        Anonymizer *parent_;
        std::array<char, base.size() + 16> buf_;

    public:
        explicit MkAnon(Anonymizer &parent) noexcept;
        std::string_view operator()();
    };

    Literal anonymize_impl(Literal const &lit);
    BlankNode anonymize_impl(BlankNode const &node);
    IRI anonymize_impl(IRI const &node);
    query::Variable anonymize_impl(query::Variable const &var);

public:
    /**
     * Initialize the anonymizer
     * @param node_storage node storage where to anonymizer will place its nodes
     */
    explicit Anonymizer(storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * @return the node storage where this anonymizer places its nodes
     */
    [[nodiscard]] storage::DynNodeStoragePtr node_storage() const noexcept;

    /**
     * Anonymize a node by replacing sensitive data with randomly generated data.
     * Passing in the same node yield the same anonymized node as last time, this is to ensure that
     * the graph structure stays intact.
     *
     * The anonymized node has the same kind as the input:
     *      - IRIs become IRIs matching <http://example.org/[a-zA-Z]{16}>
     *      - blank nodes and variables get a random name, variables keep their anonymity
     *      - literals keep their datatype and language tag but get a random lexical form,
     *        except numeric, duration, timepoint and boolean literals which are kept as they are
     *      - the null node and the default graph IRI stay as they are
     *
     * @param non_anon non-anonymized node to anonymize
     * @return anonymized node
     */
    [[nodiscard]] Node anonymize(Node const &non_anon);

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
