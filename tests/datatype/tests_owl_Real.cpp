#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_SUITE("owl:real") {
    TEST_CASE("capabilities") {
        static_assert(datatypes::LiteralDatatype<datatypes::owl::Real>);
        static_assert(datatypes::NumericLiteralDatatype<datatypes::owl::Real>);
        static_assert(datatypes::LogicalLiteralDatatype<datatypes::owl::Real>);
        static_assert(!datatypes::PromotableLiteralDatatype<datatypes::owl::Real>);
        static_assert(!datatypes::SubtypedLiteralDatatype<datatypes::owl::Real>);
    }

    TEST_CASE("to string") {
        using datatypes::owl::Real;
        using cpp_type = Real::cpp_type;

        // serialization must not lose precision (#501), i.e. lexical form -> value round-trips exactly
        for (cpp_type const &value : {cpp_type{1}, cpp_type{-42}, cpp_type{"0.1"}, cpp_type{0.1}, cpp_type{"1e400"},
                                      cpp_type{"123456789012345678901234567890.5"}, std::numeric_limits<cpp_type>::denorm_min(),
                                      std::numeric_limits<cpp_type>::max(), std::numeric_limits<cpp_type>::infinity()}) {
            auto const lit = Literal::make_typed_from_value<Real>(value);
            CHECK(Literal::make_typed(lit.lexical_form(), IRI{datatypes::registry::owl_real}).value<Real>() == value);
        }

        CHECK(Literal::make_typed_from_value<Real>(cpp_type{1}).lexical_form() == "1");
        CHECK(Literal::make_typed_from_value<Real>(cpp_type{"0.1"}).lexical_form() == "0.100000000000000000000000000000000005");
        CHECK(Literal::make_typed_from_value<Real>(cpp_type{"123456789012345678901234567890.5"}).lexical_form() == "123456789012345678901234567890.5");
    }
}