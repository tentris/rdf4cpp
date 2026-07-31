#ifndef RDF4CPP_NODE_HPP
#define RDF4CPP_NODE_HPP

#include <rdf4cpp/TriBool.hpp>
#include <rdf4cpp/storage/NodeStorage.hpp>
#include <rdf4cpp/storage/identifier/NodeBackendHandle.hpp>
#include <rdf4cpp/writer/BufWriter.hpp>

#include <memory>
#include <optional>
#include <string>

namespace rdf4cpp {

struct Literal;
struct BlankNode;
struct IRI;
namespace query {
    struct Variable;
} // namespace rdf4cpp

enum struct FetchOrSerializeResult {
    Fetched, //< result was already materialized and was fetched
    Serialized, //< result was successfully serialized
    SerializationFailed, //< result had to be serialized, but serialization failed
};

/**
 * Options to control the serialization of Nodes/Literals
 */
struct NodeSerializationOpts {
    bool use_short_form; ///< use the short form of the literals, for example: if this option is true "1"^^xsd:integer serializes to 1
    bool use_prefixes;   ///< use prefixes for fixed datatypes (by default all registered xsd and rdf datatypes), for example: if this option is true "1"^^xsd:byte serializes to "1"^^xsd:byte

    /**
     * Do not use short forms and do not use prefixes. This is used in N-Triples for example.
     *
     * Examples:
     *     "1"^^xsd:integer serializes to "1"^^<http://www.w3.org/2001/XMLSchema#integer>
     *     "1"^^xsd:byte serializes to "1"^^<http://www.w3.org/2001/XMLSchema#byte>
     */
    [[nodiscard]] static NodeSerializationOpts long_form() noexcept {
        return NodeSerializationOpts{};
    }

    /**
     * Use the short form of literals, but do not use prefixes. This is used in sparql TSV results for example.
     *
     * Examples:
     *     "1"^^xsd:integer serializes to 1
     *     "1"^^xsd:byte serializes to "1"^^<http://www.w3.org/2001/XMLSchema#byte>
     */
    [[nodiscard]] static NodeSerializationOpts short_form() noexcept {
        return NodeSerializationOpts{.use_short_form = true, .use_prefixes = false};
    }

    /**
     * Do not use the short form of literals, but do use prefixes.
     *
     * Examples:
     *     "1"^^xsd:integer serializes to "1"^^xsd:integer
     *     "1"^^xsd:byte serializes to "1"^^xsd:byte
     */
    [[nodiscard]] static NodeSerializationOpts prefixed() noexcept {
        return NodeSerializationOpts{.use_short_form = false, .use_prefixes = true};
    }

    /**
     * Use the short form of literals and use prefixes. This is commonly used in turtle (with prefix definitions).
     *
     * Examples:
     *     "1"^^xsd:integer serializes to 1
     *     "1"^^xsd:byte serializes to "1"^^xsd:byte
     */
    [[nodiscard]] static NodeSerializationOpts prefixed_and_short_form() noexcept {
        return NodeSerializationOpts{.use_short_form = true, .use_prefixes = true};
    }
};

inline constexpr storage::DynNodeStoragePtr keep_node_storage{nullptr};

/**
 * @brief Models a node in RDF <span>Dataset</span>s, RDF <span>Graphs</span>s or pattern matching tuples like <span>QuadPattern</span>s or <span>TriplePattern</span>s.
 * <p><b>Please note:</b> The edges of an RDF Graph, dubbed <span>Predicate</span>s, are <span>IRI</span>s. As such the same resource can also be used as a node.
 * For the sake of simplicity, we decided to have no separate class for edges in an RDF graph.
 * You can determine if a Node is an edge by the the fact that it is used as predicate in a Dataset, Graph, Quad, Statement, QuadPattern or TriplePattern.</p>
 *
 * @warning This type is a POD.
 */
struct Node {
protected:
    storage::identifier::NodeBackendHandle handle_;

    [[nodiscard]] storage::DynNodeStoragePtr select_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept {
        if (node_storage == keep_node_storage) {
            return handle_.storage();
        } else {
            return node_storage;
        }
    }

    /**
     * Implementation for eq() and ne()
     */
    [[nodiscard]] TriBool eq_impl(Node const &other) const;

    /**
     * Implementation for lt(), gt(), le(), ge()
     * Do not use for eq() or ne()
     */
    [[nodiscard]] std::partial_ordering compare_impl(Node const &other) const;

public:
    explicit Node(storage::identifier::NodeBackendHandle id) noexcept;

