#include "QuadPattern.hpp"

#include <rdf4cpp/writer/WriteQuad.hpp>

namespace rdf4cpp::query {

QuadPattern::QuadPattern(Node graph, Node subject, Node predicate, Node object) noexcept : entries_{graph, subject, predicate, object} {
}

bool QuadPattern::valid() const noexcept {
    return (graph().is_iri() || graph().is_variable())
            && !subject().is_literal()
            && (predicate().is_iri() || predicate().is_variable())
            && !object().null();
}

QuadPattern::operator std::string() const {
    auto s = writer::StringWriter::oneshot([this](auto &w) {
        return writer::write_quad<writer::OutputFormat::NQuads>(*this, w, nullptr);
    });

    s.pop_back(); // remove newline
    return s;
}

std::ostream &operator<<(std::ostream &os, const QuadPattern &pattern) {
    os << static_cast<std::string>(pattern);
    return os;
}

QuadPattern QuadPattern::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    QuadPattern qp;
    auto it = qp.begin();
    for (const auto &item : *this) {
        *(it++) = item.to_node_storage(node_storage);
    }
    return qp;
}

QuadPattern QuadPattern::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const {
    QuadPattern qp;
    auto it = qp.begin();
    for (const auto &item : *this) {
        *(it++) = item.try_get_in_node_storage(node_storage);
    }
    return qp;
}

TriplePattern const &QuadPattern::without_graph() const noexcept {
    static_assert(sizeof(QuadPattern) == 4 * sizeof(Node));
    static_assert(sizeof(TriplePattern) == 3 * sizeof(Node));
    return *reinterpret_cast<TriplePattern const *>(entries_.data() + 1);
}

}  // namespace rdf4cpp::query