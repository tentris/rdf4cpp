#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_CASE("rdf:langString") {
    Literal dummy;
    CHECK_THROWS(dummy = Literal::make_typed("hello", IRI{datatypes::rdf::LangString::identifier}));

    auto const lit1 = Literal::make_lang_tagged("hello", "en");
    auto const lit2 = Literal::make_typed_from_value<datatypes::rdf::LangString>(datatypes::registry::LangStringRepr{"hello", "en"});

    CHECK(lit1 == lit2);

    auto const lit3 = Literal::make_lang_tagged("hello", "de-DE");
    auto const lit4 = Literal::make_lang_tagged("hallo", "de-DE");
    auto const lit5 = Literal::make_lang_tagged("hallo", "de-de");
    CHECK(lit1 != lit3);
    CHECK(lit1 != lit4);
    CHECK(lit3 != lit4);
    CHECK(lit3 != lit5);
    CHECK(lit4 == lit5);

    SUBCASE("value extraction") {
        CHECK(static_cast<std::string>(lit1) == R"("hello"@en)");
        CHECK(static_cast<std::string>(lit2) == R"("hello"@en)");

        CHECK(lit1.lexical_form() == "hello");
        CHECK(lit2.lexical_form() == "hello");

        CHECK(lit1.language_tag() == "en");
        CHECK(lit2.language_tag() == "en");

        using datatypes::registry::LangStringRepr;

        CHECK(std::any_cast<LangStringRepr>(lit1.value()) == LangStringRepr{"hello", "en"});
        CHECK(std::any_cast<LangStringRepr>(lit2.value()) == LangStringRepr{"hello", "en"});

        CHECK(lit1.value<datatypes::rdf::LangString>() == LangStringRepr{"hello", "en"});
        CHECK(lit2.value<datatypes::rdf::LangString>() == LangStringRepr{"hello", "en"});
    }
}

TEST_CASE("rdf::langString inlining") {
    using namespace rdf4cpp::datatypes::registry;
    CHECK(DatatypeRegistry::LangTagInlines::bits_needed_for(1) == 1);
    CHECK(DatatypeRegistry::LangTagInlines::bits_needed_for(2) == 2);
    CHECK(DatatypeRegistry::LangTagInlines::bits_needed_for(3) == 2);
    CHECK(DatatypeRegistry::LangTagInlines::bits_needed_for(4) == 3);
    CHECK(DatatypeRegistry::LangTagInlines::bits_needed_for(5) == 3);

    // these 2 checks need to be modified, if the default inlined tags change
    CHECK(DatatypeRegistry::LangTagInlines::get_tags_to_inline().size() == 4);
    CHECK(DatatypeRegistry::LangTagInlines::inlined_size() == 2);

    CHECK(Literal::make_lang_tagged("hello world", "en").is_inlined());
    CHECK(Literal::make_lang_tagged("hello world", "de").is_inlined());
    CHECK(!Literal::make_lang_tagged("hello world", "en-us").is_inlined());

    CHECK(!DatatypeRegistry::LangTagInlines::try_into_inlined(storage::identifier::LiteralID{1ul << (storage::identifier::LiteralID::width - 1)}, 0).has_value());
}
