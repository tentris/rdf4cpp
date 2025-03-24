#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <rdf4cpp.hpp>
#include <rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.hpp>
#include <rdf4cpp/storage/reference_node_storage/SyncReferenceNodeStorage.hpp>

#include <thread>

using namespace rdf4cpp;
using namespace rdf4cpp::storage;

int main(int argc, char **argv) {
    {
        reference_node_storage::SyncReferenceNodeStorage syncns{};
        default_node_storage = syncns;
        auto ret = doctest::Context{argc, argv}.run();
        if (ret != 0) {
            return ret;
        }
    }

    {
        reference_node_storage::UnsyncReferenceNodeStorage unsyncns{};
        default_node_storage = unsyncns;
        return doctest::Context{argc, argv}.run();
    }
}

void check_literal_type(Node const &lit) {
    CHECK_FALSE(lit.is_blank_node());
    CHECK(lit.is_literal());
    CHECK_FALSE(lit.is_variable());
    CHECK_FALSE(lit.is_iri());
}

TEST_CASE("Literal - Check for only lexical form") {

    auto iri = IRI{"http://www.w3.org/2001/XMLSchema#string"};
    auto lit1 = Literal::make_simple("Bunny");

    check_literal_type(lit1);
    CHECK_EQ(lit1.lexical_form(), "Bunny");
    CHECK_EQ(lit1.datatype(), iri);
    CHECK_EQ(lit1.language_tag(), "");
    CHECK_EQ(std::string(lit1), "\"Bunny\"");
}

TEST_CASE("Literal - Check for lexical form with IRI") {
    SUBCASE("string datatype") {
        auto iri = IRI{"http://www.w3.org/2001/XMLSchema#string"};
        auto lit1 = Literal::make_typed("Bunny", iri);

        check_literal_type(lit1);
        CHECK_EQ(lit1.lexical_form(), "Bunny");
        CHECK_EQ(lit1.datatype(), iri);
        CHECK_EQ(lit1.language_tag(), "");
        CHECK_EQ(std::string(lit1), "\"Bunny\"");

        [[maybe_unused]] Literal no_discard_dummy;
        CHECK_THROWS_AS(no_discard_dummy = Literal::make_simple("\xc3\x28"), std::runtime_error);
        CHECK_THROWS_AS(no_discard_dummy = Literal::make_lang_tagged("\xc3\x28", "de"), std::runtime_error);
        CHECK_EQ(Literal::make_simple_normalize("\u0174", storage::default_node_storage), Literal::make_simple_normalize("W\u0302", storage::default_node_storage));  // 2 different ways of writing Ŵ
    }
    SUBCASE("int datatype") {
        auto iri = IRI{"http://www.w3.org/2001/XMLSchema#int"};
        auto lit1 = Literal::make_typed("101", iri);

        check_literal_type(lit1);
        CHECK_EQ(lit1.lexical_form(), "101");
        CHECK_EQ(lit1.datatype(), iri);
        CHECK_EQ(lit1.language_tag(), "");
        CHECK_EQ(std::string(lit1), "\"101\"^^<http://www.w3.org/2001/XMLSchema#int>");
    }
    SUBCASE("date datatype") {
        auto iri = IRI{"http://www.w3.org/2001/XMLSchema#date"};
        auto lit1 = Literal::make_typed("2021-11-21", iri);

        check_literal_type(lit1);
        CHECK_EQ(lit1.lexical_form(), "2021-11-21");
        CHECK_EQ(lit1.datatype(), iri);
        CHECK_EQ(lit1.language_tag(), "");
        CHECK_EQ(std::string(lit1), "\"2021-11-21\"^^<http://www.w3.org/2001/XMLSchema#date>");
    }
    SUBCASE("decimal datatype") {
        auto iri = IRI{"http://www.w3.org/2001/XMLSchema#decimal"};
        auto lit1 = Literal::make_typed("2.0", iri);

        check_literal_type(lit1);
        CHECK_EQ(lit1.lexical_form(), "2.0");
        CHECK_EQ(lit1.datatype(), iri);
        CHECK_EQ(lit1.language_tag(), "");
        CHECK_EQ(std::string(lit1), "\"2.0\"^^<http://www.w3.org/2001/XMLSchema#decimal>");
    }
    SUBCASE("boolean datatype - true") {
        auto iri = IRI{"http://www.w3.org/2001/XMLSchema#boolean"};
        auto lit1 = Literal::make_typed("true", iri);

        check_literal_type(lit1);
        CHECK_EQ(lit1.lexical_form(), "true");
        CHECK_EQ(lit1.datatype(), iri);
        CHECK_EQ(lit1.language_tag(), "");
        CHECK_EQ(std::string(lit1), "\"true\"^^<http://www.w3.org/2001/XMLSchema#boolean>");
    }
    SUBCASE("boolean datatype - false") {
        auto iri = IRI{"http://www.w3.org/2001/XMLSchema#boolean"};
        auto lit1 = Literal::make_typed("false", iri);

        check_literal_type(lit1);
        CHECK_EQ(lit1.lexical_form(), "false");
        CHECK_EQ(lit1.datatype(), iri);
        CHECK_EQ(lit1.language_tag(), "");
        CHECK_EQ(std::string(lit1), "\"false\"^^<http://www.w3.org/2001/XMLSchema#boolean>");
    }
    SUBCASE("boolean datatype - 0") {
        auto iri = IRI{"http://www.w3.org/2001/XMLSchema#boolean"};
        auto lit1 = Literal::make_typed("0", iri);

        check_literal_type(lit1);
        CHECK_EQ(lit1.lexical_form(), "false");
        CHECK_EQ(lit1.datatype(), iri);
        CHECK_EQ(lit1.language_tag(), "");
        CHECK_EQ(std::string(lit1), "\"false\"^^<http://www.w3.org/2001/XMLSchema#boolean>");
    }
    SUBCASE("boolean datatype - 1") {
        auto iri = IRI{"http://www.w3.org/2001/XMLSchema#boolean"};
        auto lit1 = Literal::make_typed("1", iri);

        check_literal_type(lit1);
        CHECK_EQ(lit1.lexical_form(), "true");
        CHECK_EQ(lit1.datatype(), iri);
        CHECK_EQ(lit1.language_tag(), "");
        CHECK_EQ(std::string(lit1), "\"true\"^^<http://www.w3.org/2001/XMLSchema#boolean>");
    }
}

TEST_CASE("Literal - Check for lexical form with language tag") {

    auto iri = IRI{"http://www.w3.org/1999/02/22-rdf-syntax-ns#langString"};
    auto lit1 = Literal::make_lang_tagged("Bunny", "en");

    check_literal_type(lit1);
    CHECK_EQ(lit1.lexical_form(), "Bunny");
    CHECK_EQ(lit1.datatype(), iri);
    CHECK_EQ(lit1.language_tag(), "en");
    CHECK_EQ(std::string(lit1), "\"Bunny\"@en");
}

TEST_CASE("Literal - ctor edge-case") {
    IRI const iri{"http://www.w3.org/2001/XMLSchema#int"};
    auto const lit1 = Literal::make_typed("1", iri);
    auto const lit2 = Literal::make_typed("2", iri);

    auto const expected = Literal::make_typed("3", iri);
    CHECK_EQ(lit1 + lit2, expected);
}

