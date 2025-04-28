#include "Graph.hpp"
#include <rdf4cpp/Dataset.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>
#include <rdf4cpp/writer/SerializationState.hpp>
#include <rdf4cpp/writer/WriteJsonLdQuad.hpp>

#include <utility>

namespace rdf4cpp {

storage::identifier::NodeBackendID Graph::to_node_id(Node node) noexcept {
    return node.backend_handle().id();
}

Node Graph::to_node(storage::identifier::NodeBackendID id) const noexcept {
    return Node{storage::identifier::NodeBackendHandle{id, node_storage_}};
}

Graph::Graph(storage::DynNodeStoragePtr node_storage) noexcept : node_storage_{node_storage} {
}

void Graph::add(Statement const &stmt_) {
    auto stmt = stmt_.to_node_storage(node_storage_);
    triples_.insert(triple{to_node_id(stmt.subject()), to_node_id(stmt.predicate()), to_node_id(stmt.object())});
}

bool Graph::contains(Statement const &stmt_) const noexcept {
    auto const stmt = stmt_.try_get_in_node_storage(node_storage_);
    return triples_.contains(triple{to_node_id(stmt.subject()), to_node_id(stmt.predicate()), to_node_id(stmt.object())});
}

Graph::iterator Graph::begin() const noexcept {
    return iterator{this, triples_.begin(), triples_.end()};
}

Graph::sentinel Graph::end() const noexcept {
    return sentinel{};
}

Graph::solution_sequence Graph::match(query::TriplePattern const &triple_pattern) const noexcept {
    return solution_sequence{solution_iterator{begin(), triple_pattern}};
}

size_t Graph::size() const noexcept {
    return triples_.size();
}

bool Graph::serialize(writer::BufWriterParts const writer) const noexcept {
    for (auto const &[s, p, o] : triples_) {
        Quad q{to_node(s), to_node(p), to_node(o)};
        if (!q.serialize_ntriples(writer)) {
            return false;
        }
    }

    return true;
}

bool Graph::serialize_turtle(writer::SerializationState &state, writer::BufWriterParts const writer) const noexcept {
    for (auto const &[s, p, o] : triples_) {
        Quad q{to_node(s), to_node(p), to_node(o)};
        if (!q.serialize_turtle(state, writer)) {
            return false;
        }
    }

    return true;
}

bool Graph::serialize_turtle(writer::BufWriterParts const writer) const noexcept {
    writer::SerializationState st{};
    if (!st.begin(writer)) {
        return false;
    }

    if (!serialize_turtle(st, writer)) {
        return false;
    }

    return st.flush(writer);
}

bool Graph::serialize_json_ld(writer::SerializationState &state, writer::BufWriterParts writer) const noexcept {

}

std::ostream &operator<<(std::ostream &os, Graph const &graph) {
    writer::BufOStreamWriter w{os};
    graph.serialize(w);
    w.finalize();
    return os;
}

Statement Graph::iterator::to_statement(rdf4cpp::Graph::triple const &t) const noexcept {
    return Statement{parent_->to_node(t[0]), parent_->to_node(t[1]), parent_->to_node(t[2])};
}

Graph::iterator::iterator(Graph const *parent, typename triple_storage_type::const_iterator beg, typename triple_storage_type::const_iterator end) noexcept : parent_{parent},
                                                                                                                                                              iter_{beg},
                                                                                                                                                              end_{end} {
    if (iter_ != end_) {
        cur_ = to_statement(*iter_);
    }
}

Graph::iterator &Graph::iterator::operator++() noexcept {
    ++iter_;
    if (iter_ != end_) {
        cur_ = to_statement(*iter_);
    }

    return *this;
}

Graph::reference Graph::iterator::operator*() const noexcept {
    return cur_;
}

Graph::pointer Graph::iterator::operator->() const noexcept {
    return &cur_;
}

bool Graph::iterator::operator==(Graph::sentinel) const noexcept {
    return iter_ == end_;
}

bool Graph::iterator::operator!=(Graph::sentinel) const noexcept {
    return !(*this == Graph::sentinel{});
}

bool Graph::solution_iterator::check_solution() noexcept {
    auto pat_it = pat_.begin();
    auto out_it = cur_.begin();

    for (auto const x : *iter_) {
        if (pat_it->is_variable()) {
            out_it->second = x;
            ++out_it;
        } else if (*pat_it != x) {
            return false;
        }

        ++pat_it;
    }

    return true;
}

void Graph::solution_iterator::forward_to_solution() noexcept {
    while (iter_ != std::default_sentinel && !check_solution()) {
        ++iter_;
    }
}

Graph::solution_iterator::solution_iterator(typename Graph::iterator beg,
                                            query::TriplePattern const &pat) noexcept : iter_{beg},
                                                                                        pat_{pat},
                                                                                        cur_{pat} {
    forward_to_solution();
}

Graph::solution_iterator &Graph::solution_iterator::operator++() noexcept {
    ++iter_;
    forward_to_solution();
    return *this;
}

Graph::solution_iterator::reference Graph::solution_iterator::operator*() const noexcept {
    return cur_;
}

Graph::solution_iterator::pointer Graph::solution_iterator::operator->() const noexcept {
    return &cur_;
}

bool Graph::solution_iterator::operator==(Graph::sentinel) const noexcept {
    return iter_ == Graph::sentinel{};
}

bool Graph::solution_iterator::operator!=(Graph::sentinel) const noexcept {
    return iter_ != Graph::sentinel{};
}

}  // namespace rdf4cpp
