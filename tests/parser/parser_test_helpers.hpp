#ifndef RDF4CPP_PARSER_TEST_HELPERS_HPP
#define RDF4CPP_PARSER_TEST_HELPERS_HPP

#include <rdf4cpp.hpp>
#include "tests_DataDirConfig.hpp"

#include <filesystem>
#include <fstream>

namespace rdf4cpp::parse_test_helpers {
    inline void parser_test_positive(std::string check_str, std::string truth_str, std::string_view base_iri, parser::ParsingFlags check_flags, parser::ParsingFlags truth_flags, bool deduplicate = false) {
        using namespace rdf4cpp::parser;

        CAPTURE(base_iri);

        IStreamQuadIterator::state_type state{};
        CHECK_NOTHROW(state.iri_factory.set_base(base_iri));
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
                    // term equality, the same as the graph comparison below uses
                    if (std::ranges::any_of(r, [&](const auto& x) {
                        return x == val;
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
        // try_compare_graphs_fast expects both graphs to be deduplicated
        if (read_iter_to(truth_iter, truth_results, deduplicate)) {
            return;
        }

        std::string expected = "too big";
        std::string actual = "too big";
        if (check_results.size() + truth_results.size() <= 100) {
            expected = writer::StringWriter::oneshot([&](auto& w) {
                for (const auto& e : truth_results) {
                    CHECK(rdf4cpp::Quad{e.graph(), e.subject(), e.predicate(), e.object()}.serialize_nquads(w));
                }
                return true;
            });
            actual = writer::StringWriter::oneshot([&](auto& w) {
                for (const auto& e : check_results) {
                    CHECK(rdf4cpp::Quad{e.graph(), e.subject(), e.predicate(), e.object()}.serialize_nquads(w));
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

        auto cmp = try_compare_graphs_fast<Quad>(check_results, truth_results);
        if (!cmp.has_value()) {
            CAPTURE(cmp.error().first);
            CAPTURE(cmp.error().second);
            FAIL_CHECK("graph comparison failed");
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

    inline std::string parser_test_remote_test_file_to_str(std::string_view file_name, std::filesystem::path const &base_path) {
        std::ifstream ifs{base_path / file_name};
        REQUIRE(ifs);

        std::string buf;
        std::copy(std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{}, std::back_inserter(buf));

        return buf;
    }
}  // namespace rdf4cpp::parse_test_helpers

#endif  //RDF4CPP_PARSER_TEST_HELPERS_HPP
