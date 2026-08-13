#ifndef RDF4CPP_QUAD_HPP
#define RDF4CPP_QUAD_HPP

#include <rdf4cpp/Statement.hpp>
#include <rdf4cpp/query/QuadPattern.hpp>

namespace rdf4cpp {

/**
 * A Quad is an RDF statement which has an additional graph name.
 * Quads are typically used to identify a statement in a named Graph of a RDF Dataset. The Graph name can also be the default Graph.
 */
struct Quad : query::QuadPattern {

    /**
     * Initializes the Quad entries with null <div>Node</div>s
     */
    Quad() noexcept = default;

    /**
     * The constructed Quad has the default Graph as graph name. The Quad is not validated.
     * @param subject IRI or BlankNode
     * @param predicate IRI
     * @param object IRI, Literal or BlankNode
     */
    Quad(Node subject, Node predicate, Node object) noexcept;

    /**
     * Construct a Quad. The Quad is not validated.
     * @param graph IRI or BlankNode
     * @param subject IRI or BlankNode
     * @param predicate IRI
     * @param object IRI, Literal or BlankNode
     */
    Quad(Node graph, Node subject, Node predicate, Node object) noexcept;


    [[nodiscard]] bool valid() const noexcept;

    /**
     * The constructed Quad is validated. The optional result is only present, if it is valid.
     * @param graph IRO or BlankNode, name of the graph where the statement (subject, predicate, object) is added
     * @param subject IRI or BlankNode
     * @param predicate IRI
     * @param object IRI, Literal or BlankNode
     * @return A Quad if valid otherwise std::optional is empty
     */
    static std::optional<Quad> create_validated(Node graph, Node subject, Node predicate, Node object) noexcept;

    /**
     * The constructed Quad has the default Graph as graph name and is validated. The optional result is only present, if it is valid.
     * @param subject IRI or BlankNode
     * @param predicate IRI
     * @param object IRI, Literal or BlankNode
     * @return A Quad if valid otherwise std::optional is empty
     */
    static std::optional<Quad> create_validated(Node subject, Node predicate, Node object) noexcept;

    [[nodiscard]] Statement const &without_graph() const noexcept;

    [[nodiscard]] Quad to_node_storage(storage::DynNodeStoragePtr node_storage) const;
    [[nodiscard]] Quad try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const;

    bool serialize_ntriples(writer::BufWriterParts writer) const;
    bool serialize_nquads(writer::BufWriterParts writer) const;
    bool serialize_turtle(writer::SerializationState &state, writer::BufWriterParts writer) const;
    bool serialize_trig(writer::SerializationState &state, writer::BufWriterParts writer) const;
};
}  // namespace rdf4cpp

template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::Quad> {
    static inline size_t dice_hash(rdf4cpp::Quad const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::tie(x.graph(), x.subject(), x.predicate(), x.object()));
    }
};

#endif  //RDF4CPP_QUAD_HPP