TEST_CASE("Literal - check fixed id") {
    IRI const iri{datatypes::registry::xsd_string};
    auto const lit = Literal::make_typed("hello", iri);

    CHECK(lit.backend_handle().node_id().literal_type().is_fixed());
    CHECK_LT(lit.datatype().backend_handle().node_id().to_underlying(), datatypes::registry::min_dynamic_datatype_id);
    CHECK_LT(iri.backend_handle().node_id().to_underlying(), datatypes::registry::min_dynamic_datatype_id);
}

TEST_CASE("Literal - casting") {
    using namespace datatypes::xsd;

    auto const lit1 = Literal::make_typed_from_value<datatypes::xsd::Int>(123);

    SUBCASE("id cast") {
        auto const lit1 = Literal::make_typed_from_value<String>("hello");
        auto const lit2 = lit1.template cast<String>();

        CHECK_EQ(lit2.datatype(), IRI{String::identifier});
        CHECK_EQ(lit2.template value<String>(), "hello");
    }

    SUBCASE("str -> any") {
        auto const lit1 = Literal::make_typed_from_value<String>("1.2");
        auto const lit2 = lit1.template cast<Float>();

        CHECK_EQ(lit2.datatype(), IRI{Float::identifier});
        CHECK_EQ(lit2.value<Float>(), 1.2f);
    }

    SUBCASE("str -> boolean") {
        SUBCASE("word-form") {
            auto const lit1 = Literal::make_typed_from_value<String>("true");
            auto const lit2 = lit1.template cast<Boolean>();

            CHECK_EQ(lit2.datatype(), IRI{Boolean::identifier});
            CHECK(lit2.value<Boolean>());
        }

        SUBCASE("numeric form") {
            auto const lit1 = Literal::make_typed_from_value<String>("1");
            auto const lit2 = lit1.template cast<Boolean>();

            CHECK_EQ(lit2.datatype(), IRI{Boolean::identifier});
            CHECK(lit2.value<Boolean>());
        }
    }

    SUBCASE("any -> str") {
        SUBCASE("decimal") {
            SUBCASE("integral") {
                auto const lit1 = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type{"001005.000"});
                auto const lit2 = lit1.cast<String>();

                CHECK_EQ(lit2.value<String>(), "1005");
            }

            SUBCASE("non-integral") {
                auto const lit1 = Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.5));
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "1.5");
            }
        }

        SUBCASE("float") {
            SUBCASE("fixed notation - non-integral") {
                auto const lit1 = Literal::make_typed_from_value<Float>(10.5f);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "10.5");
            }

            SUBCASE("fixed notation - integral") {
                auto const lit1 = Literal::make_typed_from_value<Float>(100000.f);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "100000");
            }

            SUBCASE("large - scientific") {
                auto const lit1 = Literal::make_typed_from_value<Float>(1000001.f);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "1.000001E6");
            }

            SUBCASE("small - scientific") {
                auto const lit1 = Literal::make_typed_from_value<Float>(0.0000009f);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "9.0E-7");
            }

            SUBCASE("zero") {
                auto const lit1 = Literal::make_typed_from_value<Float>(0.0f);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "0");
            }

            SUBCASE("minus zero") {
                auto const lit1 = Literal::make_typed_from_value<Float>(-0.0f);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "-0");
            }
        }

        SUBCASE("double") {
            SUBCASE("fixed notation - non-integral") {
                auto const lit1 = Literal::make_typed_from_value<Double>(10.5);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "10.5");
            }

            SUBCASE("fixed notation - integral") {
                auto const lit1 = Literal::make_typed_from_value<Double>(100000.0);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "100000");
            }

            SUBCASE("large - scientific") {
                auto const lit1 = Literal::make_typed_from_value<Double>(1000001.0);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "1.000001E6");
            }

            SUBCASE("small - scientific") {
                auto const lit1 = Literal::make_typed_from_value<Double>(0.0000009);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "9.0E-7");
            }

            SUBCASE("zero") {
                auto const lit1 = Literal::make_typed_from_value<Double>(0.0);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "0");
            }

            SUBCASE("minus zero") {
                auto const lit1 = Literal::make_typed_from_value<Double>(-0.0);
                auto const lit2 = lit1.template cast<String>();

                CHECK_EQ(lit2.template value<String>(), "-0");
            }
        }
    }

    SUBCASE("any -> bool") {
        auto const lit1 = Literal::make_typed_from_value<Float>(1.4);
        auto const lit2 = lit1.template cast<Boolean>();

        CHECK_EQ(lit2.datatype(), IRI{Boolean::identifier});
        CHECK_EQ(lit2.template value<Boolean>(), true);
    }

    SUBCASE("downcast: dbl -> flt") {
        auto const lit1 = Literal::make_typed_from_value<Double>(1.4);
        auto const lit2 = lit1.template cast<Float>();

        CHECK_EQ(lit2.datatype(), IRI{Float::identifier});
        CHECK_EQ(lit2.template value<Float>(), 1.4f);
    }

    SUBCASE("dec -> flt") {
        auto const lit1 = Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.0));
        auto const lit2 = lit1.template cast<Float>();

        CHECK_EQ(lit2.datatype(), IRI{Float::identifier});
        CHECK_EQ(lit2.template value<Float>(), 1.f);
    }

    SUBCASE("dec -> dbl") {
        auto const lit1 = Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.0));
        auto const lit2 = lit1.template cast<Double>();

        CHECK_EQ(lit2.datatype(), IRI{Double::identifier});
        CHECK_EQ(lit2.template value<Double>(), 1.0);
    }

    SUBCASE("dec -> int") {
        auto const lit1 = Literal::make_typed_from_value<Decimal>(rdf4cpp::BigDecimal(1.2));
        auto const lit2 = lit1.template cast<Int>();

        CHECK_EQ(lit2.datatype(), IRI{Int::identifier});
        CHECK_EQ(lit2.template value<Int>(), 1);
    }

    SUBCASE("downcast: int -> dec") {
        auto const lit1 = Literal::make_typed_from_value<Integer>(1);
        auto const lit2 = lit1.template cast<Decimal>();

        CHECK_EQ(lit2.datatype(), IRI{Decimal::identifier});
        CHECK_EQ(lit2.template value<Decimal>(), 1);
    }

    SUBCASE("int -> flt") {
        auto const lit1 = Literal::make_typed_from_value<Integer>(1);
        auto const lit2 = lit1.template cast<Float>();

        CHECK_EQ(lit2.datatype(), IRI{Float::identifier});
        CHECK_EQ(lit2.template value<Float>(), 1.f);
    }

    SUBCASE("int -> dbl") {
        auto const lit1 = Literal::make_typed_from_value<Integer>(1);
        auto const lit2 = lit1.template cast<Double>();

        CHECK_EQ(lit2.datatype(), IRI{Double::identifier});
        CHECK_EQ(lit2.template value<Double>(), 1.0);
    }

    SUBCASE("id cast") {
        auto const lit1 = Literal::make_typed_from_value<Int>(5);
        auto const lit2 = lit1.template cast<Int>();

        CHECK_EQ(lit1, lit2);
    }

    SUBCASE("bool -> numeric") {
        SUBCASE("integers") {
            SUBCASE("regular case") {
                auto const lit1 = Literal::make_typed_from_value<Boolean>(true);
                auto const lit2 = lit1.template cast<Byte>();
                CHECK(!lit2.null());
                CHECK_EQ(lit2.datatype(), IRI{Byte::identifier});
                CHECK_EQ(lit2.template value<Byte>(), 1);
            }

            SUBCASE("partially representable - representable case") {
                auto const lit3 = Literal::make_typed_from_value<Boolean>(false);
                auto const lit4 = lit3.template cast<NonPositiveInteger>();
                CHECK(!lit4.null());
                CHECK_EQ(lit4.datatype(), IRI{NonPositiveInteger::identifier});
                CHECK_EQ(lit4.template value<NonPositiveInteger>(), 0);
            }

            SUBCASE("partially representable - unrepresentable case") {
                auto const lit3 = Literal::make_typed_from_value<Boolean>(true);
                auto const lit4 = lit3.template cast<NegativeInteger>();
                CHECK(lit4.null());
            }
        }

        SUBCASE("decimal") {
            auto const lit1 = Literal::make_typed_from_value<Boolean>(false);
            auto const lit2 = lit1.template cast<Decimal>();
            CHECK(!lit2.null());
            CHECK_EQ(lit2.datatype(), IRI{Decimal::identifier});
            CHECK_EQ(lit2.template value<Decimal>(), 0.0);
        }

        SUBCASE("float") {
            auto const lit1 = Literal::make_typed_from_value<Boolean>(true);
            auto const lit2 = lit1.template cast<Float>();
            CHECK(!lit2.null());
            CHECK_EQ(lit2.datatype(), IRI{Float::identifier});
            CHECK_EQ(lit2.template value<Float>(), 1.f);
        }

        SUBCASE("double") {
            auto const lit1 = Literal::make_typed_from_value<Boolean>(false);
            auto const lit2 = lit1.template cast<Double>();
            CHECK(!lit2.null());
            CHECK_EQ(lit2.datatype(), IRI{Double::identifier});
            CHECK_EQ(lit2.template value<Double>(), 0.0);
        }
    }

    SUBCASE("cross hierarchy: int -> unsignedInt") {
        auto const lit1 = Literal::make_typed_from_value<Int>(1);
        auto const lit2 = lit1.template cast<UnsignedInt>();

        CHECK_EQ(lit2.datatype(), IRI{UnsignedInt::identifier});
        CHECK_EQ(lit2.value<UnsignedInt>(), 1);
    }

    SUBCASE("subtypes") {
        CHECK_EQ(lit1.template cast<Integer>().datatype(), IRI{Integer::identifier});
        CHECK_EQ(lit1.template cast<Float>().datatype(), IRI{Float::identifier});

        auto const lit2 = Literal::make_typed_from_value<Integer>(420);
        CHECK_EQ(lit2.template cast<Int>(), Literal::make_typed_from_value<Int>(420));
    }

    SUBCASE("value too large") {
        auto const lit1 = Literal::make_typed_from_value<Int>(67000);
        auto const lit2 = lit1.template cast<Short>();

        CHECK(lit2.null());
    }

    SUBCASE("negative to unsigned") {
        auto const lit1 = Literal::make_typed_from_value<Int>(-10);
        auto const lit2 = lit1.template cast<UnsignedInt>();

        CHECK(lit2.null());
    }

    SUBCASE("proper truncation") {
        auto const lit1 = Literal::make_typed_from_value<Float>(-7.875E0);
        auto const lit2 = lit1.cast<Integer>();

        CHECK_EQ(lit2.value<Integer>(), -7);
    }
}

