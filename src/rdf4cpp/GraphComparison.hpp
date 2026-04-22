#ifndef RDF4CPP_GRAPHCOMPARISON_HPP
#define RDF4CPP_GRAPHCOMPARISON_HPP

#include <rdf4cpp/Node.hpp>
#include <ranges>
#include <vector>

namespace rdf4cpp {
    /**
     * does a fast graph comparison, handling blank node mappings, by doing intelligent sorting.
     * if this function finds a bn mapping, then the 2 graphs are isomorph.
     * if this function does not find a mapping, a full graph isomorphism check might still find one.
     * (but this function is usually enough for parser tests).
     * expects both graphs to be deduplicated.
     * @tparam Q Quad or Triple (but anything with a std::vector-like API will work)
     * @param graph_a
     * @param graph_b
     * @return blank node mapping from graph_b to graph_a if found, first mismatched node pair otherwise
     */
    template<std::ranges::sized_range Q, std::ranges::range G>
    requires std::ranges::random_access_range<Q> && std::same_as<std::ranges::range_value_t<Q>, std::remove_cvref_t<Node>> && std::same_as<std::ranges::range_value_t<G>, std::remove_cvref_t<Q>>
    nonstd::expected<std::map<BlankNode, BlankNode>, std::pair<Node, Node>> try_compare_graphs_fast(G const & graph_a, G const & graph_b) {
        struct Quad {
            Q const *quad;
            size_t similar_count = 0;
        };

        std::vector<Quad> a{};
        for (const auto& q : graph_a) {
            a.emplace_back(&q);
        }
        std::vector<Quad> b{};
        for (const auto& q : graph_b) {
            b.emplace_back(&q);
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
        static constexpr auto count_sim = [&](Q const &p, std::vector<Quad> const & v) {
            size_t n = 0;
            for (const auto& i : v) {
                size_t sim = 0;
                RDF4CPP_ASSERT(p.size() == i.quad->size());
                for (size_t e = 0; e < p.size(); ++e) {
                    Node const &pe = p[e]; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                    Node const &ie = (*i.quad)[e]; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                    if (!pe.is_blank_node() && !ie.is_blank_node() && pe == ie) {
                        ++sim;
                    }
                }
                if (sim >= 2) {
                    ++n;
                }
            }
            return n;
        };
        static constexpr auto sort = [](std::vector<Quad> &v) {
            for (auto& e : v) {
                e.similar_count = count_sim(*e.quad, v);
            }
            static constexpr size_t not_found = std::numeric_limits<size_t>::max();
            std::vector<Node> bn_indices{};
            auto get_ind = [&](Node n) {
                for (size_t i = 0; i < bn_indices.size(); ++i) {
                    if (bn_indices[i] == n) { // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                        return i;
                    }
                }
                return not_found;
            };
            auto comp = [&](Node a, Node b) {
                if (a.is_blank_node() && b.is_blank_node()) {
                    auto ai = get_ind(a);
                    auto bi = get_ind(b);
                    if (ai != not_found && bi != not_found) {
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
                RDF4CPP_ASSERT(a.size() == b.size());
                for (size_t i = 0; i < a.size(); ++i) {
                    if (a[i] != b[i] && !a[i].is_blank_node() && !b[i].is_blank_node()) {
                        return comp(a[i], b[i]);
                    }
                }
                for (size_t i = 0; i < a.size() - 1; ++i) {
                    if (a[i] != b[i]) {
                        return comp(a[i], b[i]);
                    }
                }
                return comp(a[a.size()-1], b[b.size()-1]);
            };
            std::ranges::sort(v, compare);

            auto add = [&](Node n) {
                if (!n.is_blank_node()) {
                    return;
                }
                if (get_ind(n) == not_found) {
                    bn_indices.emplace_back(n);
                }
            };
            for (const auto& e : v) {
                for (Node const &n : *e.quad) {
                    add(n);
                }
            }
            std::ranges::sort(v, compare);
        };

        sort(a);
        sort(b);

        std::map<BlankNode, BlankNode> bn_map{};
        auto check = [&bn_map](Node to_check, Node expected) {
            if (expected.is_blank_node() && to_check.is_blank_node()) {
                auto i = bn_map.find(expected.as_blank_node());
                if (i != bn_map.end()) {
                    return to_check.as_blank_node() == i->second.as_blank_node();
                } else {
                    bn_map[expected.as_blank_node()] = to_check.as_blank_node();
                    return true;
                }
            } else {
                return to_check == expected;
            }
        };

        RDF4CPP_ASSERT(a.size() == b.size());
        for (size_t i = 0; i < a.size(); ++i) {
            auto const &ai = *a[i].quad;
            auto const &bi = *b[i].quad;
            RDF4CPP_ASSERT(ai.size() == bi.size());
            for (size_t j = 0; j < ai.size(); ++j) {
                if (!check(ai[j], bi[j])) {
                    return nonstd::unexpected{std::pair{ai[j], bi[j]}};
                }
            }
        }
        return bn_map;
    }
}

#endif //RDF4CPP_GRAPHCOMPARISON_HPP
