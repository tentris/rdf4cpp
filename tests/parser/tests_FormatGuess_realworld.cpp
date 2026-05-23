#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>

#include <rdf4cpp.hpp>
#include <rdf4cpp/parser/FormatGuess.hpp>

#include <curl/curl.h>
#include <sstream>
#include <string>

using namespace rdf4cpp;
using namespace rdf4cpp::parser;

// --- CURL helper (adopted from tests_XMLParser.cpp) ---

static size_t write_callback(void const *contents, size_t size, size_t nmemb, void *userp) {
    static_cast<std::string *>(userp)->append(static_cast<char const *>(contents), size * nmemb);
    return size * nmemb;
}

static std::string fetch_url(std::string const &url) {
    std::string result;
    CURL *curl = curl_easy_init();
    REQUIRE(curl != nullptr);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    REQUIRE_EQ(res, CURLE_OK);
    return result;
}

/**
 * Check content sniffing detects expected format, check combined guess with filename,
 * and for supported formats verify auto-mode parsing produces quads.
 * For unsupported formats, verify an exception is thrown.
 */
static void check_detection_and_parse(std::string const &content,
                                       std::string const &filename,
                                       ParsingFlag expected_syntax,
                                       bool expect_parseable) {
    INFO("file: ", filename);

    auto prefix = std::string_view{content}.substr(0, 4096);
    auto content_guess = guess_format_from_content(prefix);
    CHECK_MESSAGE(content_guess.is_known(), "Content sniffing should produce a known format for ", filename);
    CHECK_MESSAGE(content_guess.syntax == expected_syntax,
                  "Expected syntax ", static_cast<int>(expected_syntax),
                  " but got ", static_cast<int>(content_guess.syntax), " for ", filename);

    auto combined = guess_format(filename, prefix);
    CHECK_MESSAGE(combined.is_known(), "Combined guess should be known for ", filename);
    CHECK_MESSAGE(combined.syntax == expected_syntax,
                  "Combined guess expected ", static_cast<int>(expected_syntax),
                  " but got ", static_cast<int>(combined.syntax), " for ", filename);

    if (expect_parseable) {
        std::istringstream iss{content};
        IStreamQuadIterator qit{iss};
        CHECK_MESSAGE(qit.detected_format().syntax == expected_syntax,
                      "Parser detected wrong format for ", filename);
        size_t quad_count = 0;
        for (; qit != std::default_sentinel; ++qit) {
            if (qit->has_value()) ++quad_count;
        }
        CHECK_MESSAGE(quad_count > 0, "Expected at least one quad from ", filename);
    } else {
        std::istringstream iss{content};
        CHECK_THROWS_AS(IStreamQuadIterator{iss}, std::runtime_error);
    }
}

// ============================================================================
// Real-world file tests — download and verify detection + parsing
// ============================================================================