TEST_CASE("Literal - misc functions") {
    using namespace rdf4cpp;

    SUBCASE("rand") {
        SUBCASE("same thread") {
            auto const l1 = Literal::generate_random_double();
            auto const l2 = Literal::generate_random_double();

            CHECK_GE(l1, 0.0_xsd_double);
            CHECK_LT(l1, 1.0_xsd_double);

            CHECK_GE(l2, 0.0_xsd_double);
            CHECK_LT(l2, 1.0_xsd_double);
            CHECK_NE(l1, l2);  // note: non-deterministic but should basically never fail
        }

        SUBCASE("difference threads") {
            auto const l1 = Literal::generate_random_double();
            Literal l2;

            std::thread t{[&]() {
                l2 = Literal::generate_random_double();
            }};

            t.join();

            CHECK_NE(l1, l2);  // note: non-deterministic but should basically never fail
        }

        SUBCASE("provided rng determinism") {
            std::vector<Literal> lits;
            for (size_t ix = 0; ix < 100; ++ix) {
                std::default_random_engine rng{};
                lits.push_back(Literal::generate_random_double(rng));
            }

            auto first = lits[0];
            for (size_t ix = 1; ix < lits.size(); ++ix) {
                CHECK_EQ(first, lits[ix]);
            }
        }
    }

    SUBCASE("abs") {
        CHECK_EQ((-1_xsd_int).abs(), 1_xsd_integer);
        CHECK_EQ((-100.0_xsd_double).abs(), 100.0_xsd_double);
        CHECK_EQ((99.0_xsd_float).abs(), 99.0_xsd_float);
        CHECK(("hello"_xsd_string).abs().null());
    }

    SUBCASE("round") {
        CHECK_EQ((99_xsd_int).round(), 99_xsd_integer);
        CHECK_EQ((1.2_xsd_double).round(), 1.0_xsd_double);
        CHECK_EQ((1.5_xsd_double).round(), 2.0_xsd_double);
        CHECK(("hello"_xsd_string).round().null());
    }

    SUBCASE("floor") {
        CHECK_EQ((99_xsd_int).floor(), 99_xsd_integer);
        CHECK_EQ((1.2_xsd_double).floor(), 1.0_xsd_double);
        CHECK_EQ((1.5_xsd_double).floor(), 1.0_xsd_double);
        CHECK(("hello"_xsd_string).floor().null());
    }

    SUBCASE("ceil") {
        CHECK_EQ((99_xsd_int).ceil(), 99_xsd_integer);
        CHECK_EQ((1.2_xsd_double).ceil(), 2.0_xsd_double);
        CHECK_EQ((1.5_xsd_double).ceil(), 2.0_xsd_double);
        CHECK(("hello"_xsd_string).ceil().null());
    }

    SUBCASE("strlen") {
        CHECK_EQ(("12345"_xsd_string).as_strlen(), 5_xsd_integer);
        CHECK(1_xsd_int .as_strlen().null());
        CHECK_EQ(("123"_xsd_string).as_strlen(), 3_xsd_integer);
        CHECK_EQ(Literal::make_lang_tagged("hello", "en").as_strlen(), 5_xsd_integer);

        CHECK_EQ(("z\u00df\u6c34\U0001f34c"_xsd_string).as_strlen(), 4_xsd_integer);  // "zß水🍌"
    }

    SUBCASE("substr") {
        // from https://www.w3.org/TR/xpath-functions/#func-substring
        CHECK_EQ(("motor car"_xsd_string).substr(6_xsd_integer), " car"_xsd_string);
        CHECK_EQ(("metadata"_xsd_string).substr(4_xsd_integer, 3_xsd_integer), "ada"_xsd_string);
        CHECK_EQ(("12345"_xsd_string).substr("1.5"_xsd_decimal, "2.6"_xsd_decimal), "234"_xsd_string);
        CHECK_EQ(("12345"_xsd_string).substr(0_xsd_integer, 3_xsd_integer), "12"_xsd_string);
        CHECK_EQ(("12345"_xsd_string).substr(5_xsd_integer, -3_xsd_integer), ""_xsd_string);
        CHECK_EQ(("12345"_xsd_string).substr(-3_xsd_integer, 5_xsd_integer), "1"_xsd_string);
        CHECK_EQ(("12345"_xsd_string).substr(0_xsd_integer / 0.0_xsd_double, 3_xsd_integer), ""_xsd_string);
        CHECK_EQ(("12345"_xsd_string).substr(1_xsd_integer, 0_xsd_integer / 0.0_xsd_double), ""_xsd_string);
        CHECK_EQ(("12345"_xsd_string).substr(-42_xsd_integer, 1_xsd_integer / 0.0_xsd_double), "12345"_xsd_string);

        // from https://www.w3.org/TR/sparql11-query/#func-substr
        CHECK_EQ(("foobar"_xsd_string).substr(4_xsd_integer), "bar"_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("foobar", "en").substr(4_xsd_integer), Literal::make_lang_tagged("bar", "en"));
        CHECK_EQ(("foobar"_xsd_string).substr(4_xsd_integer, 1_xsd_integer), "b"_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("foobar", "en").substr(4_xsd_integer, 1_xsd_integer), Literal::make_lang_tagged("b", "en"));

        // check correct casting
        auto const s = "Hello World"_xsd_string;
        CHECK_EQ(s.substr(2_xsd_long, -1.3_xsd_double), ""_xsd_string);
        CHECK_EQ(s.substr(2.1_xsd_double, 3.2_xsd_double), "ell"_xsd_string);
        CHECK_EQ(s.substr(100_xsd_integer, 10_xsd_int), ""_xsd_string);

        // unicode
        CHECK_EQ(("met\U0001f34cdata"_xsd_string).substr(4_xsd_integer, 3_xsd_integer), "\U0001f34cda"_xsd_string);
        CHECK_EQ(("abc"_xsd_string).substr(10, 50), ""_xsd_string);
    }

    SUBCASE("langMatches") {
        CHECK(Literal::make_lang_tagged("Hello", "en").as_language_tag_matches_range("*"_xsd_string).ebv());
        CHECK(Literal::make_lang_tagged("Bonjour", "fr").as_language_tag_matches_range("FR"_xsd_string).ebv());
        CHECK(Literal::make_lang_tagged("Hello", "en-US").as_language_tag_matches_range("en-US"_xsd_string).ebv());
        CHECK((5_xsd_int).as_language_tag_matches_range("*"_xsd_string).null());
        CHECK(("Hello"_xsd_string).as_language_tag_matches_range(""_xsd_string).ebv());
        CHECK_EQ(("Hello"_xsd_string).as_language_tag_matches_range("*"_xsd_string).ebv(), TriBool::False);
    }

    static constexpr const char *case_number1 = "4.2";
    static constexpr const char *case_number2 = "4,2";
    SUBCASE("ucase") {
        // from https://www.w3.org/TR/sparql11-query/#func-ucase
        CHECK_EQ(("foo"_xsd_string).uppercase(), "FOO"_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("foo", "en").uppercase(), Literal::make_lang_tagged("FOO", "en"));
        CHECK_EQ(Literal::make_simple("\xce\xbb").uppercase(), Literal::make_simple("\xce\x9b"));  // greek lambda
        CHECK_EQ(Literal::make_simple("\xc3\xa4").uppercase(), Literal::make_simple("\xc3\x84"));  // a diaresis
        CHECK_EQ(Literal::make_simple("\xd0\xbe").uppercase(), Literal::make_simple("\xd0\x9e"));  // cyrillic o
        CHECK_EQ(Literal::make_simple(case_number1).uppercase(), Literal::make_simple(case_number1));
        CHECK_EQ(Literal::make_simple(case_number2).uppercase(), Literal::make_simple(case_number2));
        CHECK_EQ(Literal::make_simple("\xc3\x9f").uppercase(), Literal::make_simple("SS"));  // german sharp s
    }

    SUBCASE("lcase") {
        // from https://www.w3.org/TR/sparql11-query/#func-lcase
        CHECK_EQ(("BAR"_xsd_string).lowercase(), "bar"_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("BAR", "en").lowercase(), Literal::make_lang_tagged("bar", "en"));
        CHECK_EQ(Literal::make_simple("\xce\x9b").lowercase(), Literal::make_simple("\xce\xbb"));  // greek lambda
        CHECK_EQ(Literal::make_simple("\xc3\x84").lowercase(), Literal::make_simple("\xc3\xa4"));  // a diaresis
        CHECK_EQ(Literal::make_simple("\xd0\x9e").lowercase(), Literal::make_simple("\xd0\xbe"));  // cyrillic o
        CHECK_EQ(Literal::make_simple(case_number1).lowercase(), Literal::make_simple(case_number1));
        CHECK_EQ(Literal::make_simple(case_number2).lowercase(), Literal::make_simple(case_number2));
    }

    SUBCASE("contains") {
        // from https://www.w3.org/TR/sparql11-query/#func-contains
        CHECK(("foobar"_xsd_string).as_contains("bar"_xsd_string).ebv());
        CHECK(Literal::make_lang_tagged("foobar", "en").as_contains(Literal::make_lang_tagged("foo", "en")).ebv());
        CHECK(Literal::make_lang_tagged("foobar", "en").as_contains("bar"_xsd_string).ebv());

        CHECK(Literal::make_lang_tagged("hello", "en").as_contains(Literal::make_lang_tagged("o", "fr")).null());
        CHECK(("123"_xsd_string).as_contains(Literal::make_lang_tagged("1", "en")).null());

        // unicode
        CHECK(Literal::make_lang_tagged_normalize("fo\u0174obar", "en", storage::default_node_storage)
                      .as_contains(
                              Literal::make_lang_tagged_normalize("foW\u0302o", "en", storage::default_node_storage))
                      .ebv());  // 2 different ways of writing Ŵ
    }

    SUBCASE("substr_before") {
        // from https://www.w3.org/TR/sparql11-query/#func-strbefore
        CHECK_EQ(("abc"_xsd_string).substr_before("b"_xsd_string), "a"_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_before("bc"_xsd_string), Literal::make_lang_tagged("a", "en"));
        CHECK(Literal::make_lang_tagged("abc", "en").substr_before(Literal::make_lang_tagged("b", "cy")).null());
        CHECK_EQ(("abc"_xsd_string).substr_before(""_xsd_string), ""_xsd_string);
        CHECK_EQ(("abc"_xsd_string).substr_before("xyz"_xsd_string), ""_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_before(Literal::make_lang_tagged("z", "en")), ""_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_before("z"_xsd_string), ""_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_before(Literal::make_lang_tagged("", "en")), Literal::make_lang_tagged("", "en"));
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_before(""_xsd_string), Literal::make_lang_tagged("", "en"));

        // unicode
        CHECK(Literal::make_simple_normalize("abc\U0001f34c\u0174",
                                             storage::default_node_storage)
                      .substr_before(Literal::make_simple_normalize("W\u0302", storage::default_node_storage)) == "abc\U0001f34c"_xsd_string);  // 2 different ways of writing Ŵ and a 🍌
    }

    SUBCASE("substr_after") {
        // from https://www.w3.org/TR/sparql11-query/#func-strafter
        CHECK_EQ(("abc"_xsd_string).substr_after("b"_xsd_string), "c"_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_after("ab"_xsd_string), Literal::make_lang_tagged("c", "en"));
        CHECK(Literal::make_lang_tagged("abc", "en").substr_after(Literal::make_lang_tagged("b", "cy")).null());
        CHECK_EQ(("abc"_xsd_string).substr_after(""_xsd_string), "abc"_xsd_string);
        CHECK_EQ(("abc"_xsd_string).substr_after("xyz"_xsd_string), ""_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_after(Literal::make_lang_tagged("z", "en")), ""_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_after("z"_xsd_string), ""_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_after(Literal::make_lang_tagged("", "en")), Literal::make_lang_tagged("abc", "en"));
        CHECK_EQ(Literal::make_lang_tagged("abc", "en").substr_after(""_xsd_string), Literal::make_lang_tagged("abc", "en"));

        // unicode
        CHECK(Literal::make_simple_normalize("a\U0001f34cb\u0174c\U0001f34c",
                                             storage::default_node_storage)
                      .substr_after(Literal::make_simple_normalize("W\u0302", storage::default_node_storage)) == "c\U0001f34c"_xsd_string);  // 2 different ways of writing Ŵ and a 🍌
    }

    SUBCASE("str_start_with") {
        // from https://www.w3.org/TR/sparql11-query/#func-strstarts
        CHECK(("foobar"_xsd_string).as_str_starts_with("foo"_xsd_string).ebv());
        CHECK(Literal::make_lang_tagged("foobar", "en").as_str_starts_with(Literal::make_lang_tagged("foo", "en")).ebv());
        CHECK(Literal::make_lang_tagged("foobar", "en").as_str_starts_with("foo"_xsd_string).ebv());

        CHECK(Literal::make_lang_tagged("foobar", "fr").as_str_starts_with(Literal::make_lang_tagged("foo", "en")).null());
        CHECK(("foobar"_xsd_string).as_str_starts_with(Literal::make_lang_tagged("foo", "en")).null());

        // unicode
        CHECK(Literal::make_lang_tagged_normalize("\u0174foobar", "en", storage::default_node_storage)
                      .as_str_starts_with(Literal::make_simple_normalize("W\u0302foo", storage::default_node_storage))
                      .ebv());  // 2 different ways of writing Ŵ
    }

    SUBCASE("str_ends_with") {
        // from https://www.w3.org/TR/sparql11-query/#func-strstarts
        CHECK(("foobar"_xsd_string).as_str_ends_with("bar"_xsd_string).ebv());
        CHECK(Literal::make_lang_tagged("foobar", "en").as_str_ends_with(Literal::make_lang_tagged("bar", "en")).ebv());
        CHECK(Literal::make_lang_tagged("foobar", "en").as_str_ends_with("bar"_xsd_string).ebv());

        CHECK(Literal::make_lang_tagged("foobar", "fr").as_str_ends_with(Literal::make_lang_tagged("bar", "en")).null());
        CHECK(("foobar"_xsd_string).as_str_ends_with(Literal::make_lang_tagged("bar", "en")).null());

        // unicode
        CHECK(Literal::make_lang_tagged_normalize("fooba\u0174r", "en", storage::default_node_storage)
                      .as_str_ends_with(Literal::make_simple_normalize("baW\u0302r", storage::default_node_storage))
                      .ebv());  // 2 different ways of writing Ŵ
    }

    SUBCASE("concat") {
        // from https://www.w3.org/TR/sparql11-query/#func-concat
        CHECK_EQ(("foo"_xsd_string).concat("bar"_xsd_string), "foobar"_xsd_string);
        CHECK_EQ(Literal::make_lang_tagged("foo", "en").concat(Literal::make_lang_tagged("bar", "en")), Literal::make_lang_tagged("foobar", "en"));
        CHECK_EQ(Literal::make_lang_tagged("foo", "en").concat("bar"_xsd_string), "foobar"_xsd_string);

        CHECK_EQ(Literal::make_lang_tagged("foo", "fr").concat(Literal::make_lang_tagged("bar", "en")), "foobar"_xsd_string);
        CHECK_EQ(5_xsd_int .concat(" + "_xsd_string).concat(1.0_xsd_double).concat(Literal::make_lang_tagged(" = ", "en")).concat("6.0"_xsd_decimal), "5 + 1.0E0 = 6.0"_xsd_string);
    }

    SUBCASE("regex_match") {
        // from https://www.w3.org/TR/xpath-functions/#func-matches
        CHECK(("abracadabra"_xsd_string).as_regex_matches("bra"_xsd_string).ebv());
        CHECK(("abracadabra"_xsd_string).as_regex_matches("^a.*a$"_xsd_string).ebv());
        CHECK_EQ(("abracadabra"_xsd_string).as_regex_matches("^bra"_xsd_string).ebv(), TriBool::False);

        std::string_view const poem = "<poem author=\"Wilhelm Busch\">\n"
                                      "Kaum hat dies der Hahn gesehen,\n"
                                      "Fängt er auch schon an zu krähen:\n"
                                      "Kikeriki! Kikikerikih!!\n"
                                      "Tak, tak, tak! - da kommen sie.\n"
                                      "</poem>";
        auto const poem_lit = Literal::make_simple(poem);

        CHECK_EQ(poem_lit.as_regex_matches("Kaum.*krähen"_xsd_string).ebv(), TriBool::False);
        CHECK(poem_lit.as_regex_matches("^Kaum.*gesehen,$"_xsd_string, "m"_xsd_string).ebv());
        CHECK_EQ(poem_lit.as_regex_matches("^Kaum.*gesehen,$"_xsd_string).ebv(), TriBool::False);
        CHECK(poem_lit.as_regex_matches("kiki"_xsd_string, "i"_xsd_string).ebv());

        // check lang tag behaviour
        CHECK(Literal::make_lang_tagged("abcd", "en").as_regex_matches("b"_xsd_string).ebv());
        CHECK(Literal::make_lang_tagged("abcd", "en").as_regex_matches(Literal::make_lang_tagged("b", "en")).ebv());
        CHECK(Literal::make_lang_tagged("abcd", "en").as_regex_matches(Literal::make_lang_tagged("b", "fr")).null());

        CHECK(!Literal::make_simple("abc\ndef\ngh").as_regex_matches("^def$"_xsd_string).ebv());
        CHECK(Literal::make_simple("abc\ndef\ngh").as_regex_matches("^def$"_xsd_string, "m"_xsd_string).ebv());

        // most of these are adapted from examples in https://www.w3.org/TR/xpath-functions/#flags
        CHECK(!Literal::make_simple("helloworld").as_regex_matches("hello world"_xsd_string).ebv());
        CHECK(!Literal::make_simple("hello world").as_regex_matches("hello world"_xsd_string, "x"_xsd_string).ebv());
        CHECK(Literal::make_simple("helloworld").as_regex_matches("hello world"_xsd_string, "x"_xsd_string).ebv());
        CHECK(Literal::make_simple("hello world").as_regex_matches("hello[ ]world"_xsd_string, "x"_xsd_string).ebv());
        CHECK(Literal::make_simple("hello world").as_regex_matches("hello\\ sworld"_xsd_string, "x"_xsd_string).ebv());
        CHECK(Literal::make_simple("hello[world").as_regex_matches("hello\\ [worl d"_xsd_string, "x"_xsd_string).ebv());
    }

    SUBCASE("regex_replace") {
        // from https://www.w3.org/TR/sparql11-query/#func-replace
        CHECK_EQ(("abcd"_xsd_string).regex_replace("b"_xsd_string, "Z"_xsd_string), "aZcd"_xsd_string);
        CHECK_EQ(("abab"_xsd_string).regex_replace("B"_xsd_string, "Z"_xsd_string, "i"_xsd_string), "aZaZ"_xsd_string);
        CHECK_EQ(("abab"_xsd_string).regex_replace("B."_xsd_string, "Z"_xsd_string, "i"_xsd_string), "aZb"_xsd_string);

        // from https://www.w3.org/TR/xpath-functions/#func-replace
        CHECK_EQ(("abracadabra"_xsd_string).regex_replace("bra"_xsd_string, "*"_xsd_string), "a*cada*"_xsd_string);
        CHECK_EQ(("abracadabra"_xsd_string).regex_replace("a.*a"_xsd_string, "*"_xsd_string), "*"_xsd_string);
        CHECK_EQ(("abracadabra"_xsd_string).regex_replace("a.*?a"_xsd_string, "*"_xsd_string), "*c*bra"_xsd_string);
        CHECK_EQ(("abracadabra"_xsd_string).regex_replace("a"_xsd_string, ""_xsd_string), "brcdbr"_xsd_string);
        CHECK_EQ(("abracadabra"_xsd_string).regex_replace("a(.)"_xsd_string, "a$1$1"_xsd_string), "abbraccaddabbra"_xsd_string);
        CHECK_EQ(("AAAA"_xsd_string).regex_replace("A+"_xsd_string, "b"_xsd_string), "b"_xsd_string);
        CHECK_EQ(("AAAA"_xsd_string).regex_replace("A+?"_xsd_string, "b"_xsd_string), "bbbb"_xsd_string);
        CHECK_EQ(("darted"_xsd_string).regex_replace("^(.*?)d(.*)$"_xsd_string, "$1c$2"_xsd_string), "carted"_xsd_string);

        CHECK(("abracadabra"_xsd_string).regex_replace(".*?"_xsd_string, "$1"_xsd_string).null());

        CHECK_EQ(("abcd"_xsd_string).as_regex_matches(".*"_xsd_string, "q"_xsd_string).ebv(), TriBool::False);
        CHECK(("Mr. B. Obama"_xsd_string).as_regex_matches("B. OBAMA"_xsd_string, "qi"_xsd_string).ebv());

        // check lang tag behaviour
        CHECK_EQ(Literal::make_lang_tagged("abcd", "en").regex_replace("b"_xsd_string, "Z"_xsd_string), Literal::make_lang_tagged("aZcd", "en"));
        CHECK_EQ(Literal::make_lang_tagged("abcd", "en").regex_replace(Literal::make_lang_tagged("b", "en"), "Z"_xsd_string), Literal::make_lang_tagged("aZcd", "en"));
        CHECK(Literal::make_lang_tagged("abcd", "en").regex_replace(Literal::make_lang_tagged("b", "fr"), "Z"_xsd_string).null());

        CHECK(("Hello 1 World"_xsd_string).regex_replace("[0-9]"_xsd_string, "Hello \\\\hgfhf World"_xsd_string) == "Hello Hello \\hgfhf World World"_xsd_string);
        CHECK(("Hello 1 World"_xsd_string).regex_replace("[0-9]"_xsd_string, "Hello \\hgfhf World"_xsd_string).null());

        CHECK(Literal::make_simple("abc\ndef\ngh").regex_replace("(def)"_xsd_string, "y$1x"_xsd_string, ""_xsd_string) == Literal::make_simple("abc\nydefx\ngh"));
        CHECK(Literal::make_simple("abc\ndef\ngh").regex_replace("^(def)$"_xsd_string, "y$1x"_xsd_string, "m"_xsd_string) == Literal::make_simple("abc\nydefx\ngh"));

        CHECK(Literal::make_simple("hello[world").regex_replace("\\ [wo(rl) d"_xsd_string, " wo$1d"_xsd_string, "x"_xsd_string) == Literal::make_simple("hello world"));
    }

    SUBCASE("hashes") {
        static constexpr const char *pw = "password";
        CHECK_EQ(Literal::make_simple(pw).md5(), Literal::make_simple("5f4dcc3b5aa765d61d8327deb882cf99"));
        CHECK_EQ(Literal::make_simple(pw).sha1(), Literal::make_simple("5baa61e4c9b93f3f0682250b6cf8331b7ee68fd8"));  // note that this hash contains bytes < 0x10, which checks 0 padding
        CHECK_EQ(Literal::make_simple(pw).sha256(), Literal::make_simple("5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"));
        CHECK_EQ(Literal::make_simple(pw).sha384(), Literal::make_simple("a8b64babd0aca91a59bdbb7761b421d4f2bb38280d3a75ba0f21f2bebc45583d446c598660c94ce680c47d19c30783a7"));
        CHECK_EQ(Literal::make_simple(pw).sha512(), Literal::make_simple("b109f3bbbc244eb82441917ed06d618b9008dd09b3befd1b5e07394c706a8bb980b1d7785e5976ec049b46df5f1326af5a2ea6d103fd07c95385ffab0cacbc86"));
    }
}

