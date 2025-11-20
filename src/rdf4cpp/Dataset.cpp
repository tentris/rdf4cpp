#include <rdf4cpp/Dataset.hpp>
#include <rdf4cpp/Graph.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>
#include <rdf4cpp/writer/SerializationState.hpp>

#include <utility>

namespace rdf4cpp {

storage::identifier::NodeBackendID Dataset::to_node_id(Node node) noexcept {
    return node.backend_handle().id();
}

Node Dataset::to_node(storage::identifier::NodeBackendID id) const noexcept {
    return Node{storage::identifier::NodeBackendHandle{id, node_storage_}};
}

Dataset::Dataset(storage::DynNodeStoragePtr node_storage) : node_storage_{node_storage} {}

void Dataset::add(Quad const &quad) {
    auto const g = quad.graph().null() ? IRI::default_graph(node_storage_) : quad.graph().to_node_storage(node_storage_);

    auto it = graphs_.find(to_node_id(g));
    if (it == graphs_.end()) {
        it = graphs_.emplace(to_node_id(g), node_storage_).first;
    }

    it.value().add(quad.without_graph());
}

bool Dataset::contains(Quad const &quad) const noexcept {
    auto const g = quad.graph().try_get_in_node_storage(node_storage_);

    auto it = graphs_.find(to_node_id(g));
    if (it == graphs_.end()) {
        return false;
    }

    return it->second.contains(quad.without_graph());
}

Dataset::solution_sequence Dataset::match(query::QuadPattern const &pat) const noexcept {
    return solution_sequence{solution_iterator{this, pat, graphs_.begin(), graphs_.end()}};
}

size_t Dataset::size() const noexcept {
    return std::accumulate(graphs_.begin(), graphs_.end(), 0ul, [](auto acc, auto const &pair) noexcept {
        return acc + pair.second.size();
    });
}

size_t Dataset::size(IRI const &graph_name_) const noexcept {
    auto const graph_name = graph_name_.try_get_in_node_storage(node_storage_);

    auto it = graphs_.find(to_node_id(graph_name));
    if (it == graphs_.end()) {
        return 0;
    }

    return it->second.size();
}

Graph *Dataset::find_graph(Node const &graph_) {
    auto const graph = graph_.try_get_in_node_storage(node_storage_);

    auto it = graphs_.find(to_node_id(graph));
    if (it == graphs_.end()) {
        return nullptr;
    }

    return &it.value();
}

Graph *Dataset::find_graph() {
    return find_graph(IRI::default_graph(node_storage_));
}

Graph const *Dataset::find_graph(Node const &graph_) const {
    auto const graph = graph_.try_get_in_node_storage(node_storage_);

    auto it = graphs_.find(to_node_id(graph));
    if (it == graphs_.end()) {
        return nullptr;
    }

    return &it.value();
}

Graph const *Dataset::find_graph() const {
    return find_graph(IRI::default_graph(node_storage_));
}

Graph &Dataset::graph(Node const &graph_) {
    auto const graph = graph_.to_node_storage(node_storage_);

    auto it = graphs_.find(to_node_id(graph));
    if (it == graphs_.end()) {
        it = graphs_.emplace(to_node_id(graph), Graph{node_storage_}).first;
    }

    return it.value();
}

Graph &Dataset::graph() {
    return graph(IRI::default_graph(node_storage_));
}

Dataset::iterator Dataset::begin() const noexcept {
    return iterator{this, graphs_.begin(), graphs_.end()};
}

Dataset::sentinel Dataset::end() const noexcept {
    return sentinel{};
}

bool Dataset::serialize(writer::BufWriterParts const writer) const noexcept {
    for (Quad const &quad : *this) {
        if (!quad.serialize_nquads(writer)) {
            return false;
        }
    }

    return true;
}

bool Dataset::serialize_trig(writer::SerializationState &state, writer::BufWriterParts const writer) const noexcept {
    for (Quad const &quad : *this) {
        if (!quad.serialize_trig(state, writer)) {
            return false;
        }
    }

    return true;
}

bool Dataset::serialize_trig(writer::BufWriterParts const writer) const noexcept {
    writer::SerializationState st{};
    if (!st.begin(writer)) {
        return false;
    }

    if (!serialize_trig(st, writer)) {
        return false;
    }

    return st.flush(writer);
}
std::ostream &operator<<(std::ostream &os, Dataset const &ds) {
    writer::BufOStreamWriter w{os};
    ds.serialize(w);
    w.finalize();
    return os;
}

Dataset::iterator::iterator(Dataset const *parent,
                            typename storage_type::const_iterator gbeg,
                            typename storage_type::const_iterator gend) noexcept : parent_{parent}, giter_{gbeg}, gend_{gend} {
    while (giter_ != gend_) {
        iter_ = giter_->second.begin();
        if (iter_ != std::default_sentinel) {
            cur_ = Quad{parent_->to_node(giter_->first), iter_->subject(), iter_->predicate(), iter_->object()};
            return;
        }

        ++giter_;
    }
}

Dataset::iterator &Dataset::iterator::operator++() noexcept {
    ++iter_;
    while (iter_ == std::default_sentinel) {
        ++giter_;
        if (giter_ == gend_) {
            return *this;
        }

        iter_ = giter_->second.begin();
    }

    cur_ = Quad{parent_->to_node(giter_->first), iter_->subject(), iter_->predicate(), iter_->object()};
    return *this;
}

void Dataset::iterator::operator++(int) noexcept {
    ++*this;
}

Dataset::iterator::reference Dataset::iterator::operator*() const noexcept {
    return cur_;
}

Dataset::iterator::pointer Dataset::iterator::operator->() const noexcept {
    return &cur_;
}

bool operator==(Dataset::iterator const &self, Dataset::sentinel) noexcept {
    return self.giter_ == self.gend_;
}

bool operator==(Dataset::sentinel, Dataset::iterator const &self) noexcept {
    return self.giter_ == self.gend_;
}

void Dataset::solution_iterator::fill_solution() noexcept {
    if (iter_ != std::default_sentinel) {
        if (pat_.graph().is_variable()) {
            cur_[0] = parent_->to_node(giter_->first);
            std::copy(iter_->begin(), iter_->end(), std::next(cur_.begin()));
        } else {
            std::copy(iter_->begin(), iter_->end(), cur_.begin());
        }
    }
}

Dataset::solution_iterator::solution_iterator(Dataset const *parent,
                                              query::QuadPattern const &pat,
                                              typename storage_type::const_iterator beg,
                                              typename storage_type::const_iterator end) noexcept : parent_{parent}, pat_{pat}, giter_{beg}, gend_{end}, iter_{}, cur_{pat} {
    if (giter_ != gend_) {
        auto const &tpat = pat_.without_graph();

        if (pat_.graph().is_variable()) {
            iter_ = giter_->second.match(tpat).begin();
        } else if (auto const *g = parent_->find_graph(pat_.graph()); g != nullptr) {
            iter_ = g->match(tpat).begin();
        }
    }

    fill_solution();
}

Dataset::solution_iterator &Dataset::solution_iterator::operator++() noexcept {
    ++iter_;

    if (pat_.graph().is_variable()) {
        while (iter_ == std::default_sentinel) {
            ++giter_;
            if (giter_ == gend_) {
                return *this;
            }

            iter_ = giter_->second.match(pat_.without_graph()).begin();
        }
    }

    fill_solution();
    return *this;
}

void Dataset::solution_iterator::operator++(int) noexcept {
    ++*this;
}

Dataset::solution_iterator::reference Dataset::solution_iterator::operator*() const noexcept {
    return cur_;
}

Dataset::solution_iterator::pointer Dataset::solution_iterator::operator->() const noexcept {
    return &cur_;
}

bool operator==(Dataset::solution_iterator const &self, Dataset::sentinel) noexcept {
    return self.iter_ == Dataset::sentinel{};
}

bool operator==(Dataset::sentinel, Dataset::solution_iterator const &self) noexcept {
    return self.iter_ == Dataset::sentinel{};
}

Dataset Dataset::anonymize(util::Anonymizer &anonymizer) const {
    Dataset anon{anonymizer.node_storage()};

    for (auto const &[graph_id, graph] : graphs_) {
        anon.graphs_.emplace(
            anonymizer.anonymize(to_node(graph_id)).backend_handle().id(),
            graph.anonymize(anonymizer)
        );
    }

    return anon;
}


}  // namespace rdf4cpp
