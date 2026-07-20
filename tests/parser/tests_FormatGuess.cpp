#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>

#include <rdf4cpp.hpp>
#include <rdf4cpp/parser/FormatGuess.hpp>
#include <rdf4cpp/parser/RDFFileParser.hpp>

#include <sstream>

using namespace rdf4cpp;
using namespace rdf4cpp::parser;

TEST_SUITE("FormatGuess") {

    TEST_CASE("guess_format_from_extension") {
        SUBCASE("known extensions") {
            CHECK(guess_format_from_extension(".ttl").syntax == ParsingFlag::Turtle);
            CHECK(guess_format_from_extension(".ttl").confidence == GuessConfidence::High);
            CHECK(guess_format_from_extension(".turtle").syntax == ParsingFlag::Turtle);

            CHECK(guess_format_from_extension(".nt").syntax == ParsingFlag::NTriples);
            CHECK(guess_format_from_extension(".nt").confidence == GuessConfidence::High);
            CHECK(guess_format_from_extension(".ntriples").syntax == ParsingFlag::NTriples);

            CHECK(guess_format_from_extension(".nq").syntax == ParsingFlag::NQuads);
            CHECK(guess_format_from_extension(".nq").confidence == GuessConfidence::High);
            CHECK(guess_format_from_extension(".nquads").syntax == ParsingFlag::NQuads);

            CHECK(guess_format_from_extension(".trig").syntax == ParsingFlag::TriG);
            CHECK(guess_format_from_extension(".trig").confidence == GuessConfidence::High);

            CHECK(guess_format_from_extension(".rdf").syntax == ParsingFlag::RdfXml);
            CHECK(guess_format_from_extension(".rdf").confidence == GuessConfidence::High);

            CHECK(guess_format_from_extension(".owx").syntax == ParsingFlag::OwlXml);
            CHECK(guess_format_from_extension(".owx").confidence == GuessConfidence::High);

            CHECK(guess_format_from_extension(".jsonld").syntax == ParsingFlag::JsonLd);
            CHECK(guess_format_from_extension(".jsonld").confidence == GuessConfidence::High);
        }

        SUBCASE("ambiguous extensions") {
            CHECK(guess_format_from_extension(".owl").confidence == GuessConfidence::Low);
            CHECK(guess_format_from_extension(".xml").confidence == GuessConfidence::Low);
        }

        SUBCASE("unknown extensions") {
            CHECK_FALSE(guess_format_from_extension(".gz").is_known());
            CHECK_FALSE(guess_format_from_extension(".csv").is_known());
            CHECK_FALSE(guess_format_from_extension(".txt").is_known());
            CHECK_FALSE(guess_format_from_extension("").is_known());
        }

        SUBCASE("case insensitive") {
            CHECK(guess_format_from_extension(".TTL").syntax == ParsingFlag::Turtle);
            CHECK(guess_format_from_extension(".Nt").syntax == ParsingFlag::NTriples);
            CHECK(guess_format_from_extension(".NQ").syntax == ParsingFlag::NQuads);
            CHECK(guess_format_from_extension(".TRIG").syntax == ParsingFlag::TriG);
            CHECK(guess_format_from_extension(".RDF").syntax == ParsingFlag::RdfXml);
            CHECK(guess_format_from_extension(".JSONLD").syntax == ParsingFlag::JsonLd);
        }
    }

    TEST_CASE("guess_format_from_path") {
        CHECK(guess_format_from_path("/path/to/file.ttl").syntax == ParsingFlag::Turtle);
        CHECK(guess_format_from_path("/some/dir/data.nt").syntax == ParsingFlag::NTriples);
        CHECK(guess_format_from_path("file.nq").syntax == ParsingFlag::NQuads);
        CHECK(guess_format_from_path("/a/b/c.trig").syntax == ParsingFlag::TriG);
        CHECK(guess_format_from_path("ontology.rdf").syntax == ParsingFlag::RdfXml);
        CHECK(guess_format_from_path("data.jsonld").syntax == ParsingFlag::JsonLd);

        SUBCASE("no extension") {
            CHECK_FALSE(guess_format_from_path("/path/to/file").is_known());
            CHECK_FALSE(guess_format_from_path("").is_known());
        }

        SUBCASE("path separators") {
            CHECK(guess_format_from_path("C:\\Users\\data.ttl").syntax == ParsingFlag::Turtle);
            CHECK(guess_format_from_path("/home/user/data.ttl").syntax == ParsingFlag::Turtle);
        }
    }

    TEST_CASE("guess_format_from_content") {

        SUBCASE("N-Triples") {
            constexpr char const *nt_content =
                "<http://example/s> <http://example/p> \"object\" .\n"
                "<http://example/s> <http://example/p> <http://example/o> .\n";
            auto guess = guess_format_from_content(nt_content);
            CHECK(guess.syntax == ParsingFlag::NTriples);
            CHECK(guess.is_known());
        }

        SUBCASE("N-Quads") {
            constexpr char const *nq_content =
                "<http://example/s> <http://example/p> <http://example/o> <http://example/g> .\n"
                "<http://example/s2> <http://example/p2> <http://example/o2> .\n";
            auto guess = guess_format_from_content(nq_content);
            CHECK(guess.syntax == ParsingFlag::NQuads);
            CHECK(guess.is_known());
        }

        SUBCASE("Turtle with @prefix") {
            constexpr char const *ttl_content =
                "@prefix ex: <http://example.org/> .\n"
                "ex:s ex:p ex:o .\n";
            auto guess = guess_format_from_content(ttl_content);
            CHECK(guess.syntax == ParsingFlag::Turtle);
            CHECK(guess.is_known());
        }

        SUBCASE("Turtle with @base") {
            constexpr char const *ttl_content =
                "@base <http://example.org/> .\n"
                "<s> <p> <o> .\n";
            auto guess = guess_format_from_content(ttl_content);
            CHECK(guess.syntax == ParsingFlag::Turtle);
            CHECK(guess.is_known());
        }

        SUBCASE("Turtle with SPARQL-style PREFIX") {
            constexpr char const *ttl_content =
                "PREFIX ex: <http://example.org/>\n"
                "ex:s ex:p ex:o .\n";
            auto guess = guess_format_from_content(ttl_content);
            CHECK(guess.syntax == ParsingFlag::Turtle);
            CHECK(guess.is_known());
        }

        SUBCASE("TriG with GRAPH keyword") {
            constexpr char const *trig_content =
                "@prefix ex: <http://example.org/> .\n"
                "GRAPH ex:g { ex:s ex:p ex:o . }\n";
            auto guess = guess_format_from_content(trig_content);
            CHECK(guess.syntax == ParsingFlag::TriG);
            CHECK(guess.is_known());
        }

        SUBCASE("TriG with curly braces") {
            constexpr char const *trig_content =
                "@prefix ex: <http://example.org/> .\n"
                "ex:g { ex:s ex:p ex:o . }\n";
            auto guess = guess_format_from_content(trig_content);
            CHECK(guess.syntax == ParsingFlag::TriG);
            CHECK(guess.is_known());
        }

        SUBCASE("RDF/XML with xml declaration") {
            constexpr char const *rdfxml_content =
                "<?xml version=\"1.0\"?>\n"
                "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
                "</rdf:RDF>";
            auto guess = guess_format_from_content(rdfxml_content);
            CHECK(guess.syntax == ParsingFlag::RdfXml);
            CHECK(guess.is_known());
        }

        SUBCASE("RDF/XML without xml declaration") {
            constexpr char const *rdfxml_content =
                "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
                "  <rdf:Description rdf:about=\"http://example.org/s\">\n"
                "  </rdf:Description>\n"
                "</rdf:RDF>";
            auto guess = guess_format_from_content(rdfxml_content);
            CHECK(guess.syntax == ParsingFlag::RdfXml);
            CHECK(guess.is_known());
        }

        SUBCASE("OWL/XML detection") {
            constexpr char const *owlxml_content =
                "<?xml version=\"1.0\"?>\n"
                "<Ontology xmlns=\"http://www.w3.org/2002/07/owl#\">\n"
                "  <Declaration><Class IRI=\"#Foo\"/></Declaration>\n"
                "</Ontology>";
            auto guess = guess_format_from_content(owlxml_content);
            CHECK(guess.syntax == ParsingFlag::OwlXml);
            CHECK(guess.is_known());
        }

        SUBCASE("JSON-LD detection") {
            constexpr char const *jsonld_content =
                "{\n"
                "  \"@context\": \"http://schema.org/\",\n"
                "  \"@type\": \"Person\",\n"
                "  \"name\": \"Jane Doe\"\n"
                "}";
            auto guess = guess_format_from_content(jsonld_content);
            CHECK(guess.syntax == ParsingFlag::JsonLd);
            CHECK(guess.is_known());
        }

        SUBCASE("empty content") {
            CHECK_FALSE(guess_format_from_content("").is_known());
            CHECK_FALSE(guess_format_from_content("   ").is_known());
        }

        SUBCASE("BOM handling") {
            std::string bom_ttl = "\xEF\xBB\xBF@prefix ex: <http://example.org/> .\nex:s ex:p ex:o .\n";
            auto guess = guess_format_from_content(bom_ttl);
            CHECK(guess.syntax == ParsingFlag::Turtle);
        }

        SUBCASE("comment-only N-Triples") {
            constexpr char const *nt_content =
                "# This is a comment\n"
                "<http://example/s> <http://example/p> <http://example/o> .\n";
            auto guess = guess_format_from_content(nt_content);
            CHECK(guess.syntax == ParsingFlag::NTriples);
        }

        SUBCASE("blank node subject in N-Triples") {
            constexpr char const *nt_content =
                "_:b1 <http://example/p> <http://example/o> .\n";
            auto guess = guess_format_from_content(nt_content);
            CHECK(guess.syntax == ParsingFlag::NTriples);
        }
    }

    TEST_CASE("guess_format combined") {
        SUBCASE("extension and content agree") {
            constexpr char const *ttl_content = "@prefix ex: <http://example.org/> .\nex:s ex:p ex:o .\n";
            auto guess = guess_format("/path/to/file.ttl", ttl_content);
            CHECK(guess.syntax == ParsingFlag::Turtle);
            CHECK(guess.confidence == GuessConfidence::Certain);
        }

        SUBCASE("extension high, content inconclusive") {
            auto guess = guess_format("/path/to/file.ttl", "");
            CHECK(guess.syntax == ParsingFlag::Turtle);
            CHECK(guess.confidence == GuessConfidence::High);
        }

        SUBCASE("ambiguous extension, content disambiguates to RDF/XML") {
            constexpr char const *rdfxml_content =
                "<?xml version=\"1.0\"?>\n"
                "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
                "</rdf:RDF>";
            auto guess = guess_format("/path/to/ontology.owl", rdfxml_content);
            CHECK(guess.syntax == ParsingFlag::RdfXml);
            CHECK(guess.confidence == GuessConfidence::High);
        }

        SUBCASE("ambiguous extension, content disambiguates to OWL/XML") {
            constexpr char const *owlxml_content =
                "<?xml version=\"1.0\"?>\n"
                "<Ontology xmlns=\"http://www.w3.org/2002/07/owl#\">\n"
                "</Ontology>";
            auto guess = guess_format("/path/to/ontology.owl", owlxml_content);
            CHECK(guess.syntax == ParsingFlag::OwlXml);
        }

        SUBCASE("no extension, content provides guess") {
            constexpr char const *nt_content =
                "<http://example/s> <http://example/p> <http://example/o> .\n";
            auto guess = guess_format("/path/to/data", nt_content);
            CHECK(guess.syntax == ParsingFlag::NTriples);
        }
    }

    TEST_CASE("unsupported format errors") {
        SUBCASE("OWL/XML throws on IStreamQuadIterator") {
            constexpr char const *owlxml_content =
                "<?xml version=\"1.0\"?>\n"
                "<Ontology xmlns=\"http://www.w3.org/2002/07/owl#\">\n"
                "  <Declaration><Class IRI=\"#Foo\"/></Declaration>\n"
                "</Ontology>";
            std::istringstream iss{owlxml_content};
            CHECK_THROWS_AS(IStreamQuadIterator{iss
            }, std::runtime_error);
        }

        SUBCASE("JSON-LD throws on IStreamQuadIterator") {
            constexpr char const *jsonld_content =
                "{\"@context\": \"http://schema.org/\", \"name\": \"Jane\"}";
            std::istringstream iss{jsonld_content};
            CHECK_THROWS_AS(IStreamQuadIterator{iss}, std::runtime_error);
        }

        SUBCASE("explicit OwlXml flag throws") {
            std::istringstream iss{"whatever"};
            CHECK_THROWS_AS((IStreamQuadIterator{iss, ParsingFlag::OwlXml}), std::runtime_error);
        }

        SUBCASE("explicit JsonLd flag throws") {
            std::istringstream iss{"whatever"};
            CHECK_THROWS_AS((IStreamQuadIterator{iss, ParsingFlag::JsonLd}), std::runtime_error);
        }
    }

    TEST_CASE("Auto mode end-to-end with IStreamQuadIterator") {

        SUBCASE("N-Triples auto-detected") {
            constexpr char const *nt_content =
                "<http://example/s> <http://example/p> \"hello\" .\n"
                "<http://example/s> <http://example/p> <http://example/o> .\n";
            std::istringstream iss{nt_content};
            IStreamQuadIterator qit{iss};  // Auto mode (default)

            CHECK(qit.detected_format().syntax == ParsingFlag::NTriples);
            size_t n = 0;
            for (; qit != std::default_sentinel; ++qit) {
                CHECK(qit->has_value());
                ++n;
            }
            CHECK_EQ(n, 2);
        }

        SUBCASE("Turtle auto-detected via @prefix") {
            constexpr char const *ttl_content =
                "@prefix ex: <http://example.org/> .\n"
                "ex:s ex:p \"test\" .\n";
            std::istringstream iss{ttl_content};
            IStreamQuadIterator qit{iss};

            CHECK(qit.detected_format().syntax == ParsingFlag::Turtle);
            size_t n = 0;
            for (; qit != std::default_sentinel; ++qit) {
                CHECK(qit->has_value());
                ++n;
            }
            CHECK_EQ(n, 1);
        }

        SUBCASE("N-Quads auto-detected") {
            constexpr char const *nq_content =
                "<http://example/s> <http://example/p> <http://example/o> <http://example/g> .\n";
            std::istringstream iss{nq_content};
            IStreamQuadIterator qit{iss};

            CHECK(qit.detected_format().syntax == ParsingFlag::NQuads);
            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());
            CHECK(qit->value().graph() == IRI{"http://example/g"});
        }

        SUBCASE("RDF/XML auto-detected") {
            constexpr char const *rdfxml_content =
                "<?xml version=\"1.0\"?>\n"
                "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
                "         xmlns:ex=\"http://example.org/\">\n"
                "  <rdf:Description rdf:about=\"http://example.org/s\">\n"
                "    <ex:p>hello</ex:p>\n"
                "  </rdf:Description>\n"
                "</rdf:RDF>";
            std::istringstream iss{rdfxml_content};
            IStreamQuadIterator qit{iss};

            CHECK(qit.detected_format().syntax == ParsingFlag::RdfXml);
            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());
        }
    }

    TEST_CASE("Explicit flags backward compatibility") {

        SUBCASE("explicit Turtle") {
            constexpr char const *ttl_content =
                "@prefix ex: <http://example.org/> .\n"
                "ex:s ex:p \"test\" .\n";
            std::istringstream iss{ttl_content};
            IStreamQuadIterator qit{iss, ParsingFlag::Turtle};

            CHECK(qit.detected_format().syntax == ParsingFlag::Turtle);
            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());
        }

        SUBCASE("explicit NTriples") {
            std::istringstream iss{R"(<http://example/s> <http://example/p> "string" .)"};
            IStreamQuadIterator qit{iss, ParsingFlag::NTriples};

            CHECK(qit.detected_format().syntax == ParsingFlag::NTriples);
            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());
        }

        SUBCASE("explicit NQuads") {
            std::stringstream str{"<http://example/s> <http://example/p> <http://example/o> <http://example/g> .\n"};
            IStreamQuadIterator qit{str, ParsingFlag::NQuads};

            CHECK(qit.detected_format().syntax == ParsingFlag::NQuads);
            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());
        }

        SUBCASE("explicit TriG") {
            std::stringstream str{"<http://example/g> {<http://example/s> <http://example/p> <http://example/o> .}"};
            IStreamQuadIterator qit{str, ParsingFlag::TriG};

            CHECK(qit.detected_format().syntax == ParsingFlag::TriG);
            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());
        }

        SUBCASE("explicit RdfXml") {
            constexpr char const *rdfxml_content =
                "<?xml version=\"1.0\"?>\n"
                "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
                "         xmlns:ex=\"http://example.org/\">\n"
                "  <rdf:Description rdf:about=\"http://example.org/s\">\n"
                "    <ex:p>hello</ex:p>\n"
                "  </rdf:Description>\n"
                "</rdf:RDF>";
            std::istringstream iss{rdfxml_content};
            IStreamQuadIterator qit{iss, ParsingFlag::RdfXml};

            CHECK(qit.detected_format().syntax == ParsingFlag::RdfXml);
            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());
        }
    }

    TEST_CASE("RDFFileParser auto mode") {

        SUBCASE("ttl file auto-detected") {
            // tests_RDFFileParser_simple.ttl is a pure N-Triples file (no prefixes)
            // Auto mode should detect it and parse correctly
            size_t count = 0;
            for (auto const &v : RDFFileParser{"./tests_RDFFileParser_simple.ttl"}) {
                if (v.has_value()) {
                    ++count;
                } else if (count == 3) {
                    // expected error on the invalid date
                    ++count;
                }
            }
            CHECK(count == 4);
        }
    }

    TEST_CASE("Auto mode with fopen C-like API") {
        SUBCASE("N-Triples via fopen") {
            static constexpr char const *path = "/tmp/rdf4cpp-format-guess-test.nt";
            {
                auto *f = fopen(path, "w");
                fprintf(f, "<http://example/s> <http://example/p> \"hello\" .\n");
                fclose(f);
            }

            auto *f = fopen(path, "r");
            IStreamQuadIterator qit{f,
                                     reinterpret_cast<ReadFunc>(fread),
                                     reinterpret_cast<ErrorFunc>(ferror),
                                     reinterpret_cast<EOFFunc>(feof)};
            CHECK(qit.detected_format().syntax == ParsingFlag::NTriples);
            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());

            ++qit;
            CHECK(qit == std::default_sentinel);

            fclose(f);
            remove(path);
        }
    }
}
