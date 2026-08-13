#include "Statement.hpp"

#include <rdf4cpp/writer/WriteQuad.hpp>

namespace rdf4cpp {

Statement::Statement(Node subject, Node predicate, Node object) noexcept : query::TriplePattern{subject, predicate, object} {
}

bool Statement::valid() const noexcept {
    return ((subject().is_iri() || subject().is_blank_node())
            && (predicate().is_iri())
            && (object().is_iri() || object().is_literal() || object().is_blank_node()));
}

[[nodiscard]] Statement Statement::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    Statement st;
    auto it = st.begin();
    for (auto const &item : *this) {
        *(it++) = item.to_node_storage(node_storage);
    }
    return st;
}

[[nodiscard]] Statement Statement::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const {
    Statement st;
    auto it = st.begin();
    for (auto const &item : *this) {
        *(it++) = item.try_get_in_node_storage(node_storage);
    }
    return st;
}

bool Statement::serialize_ntriples(writer::BufWriterParts writer) const {
    return writer::write_quad<writer::OutputFormat::NTriples>(*this, writer, nullptr);
}

bool Statement::serialize_turtle(writer::SerializationState &state, writer::BufWriterParts writer) const {
    return writer::write_quad<writer::OutputFormat::Turtle>(*this, writer, &state);
}

}  // namespace rdf4cpp