TEST_CASE("indirect casting precision") {
    CHECK_EQ(Literal::make_typed_from_value<datatypes::xsd::Double>(2e-1) + Literal::make_typed_from_value<datatypes::xsd::Decimal>(datatypes::xsd::Decimal::cpp_type{"0.2"}), Literal::make_typed_from_value<datatypes::xsd::Double>(4e-1));
}

TEST_CASE("URI encoding") {
    static constexpr const char *data = "www.example %\t*}.com";
    static constexpr const char *data_encoded = "www.example%20%25%09%2A%7D.com";
    SUBCASE("valid URI") {
        static constexpr const char *str = "www.example123_-AZaz09~.com";
        CHECK_EQ(Literal::encode_for_uri(str), Literal::make_simple(str));
    }
    SUBCASE("ASCII escapes") {
        CHECK_EQ(Literal::encode_for_uri(" %\t*}"), Literal::make_simple("%20%25%09%2A%7D"));
    }
    SUBCASE("ASCII mixed") {
        CHECK_EQ(Literal::encode_for_uri(data), Literal::make_simple(data_encoded));
    }
    SUBCASE("high UTF-8") {
        CHECK_EQ(Literal::encode_for_uri("\xce\xa4"), Literal::make_simple("%CE%A4"));                // greek capital sigma
        CHECK_EQ(Literal::encode_for_uri("\xf0\x90\x8f\x92"), Literal::make_simple("%F0%90%8F%92"));  // old persian 2
        CHECK_EQ(Literal::encode_for_uri("\xe2\x88\x80"), Literal::make_simple("%E2%88%80"));         // math forall
    }
    SUBCASE("high UTF-8 mixed") {
        CHECK_EQ(Literal::encode_for_uri("www.e\xce\xa4\xf0\x90\x8f\x92\xe2\x88\x80xample.com"), Literal::make_simple("www.e%CE%A4%F0%90%8F%92%E2%88%80xample.com"));
    }
    SUBCASE("nonstatic") {
        CHECK_EQ(Literal::make_simple(data).encode_for_uri(), Literal::make_simple(data_encoded));
        CHECK_EQ(Literal::make_lang_tagged(data, "en").encode_for_uri(), Literal::make_simple(data_encoded));
        CHECK_EQ(Literal::make_typed(data, IRI{"http://www.w3.org/2001/XMLSchema#string"}).encode_for_uri(), Literal::make_simple(data_encoded));
    }
    SUBCASE("invalid UTF-8") {
        CHECK(Literal::encode_for_uri("\xce").null());
    }
}

