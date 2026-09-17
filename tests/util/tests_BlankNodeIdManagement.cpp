#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

#include <tuple>

using namespace rdf4cpp;
using namespace rdf4cpp::bnode_mngt;

TEST_SUITE("blank node id management") {
    TEST_CASE("blank node id generation") {
        SUBCASE("from entropy") {
            RandomIdGenerator gen;

            auto const id1 = gen.generate();
            auto const id2 = gen.generate();
            auto const id3 = gen.generate();

            CHECK(id1 != id2);
            CHECK(id2 != id3);
            CHECK(id1 != id3);

            RandomIdGenerator gen2;

            auto const id4 = gen2.generate();
            auto const id5 = gen2.generate();
            auto const id6 = gen2.generate();

            CHECK(id4 != id5);
            CHECK(id5 != id6);
            CHECK(id4 != id6);

            CHECK(std::tie(id1, id2, id3) != std::tie(id4, id5, id6));
        }

        SUBCASE("from seed") {
            auto const seed = 42;
            RandomIdGenerator gen{seed};
            auto const id1 = gen.generate();
            auto const id2 = gen.generate();
            auto const id3 = gen.generate();

            CHECK(id1 != id2);
            CHECK(id2 != id3);
            CHECK(id1 != id3);

            RandomIdGenerator gen2{seed};
            auto const id4 = gen2.generate();
            auto const id5 = gen2.generate();
            auto const id6 = gen2.generate();

            CHECK(id4 != id5);
            CHECK(id5 != id6);
            CHECK(id4 != id6);

            CHECK(std::tie(id1, id2, id3) == std::tie(id4, id5, id6));
        }
    }

    TEST_CASE("blank node id manager") {

        {
            ReferenceNodeScope<> scope;

            auto b1 = scope.get_or_generate_node("abc");
            auto b2 = scope.get_or_generate_node("bcd");
            auto b3 = scope.get_or_generate_node("abc");

            CHECK(b1 != b2);
            CHECK(b1 == b3);

            CHECK(scope.try_get_node("hello").null());

            auto fresh = scope.generate_node();
            CHECK(fresh != b1);
            CHECK(fresh != b2);

            scope.clear();
            CHECK(scope.try_get_node("abc").null());
            CHECK(scope.try_get_node("bcd").null());
            CHECK_FALSE(scope.try_get_label(b1).has_value());
            CHECK_FALSE(scope.try_get_label(b2).has_value());
        }

        ReferenceNodeScope<> mng2;
        CHECK(mng2.try_get_node("abc").null());
    }

    TEST_CASE("file parsing") {
        std::istringstream rdf_file{
            "<http://website.com#subj> <http://website.com#pred> _:bn_1 <http://website.com#graph1> ."
            "<http://website.com#subj> <http://website.com#pred> _:bn_1 <http://website.com#graph2> ."};

        query::TriplePattern const pat{IRI{"http://website.com#subj"}, IRI{"http://website.com#pred"}, query::Variable{"o"}};

        SUBCASE("rdf-merge") {
            // rdf-merge semantics
            MergeNodeScopeManager<> mng;
            parser::ParsingState st{.blank_node_scope_manager = mng};

            Graph merge_graph;
            merge_graph.load_rdf_data(rdf_file, parser::ParsingFlag::NQuads, &st);

            std::vector<query::Solution> solutions;
            for (auto const &sol : merge_graph.match(pat)) {
                solutions.push_back(sol);
            }

            CHECK_EQ(solutions.size(), 2);
            auto const bn1 = solutions[0][0];
            auto const bn2 = solutions[1][0];

            CHECK_NE(bn1, bn2);
            CHECK_NE(bn1.as_blank_node().identifier(), "bn_1");
            CHECK_NE(bn2.as_blank_node().identifier(), "bn_1");
        }

        SUBCASE("rdf-union") {
            // rdf-union semantics
            UnionNodeScopeManager<> mng;
            parser::ParsingState st{.blank_node_scope_manager = mng};

            Graph union_graph;
            union_graph.load_rdf_data(rdf_file, parser::ParsingFlag::NQuads, &st);

            std::vector<query::Solution> solutions;
            for (auto const &sol : union_graph.match(pat)) {
                solutions.push_back(sol);
            }

            CHECK_EQ(solutions.size(), 1);
            auto const bn1 = solutions[0][0];
            CHECK_NE(bn1.as_blank_node().identifier(), "bn_1");
        }
    }

    TEST_CASE("simulate resumable file parsing") {
        std::string const file_name = "/path/to/my/file.nt";
        std::string const file_content = "<http://example.org#s1> <http://example.org#p1> _:bn_1 <http://example.com#g1> ."
            "<http://example.org#s1> <http://example.org#p1> _:bn_2 <http://example.com#g1> ."
            "<http://example.org#s1> <http://example.org#p2> _:bn_2 <http://example.com#g2> ."
            "<http://example.org#s1> <http://example.org#p2> _:bn_1 <http://example.com#g2> .";

        auto const parse = [&] {
            MergeNodeScopeManager<> mng{
                .scopes = {},
                .make_scope = [file_name](std::string_view graph_name) {
                    auto const seed = dice::hash::dice_hash_templates<dice::hash::Policies::wyhash>::dice_hash(std::tie(file_name, graph_name));
                    return std::make_unique<ReferenceNodeScope<RandomIdGenerator>>(seed);
                }
            };
            parser::ParsingState st{.blank_node_scope_manager = mng};
            std::istringstream stream{file_content};

            std::vector<BlankNode> bns;

            for (parser::IStreamQuadIterator qit{stream, parser::ParsingFlag::NQuads, &st}; qit != std::default_sentinel; ++qit) {
                CHECK(qit->has_value());
                bns.push_back(qit->value().object().as_blank_node());
            }

            return bns;
        };

        auto const bns1 = parse();
        CHECK_EQ(std::set{std::from_range, bns1}.size(), bns1.size()); // all distinct

        auto const bns2 = parse();
        CHECK_EQ(std::set{std::from_range, bns2}.size(), bns2.size());

        CHECK_EQ(bns1, bns2);
    }
}
