#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_CASE("Datatype PositiveInteger") {
    CHECK(storage::default_node_storage.has_specialized_storage_for(datatypes::xsd::PositiveInteger::fixed_id));

    constexpr auto correct_type_iri_cstr = "http://www.w3.org/2001/XMLSchema#positiveInteger";

    CHECK(std::string(datatypes::xsd::PositiveInteger::identifier) == correct_type_iri_cstr);

    auto type_iri = IRI(datatypes::xsd::PositiveInteger::identifier);
    CHECK(type_iri.identifier() == correct_type_iri_cstr);

    uint64_t value = 1;
    auto lit1 = Literal::make_typed_from_value<datatypes::xsd::PositiveInteger>(value);
    CHECK(lit1.value<datatypes::xsd::PositiveInteger>() == value);
    CHECK(lit1.lexical_form() == std::to_string(value));

    value = 18446744073709;
    auto lit2 = Literal::make_typed_from_value<datatypes::xsd::PositiveInteger>(value);
    CHECK(lit2.value<datatypes::xsd::PositiveInteger>() == value);
    CHECK(lit2.lexical_form() == std::to_string(value));

    value = 2147483647;
    auto lit3 = Literal::make_typed_from_value<datatypes::xsd::PositiveInteger>(value);
    CHECK(lit3.value<datatypes::xsd::PositiveInteger>() == value);
    CHECK(lit3.lexical_form() == std::to_string(value));

    value = 1;
    auto lit4 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit4.value<datatypes::xsd::PositiveInteger>() == value);

    value = 18446744073709;
    auto lit5 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit5.value<datatypes::xsd::PositiveInteger>() == value);

    CHECK(lit1 != lit2);
    CHECK(lit2 != lit3);
    CHECK(lit1 == lit4);
    CHECK(lit2 == lit5);

    // suppress warnings regarding attribute ‘nodiscard’
    Literal no_discard_dummy;

    CHECK_THROWS(no_discard_dummy = Literal::make_typed("-1", type_iri));

    CHECK_THROWS(no_discard_dummy = Literal::make_typed("0", type_iri));

    CHECK_THROWS(no_discard_dummy = Literal::make_typed("a23dg.59566", type_iri));

    CHECK_THROWS(no_discard_dummy = Literal::make_typed("-0.01", type_iri));
}

TEST_CASE("xsd:positiveInteger inlining") {
    using datatypes::xsd::PositiveInteger;

    auto one_lit = Literal::make_typed_from_value<PositiveInteger>(1);
    CHECK(one_lit.backend_handle().is_inlined());
    CHECK(one_lit.value<PositiveInteger>() == 1);

    auto large_lit = Literal::make_typed_from_value<PositiveInteger>(1l << 42);
    CHECK(large_lit.backend_handle().is_inlined());
    CHECK(large_lit.value<PositiveInteger>() == (1l << 42));
}
