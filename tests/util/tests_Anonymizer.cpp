#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp/Dataset.hpp>
#include <rdf4cpp/IRI.hpp>
#include <rdf4cpp/BlankNode.hpp>
#include <rdf4cpp/Literal.hpp>
#include <rdf4cpp/query/Variable.hpp>
#include <rdf4cpp/namespaces/FOAF.hpp>

#include <dice/template-library/ranges.hpp>

TEST_SUITE("anonymizer") {
    using namespace rdf4cpp;
    using namespace rdf4cpp::shorthands;

    TEST_CASE("sanity check") {
        util::Anonymizer anony{};

        Node const null{};
        IRI const default_graph = IRI::default_graph();
        IRI const iri{"http://example.com#x"};
        BlankNode const bn{"bn"};
        Literal const lit = Literal::make_simple("Hello World");

        auto const anon_null = anony.anonymize(null);
        auto const anon_default_graph = anony.anonymize(default_graph);
        auto const anon_iri = anony.anonymize(iri);
        auto const anon_bn = anony.anonymize(bn);
        auto const anon_lit = anony.anonymize(lit);

        // anonymized not same as original
        CHECK_EQ(anon_null, null);
        CHECK_EQ(anon_default_graph, default_graph);
        CHECK_NE(anon_iri, iri);
        CHECK_NE(anon_bn, bn);
        CHECK_NE(anon_lit, lit);

        // generates distinct nodes for all cases
        CHECK((std::vector<Node>{anon_null, anon_default_graph, anon_iri, anon_bn, anon_lit, iri, bn, lit} | dice::template_library::all_distinct()));

        // same result for second call
        CHECK_EQ(anony.anonymize(iri), anon_iri);
        CHECK_EQ(anony.anonymize(bn), anon_bn);
        CHECK_EQ(anony.anonymize(lit), anon_lit);
    }

    template<typename DS, typename Pat>
    [[nodiscard]] query::Solution first_solution(DS const &ds, Pat const &pat) {
        auto const solutions = ds.match(pat);
        CHECK_EQ(std::ranges::distance(solutions), 1);

        return *solutions.begin();
    }

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

        util::Anonymizer anony{};
        auto anon =  g.anonymize(anony);
        CHECK_EQ(anon.size(), g.size());

        auto const rdf_type = anony.anonymize(IRI::rdf_type());

        auto foaf = namespaces::FOAF{};
        auto const foaf_person = anony.anonymize(foaf + "Person");
        auto const foaf_name = anony.anonymize(foaf + "name");
        auto const foaf_mbox = anony.anonymize(foaf + "mbox");

        auto customers = anon.match({query::Variable{"c"}, rdf_type, foaf_person});
        CHECK_EQ(std::ranges::distance(customers), 2);

        auto const c1 = (*customers.begin())[0];
        auto const c2 = (*std::next(customers.begin()))[0];
        auto const n1 = first_solution(anon, query::TriplePattern{c1, foaf_name, query::Variable{"n"}})[0];
        auto const n2 = first_solution(anon, query::TriplePattern{c2, foaf_name, query::Variable{"n"}})[0];
        auto const m1 = first_solution(anon, query::TriplePattern{c1, foaf_mbox, query::Variable{"n"}})[0];
        auto const m2 = first_solution(anon, query::TriplePattern{c2, foaf_mbox, query::Variable{"n"}})[0];

        CHECK((std::vector{c1, c2, n1, n2, m1, m2} | dice::template_library::all_distinct()));
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

        util::Anonymizer anony{storage::default_node_storage};
        auto anon = ds.anonymize(anony);
        CHECK_EQ(anon.size(), ds.size());

        auto const default_graph = anony.anonymize(IRI::default_graph());
        auto const named_graph = anony.anonymize(BlankNode{"G"});
        auto const rdf_type = anony.anonymize(IRI::rdf_type());

        auto foaf = namespaces::FOAF{};
        auto const foaf_person = anony.anonymize(foaf + "Person");
        auto const foaf_name = anony.anonymize(foaf + "name");
        auto const foaf_mbox = anony.anonymize(foaf + "mbox");

        auto const c1 = first_solution(anon, query::QuadPattern{default_graph, query::Variable{"c"}, rdf_type, foaf_person})[0];
        auto const c2 = first_solution(anon, query::QuadPattern{named_graph, query::Variable{"c"}, rdf_type, foaf_person})[0];
        auto const n1 = first_solution(anon, query::QuadPattern{query::Variable{"g"}, c1, foaf_name, query::Variable{"n"}})[1];
        auto const n2 = first_solution(anon, query::QuadPattern{query::Variable{"g"}, c2, foaf_name, query::Variable{"n"}})[1];
        auto const m1 = first_solution(anon, query::QuadPattern{query::Variable{"g"}, c1, foaf_mbox, query::Variable{"n"}})[1];
        auto const m2 = first_solution(anon, query::QuadPattern{query::Variable{"g"}, c2, foaf_mbox, query::Variable{"n"}})[1];

        CHECK((std::vector{c1, c2, n1, n2, m1, m2} | dice::template_library::all_distinct()));
    }
}