TEST_SUITE("FormatGuess real-world files") {

    // --- N-Triples (.nt) ---

    TEST_CASE("N-Triples: W3C rdf-n-triples tests") {
        auto const base = std::string("https://raw.githubusercontent.com/w3c/rdf-tests/main/rdf/rdf11/rdf-n-triples/");

        SUBCASE("literal.nt") {
            check_detection_and_parse(fetch_url(base + "literal.nt"), "literal.nt", ParsingFlag::NTriples, true);
        }
        SUBCASE("literal_all_controls.nt") {
            check_detection_and_parse(fetch_url(base + "literal_all_controls.nt"), "literal_all_controls.nt", ParsingFlag::NTriples, true);
        }
        SUBCASE("langtagged_string.nt") {
            check_detection_and_parse(fetch_url(base + "langtagged_string.nt"), "langtagged_string.nt", ParsingFlag::NTriples, true);
        }
        SUBCASE("comment_following_triple.nt") {
            check_detection_and_parse(fetch_url(base + "comment_following_triple.nt"), "comment_following_triple.nt", ParsingFlag::NTriples, true);
        }
        SUBCASE("literal_true.nt") {
            check_detection_and_parse(fetch_url(base + "literal_true.nt"), "literal_true.nt", ParsingFlag::NTriples, true);
        }
    }

    TEST_CASE("N-Triples: W3C Turtle test outputs (.nt)") {
        auto const base = std::string("https://raw.githubusercontent.com/w3c/rdf-tests/main/rdf/rdf11/rdf-turtle/");

        SUBCASE("first.nt") {
            check_detection_and_parse(fetch_url(base + "first.nt"), "first.nt", ParsingFlag::NTriples, true);
        }
        SUBCASE("IRIREF_datatype.nt") {
            check_detection_and_parse(fetch_url(base + "IRIREF_datatype.nt"), "IRIREF_datatype.nt", ParsingFlag::NTriples, true);
        }
        SUBCASE("bareword_a_predicate.nt") {
            check_detection_and_parse(fetch_url(base + "bareword_a_predicate.nt"), "bareword_a_predicate.nt", ParsingFlag::NTriples, true);
        }
    }

    TEST_CASE("N-Triples: Serd test outputs") {
        auto const base = std::string("https://raw.githubusercontent.com/drobilla/serd/main/test/extra/abbreviate/");

        SUBCASE("collapse-predicates.nt") {
            check_detection_and_parse(fetch_url(base + "collapse-predicates.nt"), "collapse-predicates.nt", ParsingFlag::NTriples, true);
        }
        SUBCASE("collapse-subjects.nt") {
            check_detection_and_parse(fetch_url(base + "collapse-subjects.nt"), "collapse-subjects.nt", ParsingFlag::NTriples, true);
        }
    }

    // --- N-Quads (.nq) ---

    TEST_CASE("N-Quads: W3C rdf-trig test outputs (.nq)") {
        auto const base = std::string("https://raw.githubusercontent.com/w3c/rdf-tests/main/rdf/rdf11/rdf-trig/");

        // NQ files with 4 terms should be detected as NQuads
        SUBCASE("alternating_iri_graphs.nq") {
            check_detection_and_parse(fetch_url(base + "alternating_iri_graphs.nq"), "alternating_iri_graphs.nq", ParsingFlag::NQuads, true);
        }
    }

    // --- Turtle (.ttl) ---

    TEST_CASE("Turtle: W3C rdf-turtle tests") {
        auto const base = std::string("https://raw.githubusercontent.com/w3c/rdf-tests/main/rdf/rdf11/rdf-turtle/");

        SUBCASE("first.ttl") {
            check_detection_and_parse(fetch_url(base + "first.ttl"), "first.ttl", ParsingFlag::Turtle, true);
        }
        SUBCASE("SPARQL_style_prefix.ttl") {
            check_detection_and_parse(fetch_url(base + "SPARQL_style_prefix.ttl"), "SPARQL_style_prefix.ttl", ParsingFlag::Turtle, true);
        }
        SUBCASE("SPARQL_style_base.ttl") {
            check_detection_and_parse(fetch_url(base + "SPARQL_style_base.ttl"), "SPARQL_style_base.ttl", ParsingFlag::Turtle, true);
        }
        SUBCASE("bareword_a_predicate.ttl") {
            check_detection_and_parse(fetch_url(base + "bareword_a_predicate.ttl"), "bareword_a_predicate.ttl", ParsingFlag::Turtle, true);
        }
        SUBCASE("collection_object.ttl") {
            check_detection_and_parse(fetch_url(base + "collection_object.ttl"), "collection_object.ttl", ParsingFlag::Turtle, true);
        }
        SUBCASE("labeled_blank_node_subject.ttl") {
            // Content is `_:s <p> <o> .` which is valid N-Triples (subset of Turtle).
            // Content sniffing correctly detects NTriples; combined guess with .ttl
            // extension overrides to Turtle.
            auto content = fetch_url(base + "labeled_blank_node_subject.ttl");
            auto prefix = std::string_view{content}.substr(0, 4096);
            auto content_guess = guess_format_from_content(prefix);
            CHECK(content_guess.syntax == ParsingFlag::NTriples);

            auto combined = guess_format("labeled_blank_node_subject.ttl", prefix);
            CHECK(combined.syntax == ParsingFlag::Turtle);

            std::istringstream iss{content};
            IStreamQuadIterator qit{iss};
            size_t count = 0;
            for (; qit != std::default_sentinel; ++qit) {
                if (qit->has_value()) ++count;
            }
            CHECK(count > 0);
        }
    }

    TEST_CASE("Turtle: Serd project file") {
        SUBCASE("serd.ttl") {
            check_detection_and_parse(fetch_url("https://raw.githubusercontent.com/drobilla/serd/main/serd.ttl"), "serd.ttl", ParsingFlag::Turtle, true);
        }
    }

    TEST_CASE("Turtle: Serd abbreviation tests") {
        auto const base = std::string("https://raw.githubusercontent.com/drobilla/serd/main/test/extra/abbreviate/");

        SUBCASE("collapse-predicates.ttl") {
            check_detection_and_parse(fetch_url(base + "collapse-predicates.ttl"), "collapse-predicates.ttl", ParsingFlag::Turtle, true);
        }
        SUBCASE("collapse-subjects.ttl") {
            check_detection_and_parse(fetch_url(base + "collapse-subjects.ttl"), "collapse-subjects.ttl", ParsingFlag::Turtle, true);
        }
    }

    // --- TriG (.trig) ---

    TEST_CASE("TriG: W3C rdf-trig tests") {
        auto const base = std::string("https://raw.githubusercontent.com/w3c/rdf-tests/main/rdf/rdf11/rdf-trig/");

        SUBCASE("LITERAL1.trig") {
            check_detection_and_parse(fetch_url(base + "LITERAL1.trig"), "LITERAL1.trig", ParsingFlag::TriG, true);
        }
        SUBCASE("trig-kw-graph-01.trig") {
            check_detection_and_parse(fetch_url(base + "trig-kw-graph-01.trig"), "trig-kw-graph-01.trig", ParsingFlag::TriG, true);
        }
        SUBCASE("alternating_iri_graphs.trig") {
            check_detection_and_parse(fetch_url(base + "alternating_iri_graphs.trig"), "alternating_iri_graphs.trig", ParsingFlag::TriG, true);
        }
        SUBCASE("anonymous_blank_node_graph.trig") {
            check_detection_and_parse(fetch_url(base + "anonymous_blank_node_graph.trig"), "anonymous_blank_node_graph.trig", ParsingFlag::TriG, true);
        }
        SUBCASE("labeled_blank_node_graph.trig") {
            check_detection_and_parse(fetch_url(base + "labeled_blank_node_graph.trig"), "labeled_blank_node_graph.trig", ParsingFlag::TriG, true);
        }
    }

    // --- RDF/XML (.rdf) ---

    TEST_CASE("RDF/XML: W3C rdf-xml tests") {
        auto const base = std::string("https://raw.githubusercontent.com/w3c/rdf-tests/main/rdf/rdf11/rdf-xml/");

        SUBCASE("amp-in-url/test001.rdf") {
            check_detection_and_parse(fetch_url(base + "amp-in-url/test001.rdf"), "test001.rdf", ParsingFlag::RdfXml, true);
        }
        SUBCASE("datatypes/test001.rdf") {
            check_detection_and_parse(fetch_url(base + "datatypes/test001.rdf"), "datatypes_test001.rdf", ParsingFlag::RdfXml, true);
        }
        SUBCASE("rdf-charmod-literals/test001.rdf") {
            check_detection_and_parse(fetch_url(base + "rdf-charmod-literals/test001.rdf"), "rdf-charmod-literals_test001.rdf", ParsingFlag::RdfXml, true);
        }
        SUBCASE("rdf-charmod-uris/test001.rdf") {
            check_detection_and_parse(fetch_url(base + "rdf-charmod-uris/test001.rdf"), "rdf-charmod-uris_test001.rdf", ParsingFlag::RdfXml, true);
        }
        SUBCASE("rdf-containers-syntax-vs-schema/test001.rdf") {
            check_detection_and_parse(fetch_url(base + "rdf-containers-syntax-vs-schema/test001.rdf"), "rdf-containers_test001.rdf", ParsingFlag::RdfXml, true);
        }
    }

    TEST_CASE("RDF/XML: .owl files with rdf:RDF root (ambiguous extension)") {
        SUBCASE("pizza.owl — RDF/XML with .owl extension") {
            auto content = fetch_url("https://raw.githubusercontent.com/owlcs/pizza-ontology/master/pizza.owl");
            auto prefix = std::string_view{content}.substr(0, 4096);
            auto guess = guess_format("pizza.owl", prefix);
            CHECK(guess.syntax == ParsingFlag::RdfXml);
            CHECK(guess.is_known());

            std::istringstream iss{content};
            IStreamQuadIterator qit{iss};
            CHECK(qit.detected_format().syntax == ParsingFlag::RdfXml);
            size_t count = 0;
            for (; qit != std::default_sentinel; ++qit) {
                if (qit->has_value()) ++count;
            }
            CHECK(count > 0);
        }
    }

    // --- OWL/XML (.owx) — detected but unsupported ---

    TEST_CASE("OWL/XML: horned-owl test files (.owx)") {
        auto const base = std::string("https://raw.githubusercontent.com/phillord/horned-owl/main/src/ont/owl-xml/");

        SUBCASE("class.owx") {
            check_detection_and_parse(fetch_url(base + "class.owx"), "class.owx", ParsingFlag::OwlXml, false);
        }
        SUBCASE("annotation.owx") {
            check_detection_and_parse(fetch_url(base + "annotation.owx"), "annotation.owx", ParsingFlag::OwlXml, false);
        }
        SUBCASE("and.owx") {
            check_detection_and_parse(fetch_url(base + "and.owx"), "and.owx", ParsingFlag::OwlXml, false);
        }
        SUBCASE("class-assertion.owx") {
            check_detection_and_parse(fetch_url(base + "class-assertion.owx"), "class-assertion.owx", ParsingFlag::OwlXml, false);
        }
        SUBCASE("ontology-annotation.owx") {
            check_detection_and_parse(fetch_url(base + "ontology-annotation.owx"), "ontology-annotation.owx", ParsingFlag::OwlXml, false);
        }
    }

    TEST_CASE("OWL/XML: .owl files with Ontology root (ambiguous extension)") {
        SUBCASE("Time.owl — OWL/XML with .owl extension") {
            auto content = fetch_url("https://raw.githubusercontent.com/usnistgov/pdso/master/OWL/Time.owl");
            auto prefix = std::string_view{content}.substr(0, 4096);
            auto guess = guess_format("Time.owl", prefix);
            CHECK(guess.syntax == ParsingFlag::OwlXml);

            std::istringstream iss{content};
            CHECK_THROWS_AS(IStreamQuadIterator{iss}, std::runtime_error);
        }
    }

    // --- JSON-LD (.jsonld) — detected but unsupported ---

    TEST_CASE("JSON-LD: W3C json-ld-api examples") {
        auto const base = std::string("https://raw.githubusercontent.com/w3c/json-ld-api/main/examples/");

        SUBCASE("Sample-JSON-LD-document.jsonld") {
            check_detection_and_parse(fetch_url(base + "Sample-JSON-LD-document.jsonld"), "Sample-JSON-LD-document.jsonld", ParsingFlag::JsonLd, false);
        }
        SUBCASE("Compacted-sample-document-compacted.jsonld") {
            check_detection_and_parse(fetch_url(base + "Compacted-sample-document-compacted.jsonld"), "Compacted-sample-document-compacted.jsonld", ParsingFlag::JsonLd, false);
        }
        SUBCASE("Expanded-sample-document.jsonld") {
            check_detection_and_parse(fetch_url(base + "Expanded-sample-document.jsonld"), "Expanded-sample-document.jsonld", ParsingFlag::JsonLd, false);
        }
        SUBCASE("JSON-LD-document-in-compact-form.jsonld") {
            check_detection_and_parse(fetch_url(base + "JSON-LD-document-in-compact-form.jsonld"), "JSON-LD-document-in-compact-form.jsonld", ParsingFlag::JsonLd, false);
        }
    }

    TEST_CASE("JSON-LD: json-ld.org context files") {
        SUBCASE("person.jsonld") {
            check_detection_and_parse(fetch_url("https://raw.githubusercontent.com/json-ld/json-ld.org/main/contexts/person.jsonld"), "person.jsonld", ParsingFlag::JsonLd, false);
        }
    }
}
