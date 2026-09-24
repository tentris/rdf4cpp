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
     * The result of a successful request_url call.
     * data is the body of the requested document.
     * final_url is the url of the document after the redirects (documentUrl in the JSON-LD API).
     * For a remote context, relative context urls inside the document resolve against it.
     * If the request was not redirected, it can stay empty, then the requested url is used.
     * For `@import` it is not used: relative urls in the imported context resolve against
     * the same base url as the context that contains the `@import`.
     */
    struct RequestResult {
        std::string data;
        std::string final_url;
    };

    /**
     * A function that is called for each URL requested by a parser (currently only JSON_LD remote context & import).
     * The function should return the result of querying that URL or an error message.
     * The passed URL is already absolute and no pre-parsing of the servers data is necessary
     * (like stripping the top level object and only passing its context member).
     * The passed URL is only valid during the call, copy it if you want to keep it longer.
     * Results are cached only per IStreamQuadIterator.
     * Default behavior is to always return an error.
     */
    std::function<nonstd::expected<RequestResult, std::string>(std::string_view)> request_url = [](std::string_view) {
        return nonstd::unexpected{"remote context not supported"};
    };

    /**
     * Limit of the JSON-LD parser on remote contexts (the remote contexts array of the context processing).
     * It counts the chain of remote contexts that load each other, and in each `@context` array on that chain
     * also the remote contexts before the entry. Each `@context` of the document starts again at 0.
     * If the count is larger than `remote_context_size_limit` when the next remote context is loaded,
     * the parser reports "context overflow". So up to `remote_context_size_limit + 1` remote contexts load,
     * and with 0 one remote context still loads. The default is 100.
     */
    size_t remote_context_size_limit = 100;
};

}  //namespace rdf4cpp::parser

#endif  //RDF4CPP_RDF_PARSER_PARSINGSTATE_HPP
