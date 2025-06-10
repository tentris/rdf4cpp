#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_CASE("decimal capabilities") {
    static_assert(datatypes::LiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::NumericLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::LogicalLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::PromotableLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::xsd::Decimal::promotion_rank == 2);
    static_assert(!datatypes::SubtypedLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::ComparableLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::FixedIdLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::InlineableLiteralDatatype<datatypes::xsd::Decimal>);
}

TEST_CASE("Datatype Decimal") {
    CHECK(storage::default_node_storage.has_specialized_storage_for(datatypes::xsd::Decimal::fixed_id));

    constexpr auto correct_type_iri_cstr = "http://www.w3.org/2001/XMLSchema#decimal";

    CHECK(std::string(datatypes::xsd::Decimal::identifier) == correct_type_iri_cstr);

    auto type_iri = IRI(datatypes::xsd::Decimal::identifier);
    CHECK(type_iri.identifier() == correct_type_iri_cstr);

    using type = datatypes::xsd::Decimal::cpp_type;

    std::string rdf_dbl_1_0{"1.0"};
    std::string rdf_dbl_0_0{"0.0"};

    type value = type(1.00);
    auto lit1 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit1.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit1.lexical_form() == rdf_dbl_1_0);

    value = type(64582165456988.6235896);
    auto lit2 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit2.value<datatypes::xsd::Decimal>() == value);

    value = type(-64524654389.12345678);
    auto lit3 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit3.value<datatypes::xsd::Decimal>() == value);

    value = type(1.0);
    auto lit4 = Literal::make_typed(to_string(value), type_iri);
    CHECK(lit4.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit4.lexical_form() == rdf_dbl_1_0);

    value = type{"64582165456988.235046"};
    auto lit5 = Literal::make_typed("64582165456988.235046", type_iri);
    CHECK(lit5.value<datatypes::xsd::Decimal>() == value);

    value = type{1};
    auto lit6 = Literal::make_typed("1.", type_iri);
    CHECK(lit6.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit6.lexical_form() == rdf_dbl_1_0);

    auto lit7 = Literal::make_typed(rdf_dbl_1_0, type_iri);
    CHECK(lit7.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit7.lexical_form() == rdf_dbl_1_0);

    auto lit8 = Literal::make_typed("1.00", type_iri);
    CHECK(lit8.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit8.lexical_form() == rdf_dbl_1_0);

    value = type{"3.111"};
    auto lit10 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit10.value<datatypes::xsd::Decimal>() == value);

    value = type{0};
    auto lit11 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit11.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit11.lexical_form() == rdf_dbl_0_0);

    value = type{1.0};
    auto lit12 = Literal::make_typed("+1.0000", type_iri);
    CHECK(lit12.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit12.lexical_form() == rdf_dbl_1_0);

    value = type{"0.000000005"};
    auto lit13 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit13.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit13.lexical_form() == "0.000000005");

    value = type{6000000000.0};
    auto lit14 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit14.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit14.lexical_form() == "6000000000.0");

    CHECK(lit1 != lit2);
    CHECK(lit2 != lit3);
    CHECK(lit1 == lit4);
    CHECK(lit6 == lit7);
    CHECK(lit6 == lit8);
    CHECK(lit7 == lit8);
    CHECK(lit1 == lit12);
    CHECK(lit4 == lit12);
    CHECK(lit6 == lit12);
    CHECK(lit7 == lit12);
    CHECK(lit8 == lit12);
    CHECK(lit6.value<datatypes::xsd::Decimal>() == lit7.value<datatypes::xsd::Decimal>());
    CHECK(lit6.value<datatypes::xsd::Decimal>() == lit8.value<datatypes::xsd::Decimal>());
    CHECK(lit7.value<datatypes::xsd::Decimal>() == lit7.value<datatypes::xsd::Decimal>());

    // suppress warnings regarding attribute ‘nodiscard’
    Literal no_discard_dummy;

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("NAN", type_iri), "http://www.w3.org/2001/XMLSchema#decimal parsing error: non-numeric char found", InvalidNode);

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("INF", type_iri), "http://www.w3.org/2001/XMLSchema#decimal parsing error: non-numeric char found", InvalidNode);

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("454sdsd", type_iri), "http://www.w3.org/2001/XMLSchema#decimal parsing error: non-numeric char found", InvalidNode);

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("2.225E-307", type_iri), "http://www.w3.org/2001/XMLSchema#decimal parsing error: non-numeric char found", InvalidNode);
}

TEST_CASE("precision") {
    // xsd:decimal requires totalDigits to be >= 18
    // see: https://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#dt-decimal

    using datatypes::xsd::Decimal;
    using cpp_type = Decimal::cpp_type;

    static_assert(std::numeric_limits<cpp_type>::digits10 >= 18);

    cpp_type const i = pow(cpp_type{10}, 18) - cpp_type{1};
    cpp_type const x = i * (cpp_type{1} / pow(cpp_type{10}, 18));

    Literal const lit = Literal::make_typed_from_value<datatypes::xsd::Decimal>(x);
    CHECK(lit.lexical_form() == "0.999999999999999999");
}

