#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp::regex;

namespace rdf4cpp {
    std::ostream &operator<<(std::ostream &os, TriBool value) {
        if (value == TriBool::Err) {
            os << "Err";
        }
        else if (value == TriBool::True) {
            os << "true";
        }
        else {
            os << "false";
        }
        return os;
    }
}

TEST_SUITE("regex") {
    void syntax_extra_flag(RegexFlags f) {
        CHECK(Regex{"abc", f}.regex_match("abc") == rdf4cpp::TriBool::True);
        CHECK(Regex{"abc", f}.regex_match("_abc") == rdf4cpp::TriBool::False);
        CHECK(Regex{"abc", f}.regex_match("abc_") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a|b", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a|b", f}.regex_match("b") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a|b", f}.regex_match("c") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a?", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a?", f}.regex_match("") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a?", f}.regex_match("aa") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a*", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a*", f}.regex_match("aaaa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a*", f}.regex_match("") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a*", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a+", f}.regex_match("") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a+", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a+", f}.regex_match("aaaaa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a+", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{2,5}", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2,5}", f}.regex_match("aa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2,5}", f}.regex_match("aaaaa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2,5}", f}.regex_match("aaaaaa") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2,5}", f}.regex_match("bbb") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{2}", f}.regex_match("aa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2}", f}.regex_match("aaa") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2}", f}.regex_match("aaaa") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2}", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{2,}", f}.regex_match("aa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2,}", f}.regex_match("aaa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2,}", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2,}", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{0,2}", f}.regex_match("aa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{0,2}", f}.regex_match("aaa") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{0,2}", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{0,2}", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{0,0}", f}.regex_match("") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{0,0}", f}.regex_match("a") == rdf4cpp::TriBool::False);

        CHECK(Regex{"\\.\\\\\\?\\*\\+\\{\\}\\(\\)\\[\\]\\|\\-\\^\\n\\r\\t", f}.regex_match(".\\?*+{}()[]|-^\n\r\t") == rdf4cpp::TriBool::True);
        CHECK(Regex{"\\.", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\\\", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\?", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\*", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\+", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\{", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\}", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\(", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\[", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\]", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\|", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\-", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\^", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\n", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\r", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\t", f}.regex_match("a") == rdf4cpp::TriBool::False);

        CHECK(Regex{"(abc)", f}.regex_match("abc") == rdf4cpp::TriBool::True);
        CHECK(Regex{"(abc)", f}.regex_match("adc") == rdf4cpp::TriBool::False);


        CHECK(Regex{"[abc]", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"[abc]", f}.regex_match("b") == rdf4cpp::TriBool::True);
        CHECK(Regex{"[abc]", f}.regex_match("c") == rdf4cpp::TriBool::True);
        CHECK(Regex{"[abc]", f}.regex_match("d") == rdf4cpp::TriBool::False);
        CHECK(Regex{"[^abc]", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"[^abc]", f}.regex_match("d") == rdf4cpp::TriBool::True);
        CHECK(Regex{"[a-c]", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"[a-c]", f}.regex_match("b") == rdf4cpp::TriBool::True);
        CHECK(Regex{"[a-c]", f}.regex_match("c") == rdf4cpp::TriBool::True);
        CHECK(Regex{"[a-c]", f}.regex_match("d") == rdf4cpp::TriBool::False);
        CHECK(Regex{"[^a-c]", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"[^a-c]", f}.regex_match("d") == rdf4cpp::TriBool::True);
        // CHECK(!Regex{"[^abc-[e]]", f}.regex_match("e"));
        // CHECK(Regex{"[^abc-[e]]", f}.regex_match("d"));


        CHECK(Regex{"\\p{Lu}", f}.regex_match("A") == rdf4cpp::TriBool::True);
        CHECK(Regex{"\\p{Lu}", f}.regex_match("Ä") == rdf4cpp::TriBool::True);
        CHECK(Regex{"\\p{Lu}", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\p{Lu}", f}.regex_match("ä") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\p{Lu}", f}.regex_match("5") == rdf4cpp::TriBool::False);

        // CHECK(Regex{"\\p{IsBasicLatin}", f}.regex_match("5"));
        // CHECK(!Regex{"\\p{IsBasicLatin}", f}.regex_match("ü"));

        CHECK(Regex{".", f}.regex_match("5") == rdf4cpp::TriBool::True);
        CHECK(Regex{".", f}.regex_match("\n") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\s", f}.regex_match(" ") == rdf4cpp::TriBool::True);
        CHECK(Regex{"\\s", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\S", f}.regex_match("a") == rdf4cpp::TriBool::True);
        // CHECK(Regex{"\\c", f}.regex_match("a"));
        // CHECK(!Regex{"\\c", f}.regex_match("<"));
        // CHECK(Regex{"\\C", f}.regex_match("<"));
        CHECK(Regex{"\\d", f}.regex_match("5") == rdf4cpp::TriBool::True);
        CHECK(Regex{"\\d", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\D", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"\\w\\w", f}.regex_match("a5") == rdf4cpp::TriBool::True);
        CHECK(Regex{"\\w", f}.regex_match("-") == rdf4cpp::TriBool::False);
        CHECK(Regex{"\\W", f}.regex_match("-") == rdf4cpp::TriBool::True);

        CHECK(Regex{"a", f}.regex_search("_a_") == rdf4cpp::TriBool::True);
        CHECK(Regex{"^a", f}.regex_search("_a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a$", f}.regex_search("a_") == rdf4cpp::TriBool::False);


        CHECK(Regex{"a??", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a??", f}.regex_match("") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a??", f}.regex_match("aa") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a*?", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a*?", f}.regex_match("aaaa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a*?", f}.regex_match("") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a*?", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a+?", f}.regex_match("") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a+?", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a+?", f}.regex_match("aaaaa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a+?", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{2,5}?", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2,5}?", f}.regex_match("aa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2,5}?", f}.regex_match("aaaaa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2,5}?", f}.regex_match("aaaaaa") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2,5}?", f}.regex_match("bbb") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{2}?", f}.regex_match("aa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2}?", f}.regex_match("aaa") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2}?", f}.regex_match("aaaa") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2}?", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{2,}?", f}.regex_match("aa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2,}?", f}.regex_match("aaa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{2,}?", f}.regex_match("a") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{2,}?", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{0,2}?", f}.regex_match("aa") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{0,2}?", f}.regex_match("aaa") == rdf4cpp::TriBool::False);
        CHECK(Regex{"a{0,2}?", f}.regex_match("a") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{0,2}?", f}.regex_match("b") == rdf4cpp::TriBool::False);

        CHECK(Regex{"a{0,0}?", f}.regex_match("") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a{0,0}?", f}.regex_match("a") == rdf4cpp::TriBool::False);


        CHECK(Regex{"(abc)", f}.regex_match("abc") == rdf4cpp::TriBool::True);
        CHECK(Regex{"(?:abc)", f}.regex_match("abc") == rdf4cpp::TriBool::True);
        CHECK(Regex{"(.)bc\\1", f}.regex_match("abca") == rdf4cpp::TriBool::True);
        CHECK(Regex{"(.)bc\\1", f}.regex_match("xbcx") == rdf4cpp::TriBool::True);
        CHECK(Regex{"(.)bc\\1", f}.regex_match("abcx") == rdf4cpp::TriBool::False);

        CHECK(Regex{".", f | RegexFlag::DotAll}.regex_match("\n") == rdf4cpp::TriBool::True);
        CHECK(Regex{"^a$", f | RegexFlag::Multiline}.regex_search("a\nxyz") == rdf4cpp::TriBool::True);
        CHECK(Regex{"abc", f | RegexFlag::CaseInsensitive}.regex_match("ABC") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a b c", f | RegexFlag::RemoveWhitespace}.regex_match("abc") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a\\ sb", f | RegexFlag::RemoveWhitespace}.regex_match("a b") == rdf4cpp::TriBool::True);
        CHECK(Regex{"a\\ [b", f | RegexFlag::RemoveWhitespace}.regex_match("a[b") == rdf4cpp::TriBool::True);
        CHECK(Regex{".+", f | RegexFlag::Literal}.regex_match(".+") == rdf4cpp::TriBool::True);
        CHECK(Regex{".", f | RegexFlag::Literal}.regex_match("a") == rdf4cpp::TriBool::False);

        // infinite recursion
        CHECK(Regex{"a?(?R)?z", f}.regex_search("azz") == rdf4cpp::TriBool::Err);

        std::string d = "a_a_x";
        Regex{"a", f}.make_replacer("b").regex_replace(d);
        CHECK(d == "b_b_x");
        Regex{"_(\\w)", f}.make_replacer(R"(-$0$1\$\\)").regex_replace(d);
        CHECK(d == "b-_bb$\\-_xx$\\");

        CHECK_THROWS_WITH_AS(static_cast<void>(Regex{"a", f}.make_replacer("\\")), "incomplete escape sequence in replacement string", RegexError);
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
