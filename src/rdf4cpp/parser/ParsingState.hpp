#ifndef RDF4CPP_RDF_PARSER_PARSINGSTATE_HPP
#define RDF4CPP_RDF_PARSER_PARSINGSTATE_HPP

#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/bnode_mngt/reference_backends/scope_manager/MergeNodeScopeManager.hpp>
#include <rdf4cpp/bnode_mngt/reference_backends/scope_manager/UnionNodeScopeManager.hpp>
#include <rdf4cpp/storage/NodeStorage.hpp>

namespace rdf4cpp::parser {

/**
 * The publicly known internal state of the IStreamQuadIterator.
 * Values of this type can be used to specify the initial state of IStreamQuadIterators, and
 * therefore states can be reused between different instantiations.
 *
 * @example
 * @code
 * IStreamQuadIterator::state_type state{};
 * {
 *     std::ifstream ifs_1{"some_file.txt"};
 *     IStreamQuadIterator qit_1{ifs_1, ParsingFlags::none(), &state};
 *     // consume qit_1 ...
 * }
 *
 * {
 *     std::ifstream ifs_2{"some_other_file.txt"};
 *     IStreamQuadIterator qit_2{ifs_2, ParsingFlags::none(), &state};
 *     // consume qit_2 with state inherited from qit_1 ...
 * }
 * @endcode
 */
struct ParsingState {
    /**
     * The initial prefixes the parser has knowledge of
     * @note default value is an empty map
     */
    IRIFactory iri_factory{};

    /**
     * The node storage to put the parsed quads into
     */
    storage::DynNodeStoragePtr node_storage = storage::default_node_storage;

    /**
     * The node scope manager to use while parsing files
     * the scopes names passed in are the graph identifiers.
     *
     * By default no scope is used, this means all blank nodes will keep the labels from the file.
     */
    bnode_mngt::DynNodeScopeManagerPtr blank_node_scope_manager = nullptr;

    /**
     * A function that is called for each node that is parsed.
     * To discard a triple throw an exception from this function.
     */
    std::function<void(Node const &)> inspect_node_func = []([[maybe_unused]] Node const &n) { /* noop */ };

    /**
     * A function that is called for each URL requested by a parser (currently only JSON_LD remote contexts).
     * The function should return the result of querying that URL or an error message.
     * Default behavior is to always return an error.
     */
    std::function<nonstd::expected<std::string, std::string>(std::string_view)> request_url = [](std::string_view) {
        return nonstd::unexpected{"remote context not supported"};
    };
};

}  //namespace rdf4cpp::parser

#endif  //RDF4CPP_RDF_PARSER_PARSINGSTATE_HPP
