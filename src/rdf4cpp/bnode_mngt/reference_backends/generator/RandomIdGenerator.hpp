#ifndef RDF4CPP_RDF_UTIL_REFERENCEBACKENDS_RANDOMBLANKNODEIDGENERATOR_HPP
#define RDF4CPP_RDF_UTIL_REFERENCEBACKENDS_RANDOMBLANKNODEIDGENERATOR_HPP

#include <rdf4cpp/bnode_mngt/NodeGenerator.hpp>

#include <random>
#include <mutex>

namespace rdf4cpp::bnode_mngt {

/**
 * Generates 32-char long random ids consisting of [0-9a-z]
 */
struct RandomIdGenerator {
private:
    std::mutex mutable mutex_;
    std::mt19937_64 rng_;
    std::uniform_int_distribution<size_t> dist_;

    char next_char();
public:
    /**
     * Creates a generator by seeding it with os entropy
     */
    RandomIdGenerator();

    /**
     * Creates a generator by seeding it with the given seed
     * @param seed seed for the random generator
     */
    explicit RandomIdGenerator(uint64_t seed);

    BlankNode generate(storage::DynNodeStoragePtr node_storage = storage::default_node_storage);
};
static_assert(NodeGenerator<RandomIdGenerator>);

}  //namespace rdf4cpp::bnode_mngt

#endif  //RDF4CPP_RDF_UTIL_REFERENCEBACKENDS_RANDOMBLANKNODEIDGENERATOR_HPP