    /**
     * Registers this node in the given node storage (if it does not already exist)
     * @param node_storage node storage to register this node in
     * @return this node but in node storage
     */
    Node to_node_storage(storage::DynNodeStoragePtr node_storage) const;

    /**
     * Tries to retrieve this nodes equivalent node in the given node storage
     * @param node_storage node storage to try to retrieve the node from
     * @return this node but in node storage, or the null node if it does not exist in node_storage
     */
    [[nodiscard]] Node try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const;

    /**
     * Default construction produces null() const Node. This node models an unset or invalid Node.
     * null() const <span>Node</span>s should only be used as temporary placeholders. They cannot be inserted into a Graph or Dataset.
     *
     * @warning This type is POD. The constructor needs to be invoked explicitly. Alternatively: call Node::make_null()
     */
    Node() noexcept = default;

    /**
     * Construct the null-node
     */
    [[nodiscard]] static Node make_null() noexcept;

    /**
     * Serialize the string representation of the given node in N-format as defined by <a href="https://www.w3.org/TR/n-triples/">N-Triples</a> and <a href="https://www.w3.org/TR/n-quads/">N-Quads</a>.
     *
     * @param writer Typically, you pass in an instance of a writer::BufWriter (Concept) instance. Any writer::BufWriter instance is implicitly convertible to writer::BufWriterParts.
     * @param opts determines the formatting style. See doc of NodeSerializationOpts for details.
     * @return true if serialization was successful, false if a call to flush was not able to make room
     *
     * For specific usage examples have a look at tests/bench_SerDe.cpp#serialize.
     */
    bool serialize(writer::BufWriterParts writer, NodeSerializationOpts opts = NodeSerializationOpts::long_form()) const;

    /**
     * Returns a string representation of the given node in N-format as defined by <a href="https://www.w3.org/TR/n-triples/">N-Triples</a> and <a href="https://www.w3.org/TR/n-quads/">N-Quads</a>.
     * @return string representation in N-format
     */
    [[nodiscard]] explicit operator std::string() const;

    /**
     * @see operator std::string() const
     */
    friend std::ostream &operator<<(std::ostream &os, const Node &node);

    /**
     * Checks whether the node is a Literal
     * @return err if this is null, otherwise true iff this is a literal
     */
    [[nodiscard]] TriBool is_literal() const noexcept;

    /**
     * Checks whether the node is a variable
     * @return err if this is null, otherwise true iff this is a variable
     */
    [[nodiscard]] TriBool is_variable() const noexcept;

    /**
     * Checks whether the node is a blank node
     * @return err if this is null, otherwise true iff this is a blank node
     */
    [[nodiscard]] TriBool is_blank_node() const noexcept;

    /**
     * Checks whether the node is a iri
     * @return err if this is null, otherwise true iff this is a iri
     */
    [[nodiscard]] TriBool is_iri() const noexcept;

    /**
     * @return if the current value of this node is stored inside the handle instead of the node storage
     */
    [[nodiscard]] bool is_inlined() const noexcept;


    /**
     * Due to the split definition of ==/!= and </<=/>/>= in SPARQL
     * we cannot provide a "compare" function for FILTER semantics.
     * Specifically, in SPARQL IRIs,BlankNodes and Variables are comparable via == and !=, but not
     * via <,<=,>,>=.
     *
     * https://www.w3.org/TR/sparql11-query/#OperatorMapping
     */
    // [[nodiscard]] std::partial_ordering compare(Node const &other) const noexcept;

    /**
     * The comparison function for SPARQL orderings (ORDER BY).
     *
     * For FILTER semantics, use eq,ne,lt,le,gt,ge.
     *
     * The difference between this and FILTER semantics, is that here BlankNode, Variable, IRI are compared
     * based on their string representation, and thus have an ordering.
     * For Literals you can find information about the differences in Literal::order
     */
    [[nodiscard]] std::strong_ordering order(Node const &other) const;