TEST_CASE("UUID") {
    Literal uuid = Literal::make_string_uuid();
    Literal uuid2 = Literal::make_string_uuid();

    CHECK_EQ(uuid.datatype(), IRI{"http://www.w3.org/2001/XMLSchema#string"});
    CHECK_NE(uuid, uuid2);  // note: non-deterministic but should basically never fail
    CHECK_EQ(uuid.regex_matches(regex::Regex{"^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"}), TriBool::True);
}

TEST_CASE("to_node_storage") {
    storage::reference_node_storage::SyncReferenceNodeStorage ns2{};

    SUBCASE("no non-inline storage available") {
        auto lit = Literal::make_typed_from_value<datatypes::xsd::Int>(5);
        assert(lit.is_inlined());

        auto lit2 = lit.to_node_storage(ns2);
        CHECK(lit2.is_inlined());
        CHECK_EQ(lit.value<datatypes::xsd::Int>(), lit2.value<datatypes::xsd::Int>());
        CHECK_EQ(lit.backend_handle().node_id().literal_id(), lit2.backend_handle().node_id().literal_id());
        CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
        CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
    }

    SUBCASE("specialized storage") {
        SUBCASE("inlined") {
            auto lit = Literal::make_typed_from_value<datatypes::xsd::Long>(10);
            assert(lit.is_inlined());

            auto lit2 = lit.to_node_storage(ns2);
            CHECK(lit2.is_inlined());
            CHECK_EQ(lit.value<datatypes::xsd::Long>(), lit2.value<datatypes::xsd::Long>());
            CHECK_EQ(lit.backend_handle().node_id().literal_id(), lit2.backend_handle().node_id().literal_id());
            CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
            CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
        }

        SUBCASE("not inlined") {
            auto lit = Literal::make_typed_from_value<datatypes::xsd::Long>(std::numeric_limits<datatypes::xsd::Long::cpp_type>::max());
            assert(!lit.is_inlined());

            auto lit2 = lit.to_node_storage(ns2);
            CHECK(!lit2.is_inlined());
            CHECK_EQ(lit.value<datatypes::xsd::Long>(), lit2.value<datatypes::xsd::Long>());
            CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
            CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
        }
    }

    SUBCASE("lexical storage") {
        SUBCASE("rdf:langString") {
            SUBCASE("tag inlined") {
                auto lit = Literal::make_lang_tagged("test", "en");
                assert(lit.is_inlined());

                auto lit2 = lit.to_node_storage(ns2);
                CHECK(lit2.is_inlined());
                CHECK_EQ(lit.value<datatypes::rdf::LangString>(), lit2.value<datatypes::rdf::LangString>());
                CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
                CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
            }

            SUBCASE("tag not inlined") {
                auto lit = Literal::make_lang_tagged("test", "spherical");
                assert(!lit.is_inlined());

                auto lit2 = lit.to_node_storage(ns2);
                CHECK(!lit2.is_inlined());
                CHECK_EQ(lit.value<datatypes::rdf::LangString>(), lit2.value<datatypes::rdf::LangString>());
                CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
                CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
            }
        }

        SUBCASE("xsd:string") {
            auto lit = Literal::make_simple("test");
            assert(!lit.is_inlined());

            auto lit2 = lit.to_node_storage(ns2);
            CHECK(!lit2.is_inlined());
            CHECK_EQ(lit.value<datatypes::xsd::String>(), lit2.value<datatypes::xsd::String>());
            CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
            CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
        }
    }
}

