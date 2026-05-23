#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_CASE("Datatype NonPositiveInteger") {
    CHECK(storage::default_node_storage.has_specialized_storage_for(datatypes::xsd::NonPositiveInteger::fixed_id));

    constexpr auto correct_type_iri_cstr = "http://www.w3.org/2001/XMLSchema#nonPositiveInteger";

    CHECK(std::string(datatypes::xsd::NonPositiveInteger::identifier) == correct_type_iri_cstr);

    auto type_iri = IRI(datatypes::xsd::NonPositiveInteger::identifier);
    CHECK(type_iri.identifier() == correct_type_iri_cstr);

    int64_t value = -1;
    auto lit1 = Literal::make_typed_from_value<datatypes::xsd::NonPositiveInteger>(value);
    CHECK(lit1.value<datatypes::xsd::NonPositiveInteger>() == value);
    CHECK(lit1.lexical_form() == std::to_string(value));

    value = -18446744073709;
    auto lit2 = Literal::make_typed_from_value<datatypes::xsd::NonPositiveInteger>(value);
    CHECK(lit2.value<datatypes::xsd::NonPositiveInteger>() == value);
    CHECK(lit2.lexical_form() == std::to_string(value));

    value = 0;
    auto lit3 = Literal::make_typed_from_value<datatypes::xsd::NonPositiveInteger>(value);
    CHECK(lit3.value<datatypes::xsd::NonPositiveInteger>() == value);
    CHECK(lit3.lexical_form() == std::to_string(value));

    value = -1;
    auto lit4 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit4.value<datatypes::xsd::NonPositiveInteger>() == value);

    value = -18446744073709;
    auto lit5 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit5.value<datatypes::xsd::NonPositiveInteger>() == value);

    CHECK(lit1 != lit2);
    CHECK(lit2 != lit3);
    CHECK(lit1 == lit4);
    CHECK(lit2 == lit5);

    // suppress warnings regarding attribute ‘nodiscard’
    Literal no_discard_dummy;

    CHECK_THROWS(no_discard_dummy = Literal::make_typed("qwerty", type_iri));

    CHECK_THROWS(no_discard_dummy = Literal::make_typed("1", type_iri));

    CHECK_THROWS(no_discard_dummy = Literal::make_typed("a23dg.59566", type_iri));

    CHECK_THROWS(no_discard_dummy = Literal::make_typed("0.01", type_iri));
}

TEST_CASE("xsd:nonPositiveInteger inlining") {
    using datatypes::xsd::NonPositiveInteger;

    auto zero_lit = Literal::make_typed_from_value<NonPositiveInteger>(0);
    CHECK(zero_lit.backend_handle().is_inlined());
    CHECK(zero_lit.value<NonPositiveInteger>() == 0);

    auto one_lit = Literal::make_typed_from_value<NonPositiveInteger>(-1);
    CHECK(one_lit.backend_handle().is_inlined());
    CHECK(one_lit.value<NonPositiveInteger>() == -1);

    auto large_lit = Literal::make_typed_from_value<NonPositiveInteger>(-(1l << 42) + 1);
    CHECK(large_lit.backend_handle().is_inlined());
    CHECK(large_lit.value<NonPositiveInteger>() == (-(1l << 42) + 1));
}
