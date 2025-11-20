#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp/Dataset.hpp>
#include <rdf4cpp/IRI.hpp>
#include <rdf4cpp/BlankNode.hpp>
#include <rdf4cpp/Literal.hpp>


TEST_SUITE("anonymizer") {
    using namespace rdf4cpp;
    using namespace rdf4cpp::shorthands;

    TEST_CASE("graph") {
        std::string const data{R"(
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

_:customer1 a foaf:Person ;
    foaf:name "Mustermann" ;
    foaf:mbox <mailto:mustermann@example.com> .

_:customer2 a foaf:Person ;
    foaf:name "Max" ;
    foaf:mbox <mailto:max@example.com> .
)"};

        std::istringstream iss{data};

        Graph g;
        g.load_rdf_data(iss);

        util::Anonymizer anony{storage::default_node_storage, 42};
        auto anon =  g.anonymize(anony);

        std::vector<Statement> stmts;
        std::ranges::copy(anon, std::back_inserter(stmts));
        std::ranges::sort(stmts);

        std::vector<Statement> const expected{
            Statement{"http://example.org/IvwedQyxZEGDSyQz"_iri, "http://example.org/MlBgOmeUyfKgLFsN"_iri, "http://example.org/QweTPUSbMdkGxrsK"_iri},
            Statement{"http://example.org/IvwedQyxZEGDSyQz"_iri, "http://example.org/NxcdMhwfheBxOgMq"_iri, "http://example.org/NPcWMvpbKxBzlFzt"_iri},
            Statement{"http://example.org/IvwedQyxZEGDSyQz"_iri, "http://example.org/xqpBsNpJLHQOHPuW"_iri, "http://example.org/spMEshGEBFdtMFuW"_iri},

            Statement{"http://example.org/NHNhUeDtouaBJHQX"_iri, "http://example.org/MlBgOmeUyfKgLFsN"_iri, "http://example.org/WZutRNITdqpviSzp"_iri},
            Statement{"http://example.org/NHNhUeDtouaBJHQX"_iri, "http://example.org/NxcdMhwfheBxOgMq"_iri, "http://example.org/NPcWMvpbKxBzlFzt"_iri},
            Statement{"http://example.org/NHNhUeDtouaBJHQX"_iri, "http://example.org/xqpBsNpJLHQOHPuW"_iri, "http://example.org/rMiHeusAILABuPjs"_iri},
        };

        CHECK_EQ(stmts, expected);
    }


    TEST_CASE("dataset") {
        std::string const data{R"(
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

_:customer1 a foaf:Person ;
    foaf:name "Mustermann" ;
    foaf:mbox <mailto:mustermann@example.com> .

_:G {
_:customer2 a foaf:Person ;
    foaf:name "Max" ;
    foaf:mbox <mailto:max@example.com> .
}
)"};

        std::istringstream iss{data};

        Dataset ds;
        ds.load_rdf_data(iss);

        util::Anonymizer anony{storage::default_node_storage, 42};
        auto anon = ds.anonymize(anony);

        std::vector<Quad> quads;
        std::ranges::copy(anon, std::back_inserter(quads));
        std::ranges::sort(quads);

        std::vector<Quad> const expected{
            Quad{"http://example.org/QweTPUSbMdkGxrsK"_iri, "http://example.org/MlBgOmeUyfKgLFsN"_iri, "http://example.org/CemwylFZUtzpxqpx"_iri},
            Quad{"http://example.org/QweTPUSbMdkGxrsK"_iri, "http://example.org/NPcWMvpbKxBzlFzt"_iri, "http://example.org/rMiHeusAILABuPjs"_iri},
            Quad{"http://example.org/QweTPUSbMdkGxrsK"_iri, "http://example.org/xqpBsNpJLHQOHPuW"_iri, "http://example.org/spMEshGEBFdtMFuW"_iri},

            Quad{"http://example.org/NHNhUeDtouaBJHQX"_iri, "http://example.org/NxcdMhwfheBxOgMq"_iri, "http://example.org/MlBgOmeUyfKgLFsN"_iri, "http://example.org/WZutRNITdqpviSzp"_iri},
            Quad{"http://example.org/NHNhUeDtouaBJHQX"_iri, "http://example.org/NxcdMhwfheBxOgMq"_iri, "http://example.org/NPcWMvpbKxBzlFzt"_iri, "http://example.org/IvwedQyxZEGDSyQz"_iri},
            Quad{"http://example.org/NHNhUeDtouaBJHQX"_iri, "http://example.org/NxcdMhwfheBxOgMq"_iri, "http://example.org/xqpBsNpJLHQOHPuW"_iri, "http://example.org/spMEshGEBFdtMFuW"_iri},
        };

        CHECK_EQ(quads, expected);
    }
}
