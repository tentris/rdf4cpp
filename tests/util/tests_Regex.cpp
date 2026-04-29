#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp::regex;

TEST_SUITE("regex") {
    void syntax_extra_flag(RegexFlags f) {
        CHECK(Regex{"abc", f}.regex_match("abc"));
        CHECK(!Regex{"abc", f}.regex_match("_abc"));
        CHECK(!Regex{"abc", f}.regex_match("abc_"));

        CHECK(Regex{"a|b", f}.regex_match("a"));
        CHECK(Regex{"a|b", f}.regex_match("b"));
        CHECK(!Regex{"a|b", f}.regex_match("c"));

        CHECK(Regex{"a?", f}.regex_match("a"));
        CHECK(Regex{"a?", f}.regex_match(""));
        CHECK(!Regex{"a?", f}.regex_match("aa"));

        CHECK(Regex{"a*", f}.regex_match("a"));
        CHECK(Regex{"a*", f}.regex_match("aaaa"));
        CHECK(Regex{"a*", f}.regex_match(""));
        CHECK(!Regex{"a*", f}.regex_match("b"));

        CHECK(!Regex{"a+", f}.regex_match(""));
        CHECK(Regex{"a+", f}.regex_match("a"));
        CHECK(Regex{"a+", f}.regex_match("aaaaa"));
        CHECK(!Regex{"a+", f}.regex_match("b"));

        CHECK(!Regex{"a{2,5}", f}.regex_match("a"));
        CHECK(Regex{"a{2,5}", f}.regex_match("aa"));
        CHECK(Regex{"a{2,5}", f}.regex_match("aaaaa"));
        CHECK(!Regex{"a{2,5}", f}.regex_match("aaaaaa"));
        CHECK(!Regex{"a{2,5}", f}.regex_match("bbb"));

        CHECK(Regex{"a{2}", f}.regex_match("aa"));
        CHECK(!Regex{"a{2}", f}.regex_match("aaa"));
        CHECK(!Regex{"a{2}", f}.regex_match("aaaa"));
        CHECK(!Regex{"a{2}", f}.regex_match("b"));

        CHECK(Regex{"a{2,}", f}.regex_match("aa"));
        CHECK(Regex{"a{2,}", f}.regex_match("aaa"));
        CHECK(!Regex{"a{2,}", f}.regex_match("a"));
        CHECK(!Regex{"a{2,}", f}.regex_match("b"));

        CHECK(Regex{"a{0,2}", f}.regex_match("aa"));
        CHECK(!Regex{"a{0,2}", f}.regex_match("aaa"));
        CHECK(Regex{"a{0,2}", f}.regex_match("a"));
        CHECK(!Regex{"a{0,2}", f}.regex_match("b"));

        CHECK(Regex{"a{0,0}", f}.regex_match(""));
        CHECK(!Regex{"a{0,0}", f}.regex_match("a"));

        CHECK(Regex{"\\.\\\\\\?\\*\\+\\{\\}\\(\\)\\[\\]\\|\\-\\^\\n\\r\\t", f}.regex_match(".\\?*+{}()[]|-^\n\r\t"));
        CHECK(!Regex{"\\.", f}.regex_match("a"));
        CHECK(!Regex{"\\\\", f}.regex_match("a"));
        CHECK(!Regex{"\\?", f}.regex_match("a"));
        CHECK(!Regex{"\\*", f}.regex_match("a"));
        CHECK(!Regex{"\\+", f}.regex_match("a"));
        CHECK(!Regex{"\\{", f}.regex_match("a"));
        CHECK(!Regex{"\\}", f}.regex_match("a"));
        CHECK(!Regex{"\\(", f}.regex_match("a"));
        CHECK(!Regex{"\\[", f}.regex_match("a"));
        CHECK(!Regex{"\\]", f}.regex_match("a"));
        CHECK(!Regex{"\\|", f}.regex_match("a"));
        CHECK(!Regex{"\\-", f}.regex_match("a"));
        CHECK(!Regex{"\\^", f}.regex_match("a"));
        CHECK(!Regex{"\\n", f}.regex_match("a"));
        CHECK(!Regex{"\\r", f}.regex_match("a"));
        CHECK(!Regex{"\\t", f}.regex_match("a"));

        CHECK(Regex{"(abc)", f}.regex_match("abc"));
        CHECK(!Regex{"(abc)", f}.regex_match("adc"));


        CHECK(Regex{"[abc]", f}.regex_match("a"));
        CHECK(Regex{"[abc]", f}.regex_match("b"));
        CHECK(Regex{"[abc]", f}.regex_match("c"));
        CHECK(!Regex{"[abc]", f}.regex_match("d"));
        CHECK(!Regex{"[^abc]", f}.regex_match("a"));
        CHECK(Regex{"[^abc]", f}.regex_match("d"));
        CHECK(Regex{"[a-c]", f}.regex_match("a"));
        CHECK(Regex{"[a-c]", f}.regex_match("b"));
        CHECK(Regex{"[a-c]", f}.regex_match("c"));
        CHECK(!Regex{"[a-c]", f}.regex_match("d"));
        CHECK(!Regex{"[^a-c]", f}.regex_match("a"));
        CHECK(Regex{"[^a-c]", f}.regex_match("d"));
        // CHECK(!Regex{"[^abc-[e]]", f}.regex_match("e"));
        // CHECK(Regex{"[^abc-[e]]", f}.regex_match("d"));


        CHECK(Regex{"\\p{Lu}", f}.regex_match("A"));
        CHECK(Regex{"\\p{Lu}", f}.regex_match("Ä"));
        CHECK(!Regex{"\\p{Lu}", f}.regex_match("a"));
        CHECK(!Regex{"\\p{Lu}", f}.regex_match("ä"));
        CHECK(!Regex{"\\p{Lu}", f}.regex_match("5"));

        // CHECK(Regex{"\\p{IsBasicLatin}", f}.regex_match("5"));
        // CHECK(!Regex{"\\p{IsBasicLatin}", f}.regex_match("ü"));

        CHECK(Regex{".", f}.regex_match("5"));
        CHECK(!Regex{".", f}.regex_match("\n"));
        CHECK(Regex{"\\s", f}.regex_match(" "));
        CHECK(!Regex{"\\s", f}.regex_match("a"));
        CHECK(Regex{"\\S", f}.regex_match("a"));
        // CHECK(Regex{"\\c", f}.regex_match("a"));
        // CHECK(!Regex{"\\c", f}.regex_match("<"));
        // CHECK(Regex{"\\C", f}.regex_match("<"));
        CHECK(Regex{"\\d", f}.regex_match("5"));
        CHECK(!Regex{"\\d", f}.regex_match("a"));
        CHECK(Regex{"\\D", f}.regex_match("a"));
        CHECK(Regex{"\\w\\w", f}.regex_match("a5"));
        CHECK(!Regex{"\\w", f}.regex_match("-"));
        CHECK(Regex{"\\W", f}.regex_match("-"));

        CHECK(Regex{"a", f}.regex_search("_a_"));
        CHECK(!Regex{"^a", f}.regex_search("_a"));
        CHECK(!Regex{"a$", f}.regex_search("a_"));


        CHECK(Regex{"a??", f}.regex_match("a"));
        CHECK(Regex{"a??", f}.regex_match(""));
        CHECK(!Regex{"a??", f}.regex_match("aa"));

        CHECK(Regex{"a*?", f}.regex_match("a"));
        CHECK(Regex{"a*?", f}.regex_match("aaaa"));
        CHECK(Regex{"a*?", f}.regex_match(""));
        CHECK(!Regex{"a*?", f}.regex_match("b"));

        CHECK(!Regex{"a+?", f}.regex_match(""));
        CHECK(Regex{"a+?", f}.regex_match("a"));
        CHECK(Regex{"a+?", f}.regex_match("aaaaa"));
        CHECK(!Regex{"a+?", f}.regex_match("b"));

        CHECK(!Regex{"a{2,5}?", f}.regex_match("a"));
        CHECK(Regex{"a{2,5}?", f}.regex_match("aa"));
        CHECK(Regex{"a{2,5}?", f}.regex_match("aaaaa"));
        CHECK(!Regex{"a{2,5}?", f}.regex_match("aaaaaa"));
        CHECK(!Regex{"a{2,5}?", f}.regex_match("bbb"));

        CHECK(Regex{"a{2}?", f}.regex_match("aa"));
        CHECK(!Regex{"a{2}?", f}.regex_match("aaa"));
        CHECK(!Regex{"a{2}?", f}.regex_match("aaaa"));
        CHECK(!Regex{"a{2}ß", f}.regex_match("b"));

        CHECK(Regex{"a{2,}?", f}.regex_match("aa"));
        CHECK(Regex{"a{2,}?", f}.regex_match("aaa"));
        CHECK(!Regex{"a{2,}?", f}.regex_match("a"));
        CHECK(!Regex{"a{2,}?", f}.regex_match("b"));

        CHECK(Regex{"a{0,2}?", f}.regex_match("aa"));
        CHECK(!Regex{"a{0,2}?", f}.regex_match("aaa"));
        CHECK(Regex{"a{0,2}?", f}.regex_match("a"));
        CHECK(!Regex{"a{0,2}?", f}.regex_match("b"));

        CHECK(Regex{"a{0,0}?", f}.regex_match(""));
        CHECK(!Regex{"a{0,0}?", f}.regex_match("a"));


        CHECK(Regex{"(abc)", f}.regex_match("abc"));
        CHECK(Regex{"(?:abc)", f}.regex_match("abc"));
        CHECK(Regex{"(.)bc\\1", f}.regex_match("abca"));
        CHECK(Regex{"(.)bc\\1", f}.regex_match("xbcx"));
        CHECK(!Regex{"(.)bc\\1", f}.regex_match("abcx"));

        CHECK(Regex{".", f | RegexFlag::DotAll}.regex_match("\n"));
        CHECK(Regex{"^a$", f | RegexFlag::Multiline}.regex_search("a\nxyz"));
        CHECK(Regex{"abc", f | RegexFlag::CaseInsensitive}.regex_match("ABC"));
        CHECK(Regex{"a b c", f | RegexFlag::RemoveWhitespace}.regex_match("abc"));
        CHECK(Regex{"a\\ sb", f | RegexFlag::RemoveWhitespace}.regex_match("a b"));
        CHECK(Regex{"a\\ [b", f | RegexFlag::RemoveWhitespace}.regex_match("a[b"));
        CHECK(Regex{".+", f | RegexFlag::Literal}.regex_match(".+"));
        CHECK(!Regex{".", f | RegexFlag::Literal}.regex_match("a"));

        std::string d = "a_a_x";
        Regex{"a", f}.make_replacer("b").regex_replace(d);
        CHECK(d == "b_b_x");
        Regex{"_(\\w)", f}.make_replacer(R"(-$0$1\$\\)").regex_replace(d);
        CHECK(d == "b-_bb$\\-_xx$\\");
    }
    TEST_CASE("basic syntax") {
        SUBCASE("normal") {
            syntax_extra_flag(RegexFlags::none());
        }
        SUBCASE("optimize") {
            syntax_extra_flag(RegexFlag::Optimize);
        }
    }

    TEST_CASE("replacement translation") {
        Regex const r{"[0-9]"};

        SUBCASE("no escape") {
            {
                std::string s = "";
                CHECK_THROWS(r.make_replacer("$").regex_replace(s));
            }

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
