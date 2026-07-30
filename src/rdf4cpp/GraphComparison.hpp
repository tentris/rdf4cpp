#ifndef RDF4CPP_GRAPHCOMPARISON_HPP
#define RDF4CPP_GRAPHCOMPARISON_HPP

#include <rdf4cpp/Node.hpp>

#include <map>
#include <ranges>
#include <unordered_map>
#include <vector>

namespace rdf4cpp {
    /**
     * Compares two graphs, handling blank node mappings, by sorting both into a canonical order first.
     * If this function finds a blank node mapping, then the two graphs are isomorphic.
     * If it does not find one, a full graph isomorphism check might still find one.
     * This is usually enough for parser tests.
     * Expects both graphs to be deduplicated.
     * @tparam Q Quad or Triple (but anything with a std::vector-like API will work)
     * @param graph_a the graph to map to
     * @param graph_b the graph to map from
     * @return blank node mapping from graph_b to graph_a if found, first mismatched node pair otherwise.
     * Two null nodes are returned if the graphs differ in size or in the size of one of their elements.
     */
    template<std::ranges::sized_range Q, std::ranges::range G>
    requires std::ranges::random_access_range<Q> && std::same_as<std::ranges::range_value_t<Q>, std::remove_cvref_t<Node>> && std::same_as<std::ranges::range_value_t<G>, std::remove_cvref_t<Q>>
    nonstd::expected<std::map<BlankNode, BlankNode>, std::pair<Node, Node>> try_compare_graphs_fast(G const & graph_a, G const & graph_b) {
        struct Quad {
            Q const *quad;
            size_t similar_count = 0;
        };
        static constexpr auto size_mismatch = []() {
            return nonstd::unexpected{std::pair{Node::make_null(), Node::make_null()}};
        };

        std::vector<Quad> a{};
        for (const auto& q : graph_a) {
            a.emplace_back(&q);
        }
        std::vector<Quad> b{};
        for (const auto& q : graph_b) {
            b.emplace_back(&q);
        }

        if (a.size() != b.size()) {
            return size_mismatch();
        }
        // everything below indexes all elements with the arity of the first one
        size_t const arity = a.empty() ? 0 : a.front().quad->size();
        for (auto const &v : {std::cref(a), std::cref(b)}) {
            for (auto const &e : v.get()) {
                if (e.quad->size() != arity) {
                    return size_mismatch();
                }
            }
        }

        static constexpr auto num_blanks = [](Q const &p) {
            size_t n = 0;
            for (Node const &e : p) {
                if (e.is_blank_node()) {
                    ++n;
                }
            }
            return n;
        };
        // counts the quads of v that share at least two non blank nodes at the same position with p.
        // the index maps a pair of positions and the nodes at them to the quads that have them.
        static constexpr auto count_all_sim = [](std::vector<Quad> &v, size_t arity) {
            struct PairKey {
                size_t pos_a;
                size_t pos_b;
                Node node_a;
                Node node_b;

                bool operator==(PairKey const &) const noexcept = default;
            };
            struct PairKeyHash {
                size_t operator()(PairKey const &k) const noexcept {
                    size_t h = std::hash<Node>{}(k.node_a);
                    h = h * 31 + std::hash<Node>{}(k.node_b);
                    h = h * 31 + k.pos_a;
                    return h * 31 + k.pos_b;
                }
            };

            std::unordered_map<PairKey, std::vector<size_t>, PairKeyHash> index{};
            for (size_t i = 0; i < v.size(); ++i) {
                Q const &q = *v[i].quad; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                for (size_t pa = 0; pa < arity; ++pa) {
                    if (q[pa].is_blank_node()) {
                        continue;
                    }
                    for (size_t pb = pa + 1; pb < arity; ++pb) {
                        if (q[pb].is_blank_node()) {
                            continue;
                        }
                        index[PairKey{pa, pb, q[pa], q[pb]}].push_back(i);
                    }
                }
            }

            std::vector<size_t> found{};
            for (size_t i = 0; i < v.size(); ++i) {
                Q const &q = *v[i].quad; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                found.clear();
                for (size_t pa = 0; pa < arity; ++pa) {
                    if (q[pa].is_blank_node()) {
                        continue;
                    }
                    for (size_t pb = pa + 1; pb < arity; ++pb) {
                        if (q[pb].is_blank_node()) {
                            continue;
                        }
                        auto const &matches = index[PairKey{pa, pb, q[pa], q[pb]}];
                        found.insert(found.end(), matches.begin(), matches.end());
                    }
                }
                std::ranges::sort(found);
                auto const duplicates = std::ranges::unique(found);
                v[i].similar_count = static_cast<size_t>(std::ranges::distance(found.begin(), duplicates.begin())); // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
            }
        };
        static constexpr auto sort = [](std::vector<Quad> &v, size_t arity) {
            count_all_sim(v, arity);
            static constexpr size_t not_found = std::numeric_limits<size_t>::max();
            // blank node handles are arbitrary, so blank nodes are ordered by the position of their
            // first appearance instead. non blank nodes sort before all blank nodes, which keeps the
            // order independent of the handles and makes it a strict weak ordering.
            std::unordered_map<Node, size_t> bn_indices{};
            auto get_ind = [&](Node n) {
                auto i = bn_indices.find(n);
                return i == bn_indices.end() ? not_found : i->second;
            };
            auto comp = [&](Node a, Node b) -> bool {
                bool const a_is_bn = a.is_blank_node();
                bool const b_is_bn = b.is_blank_node();
                if (a_is_bn != b_is_bn) {
                    return b_is_bn;
                }
                if (a_is_bn) {
                    auto ai = get_ind(a);
                    auto bi = get_ind(b);
                    if (ai != bi) {
                        return std::less{}(ai, bi);
                    }
                }
                return std::less{}(a, b);
            };
            auto compare = [&](Quad const &aq,Quad const &bq) {
                auto a_dup = aq.similar_count;
                auto b_dup = bq.similar_count;
                if (a_dup != b_dup) {
                    return std::less{}(a_dup, b_dup);
                }
                Q const &a = *aq.quad;
                Q const &b = *bq.quad;
                auto a_bl = num_blanks(a);
                auto b_bl = num_blanks(b);
                if (a_bl != b_bl) {
                    return std::less{}(a_bl, b_bl);
                }
                for (size_t i = 0; i < arity; ++i) {
                    if (a[i] != b[i] && !a[i].is_blank_node() && !b[i].is_blank_node()) {
                        return comp(a[i], b[i]);
                    }
                }
                if (arity == 0) {
                    return false;
                }
                for (size_t i = 0; i < arity - 1; ++i) {
                    if (a[i] != b[i]) {
                        return comp(a[i], b[i]);
                    }
                }
                return comp(a[arity-1], b[arity-1]);
            };
            std::ranges::sort(v, compare);

            auto add = [&](Node n) {
                if (!n.is_blank_node()) {
                    return;
                }
                bn_indices.try_emplace(n, bn_indices.size());
            };
            for (const auto& e : v) {
                for (Node const &n : *e.quad) {
                    add(n);
                }
            }
            std::ranges::sort(v, compare);
        };

        sort(a, arity);
        sort(b, arity);

        // the mapping has to be injective, so both directions are tracked
        std::map<BlankNode, BlankNode> bn_map{};
        std::map<BlankNode, BlankNode> reverse_bn_map{};
        auto check = [&bn_map, &reverse_bn_map](Node to_check, Node expected) {
            if (expected.is_blank_node() && to_check.is_blank_node()) {
                auto i = bn_map.find(expected.as_blank_node());
                if (i != bn_map.end()) {
                    return to_check.as_blank_node() == i->second.as_blank_node();
                }
                if (reverse_bn_map.contains(to_check.as_blank_node())) {
                    return false;
                }
                bn_map[expected.as_blank_node()] = to_check.as_blank_node();
                reverse_bn_map[to_check.as_blank_node()] = expected.as_blank_node();
                return true;
            } else {
                return to_check == expected;
            }
        };

        for (size_t i = 0; i < a.size(); ++i) {
            auto const &ai = *a[i].quad;
            auto const &bi = *b[i].quad;
            for (size_t j = 0; j < arity; ++j) {
                if (!check(ai[j], bi[j])) {
                    return nonstd::unexpected{std::pair{ai[j], bi[j]}};
                }
            }
        }
        return bn_map;
    }
}

#endif //RDF4CPP_GRAPHCOMPARISON_HPP