    /**
     * The equality function for SPARQL filters (FILTER).
     * Due to the split definition of ==/!= and </<=/>/>= in SPARQL
     * we cannot provide a "compare" function for FILTER semantics.
     */
    [[nodiscard]] TriBool eq(Node const &other) const;
    [[nodiscard]] bool order_eq(Node const &other) const;
    [[nodiscard]] TriBool ne(Node const &other) const;
    [[nodiscard]] bool order_ne(Node const &other) const;
    [[nodiscard]] TriBool lt(Node const &other) const;
    [[nodiscard]] bool order_lt(Node const &other) const;
    [[nodiscard]] TriBool le(Node const &other) const;
    [[nodiscard]] bool order_le(Node const &other) const;
    [[nodiscard]] TriBool gt(Node const &other) const;
    [[nodiscard]] bool order_gt(Node const &other) const;
    [[nodiscard]] TriBool ge(Node const &other) const;
    [[nodiscard]] bool order_ge(Node const &other) const;

    [[nodiscard]] Literal as_eq(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_order_eq(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_ne(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_order_ne(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_lt(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_order_lt(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_le(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_order_le(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_gt(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_order_gt(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_ge(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;
    [[nodiscard]] Literal as_order_ge(Node const &other, storage::DynNodeStoragePtr node_storage = keep_node_storage) const;

    /**
     * Equivalent to this->order_eq(other)
     */
    bool operator==(const Node &other) const;

    /**
     * Equivalent to this->order(other)
     */
    std::partial_ordering operator<=>(Node const &other) const;

    /**
     * @return the effective boolean value of this
     */
    [[nodiscard]] TriBool ebv() const;

    /**
     * @return the effective boolean value of this as xsd:boolean (or null literal in case of Err)
     */
    [[nodiscard]] Literal as_ebv(storage::DynNodeStoragePtr node_storage = keep_node_storage) const;

    explicit operator bool() const;

    /**
     * Casts this Node into a BlankNode if it is one, otherwise returns the null BlankNode.
     * @return a copy of this as BlankNode if it is one, else null BlankNode
     */
    [[nodiscard]] BlankNode as_blank_node() const noexcept;

    /**
     * Casts this Node into a IRI if it is one, otherwise returns the null IRI.
     * @return a copy of this as IRI if it is one, else null IRI
     */
    [[nodiscard]] IRI as_iri() const noexcept;

    /**
     * Casts this Node into a Literal if it is one, otherwise returns the null Literal.
     * @return a copy of this as Literal if it is one, else null Literal
     */
    [[nodiscard]] Literal as_literal() const noexcept;

    /**
     * Casts this Node into a Variable if it is one, otherwise returns the null Variable.
     * @return a copy of this as Variable if it is one, else null Variable
     */
    [[nodiscard]] query::Variable as_variable() const noexcept;


    /**
     * Check whether the Node has no value or an invalid value.
     * @return if value is unset or invalid
     */
    [[nodiscard]] bool null() const noexcept;

    /**
     * Exposes the const NodeBackendHandle.
     *
     * This function is unsafe! Make sure this is not null() const.
     * @return its NodeBackendHandle.
     */
    [[nodiscard]] storage::identifier::NodeBackendHandle const &backend_handle() const noexcept;

    /**
     * Exposes the NodeBackendHandle.
     *
     * This function is unsafe! Make sure this is not null() const.
     * @return its NodeBackendHandle.
     */
    [[nodiscard]] storage::identifier::NodeBackendHandle &backend_handle() noexcept;
};

static_assert(sizeof(Node) == sizeof(void *) * 3);
static_assert(alignof(Node) == alignof(void *));

}  // namespace rdf4cpp

template<>
struct std::hash<rdf4cpp::Node> {
    inline size_t operator()(rdf4cpp::Node const &v) const noexcept {
        return std::hash<rdf4cpp::storage::identifier::NodeBackendHandle>()(v.backend_handle());
    }
};

template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::Node> {
    static inline size_t dice_hash(rdf4cpp::Node const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(x.backend_handle());
    }
};

template<>
struct std::formatter<rdf4cpp::Node> {
    constexpr auto parse(format_parse_context &ctx) {
        auto b = ctx.begin();
        if (b != ctx.end() && *b == '}') {
            return b;
        }
        if (b != ctx.end())
            throw std::format_error("parameters");
        return ctx.end();
    }
    auto format(rdf4cpp::Node n, format_context &ctx) const -> decltype(ctx.out());
};

#include <ostream>
#include <rdf4cpp/BlankNode.hpp>
#include <rdf4cpp/IRI.hpp>
#include <rdf4cpp/Literal.hpp>
#include <rdf4cpp/query/Variable.hpp>

#endif  //RDF4CPP_NODE_HPP
