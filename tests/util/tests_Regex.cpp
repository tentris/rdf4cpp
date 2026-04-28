#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp::regex;

TEST_SUITE("regex") {
    TEST_CASE("basic syntax") {
        CHECK(Regex{"abc"}.regex_match("abc"));
        CHECK(!Regex{"abc"}.regex_match("_abc"));
        CHECK(!Regex{"abc"}.regex_match("abc_"));

        CHECK(Regex{"a|b"}.regex_match("a"));
        CHECK(Regex{"a|b"}.regex_match("b"));
        CHECK(!Regex{"a|b"}.regex_match("c"));

        CHECK(Regex{"a?"}.regex_match("a"));
        CHECK(Regex{"a?"}.regex_match(""));
        CHECK(!Regex{"a?"}.regex_match("aa"));

        CHECK(Regex{"a*"}.regex_match("a"));
        CHECK(Regex{"a*"}.regex_match("aaaa"));
        CHECK(Regex{"a*"}.regex_match(""));
        CHECK(!Regex{"a*"}.regex_match("b"));

        CHECK(!Regex{"a+"}.regex_match(""));
        CHECK(Regex{"a+"}.regex_match("a"));
        CHECK(Regex{"a+"}.regex_match("aaaaa"));
        CHECK(!Regex{"a+"}.regex_match("b"));

        CHECK(!Regex{"a{2,5}"}.regex_match("a"));
        CHECK(Regex{"a{2,5}"}.regex_match("aa"));
        CHECK(Regex{"a{2,5}"}.regex_match("aaaaa"));
        CHECK(!Regex{"a{2,5}"}.regex_match("aaaaaa"));
        CHECK(!Regex{"a{2,5}"}.regex_match("bbb"));

        CHECK(Regex{"a{2}"}.regex_match("aa"));
        CHECK(!Regex{"a{2}"}.regex_match("aaa"));
        CHECK(!Regex{"a{2}"}.regex_match("aaaa"));
        CHECK(!Regex{"a{2}"}.regex_match("b"));

        CHECK(Regex{"a{2,}"}.regex_match("aa"));
        CHECK(Regex{"a{2,}"}.regex_match("aaa"));
        CHECK(!Regex{"a{2,}"}.regex_match("a"));
        CHECK(!Regex{"a{2,}"}.regex_match("b"));

        CHECK(Regex{"a{0,2}"}.regex_match("aa"));
        CHECK(!Regex{"a{0,2}"}.regex_match("aaa"));
        CHECK(Regex{"a{0,2}"}.regex_match("a"));
        CHECK(!Regex{"a{0,2}"}.regex_match("b"));

        CHECK(Regex{"a{0,0}"}.regex_match(""));
        CHECK(!Regex{"a{0,0}"}.regex_match("a"));

        CHECK(Regex{"\\.\\\\\\?\\*\\+\\{\\}\\(\\)\\[\\]\\|\\-\\^\\n\\r\\t"}.regex_match(".\\?*+{}()[]|-^\n\r\t"));
        CHECK(!Regex{"\\."}.regex_match("a"));
        CHECK(!Regex{"\\\\"}.regex_match("a"));
        CHECK(!Regex{"\\?"}.regex_match("a"));
        CHECK(!Regex{"\\*"}.regex_match("a"));
        CHECK(!Regex{"\\+"}.regex_match("a"));
        CHECK(!Regex{"\\{"}.regex_match("a"));
        CHECK(!Regex{"\\}"}.regex_match("a"));
        CHECK(!Regex{"\\("}.regex_match("a"));
        CHECK(!Regex{"\\["}.regex_match("a"));
        CHECK(!Regex{"\\]"}.regex_match("a"));
        CHECK(!Regex{"\\|"}.regex_match("a"));
        CHECK(!Regex{"\\-"}.regex_match("a"));
        CHECK(!Regex{"\\^"}.regex_match("a"));
        CHECK(!Regex{"\\n"}.regex_match("a"));
        CHECK(!Regex{"\\r"}.regex_match("a"));
        CHECK(!Regex{"\\t"}.regex_match("a"));

        CHECK(Regex{"(abc)"}.regex_match("abc"));
        CHECK(!Regex{"(abc)"}.regex_match("adc"));


        CHECK(Regex{"[abc]"}.regex_match("a"));
        CHECK(Regex{"[abc]"}.regex_match("b"));
        CHECK(Regex{"[abc]"}.regex_match("c"));
        CHECK(!Regex{"[abc]"}.regex_match("d"));
        CHECK(!Regex{"[^abc]"}.regex_match("a"));
        CHECK(Regex{"[^abc]"}.regex_match("d"));
        CHECK(Regex{"[a-c]"}.regex_match("a"));
        CHECK(Regex{"[a-c]"}.regex_match("b"));
        CHECK(Regex{"[a-c]"}.regex_match("c"));
        CHECK(!Regex{"[a-c]"}.regex_match("d"));
        CHECK(!Regex{"[^a-c]"}.regex_match("a"));
        CHECK(Regex{"[^a-c]"}.regex_match("d"));
        // CHECK(!Regex{"[^abc-e]"}.regex_match("e"));
        // CHECK(Regex{"[^abc-e]"}.regex_match("d"));


        CHECK(Regex{"\\p{Lu}"}.regex_match("A"));
        CHECK(Regex{"\\p{Lu}"}.regex_match("Ä"));
        CHECK(!Regex{"\\p{Lu}"}.regex_match("a"));
        CHECK(!Regex{"\\p{Lu}"}.regex_match("ä"));
        CHECK(!Regex{"\\p{Lu}"}.regex_match("5"));

        // CHECK(Regex{"\\p{IsBasicLatin}"}.regex_match("5"));
        // CHECK(!Regex{"\\p{IsBasicLatin}"}.regex_match("ü"));

        CHECK(Regex{"."}.regex_match("5"));
        CHECK(!Regex{"."}.regex_match("\n"));
        CHECK(Regex{"\\s"}.regex_match(" "));
        CHECK(!Regex{"\\s"}.regex_match("a"));
        CHECK(Regex{"\\S"}.regex_match("a"));
        // CHECK(Regex{"\\c"}.regex_match("a"));
        // CHECK(!Regex{"\\c"}.regex_match("<"));
        // CHECK(Regex{"\\C"}.regex_match("<"));
        CHECK(Regex{"\\d"}.regex_match("5"));
        CHECK(!Regex{"\\d"}.regex_match("a"));
        CHECK(Regex{"\\D"}.regex_match("a"));
        CHECK(Regex{"\\w\\w"}.regex_match("a5"));
        CHECK(!Regex{"\\w"}.regex_match("-"));
        CHECK(Regex{"\\W"}.regex_match("-"));

        CHECK(Regex{"a"}.regex_search("_a_"));
        CHECK(!Regex{"^a"}.regex_search("_a"));
        CHECK(!Regex{"a$"}.regex_search("a_"));


        CHECK(Regex{"a??"}.regex_match("a"));
        CHECK(Regex{"a??"}.regex_match(""));
        CHECK(!Regex{"a??"}.regex_match("aa"));

        CHECK(Regex{"a*?"}.regex_match("a"));
        CHECK(Regex{"a*?"}.regex_match("aaaa"));
        CHECK(Regex{"a*?"}.regex_match(""));
        CHECK(!Regex{"a*?"}.regex_match("b"));

        CHECK(!Regex{"a+?"}.regex_match(""));
        CHECK(Regex{"a+?"}.regex_match("a"));
        CHECK(Regex{"a+?"}.regex_match("aaaaa"));
        CHECK(!Regex{"a+?"}.regex_match("b"));

        CHECK(!Regex{"a{2,5}?"}.regex_match("a"));
        CHECK(Regex{"a{2,5}?"}.regex_match("aa"));
        CHECK(Regex{"a{2,5}?"}.regex_match("aaaaa"));
        CHECK(!Regex{"a{2,5}?"}.regex_match("aaaaaa"));
        CHECK(!Regex{"a{2,5}?"}.regex_match("bbb"));

        CHECK(Regex{"a{2}?"}.regex_match("aa"));
        CHECK(!Regex{"a{2}?"}.regex_match("aaa"));
        CHECK(!Regex{"a{2}?"}.regex_match("aaaa"));
        CHECK(!Regex{"a{2}ß"}.regex_match("b"));

        CHECK(Regex{"a{2,}?"}.regex_match("aa"));
        CHECK(Regex{"a{2,}?"}.regex_match("aaa"));
        CHECK(!Regex{"a{2,}?"}.regex_match("a"));
        CHECK(!Regex{"a{2,}?"}.regex_match("b"));

        CHECK(Regex{"a{0,2}?"}.regex_match("aa"));
        CHECK(!Regex{"a{0,2}?"}.regex_match("aaa"));
        CHECK(Regex{"a{0,2}?"}.regex_match("a"));
        CHECK(!Regex{"a{0,2}?"}.regex_match("b"));

        CHECK(Regex{"a{0,0}?"}.regex_match(""));
        CHECK(!Regex{"a{0,0}?"}.regex_match("a"));


        CHECK(Regex{"(abc)"}.regex_match("abc"));
        CHECK(Regex{"(?:abc)"}.regex_match("abc"));
        // CHECK(Regex{"(.)bc\\1"}.regex_match("abca"));
        // CHECK(Regex{"(.)bc\\1"}.regex_match("xbcx"));
        // CHECK(!Regex{"(.)bc\\1"}.regex_match("abcx"));

        CHECK(Regex{".", RegexFlag::DotAll}.regex_match("\n"));
        CHECK(Regex{"^a$", RegexFlag::Multiline}.regex_search("a\nxyz"));
        CHECK(Regex{"abc", RegexFlag::CaseInsensitive}.regex_match("ABC"));
        CHECK(Regex{"a b c", RegexFlag::RemoveWhitespace}.regex_match("abc"));
        CHECK(Regex{".+", RegexFlag::Literal}.regex_match(".+"));
        CHECK(!Regex{".", RegexFlag::Literal}.regex_match("a"));

        std::string d = "a_a_x";
        Regex{"a"}.make_replacer("b").regex_replace(d);
        CHECK(d == "b_b_x");
        Regex{"_(\\w)"}.make_replacer("-$0$1").regex_replace(d);
        CHECK(d == "b-_bb-_xx");
    }

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
