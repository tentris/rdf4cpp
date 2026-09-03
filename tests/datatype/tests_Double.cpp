#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>
#include <cmath>

using namespace rdf4cpp;

TEST_CASE("Datatype Double") {
    using type = datatypes::xsd::Double::cpp_type;

    constexpr auto correct_type_iri_cstr = "http://www.w3.org/2001/XMLSchema#double";
    CHECK(std::string(datatypes::xsd::Double::identifier) == correct_type_iri_cstr);
    auto type_iri = IRI(datatypes::xsd::Double::identifier);
    CHECK(type_iri.identifier() == correct_type_iri_cstr);

    std::string rdf_dbl_1_0{"1.0E0"};

    double value = 1.00;
    auto lit1 = Literal::make_typed_from_value<datatypes::xsd::Double>(value);
    CHECK(lit1.value<datatypes::xsd::Double>() == value);
    CHECK(lit1.lexical_form() == rdf_dbl_1_0);

    value = 987456321123456.123586987;
    auto lit2 = Literal::make_typed_from_value<datatypes::xsd::Double>(value);
    CHECK(lit2.value<datatypes::xsd::Double>() == value);

    value = -64545352389.2352345670;
    auto lit3 = Literal::make_typed_from_value<datatypes::xsd::Double>(value);
    CHECK(lit3.value<datatypes::xsd::Double>() == value);

    value = 1;
    auto lit4 = Literal::make_typed_from_value<datatypes::xsd::Double>(value);
    CHECK(lit4.value<datatypes::xsd::Double>() == value);
    CHECK(lit4.lexical_form() == rdf_dbl_1_0);

    value = 1;
    auto lit6 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit6.value<datatypes::xsd::Double>() == value);
    CHECK(lit6.lexical_form() == rdf_dbl_1_0);

    value = 987456321123456.123586987;
    auto lit7 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit7.value<datatypes::xsd::Double>() == value);

    auto lit8 = Literal::make_typed("NaN", type_iri);
    CHECK(std::isnan(lit8.value<datatypes::xsd::Double>()));

    auto lit9 = Literal::make_typed("INF", type_iri);
    CHECK(std::isinf(lit9.value<datatypes::xsd::Double>()));

    value = INFINITY;
    auto lit10 = Literal::make_typed_from_value<datatypes::xsd::Double>(value);
    CHECK(std::isinf(lit10.value<datatypes::xsd::Double>()));

    value = NAN;
    auto lit11 = Literal::make_typed_from_value<datatypes::xsd::Double>(value);
    CHECK(std::isnan(lit11.value<datatypes::xsd::Double>()));

    value = 2.22e-308;
    auto lit12 = Literal::make_typed("2.22e-308", type_iri);
    CHECK(lit12.value<datatypes::xsd::Double>() == value);

    auto lit13 = Literal::make_typed("+INF", type_iri);
    CHECK(std::isinf(lit13.value<datatypes::xsd::Double>()));

    auto lit14 = Literal::make_typed("-INF", type_iri);
    CHECK(std::isinf(lit14.value<datatypes::xsd::Double>()));
    CHECK(lit14.value<datatypes::xsd::Double>() == -std::numeric_limits<type>::infinity());

    value = -INFINITY;
    auto lit15 = Literal::make_typed_from_value<datatypes::xsd::Double>(value);
    CHECK(std::isinf(lit15.value<datatypes::xsd::Double>()));
    CHECK(lit15.value<datatypes::xsd::Double>() == -std::numeric_limits<type>::infinity());

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

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("454sdsd", type_iri), "http://www.w3.org/2001/XMLSchema#double parsing error: found s, invalid for datatype", rdf4cpp::InvalidNode);
    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("-", type_iri), "http://www.w3.org/2001/XMLSchema#double parsing error: literal is empty", rdf4cpp::InvalidNode);
}

TEST_CASE("round-trip") {
    datatypes::xsd::Double::cpp_type const value = -0.1234567890001;
    auto const lit = Literal::make_typed_from_value<datatypes::xsd::Double>(value);
    std::cout << lit.lexical_form() << std::endl;
    CHECK(lit.value<datatypes::xsd::Double>() == value);
}

TEST_CASE("double inlining") {
    double value = 9999;
    auto lit = Literal::make_typed_from_value<datatypes::xsd::Double>(value);
    CHECK(lit.backend_handle().is_inlined());
    CHECK(lit.value<datatypes::xsd::Double>() == value);
}

TEST_CASE("double inlining large") {
    size_t total = 0;
    size_t num_inlined = 0;

    for (auto const &quad : parser::RDFFileParser{"doubles.ttl"}) {
        CHECK(quad.has_value());
        CHECK(quad->object().is_literal());

        total += 1;
        num_inlined += quad->object().is_inlined();
    }

    auto const inlining_percentage = static_cast<double>(num_inlined) / static_cast<double>(total) * 100.0;

    std::cout << std::format("{:.2f}% inlined ({}/{})", inlining_percentage, num_inlined, total) << std::endl;
}
