#ifndef RDF4CPP_PARSER_TEST_HELPERS_HPP
#define RDF4CPP_PARSER_TEST_HELPERS_HPP

#include <rdf4cpp.hpp>

namespace rdf4cpp::parse_test_helpers {
    inline void jsonld_test_positive(std::string check_str, std::string truth_str, std::string_view base_iri, parser::ParsingFlags check_flags, parser::ParsingFlags truth_flags, bool deduplicate = false) {  // TODO move test logic to shared with xml
        using namespace rdf4cpp::parser;

        CAPTURE(base_iri);

        IStreamQuadIterator::state_type state{};
        CHECK(state.iri_factory.set_base(base_iri) == IRIFactoryError::Ok);
        std::stringstream check_stream{std::move(check_str)};
        IStreamQuadIterator check_iter{check_stream, check_flags, &state};
        std::vector<query::QuadPattern> check_results;

        std::stringstream truth_stream{std::move(truth_str)};
        IStreamQuadIterator truth_iter{truth_stream, truth_flags};
        std::vector<query::QuadPattern> truth_results;

        static constexpr auto read_iter_to = [](IStreamQuadIterator &i, std::vector<query::QuadPattern> &r, bool dedup) {
            for (;i != std::default_sentinel; ++i) {
                if (!i->has_value()) {
                    FAIL(i->error().message);
                }
                auto& val = i->value();
                if (dedup) {
                    if (std::ranges::any_of(r, [&](const auto& x) {
                        return x.graph().eq(val.graph()) && x.subject().eq(val.subject()) && x.predicate().eq(val.predicate()) && x.object().eq(val.object());
                    })) {
                        continue;
                    }
                }
                r.emplace_back(val);
            }
        };
        read_iter_to(check_iter, check_results, deduplicate);
        read_iter_to(truth_iter, truth_results, false);

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
        static constexpr auto sort = [](std::vector<query::QuadPattern> &v) {
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
            auto sort = [&](query::QuadPattern const &a, query::QuadPattern const &b) {
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
            std::ranges::sort(v, sort);

            auto add = [&](Node n) {
                if (!n.is_blank_node()) {
                    return;
                }
                if (get_ind(n) == not_found) {
                    bn_indices.emplace_back(n);
                }
            };
            for (const auto& e : v) {
                add(e.graph());
                add(e.subject());
                add(e.predicate());
                add(e.object());
            }
            std::ranges::sort(v, sort);
        };
        sort(check_results);
        sort(truth_results);

        std::string capture = "too big";
        if (check_results.size() + truth_results.size() <= 100) {
            capture.clear();
            writer::StringWriter w{capture};
            writer::write_str("expected:\n", w);
            for (const auto& e : truth_results) {
                CHECK(Quad{e.graph(), e.subject(), e.predicate(), e.object()}.serialize_nquads(w));
            }
            writer::write_str("actual:\n", w);
            for (const auto& e : check_results) {
                CHECK(Quad{e.graph(), e.subject(), e.predicate(), e.object()}.serialize_nquads(w));
            }
            w.finalize();
        }
        CAPTURE(capture);
        REQUIRE(check_results.size() == truth_results.size());

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
            check(check_results.at(i).graph(), truth_results.at(i).graph(), "graph");
            check(check_results.at(i).subject(), truth_results.at(i).subject(), "subject");
            check(check_results.at(i).predicate(), truth_results.at(i).predicate(), "predicate");
            check(check_results.at(i).object(), truth_results.at(i).object(), "object");
        }
    }
}  // namespace rdf4cpp::parse_test_helpers

#endif  //RDF4CPP_PARSER_TEST_HELPERS_HPP
