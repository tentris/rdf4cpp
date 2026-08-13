#ifndef RDF4CPP_STATEMENT_HPP
#define RDF4CPP_STATEMENT_HPP

#include <rdf4cpp/query/TriplePattern.hpp>

namespace rdf4cpp {

struct Statement : query::TriplePattern {
    // TODO: adjust API to Quad

    Statement() noexcept = default;
    Statement(Node subject, Node predicate, Node object) noexcept;

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] Statement to_node_storage(storage::DynNodeStoragePtr node_storage) const;
    [[nodiscard]] Statement try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const;

    bool serialize_ntriples(writer::BufWriterParts writer) const;
    bool serialize_turtle(writer::SerializationState &state, writer::BufWriterParts writer) const;
};
}  // namespace rdf4cpp
#endif  //RDF4CPP_STATEMENT_HPP