TEST_CASE("Datatype Decimal buffer overread UB") {
    std::string const s = "123.456";
    std::string_view const sv{s.data(), 5};

    auto const lit = Literal::make_typed(sv, IRI{datatypes::xsd::Decimal::identifier});
    CHECK(lit.value<datatypes::xsd::Decimal>() == datatypes::xsd::Decimal::cpp_type{"123.4"});
}

TEST_CASE("decimal inlining sanity check") {
    using namespace datatypes::xsd;

    SUBCASE("sanity check") {
        auto const l = Literal::make_typed<Decimal>("4.2");
        CHECK(l.is_inlined());
        CHECK(static_cast<double>(l.value<Decimal>()) == 4.2);

        auto const l2 = Literal::make_typed<Decimal>("-1.0");
        CHECK_EQ(l2.value<Decimal>(), -1);

        auto const l3 = Literal::make_typed<Decimal>("1.984612364642233");
        CHECK(!l3.is_inlined());
        CHECK_EQ(l3.value<Decimal>(), Decimal::cpp_type{"1.984612364642233"});

        auto const l4 = Literal::make_typed<Decimal>("-1.984612364642233");
        CHECK(!l4.is_inlined());
        CHECK_EQ(l4.value<Decimal>(), Decimal::cpp_type{"-1.984612364642233"});

        auto const l5 = Literal::make_typed<Decimal>("1.8743");
        CHECK(l5.is_inlined());
        CHECK_EQ(l5.value<Decimal>(), Decimal::cpp_type{"1.8743"});

        auto const l6 = Literal::make_typed<Decimal>("-1.9846");
        CHECK(l6.is_inlined());
        CHECK_EQ(l6.value<Decimal>(), Decimal::cpp_type{"-1.9846"});
    }

    SUBCASE("normalization") {
        auto l = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type{40, 1});
        auto l2 = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type{4, 0});

        CHECK(l.is_inlined());
        CHECK(l2.is_inlined());
        CHECK_EQ(l, l2);
        CHECK_EQ(l.backend_handle(), l2.backend_handle());
    }

    SUBCASE("limits") {
        SUBCASE("unscaled value") {
            auto const very_big_value = std::numeric_limits<rdf4cpp::util::Int128>::max();
            CHECK_GT(very_big_value, std::numeric_limits<int64_t>::max());

            // way over the limit
            auto const l = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type{very_big_value, 1U});
            CHECK(!l.is_inlined());
            CHECK(l.value<Decimal>() == Decimal::cpp_type{very_big_value, 1U});

            auto const l2 = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type{-very_big_value, 1U});
            CHECK(!l2.is_inlined());
            CHECK(l2.value<Decimal>() == Decimal::cpp_type{-very_big_value, 1U});

            // right above the inlining limit
            auto const l3 = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type(1L << 31, 0));
            CHECK(!l3.is_inlined());
            CHECK(l3.value<Decimal>() == Decimal::cpp_type(1L << 31, 0));

            auto const l4 = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type(-(1L << 31) - 1, 0));
            CHECK(!l4.is_inlined());
            CHECK(l4.value<Decimal>() == Decimal::cpp_type(-(1L << 31) - 1, 0));

            // right below the inlining limit
            auto const l5 = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type((1L << 31) - 1, 0));
            CHECK(l5.is_inlined());
            CHECK(l5.value<Decimal>() == Decimal::cpp_type((1L << 31) - 1, 0));

            auto const l6 = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type(-(1L << 31), 0));
            CHECK(l6.is_inlined());
            CHECK(l6.value<Decimal>() == Decimal::cpp_type(-(1L << 31), 0));
        }

        SUBCASE("exponent") {
            auto const l = Literal::make_typed_from_value<Decimal>(Decimal::cpp_type{rdf4cpp::util::BigInt{5}, 1U << 10});
            CHECK(!l.is_inlined());
            CHECK(l.value<Decimal>() == Decimal::cpp_type(5, 1U << 10));
        }
    }
}

TEST_CASE("decimal possible bug") {
    // This bug only occurs in boost versions < 1.86.0
    auto const lit1 = Literal::make_typed_from_value<datatypes::xsd::Double>(5).cast<rdf4cpp::datatypes::xsd::Decimal>();
    auto const lit2 = Literal::make_typed_from_value<datatypes::xsd::Double>(std::numeric_limits<double>::min()).cast<rdf4cpp::datatypes::xsd::Decimal>();

    auto const div_res = lit1 / lit2; // program crashes here if the bug is present
    CHECK(div_res.null());
}
