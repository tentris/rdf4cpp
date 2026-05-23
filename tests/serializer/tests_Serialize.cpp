#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

#include <filesystem>
#include <format>
#include <random>

using namespace rdf4cpp;

TEST_SUITE("Serialize") {
    template<typename NodeT>
    void run_ser_test(NodeT node, std::optional<std::string> expected = std::nullopt) {
        if (!expected.has_value()) {
            expected = std::string{node};
        }

        auto const result = writer::StringWriter::oneshot([node](auto &w) noexcept {
            return node.serialize(w);
        });

        CHECK_EQ(expected, result);
    }

    TEST_CASE("Serialize Variable") {
        SUBCASE("named") {
            auto var = query::Variable::make_named("x");
            run_ser_test(var);
        }

        SUBCASE("named") {
            auto var = query::Variable::make_anonymous("y");
            run_ser_test(var);
        }

        SUBCASE("null") {
            run_ser_test(query::Variable());
        }
    }

    TEST_CASE("Serialize IRI") {
        auto iri = IRI::make("http://url.com#some-iri");
        run_ser_test(iri);
        run_ser_test(IRI());
    }

    TEST_CASE("Serialize BNode") {
        auto bnode = BlankNode::make("123abc");
        run_ser_test(bnode);
        run_ser_test(BlankNode());
    }

    TEST_CASE("Serialize Literal") {
        using namespace rdf4cpp::datatypes;

        SUBCASE("null") {
            run_ser_test(Literal());
        }

        SUBCASE("xsd:string") {
            auto lit = Literal::make_simple("simple");
            run_ser_test(lit, "\"simple\"");
        }
        SUBCASE("non-inlined xsd:integer") {
            auto lit = Literal::make_typed<xsd::Integer>("87960930222089");
            run_ser_test(lit, "\"87960930222089\"^^<http://www.w3.org/2001/XMLSchema#integer>");
        }
        SUBCASE("inlined xsd:int") {
            auto lit = Literal::make_typed_from_value<xsd::Int>(754);
            run_ser_test(lit, "\"754\"^^<http://www.w3.org/2001/XMLSchema#int>");
        }
        SUBCASE("rdf:langString, inlined language") {
            auto lit = Literal::make_lang_tagged("spherical cow", "en");
            run_ser_test(lit, "\"spherical cow\"@en");
        }
        SUBCASE("rdf:langString, non-inlined language") {
            auto lit = Literal::make_lang_tagged("spherical cow", "nonexistent");
            run_ser_test(lit, "\"spherical cow\"@nonexistent");
        }
        SUBCASE("unknown datatype") {
            auto lit = Literal::make_typed("very value", IRI::make("http://datatypes.com#dt"));
            run_ser_test(lit, "\"very value\"^^<http://datatypes.com#dt>");
        }
    }


    constexpr char const *triples = "<http://url.com/subj#1> <http://url.com#pred> \"search\" .\n" // xsd:string
                                    "<http://url.com/subj#2> <http://url.com#pred> \"8796093022208\"^^<http://www.w3.org/2001/XMLSchema#integer> .\n" // non-inlined xsd:integer
                                    "<http://url.com/subj#3> <http://url.com#pred> \"123\"^^<http://www.w3.org/2001/XMLSchema#int> .\n" // inlined xsd:int
                                    "<http://url.com/subj#4> <http://url.com#pred> _:123 .\n" // blank node
                                    "<http://url.com/subj#5> <http://url.com#pred> \"Spherical Cow\"@en .\n" // rdf:langString, inlined language
                                    "<http://url.com/subj#6> <http://url.com#pred> \"Spherical Cow\"@jk .\n" // rdf:langString, non-inlined language
                                    "<http://url.com/subj#7> <http://url.com#pred> \"Spherical Cow\"^^<http://unknow-datatype.org#dt> .\n" // unknown datatype
                                    "<http://url.com_(test)#8> <http://url.com#pred> \"Test \\\"Quoted\\\" String\"@en .\n";

    static auto long_lit_triples = []() {
        std::string long_lit;
        for (size_t ix = 0; ix < 2000; ++ix) {
            long_lit.push_back('a');
        }

        return std::format("<http://website.com#subj> <http://website.com#pred> \"{}\" .\n", long_lit);
    }();

    TEST_CASE("Reserialize NTriples StringWriter") {
        auto do_test = [](auto triples) {
            using namespace rdf4cpp::parser;

            std::string buf;
            buf.reserve(256);
            writer::StringWriter ser{buf};

            std::istringstream iss{triples};
            for (IStreamQuadIterator qit{iss, ParsingFlag::KeepBlankNodeIds}; qit != std::default_sentinel; ++qit) {
                REQUIRE(qit->has_value());
                (*qit)->subject().serialize(ser);
                write_str(" ", ser);
                (*qit)->predicate().serialize(ser);
                write_str(" ", ser);
                (*qit)->object().serialize(ser);
                write_str(" .\n", ser);
            }

            ser.finalize();

            CHECK_EQ(buf, std::string_view{triples});
        };

        SUBCASE("variants") {
            do_test(triples);
        }

        SUBCASE("long input") {
            do_test(long_lit_triples);
        }
    }

    TEST_CASE("Reserialize NTriples BufCFileWriter") {
        auto do_test = [](auto triples) {
            using namespace rdf4cpp::parser;
            std::filesystem::path const path = std::format("/tmp/rdf4cpp-ser-cfile-{}", std::random_device{}());

            {
                FILE *out_file = fopen(path.c_str(), "w");
                REQUIRE(out_file != nullptr);
                setbuf(out_file, nullptr);

                writer::BufCFileWriter ser{out_file};

                std::istringstream iss{triples};
                for (IStreamQuadIterator qit{iss, ParsingFlag::KeepBlankNodeIds}; qit != std::default_sentinel; ++qit) {
                    REQUIRE(qit->has_value());
                    (*qit)->subject().serialize(ser);
                    write_str(" ", ser);
                    (*qit)->predicate().serialize(ser);
                    write_str(" ", ser);
                    (*qit)->object().serialize(ser);
                    write_str(" .\n", ser);
                }

                ser.finalize();
                fclose(out_file);
            }

            {
                std::ifstream ifs{path};
                REQUIRE(ifs.is_open());

                std::string result;
                std::copy(std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{}, std::back_inserter(result));

                CHECK_EQ(result, std::string_view{triples});
            }

            std::filesystem::remove(path);
        };

        SUBCASE("variants") {
            do_test(triples);
        }

        SUBCASE("long input") {
            do_test(long_lit_triples);
        }
    }

    TEST_CASE("Reserialize NTriples BufOStreamWriter") {
        auto do_test = [](auto triples) {
            using namespace rdf4cpp::parser;
            std::filesystem::path const path = std::format("/tmp/rdf4cpp-ser-ostream-{}", std::random_device{}());

            {
                std::ofstream ofs{path};
                REQUIRE(ofs.is_open());

                writer::BufOStreamWriter ser{ofs};

                std::istringstream iss{triples};
                for (IStreamQuadIterator qit{iss, ParsingFlag::KeepBlankNodeIds}; qit != std::default_sentinel; ++qit) {
                    REQUIRE(qit->has_value());
                    (*qit)->subject().serialize(ser);
                    write_str(" ", ser);
                    (*qit)->predicate().serialize(ser);
                    write_str(" ", ser);
                    (*qit)->object().serialize(ser);
                    write_str(" .\n", ser);
                }

                ser.finalize();
            }

            {
                std::ifstream ifs{path};
                REQUIRE(ifs.is_open());

                std::string result;
                std::copy(std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{}, std::back_inserter(result));

                CHECK_EQ(result, std::string_view{triples});
            }

            std::filesystem::remove(path);
        };

        SUBCASE("variants") {
            do_test(triples);
        }

        SUBCASE("long input") {
            do_test(long_lit_triples);
        }
    }

    TEST_CASE("serialize pseudo format") {
        std::vector<Literal> lits{
                Literal::make_typed_from_value<datatypes::xsd::Int>(5),
                Literal::make_typed_from_value<datatypes::xsd::Long>(10),
                Literal::make_typed_from_value<datatypes::xsd::Long>(std::numeric_limits<datatypes::xsd::Long::cpp_type>::max()),
                Literal::make_simple("Spherical Cow"),
                Literal::make_lang_tagged("Spherical Cow", "en"),
                Literal::make_lang_tagged("Cow", "spherical")
        };

        std::string_view const expected = R"(lex:5,dt:http://www.w3.org/2001/XMLSchema#int,lang:
lex:10,dt:http://www.w3.org/2001/XMLSchema#long,lang:
lex:9223372036854775807,dt:http://www.w3.org/2001/XMLSchema#long,lang:
lex:Spherical Cow,dt:http://www.w3.org/2001/XMLSchema#string,lang:
lex:Spherical Cow,dt:http://www.w3.org/1999/02/22-rdf-syntax-ns#langString,lang:en
lex:Cow,dt:http://www.w3.org/1999/02/22-rdf-syntax-ns#langString,lang:spherical
)";

        std::string buf;
        writer::StringWriter w{buf};

        std::string lexbuf;
        writer::StringWriter lexw{lexbuf};

        for (auto l : lits) {
            writer::write_str("lex:", w);

            {
                lexw.clear();
                std::string_view lex;
                auto const r = l.fetch_or_serialize_lexical_form(lex, lexw);

                switch (r) {
                    case FetchOrSerializeResult::Fetched: {
                        writer::write_str(lex, w);
                        break;
                    }
                    case FetchOrSerializeResult::Serialized: {
                        writer::write_str(lexw.view(), w);
                        break;
                    }
                    default: {
                        FAIL("Serialization failed");
                        break;
                    }
                }
            }

            writer::write_str(",dt:", w);
            writer::write_str(l.datatype().identifier(), w);
            writer::write_str(",lang:", w);
            writer::write_str(l.language_tag(), w);
            writer::write_str("\n", w);
        }

        CHECK_EQ(w.view(), expected);
    }

    struct TinyBuffer {
        char buffer;
    };

    /**
     * A serializer that fails to flush its data and has only a single byte of buffer space
     * Implements `BufWriter`
     */
    struct FailWriter : writer::BufWriterBase<FailWriter, TinyBuffer> {
        using Buffer = TinyBuffer;

        FailWriter() {
            write_area() = &buffer().buffer;
            write_area_size() = 1;
        }

        bool finalize() noexcept {
            return false; // fail
        }

        static void flush_impl([[maybe_unused]] Buffer &buffer,
                               [[maybe_unused]] char *&write_area,
                               [[maybe_unused]] size_t &write_area_size,
                               [[maybe_unused]] size_t additional_cap) noexcept {
            // pretend that flush failed, noop
        }
    };

    TEST_CASE("fetch_or_serialize fail") {
        SUBCASE("bnode") {
            auto bn1 = BlankNode::make("abc");
            CHECK(bn1.is_inlined());

            FailWriter w;
            std::string_view fetched;
            auto const res = bn1.fetch_or_serialize_identifier(fetched, w);
            CHECK_EQ(res, FetchOrSerializeResult::SerializationFailed);
        }

        SUBCASE("variable") {
            SUBCASE("named") {
                auto v = query::Variable::make_named("abc");
                CHECK(v.is_inlined());

                FailWriter w;
                std::string_view fetched;
                auto const res = v.fetch_or_serialize_name(fetched, w);
                CHECK_EQ(res, FetchOrSerializeResult::SerializationFailed);
            }

            SUBCASE("anon") {
                auto v = query::Variable::make_anonymous("abc");
                CHECK(v.is_inlined());

                FailWriter w;
                std::string_view fetched;
                auto const res = v.fetch_or_serialize_name(fetched, w);
                CHECK_EQ(res, FetchOrSerializeResult::SerializationFailed);
            }
        }

        SUBCASE("literal") {
            auto l = Literal::make_typed_from_value<datatypes::xsd::Float>(1.3);
            CHECK(l.is_inlined());

            FailWriter w;
            std::string_view fetched;

            SUBCASE("lexical_form") {
                auto const res = l.fetch_or_serialize_lexical_form(fetched, w);
                CHECK_EQ(res, FetchOrSerializeResult::SerializationFailed);
            }

            SUBCASE("lexical_form") {
                auto const res = l.fetch_or_serialize_simplified_lexical_form(fetched, w);
                CHECK_EQ(res, FetchOrSerializeResult::SerializationFailed);
            }
        }
    }
}
