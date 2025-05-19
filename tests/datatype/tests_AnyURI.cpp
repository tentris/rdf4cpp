#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_CASE("anyURI capabilities") {
    static_assert(datatypes::LiteralDatatype<datatypes::xsd::AnyURI>);
    static_assert(datatypes::LogicalLiteralDatatype<datatypes::xsd::AnyURI>);
    static_assert(!datatypes::NumericLiteralDatatype<datatypes::xsd::AnyURI>);
    static_assert(!datatypes::PromotableLiteralDatatype<datatypes::xsd::AnyURI>);
    static_assert(!datatypes::SubtypedLiteralDatatype<datatypes::xsd::AnyURI>);
    static_assert(datatypes::ComparableLiteralDatatype<datatypes::xsd::AnyURI>);
    static_assert(datatypes::FixedIdLiteralDatatype<datatypes::xsd::AnyURI>);
}

TEST_CASE("Datatype AnyURI") {

    constexpr std::string_view correct_type_iri_cstr = "http://www.w3.org/2001/XMLSchema#anyURI";

    CHECK(datatypes::xsd::AnyURI::identifier == correct_type_iri_cstr);

    CHECK(Literal::make_typed<datatypes::xsd::AnyURI>("https://example.com").lexical_form() == "https://example.com");
    CHECK(Literal::make_typed<datatypes::xsd::AnyURI>("https://example.com").ebv() == TriBool::True);
    CHECK(Literal::make_typed<datatypes::xsd::AnyURI>("").ebv() == TriBool::False);
    CHECK(Literal::make_typed<datatypes::xsd::AnyURI>("/relative/iri?to?somewhere").lexical_form() == "/relative/iri?to?somewhere");
    CHECK(Literal::make_typed<datatypes::xsd::AnyURI>("//relative/iri?to?somewhere").lexical_form() == "//relative/iri?to?somewhere");
    Literal e{};
    CHECK_THROWS_AS(e = Literal::make_typed<datatypes::xsd::AnyURI>("invalid uri"), rdf4cpp::InvalidNode);

    CHECK(e.null());
}
