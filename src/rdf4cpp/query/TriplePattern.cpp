#include "TriplePattern.hpp"

#include <rdf4cpp/writer/WriteQuad.hpp>

namespace rdf4cpp::query {

TriplePattern::TriplePattern(Node subject, Node predicate, Node object) noexcept : entries_{subject, predicate, object} {
}

bool TriplePattern::valid() const noexcept {
    return !subject().null() && !subject().is_literal()
            && !predicate().null() && (predicate().is_iri() || predicate().is_variable())
            && !object().null();
}

TriplePattern::operator std::string() const {
    auto s = writer::StringWriter::oneshot([this](auto &w) {
        return writer::write_quad<writer::OutputFormat::NTriples>(*this, w, nullptr);
    });

    s.pop_back(); // remove newline
    return s;
}

std::ostream &operator<<(std::ostream &os, TriplePattern const &pattern) {
    os << static_cast<std::string>(pattern);
    return os;
}

TriplePattern TriplePattern::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    TriplePattern tp;
    auto it = tp.begin();
    for (const auto &item : *this) {
        *(it++) = item.to_node_storage(node_storage);
    }
    return tp;
}

TriplePattern TriplePattern::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const {
    TriplePattern tp;
    auto it = tp.begin();
    for (const auto &item : *this) {
        *(it++) = item.try_get_in_node_storage(node_storage);
    }
    return tp;
}

}  // namespace rdf4cpp::query