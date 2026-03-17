#ifndef RDF4CPP_PARSER_TEST_HELPERS_HPP
#define RDF4CPP_PARSER_TEST_HELPERS_HPP

#include <rdf4cpp.hpp>

namespace rdf4cpp::parse_test_helpers {
    inline void jsonld_test_positive(std::string check_str, std::string truth_str, std::string_view base_iri, parser::ParsingFlags check_flags, parser::ParsingFlags truth_flags) {  // TODO move test logic to shared with xml
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

        static constexpr auto read_iter_to = [](IStreamQuadIterator &i, std::vector<query::QuadPattern> &r) {
            while (i != std::default_sentinel) {
                if (!i->has_value()) {
                    FAIL(i->error().message);
                }
                r.emplace_back(i->value());
                ++i;
            }
        };
        read_iter_to(check_iter, check_results);
        read_iter_to(truth_iter, truth_results);

        REQUIRE(check_results.size() == truth_results.size());

        static constexpr auto num_blanks = [](query::QuadPattern const &p) {
            size_t n = 0;
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
            std::sort(v.begin(), v.end(), [](query::QuadPattern const &a, query::QuadPattern const &b) {
                auto a_bl = num_blanks(a);
                auto b_bl = num_blanks(b);
                if (a_bl != b_bl) {
                    return std::less{}(a_bl, b_bl);
                }
                if (a.subject() != b.subject() && !a.subject().is_blank_node() && !b.subject().is_blank_node()) {
                    return std::less{}(a.subject(), b.subject());
                }
                if (a.predicate() != b.predicate() && !a.predicate().is_blank_node() && !b.predicate().is_blank_node()) {
                    return std::less{}(a.predicate(), b.predicate());
                }
                if (!a.object().is_blank_node() && !b.object().is_blank_node()) {
                    return std::less{}(a.object(), b.object());
                }
                if (a.subject() != b.subject()) {
                    return std::less{}(a.subject(), b.subject());
                }
                if (a.predicate() != b.predicate()) {
                    return std::less{}(a.predicate(), b.predicate());
                }
                return std::less{}(a.object(), b.object());
            });
        };
        sort(check_results);
        sort(truth_results);

        std::map<BlankNode, BlankNode> bn_map{};
        auto check = [&bn_map](Node xml, Node nt, std::string_view pos) {
            CAPTURE(pos);
            if (nt.is_blank_node() && xml.is_blank_node()) {
                auto i = bn_map.find(nt.as_blank_node());
                if (i != bn_map.end()) {
                    CHECK(xml.as_blank_node() == i->second.as_blank_node());
                } else {
                    bn_map[nt.as_blank_node()] = xml.as_blank_node();
                }
            } else {
                CHECK(xml == nt);
            }
        };

        for (size_t i = 0; i < truth_results.size(); ++i) {
            check(check_results.at(i).subject(), truth_results.at(i).subject(), "subject");
            check(check_results.at(i).predicate(), truth_results.at(i).predicate(), "predicate");
            check(check_results.at(i).object(), truth_results.at(i).object(), "object");
        }
    }
}  // namespace rdf4cpp::parse_test_helpers

#endif  //RDF4CPP_PARSER_TEST_HELPERS_HPP
