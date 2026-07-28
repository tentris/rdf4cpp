#include "Quad.hpp"

#include <rdf4cpp/writer/WriteQuad.hpp>

namespace rdf4cpp {

Quad::Quad(Node subject, Node predicate, Node object) noexcept : QuadPattern(IRI::default_graph(), subject, predicate, object) {}
Quad::Quad(Node graph, Node subject, Node predicate, Node object) noexcept : QuadPattern(graph, subject, predicate, object) {}

bool Quad::valid() const noexcept {
    return (graph().is_iri() || graph().is_blank_node())
            && (subject().is_iri() || subject().is_blank_node())
            && (predicate().is_iri())
            && (object().is_iri() || object().is_literal() || object().is_blank_node());
}

std::optional<Quad> Quad::create_validated(Node graph, Node subject, Node predicate, Node object) noexcept {
    Quad quad{graph, subject, predicate, object};
    if (quad.valid()) {
        return quad;
    } else {
        return std::nullopt;
    }
}

std::optional<Quad> Quad::create_validated(Node subject, Node predicate, Node object) noexcept {
    Quad quad{subject, predicate, object};
    if (quad.valid()) {
        return quad;
    } else {
        return std::nullopt;
    }
}

Statement const &Quad::without_graph() const noexcept {
    static_assert(sizeof(Quad) == 4 * sizeof(Node));
    static_assert(sizeof(Statement) == 3 * sizeof(Node));

    return *reinterpret_cast<Statement const *>(entries_.data() + 1);
}

Quad Quad::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    Quad qu;
    auto it = qu.begin();
    for (auto const item : *this) {
        *(it++) = item.to_node_storage(node_storage);
    }
    return qu;
}

Quad Quad::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const {
    Quad qu;
    auto it = qu.begin();
    for (auto const item : *this) {
        *(it++) = item.try_get_in_node_storage(node_storage);
    }
    return qu;
}

bool Quad::serialize_ntriples(writer::BufWriterParts const writer) const {
    return writer::write_quad<writer::OutputFormat::NTriples>(*this, writer, nullptr);
}

bool Quad::serialize_nquads(writer::BufWriterParts const writer) const {
    return writer::write_quad<writer::OutputFormat::NQuads>(*this, writer, nullptr);
}

bool Quad::serialize_turtle(writer::SerializationState &state, writer::BufWriterParts const writer) const {
    return writer::write_quad<writer::OutputFormat::Turtle>(*this, writer, &state);
}

bool Quad::serialize_trig(writer::SerializationState &state, writer::BufWriterParts const writer) const {
    return writer::write_quad<writer::OutputFormat::TriG>(*this, writer, &state);
}

}  // namespace rdf4cpp