TEST_CASE("try_get_in_node_storage") {
    auto ns2 = storage::reference_node_storage::SyncReferenceNodeStorage{};

    SUBCASE("no non-inline storage available") {
        auto lit = Literal::make_typed_from_value<datatypes::xsd::Int>(5);
        assert(lit.is_inlined());

        auto lit2 = lit.try_get_in_node_storage(ns2);
        CHECK(!lit2.null());
        CHECK(lit2.is_inlined());
        CHECK_EQ(lit.value<datatypes::xsd::Int>(), lit2.value<datatypes::xsd::Int>());
        CHECK_EQ(lit.backend_handle().node_id().literal_id(), lit2.backend_handle().node_id().literal_id());
        CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
        CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
    }

    SUBCASE("specialized storage") {
        SUBCASE("inlined") {
            auto lit = Literal::make_typed_from_value<datatypes::xsd::Long>(10);
            assert(lit.is_inlined());

            auto lit2 = lit.try_get_in_node_storage(ns2);
            CHECK(!lit2.null());
            CHECK(lit2.is_inlined());
            CHECK_EQ(lit.value<datatypes::xsd::Long>(), lit2.value<datatypes::xsd::Long>());
            CHECK_EQ(lit.backend_handle().node_id().literal_id(), lit2.backend_handle().node_id().literal_id());
            CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
            CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
        }

        SUBCASE("not inlined") {
            auto lit = Literal::make_typed_from_value<datatypes::xsd::Long>(std::numeric_limits<datatypes::xsd::Long::cpp_type>::max());
            assert(!lit.is_inlined());

            auto lit2 = lit.try_get_in_node_storage(ns2);
            CHECK(lit2.null());
            lit.to_node_storage(ns2);

            lit2 = lit.try_get_in_node_storage(ns2);
            CHECK(!lit2.null());
            CHECK(!lit2.is_inlined());
            CHECK_EQ(lit.value<datatypes::xsd::Long>(), lit2.value<datatypes::xsd::Long>());
            CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
            CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
        }
    }

    SUBCASE("lexical storage") {
        SUBCASE("rdf:langString") {
            SUBCASE("tag inlined") {
                auto lit = Literal::make_lang_tagged("test", "en");
                assert(lit.is_inlined());

                auto lit2 = lit.try_get_in_node_storage(ns2);
                CHECK(lit2.null());
                lit.to_node_storage(ns2);

                lit2 = lit.try_get_in_node_storage(ns2);
                CHECK(!lit2.null());
                CHECK(lit2.is_inlined());
                CHECK_EQ(lit.value<datatypes::rdf::LangString>(), lit2.value<datatypes::rdf::LangString>());
                CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
                CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
            }

            SUBCASE("tag not inlined") {
                auto lit = Literal::make_lang_tagged("test", "spherical");
                assert(!lit.is_inlined());

                auto lit2 = lit.try_get_in_node_storage(ns2);
                CHECK(lit2.null());
                lit.to_node_storage(ns2);

                lit2 = lit.try_get_in_node_storage(ns2);
                CHECK(!lit2.null());
                CHECK(!lit2.is_inlined());
                CHECK_EQ(lit.value<datatypes::rdf::LangString>(), lit2.value<datatypes::rdf::LangString>());
                CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
                CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
            }
        }

        SUBCASE("xsd:string") {
            auto lit = Literal::make_simple("test");
            assert(!lit.is_inlined());

            auto lit2 = lit.try_get_in_node_storage(ns2);
            CHECK(lit2.null());
            lit.to_node_storage(ns2);

            lit2 = lit.try_get_in_node_storage(ns2);
            CHECK(!lit2.null());
            CHECK(!lit2.is_inlined());
            CHECK_EQ(lit.value<datatypes::xsd::String>(), lit2.value<datatypes::xsd::String>());
            CHECK_EQ(lit.backend_handle().node_id().literal_type(), lit2.backend_handle().node_id().literal_type());
            CHECK_NE(lit.backend_handle().storage(), lit2.backend_handle().storage());
        }
    }
}

