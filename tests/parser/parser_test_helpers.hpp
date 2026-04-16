#ifndef RDF4CPP_PARSER_TEST_HELPERS_HPP
#define RDF4CPP_PARSER_TEST_HELPERS_HPP

#include <rdf4cpp.hpp>

namespace rdf4cpp::parse_test_helpers {
    // TODO move graph comparison out of tests
    inline void jsonld_test_positive(std::string check_str, std::string truth_str, std::string_view base_iri, parser::ParsingFlags check_flags, parser::ParsingFlags truth_flags, bool deduplicate = false) {
        using namespace rdf4cpp::parser;

        struct Quad {
            query::QuadPattern quad;
            size_t similar_count = 0;
        };

        CAPTURE(base_iri);

        IStreamQuadIterator::state_type state{};
        CHECK(state.iri_factory.set_base(base_iri) == IRIFactoryError::Ok);
        std::stringstream check_stream{std::move(check_str)};
        IStreamQuadIterator check_iter{check_stream, check_flags, &state};
        std::vector<Quad> check_results;

        std::stringstream truth_stream{std::move(truth_str)};
        IStreamQuadIterator truth_iter{truth_stream, truth_flags};
        std::vector<Quad> truth_results;

        static constexpr auto read_iter_to = [](IStreamQuadIterator &i, std::vector<Quad> &r, bool dedup) {
            for (;i != std::default_sentinel; ++i) {
                if (!i->has_value()) {
                    FAIL_CHECK(i->error().message);
                    return true;
                }
                auto& val = i->value();
                if (dedup) {
                    if (std::ranges::any_of(r, [&](const auto& y) {
                        const auto& x = y.quad;
                        return x.graph().eq(val.graph()) && x.subject().eq(val.subject()) && x.predicate().eq(val.predicate()) && x.object().eq(val.object());
                    })) {
                        continue;
                    }
                }
                r.emplace_back(val);
            }
            return false;
        };
        if (read_iter_to(check_iter, check_results, deduplicate)) {
            return;
        }
        if (read_iter_to(truth_iter, truth_results, false)) {
            return;
        }

        static constexpr auto num_blanks = [](query::QuadPattern const &p) {
            size_t n = 0;
            if (p.graph().is_blank_node()) {
                ++n;
            }
            if (p.subject().is_blank_node()) {
                ++n;
            }
            if (p.predicate().is_blank_node()) {
                ++n;
            }
            if (p.object().is_blank_node()) {
                ++n;
            }
            return n;
        };
        static constexpr auto count_sim = [&](query::QuadPattern const &p, std::vector<Quad> const & v) {
            size_t n = 0;
            for (const auto& i : v) {
                size_t sim = 0;
                for (size_t e = 0; e < p.size(); ++e) {
                    const auto& pe = p[e]; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
                    const auto& ie = i.quad[e]; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
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
                e.similar_count = count_sim(e.quad, v);
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
                const auto& a = aq.quad;
                const auto& b = bq.quad;
                auto a_bl = num_blanks(a);
                auto b_bl = num_blanks(b);
                if (a_bl != b_bl) {
                    return std::less{}(a_bl, b_bl);
                }
                if (a.graph() != b.graph() && !a.graph().is_blank_node() && !b.graph().is_blank_node()) {
                    return comp(a.graph(), b.graph());
                }
                if (a.subject() != b.subject() && !a.subject().is_blank_node() && !b.subject().is_blank_node()) {
                    return comp(a.subject(), b.subject());
                }
                if (a.predicate() != b.predicate() && !a.predicate().is_blank_node() && !b.predicate().is_blank_node()) {
                    return comp(a.predicate(), b.predicate());
                }
                if (a.object() != b.object() && !a.object().is_blank_node() && !b.object().is_blank_node()) {
                    return comp(a.object(), b.object());
                }
                if (a.graph() != b.graph()) {
                    return comp(a.graph(), b.graph());
                }
                if (a.subject() != b.subject()) {
                    return comp(a.subject(), b.subject());
                }
                if (a.predicate() != b.predicate()) {
                    return comp(a.predicate(), b.predicate());
                }
                return comp(a.object(), b.object());
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
                add(e.quad.graph());
                add(e.quad.subject());
                add(e.quad.predicate());
                add(e.quad.object());
            }
            std::ranges::sort(v, compare);
        };
        sort(check_results);
        sort(truth_results);

        std::string expected = "too big";
        std::string actual = "too big";
        if (check_results.size() + truth_results.size() <= 100) {
            expected = writer::StringWriter::oneshot([&](auto& w) {
                for (const auto& e : truth_results) {
                    CHECK(rdf4cpp::Quad{e.quad.graph(), e.quad.subject(), e.quad.predicate(), e.quad.object()}.serialize_nquads(w));
                }
                return true;
            });
            actual = writer::StringWriter::oneshot([&](auto& w) {
                for (const auto& e : check_results) {
                    CHECK(rdf4cpp::Quad{e.quad.graph(), e.quad.subject(), e.quad.predicate(), e.quad.object()}.serialize_nquads(w));
                }
                return true;
            });
        }
        CAPTURE(expected);
        CAPTURE(actual);
        CHECK(check_results.size() == truth_results.size());
        if (check_results.size() != truth_results.size()) {
            return;
        }

        std::map<BlankNode, BlankNode> bn_map{};
        auto check = [&bn_map](Node to_check, Node expected, std::string_view pos) {
            CAPTURE(pos);
            if (expected.is_blank_node() && to_check.is_blank_node()) {
                auto i = bn_map.find(expected.as_blank_node());
                if (i != bn_map.end()) {
                    CHECK(to_check.as_blank_node() == i->second.as_blank_node());
                } else {
                    bn_map[expected.as_blank_node()] = to_check.as_blank_node();
                }
            } else {
                CHECK(to_check == expected);
            }
        };

        for (size_t i = 0; i < truth_results.size(); ++i) {
            check(check_results.at(i).quad.graph(), truth_results.at(i).quad.graph(), "graph");
            check(check_results.at(i).quad.subject(), truth_results.at(i).quad.subject(), "subject");
            check(check_results.at(i).quad.predicate(), truth_results.at(i).quad.predicate(), "predicate");
            check(check_results.at(i).quad.object(), truth_results.at(i).quad.object(), "object");
        }
    }

    inline void parser_test_negative(std::string check_str, std::string_view base_iri, parser::ParsingFlags flags) {
        using namespace rdf4cpp::parser;

        CAPTURE(base_iri);

        std::stringstream xml{std::move(check_str)};
        IStreamQuadIterator xml_iter{xml, flags};

        bool had_error = false;
        while (xml_iter != std::default_sentinel) {
            if (xml_iter->has_value()) {
                ++xml_iter;
                continue;
            }
            had_error = true;
            ++xml_iter;
        }
        CHECK(had_error);
    }
}  // namespace rdf4cpp::parse_test_helpers

#endif  //RDF4CPP_PARSER_TEST_HELPERS_HPP
