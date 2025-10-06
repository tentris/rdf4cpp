#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp::regex;

TEST_SUITE("regex") {
    TEST_CASE("replacement translation") {
        Regex const r{"[0-9]"};

        SUBCASE("no escape") {
            CHECK_THROWS((void) r.make_replacer("$"));

            {
                std::string s = "Hello 99 World";
                r.make_replacer(R"($0_)").regex_replace(s);
                CHECK(s == "Hello 9_9_ World");
            }
        }

        SUBCASE("1 escape") {
            {
                std::string s = "Hello 99 World";
                r.make_replacer(R"(\$)").regex_replace(s);
                CHECK(s == "Hello $$ World");
            }

            {
                std::string s = "Hello 99 World";
                r.make_replacer(R"(\$0)").regex_replace(s);
                CHECK(s == "Hello $0$0 World");
            }
        }

        SUBCASE("2 escape") {
            CHECK_THROWS((void) r.make_replacer(R"(\\$)"));

            {
                std::string s = "Hello 99 World";
                r.make_replacer(R"(\\$0)").regex_replace(s);
                CHECK(s == R"(Hello \9\9 World)");
            }
        }

        SUBCASE("3 escape") {
            {
                std::string s = "Hello 99 World";
                r.make_replacer(R"(\\\$)").regex_replace(s);
                CHECK(s == R"(Hello \$\$ World)");
            }

            {
                std::string s = "Hello 99 World";
                r.make_replacer(R"(\\\$0)").regex_replace(s);
                CHECK(s == R"(Hello \$0\$0 World)");
            }
        }

        SUBCASE("4 escape") {
            CHECK_THROWS((void) r.make_replacer(R"(\\\\$)"));

            {
                std::string s = "Hello 99 World";
                r.make_replacer(R"(\\\\$0)").regex_replace(s);
                CHECK(s == R"(Hello \\9\\9 World)");
            }
        }

        SUBCASE("stray backslash") {
            // TODO
            // CHECK_THROWS(r.make_replacer(R"(Hello \ World)"));
        }
    }
}