namespace rdf4cpp::datatypes::registry {
inline constexpr util::ConstexprString fake_datatype = "http://foo/bar";
template<>
struct DatatypeMapping<fake_datatype> {
    using cpp_datatype = int;
};
template<>
inline capabilities::Default<fake_datatype>::cpp_type capabilities::Default<fake_datatype>::from_string(std::string_view s) {
    return util::from_chars<int, "fake">(s);
}
template<>
bool capabilities::Default<fake_datatype>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    auto const s = std::format("{}", value);
    return writer::write_str(s, writer);
}
}
struct FakeDatatype : rdf4cpp::datatypes::registry::LiteralDatatypeImpl<rdf4cpp::datatypes::registry::fake_datatype> {};

template<typename T>
struct get_find_values {};

template<>
struct get_find_values<datatypes::xsd::String> {
    static constexpr std::string_view av = "foo", bv = "bar";
    static constexpr std::string_view as = "foo", bs = "bar";
};
template<>
struct get_find_values<datatypes::rdf::LangString> {
    static constexpr datatypes::registry::LangStringRepr av = {"foo", "en"}, bv = {"bar", "en"};
};
template<>
struct get_find_values<datatypes::xsd::Int> {  // always inlined
    static constexpr int32_t inl = 42;
    static constexpr std::string_view inls = "42";
};
template<>
struct get_find_values<datatypes::xsd::Date> {  // inlined == has timezone
    static constexpr std::pair<YearMonthDay, rdf4cpp::OptionalTimezone> av{YearMonthDay{Year{1342}, std::chrono::month{5}, std::chrono::day{4}}, rdf4cpp::Timezone{std::chrono::hours{1}}};
    static constexpr std::pair<YearMonthDay, rdf4cpp::OptionalTimezone> bv{YearMonthDay{Year{1342}, std::chrono::month{5}, std::chrono::day{5}}, rdf4cpp::Timezone{std::chrono::hours{1}}};
    static constexpr std::pair<YearMonthDay, rdf4cpp::OptionalTimezone> inl{YearMonthDay{Year{1342}, std::chrono::month{5}, std::chrono::day{6}}, std::nullopt};
    static constexpr std::string_view as = "1342-5-4+1:0";
    static constexpr std::string_view bs = "1342-5-5+1:0";
    static constexpr std::string_view inls = "1342-5-6";
};
template<>
struct get_find_values<FakeDatatype> { // no fixed id
    static constexpr int av = 42, bv = 4242;
    static constexpr std::string_view as = "42", bs = "4242";
};


