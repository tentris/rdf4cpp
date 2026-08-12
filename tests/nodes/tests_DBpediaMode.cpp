#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_CASE("IRI") {
    IRIFactory f{};
    static constexpr std::string_view iri = "http//:example:foo/bar";

    datatypes::registry::relaxed_parsing_mode = false;
    CHECK_THROWS_AS((void) f.create_and_validate(iri), InvalidIRI);

    datatypes::registry::relaxed_parsing_mode = true;
    CHECK_NOTHROW((void) f.create_and_validate(iri));
    CHECK(f.create_and_validate(iri).identifier() == iri);
}

TEST_CASE("date/time") {
    Literal u{};
    datatypes::registry::relaxed_parsing_mode = false;
    CHECK_THROWS_AS(u = Literal::make_typed<datatypes::xsd::Date>("1742-0-0"), std::runtime_error);
    CHECK_THROWS_AS(u = Literal::make_typed<datatypes::xsd::Date>("1742-2-40"), std::runtime_error);
    CHECK_THROWS_AS(u = Literal::make_typed<datatypes::xsd::Time>("10:90:00"), std::runtime_error);
    CHECK_THROWS_AS(u = Literal::make_typed<datatypes::xsd::Time>("26:90:60"), std::runtime_error);
    CHECK_THROWS_AS(u = Literal::make_typed<datatypes::xsd::DateTime>("1742-2-40T26:90:60"), std::runtime_error);
    CHECK_THROWS_AS(u = Literal::make_typed<datatypes::xsd::DateTimeStamp>("1742-2-40T26:90:60Z"), std::runtime_error);

    datatypes::registry::relaxed_parsing_mode = true;
    CHECK(Literal::make_typed<datatypes::xsd::Date>("1742-0-0") == Literal::make_typed<datatypes::xsd::Date>("1741-11-30"));
    CHECK(Literal::make_typed<datatypes::xsd::Date>("1742-2-40") == Literal::make_typed<datatypes::xsd::Date>("1742-3-12"));
    CHECK(Literal::make_typed<datatypes::xsd::Time>("10:90:00") == Literal::make_typed<datatypes::xsd::Time>("11:30:00"));
    CHECK_THROWS_AS(u = Literal::make_typed<datatypes::xsd::Time>("26:90:60"), std::runtime_error);
    CHECK(Literal::make_typed<datatypes::xsd::DateTime>("1742-2-40T26:90:60") == Literal::make_typed<datatypes::xsd::DateTime>("1742-3-13T3:31:0"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("1742-2-40T26:90:60Z") == Literal::make_typed<datatypes::xsd::DateTimeStamp>("1742-3-13T3:31:0Z"));
    CHECK(Literal::make_typed<datatypes::xsd::DateTimeStamp>("1742-2-40T10:10:10Z") == Literal::make_typed<datatypes::xsd::DateTimeStamp>("1742-3-12T10:10:10Z"));
    CHECK(u.null());
}
