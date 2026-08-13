#ifndef RDF4CPP_PARSER_TEST_HELPERS_HPP
#define RDF4CPP_PARSER_TEST_HELPERS_HPP

#include <rdf4cpp.hpp>

#include <filesystem>
#include <curl/curl.h>

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

    // source: https://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c/9786295#9786295
    inline size_t write_callback(void const *contents, size_t size, size_t nmemb, void *userp) {
        static_cast<std::string *>(userp)->append(static_cast<char const *>(contents), size * nmemb);
        return size * nmemb;
    }

    std::string inline parser_test_remote_test_file_to_str(std::string_view file_name, std::string_view base_url, std::string_view cache_file) {
        if (!std::filesystem::exists(cache_file)) { // this ends up in the cmake folder, next to the executable
            std::filesystem::create_directories(cache_file);
        }
        auto cache = std::filesystem::path(cache_file) / file_name;
        if (std::filesystem::exists(cache)) {
            std::ifstream ifs(cache);
            return std::string{std::istreambuf_iterator{ifs}, {}};
        }
        CURLcode curl_res = CURLE_FAILED_INIT;
        long response_code = 0;
        auto const url = std::format("{}/{}", base_url, file_name);
        std::string file_contents_as_str;
        CURL *curl = curl_easy_init();
        REQUIRE(curl != nullptr);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file_contents_as_str);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // for https
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_cleanup(curl);
        CAPTURE(url);
        CAPTURE(response_code);
        // an error page must not end up in the cache, it would be replayed by every later run
        REQUIRE_EQ(curl_res, CURLE_OK);
        REQUIRE_EQ(response_code, 200);
        std::filesystem::create_directories(cache.parent_path());
        std::ofstream ofs(cache);
        ofs << file_contents_as_str;
        return file_contents_as_str;
    }
}  // namespace rdf4cpp::parse_test_helpers

#endif  //RDF4CPP_PARSER_TEST_HELPERS_HPP