TEST_CASE_TEMPLATE("Literal::find", T, datatypes::xsd::String, datatypes::rdf::LangString, datatypes::xsd::Int, datatypes::xsd::Date, FakeDatatype) {
    if constexpr (requires { get_find_values<T>::av; }) {
        static constexpr auto av = get_find_values<T>::av;
        static constexpr auto bv = get_find_values<T>::bv;
        auto nst = storage::reference_node_storage::SyncReferenceNodeStorage{};

        CHECK(Literal::find_typed_from_value<T>(av, nst).null());
        Literal l = Literal::make_typed_from_value<T>(av, nst);
        CHECK(Literal::find_typed_from_value<T>(av, nst) == l);
        CHECK(Literal::find_typed_from_value<T>(av, nst).backend_handle() == l.backend_handle());
        CHECK(Literal::find_typed_from_value<T>(bv, nst).null());
    }
    if constexpr (requires { get_find_values<T>::inl; }) {
        auto nst = storage::reference_node_storage::SyncReferenceNodeStorage{};
        auto l = Literal::find_typed_from_value<T>(get_find_values<T>::inl, nst);
        CHECK(l == Literal::make_typed_from_value<T>(get_find_values<T>::inl));
    }
    if constexpr (requires { get_find_values<T>::as; }) {
        static constexpr auto as = get_find_values<T>::as;
        static constexpr auto bs = get_find_values<T>::bs;
        auto nst = storage::reference_node_storage::SyncReferenceNodeStorage{};

        CHECK(Literal::find_typed<T>(as, nst).null());
        Literal l = Literal::make_typed<T>(as, nst);
        CHECK(Literal::find_typed<T>(as, nst) == l);
        CHECK(Literal::find_typed<T>(as, nst).backend_handle() == l.backend_handle());
        CHECK(Literal::find_typed<T>(bs, nst).null());
    }
    if constexpr (requires { get_find_values<T>::inls; }) {
        auto nst = storage::reference_node_storage::SyncReferenceNodeStorage{};
        auto l = Literal::find_typed<T>(get_find_values<T>::inls, nst);
        CHECK(l == Literal::make_typed<T>(get_find_values<T>::inls));
    }
}

TEST_CASE("Literal::fetch_or_serialize_lexical_form") {
    std::string buf;
    writer::StringWriter w{buf};

    SUBCASE("no non-inline storage available") {
        auto lit = Literal::make_typed_from_value<datatypes::xsd::Int>(5);
        assert(lit.is_inlined());

        std::string_view s;
        auto r = lit.fetch_or_serialize_lexical_form(s, w);
        CHECK_EQ(r, FetchOrSerializeResult::Serialized);
        CHECK_EQ(w.view(), "5");
    }

    SUBCASE("specialized storage") {
        SUBCASE("inlined") {
            auto lit = Literal::make_typed_from_value<datatypes::xsd::Long>(10);
            assert(lit.is_inlined());

            std::string_view s;
            auto r = lit.fetch_or_serialize_lexical_form(s, w);
            CHECK_EQ(r, FetchOrSerializeResult::Serialized);
            CHECK_EQ(w.view(), "10");
        }

        SUBCASE("not inlined") {
            auto lit = Literal::make_typed_from_value<datatypes::xsd::Long>(std::numeric_limits<datatypes::xsd::Long::cpp_type>::max());
            assert(!lit.is_inlined());

            std::string_view s;
            auto r = lit.fetch_or_serialize_lexical_form(s, w);
            CHECK_EQ(r, FetchOrSerializeResult::Serialized);
            CHECK_EQ(w.view(), std::to_string(std::numeric_limits<datatypes::xsd::Long::cpp_type>::max()));
        }
    }

    SUBCASE("lexical storage") {
        SUBCASE("rdf:langString") {
            SUBCASE("tag inlined") {
                auto lit = Literal::make_lang_tagged("test", "en");
                assert(lit.is_inlined());

                std::string_view s;
                auto r = lit.fetch_or_serialize_lexical_form(s, w);
                CHECK_EQ(r, FetchOrSerializeResult::Fetched);
                CHECK_EQ(s, "test");
            }

            SUBCASE("tag not inlined") {
                auto lit = Literal::make_lang_tagged("test", "spherical");
                assert(!lit.is_inlined());

                std::string_view s;
                auto r = lit.fetch_or_serialize_lexical_form(s, w);
                CHECK_EQ(r, FetchOrSerializeResult::Fetched);
                CHECK_EQ(s, "test");
            }
        }

        SUBCASE("xsd:string") {
            auto lit = Literal::make_simple("test");
            assert(!lit.is_inlined());

            std::string_view s;
            auto r = lit.fetch_or_serialize_lexical_form(s, w);
            CHECK_EQ(r, FetchOrSerializeResult::Fetched);
            CHECK_EQ(s, "test");
        }
    }
}

TEST_CASE_TEMPLATE("leading zeros parsing", L, datatypes::xsd::Integer,
                                               datatypes::xsd::Int,
                                               datatypes::xsd::Float,
                                               datatypes::xsd::Double,
                                               datatypes::xsd::Decimal) {
    auto lit = Literal::make_typed<L>("09");
    CHECK_FALSE(lit.null());
    CHECK_EQ(lit.template value<L>(), 9);

    lit = Literal::make_typed<L>("000");
    CHECK_FALSE(lit.null());
    CHECK_EQ(lit.template value<L>(), 0);
}

