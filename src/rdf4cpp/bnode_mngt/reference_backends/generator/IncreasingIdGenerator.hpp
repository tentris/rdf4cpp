#ifndef RDF4CPP_RDF_UTIL_REFERENCEBACKENDS_INCREASINGBLANKNODEIDGENERATOR_HPP
#define RDF4CPP_RDF_UTIL_REFERENCEBACKENDS_INCREASINGBLANKNODEIDGENERATOR_HPP

#include <rdf4cpp/bnode_mngt/NodeGenerator.hpp>

#include <atomic>
#include <string>

namespace rdf4cpp::bnode_mngt {

/**
 * Generates identifiers consisting of a optional prefix and an integer in increasing value.
 */
struct IncreasingIdGenerator {
private:
    std::string prefix_;
    std::atomic<size_t> counter_;

public:
    /**
     * Creates a generator from a given start value and a prefix
     *
     * @param prefix prefix for the generated ids
     * @param initial_value initial value of the counter
     * @throws ParsingError if the prefix is not a valid blank node identifier
     */
    explicit IncreasingIdGenerator(std::string prefix = "", size_t initial_value = 0) noexcept;

    BlankNode generate(storage::DynNodeStoragePtr node_storage);
};
static_assert(NodeGenerator<IncreasingIdGenerator>);

}  //namespace rdf4cpp::bnode_mngt

#endif  //RDF4CPP_RDF_UTIL_REFERENCEBACKENDS_INCREASINGBLANKNODEIDGENERATOR_HPP
