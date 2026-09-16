#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>
#include <cmath>

using namespace rdf4cpp;

TEST_CASE("float capabilities") {
    static_assert(datatypes::LiteralDatatype<datatypes::xsd::Float>);
    static_assert(datatypes::NumericLiteralDatatype<datatypes::xsd::Float>);
    static_assert(datatypes::LogicalLiteralDatatype<datatypes::xsd::Float>);
    static_assert(datatypes::PromotableLiteralDatatype<datatypes::xsd::Float>);
    static_assert(!datatypes::SubtypedLiteralDatatype<datatypes::xsd::Float>);
    static_assert(datatypes::ComparableLiteralDatatype<datatypes::xsd::Float>);
    static_assert(datatypes::FixedIdLiteralDatatype<datatypes::xsd::Float>);
}

TEST_CASE("Datatype Float") {

    constexpr auto correct_type_iri_cstr = "http://www.w3.org/2001/XMLSchema#float";

    CHECK(std::string(datatypes::xsd::Float::identifier) == correct_type_iri_cstr);

    auto type_iri = IRI(datatypes::xsd::Float::identifier);
    CHECK(type_iri.identifier() == correct_type_iri_cstr);

    using type = datatypes::xsd::Float::cpp_type;

    CHECK(std::is_same_v<type, float>);

    std::string const rdf_float_1_0{"1.0E0"};

    type value = 1.00f;
    auto lit1 = Literal::make_typed_from_value<datatypes::xsd::Float>(value);
    CHECK(lit1.value<datatypes::xsd::Float>() == value);
    CHECK(lit1.lexical_form() == rdf_float_1_0);

    value = 32568.2350f;
    auto lit2 = Literal::make_typed_from_value<datatypes::xsd::Float>(value);
    CHECK(lit2.value<datatypes::xsd::Float>() == value);

    value = -14523.2350f;
    auto lit3 = Literal::make_typed_from_value<datatypes::xsd::Float>(value);
    CHECK(lit3.value<datatypes::xsd::Float>() == value);

    value = 1;
    auto lit4 = Literal::make_typed_from_value<datatypes::xsd::Float>(value);
    CHECK(lit4.value<datatypes::xsd::Float>() == value);
    CHECK(lit4.lexical_form() == rdf_float_1_0);

    value = 1;
    auto lit6 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit6.value<datatypes::xsd::Float>() == value);
    CHECK(lit6.lexical_form() == rdf_float_1_0);

    value = 32568.2350f;
    auto lit7 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit7.value<datatypes::xsd::Float>() == value);

    auto lit8 = Literal::make_typed("NaN", type_iri);
    CHECK(std::isnan(lit8.value<datatypes::xsd::Float>()));

    auto lit9 = Literal::make_typed("INF", type_iri);
    CHECK(std::isinf(lit9.value<datatypes::xsd::Float>()));

    value = INFINITY;
    auto lit10 = Literal::make_typed_from_value<datatypes::xsd::Float>(value);
    CHECK(std::isinf(lit10.value<datatypes::xsd::Float>()));

    value = NAN;
    auto lit11 = Literal::make_typed_from_value<datatypes::xsd::Float>(value);
    CHECK(std::isnan(lit11.value<datatypes::xsd::Float>()));

    value = 1.17e-38;
    auto lit12 = Literal::make_typed("1.17e-38", type_iri);
    CHECK(lit12.value<datatypes::xsd::Float>() == value);

    auto lit13 = Literal::make_typed("+INF", type_iri);
    CHECK(std::isinf(lit13.value<datatypes::xsd::Float>()));

    auto lit14 = Literal::make_typed("-INF", type_iri);
    CHECK(std::isinf(lit14.value<datatypes::xsd::Float>()));
    CHECK(lit14.value<datatypes::xsd::Float>() == -std::numeric_limits<type>::infinity());

    value = -INFINITY;
    auto lit15 = Literal::make_typed_from_value<datatypes::xsd::Float>(value);
    CHECK(std::isinf(lit15.value<datatypes::xsd::Float>()));
    CHECK(lit15.value<datatypes::xsd::Float>() == -std::numeric_limits<type>::infinity());

    CHECK(lit1 != lit2);
    CHECK(lit2 != lit3);
    CHECK(lit1 == lit4);
    CHECK(lit1 == lit6);
    CHECK(lit2 == lit7);
    CHECK(lit9 == lit10);
    CHECK(lit9 == lit13);
    CHECK(lit10 == lit13);
    CHECK(lit14 == lit15);

    // suppress warnings regarding attribute ‘nodiscard’
    Literal no_discard_dummy;

    CHECK_THROWS(no_discard_dummy = Literal::make_typed("454sdsd", type_iri));
}

TEST_CASE("round-trip") {
    datatypes::xsd::Float::cpp_type const value = -0.123456789f;
    auto const lit = Literal::make_typed_from_value<datatypes::xsd::Float>(value);
    std::cout << lit.lexical_form() << std::endl;
    CHECK(lit.value<datatypes::xsd::Float>() == value);
}

TEST_CASE("inlining") {
    using datatypes::xsd::Float;

    Float::cpp_type f = 123.89f;
    auto const inlined = Float::try_into_inlined(f);
    auto const deinlined = Float::from_inlined(*inlined);

    CHECK(f == deinlined);
}

TEST_CASE("float over/underflow from string") {
    // https://www.w3.org/TR/xmlschema11-2/#f-floatPtRound
    using type = datatypes::xsd::Float::cpp_type;
    auto const parse = [](char const *s) { return Literal::make_typed<datatypes::xsd::Float>(s).value<datatypes::xsd::Float>(); };

    CHECK_EQ(parse("1E39"), std::numeric_limits<type>::infinity());
    CHECK_EQ(parse("-1E39"), -std::numeric_limits<type>::infinity());
    CHECK_EQ(parse("340282356779733661637539395458142568448"), std::numeric_limits<type>::infinity());

    CHECK_EQ(parse("1E-50"), 0.0f);
    CHECK_FALSE(std::signbit(parse("1E-50")));
    CHECK_EQ(parse("-1E-50"), 0.0f);
    CHECK(std::signbit(parse("-1E-50")));
    CHECK_EQ(parse("1E-45"), std::numeric_limits<type>::denorm_min());
}
