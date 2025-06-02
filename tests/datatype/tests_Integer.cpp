#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp;
using namespace datatypes;

TEST_CASE("integer capabilities") {
    static_assert(datatypes::LiteralDatatype<datatypes::xsd::Integer>);
    static_assert(datatypes::NumericLiteralDatatype<datatypes::xsd::Integer>);
    static_assert(datatypes::LogicalLiteralDatatype<datatypes::xsd::Integer>);
    static_assert(!datatypes::PromotableLiteralDatatype<datatypes::xsd::Integer>);
    static_assert(datatypes::SubtypedLiteralDatatype<datatypes::xsd::Integer>);
    static_assert(datatypes::xsd::Integer::subtype_rank == 1);
    static_assert(datatypes::ComparableLiteralDatatype<datatypes::xsd::Integer>);
    static_assert(datatypes::FixedIdLiteralDatatype<datatypes::xsd::Integer>);
    static_assert(std::numeric_limits<datatypes::xsd::Integer::cpp_type>::digits10 > 16);
}

TEST_CASE("Datatype Integer") {

    constexpr auto correct_type_iri_cstr = "http://www.w3.org/2001/XMLSchema#integer";

    CHECK(std::string(datatypes::xsd::Integer::identifier) == correct_type_iri_cstr);

    auto type_iri = IRI(datatypes::xsd::Integer::identifier);
    CHECK(type_iri.identifier() == correct_type_iri_cstr);

    int64_t value = 1.00;
    auto lit1 = Literal::make_typed_from_value<datatypes::xsd::Integer>(value);
    CHECK(lit1.value<datatypes::xsd::Integer>() == value);
    CHECK(lit1.lexical_form() == std::to_string(value));

    value = -2147483648;
    auto lit2 = Literal::make_typed_from_value<datatypes::xsd::Integer>(value);
    CHECK(lit2.value<datatypes::xsd::Integer>() == value);
    CHECK(lit2.lexical_form() == std::to_string(value));

    value = 2147483647;
    auto lit3 = Literal::make_typed_from_value<datatypes::xsd::Integer>(value);
    CHECK(lit3.value<datatypes::xsd::Integer>() == value);
    CHECK(lit3.lexical_form() == std::to_string(value));

    //Testing Literal Constructor
    value = 1;
    auto lit4 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit4.value<datatypes::xsd::Integer>() == value);

    value = 2147483647;
    auto lit5 = Literal::make_typed(std::to_string(value), type_iri);
    CHECK(lit5.value<datatypes::xsd::Integer>() == value);

    auto lit6 = Literal::make_typed("+1", type_iri);
    CHECK(lit6.value<datatypes::xsd::Integer>() == 1);

    value = std::numeric_limits<int64_t>::max();
    auto lit7 = Literal::make_typed_from_value<datatypes::xsd::Integer>(value);
    CHECK(lit7.value<datatypes::xsd::Integer>() == value);
    CHECK(lit7.lexical_form() == std::to_string(value));

    value = std::numeric_limits<int64_t>::min();
    auto lit8 = Literal::make_typed_from_value<datatypes::xsd::Integer>(value);
    CHECK(lit8.value<datatypes::xsd::Integer>() == value);
    CHECK(lit8.lexical_form() == std::to_string(value));

    CHECK(lit1 != lit2);
    CHECK(lit2 != lit3);
    CHECK(lit1 == lit4);
    CHECK(lit3 == lit5);
    CHECK(lit4 == lit6);
    CHECK(lit4.value<datatypes::xsd::Integer>() == lit6.value<datatypes::xsd::Integer>());

    // suppress warnings regarding attribute ‘nodiscard’
    Literal no_discard_dummy;

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("a23dg", type_iri), "http://www.w3.org/2001/XMLSchema#integer parsing error: found a, invalid for datatype", InvalidNode);
    CHECK_THROWS(no_discard_dummy = Literal::make_typed("2.2e-308", type_iri));
}

TEST_CASE("Datatype Integer overread UB") {
    std::string const s = "123456";
    std::string_view const sv{ s.data(), 3 };

    auto const lit = Literal::make_typed(sv, IRI{datatypes::xsd::Integer::identifier});
    CHECK(lit.value<datatypes::xsd::Integer>() == 123);
}

TEST_CASE("other int types serializing") {
    using namespace datatypes::xsd;

    auto lit1 = Literal::make_typed_from_value<Int>(std::numeric_limits<Int::cpp_type>::min());
    CHECK(lit1.lexical_form() == std::to_string(std::numeric_limits<Int::cpp_type>::min()));
}

TEST_CASE("integer inlining") {
    SUBCASE("small") {
        auto lit = Literal::make_typed_from_value<xsd::Integer>((1l << 41) - 1);
        CHECK(lit.backend_handle().is_inlined());
    }

    SUBCASE("too large") {
        auto lit = Literal::make_typed<xsd::Integer>("9999999999999999999999");
        CHECK(!lit.backend_handle().is_inlined());
    }
}
