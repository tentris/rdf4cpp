#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp/Dataset.hpp>
#include <rdf4cpp/IRI.hpp>
#include <rdf4cpp/BlankNode.hpp>
#include <rdf4cpp/Literal.hpp>
#include <rdf4cpp/query/Variable.hpp>
#include <rdf4cpp/namespaces/FOAF.hpp>
#include <rdf4cpp/datatypes/xsd.hpp>
#include <rdf4cpp/datatypes/owl.hpp>
#include <rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.hpp>

#include <unordered_set>
#include <vector>

TEST_SUITE("anonymizer") {
    using namespace rdf4cpp;
    using namespace rdf4cpp::shorthands;

    /**
     * Checks that no two elements of the given vector are equal.
     * Equality is the element's own operator==, and lookup goes through std::hash.
     */
    template<typename T>
    [[nodiscard]] bool all_distinct(std::vector<T> const &elements) {
        std::unordered_set<T> const seen{elements.begin(), elements.end()};
        return seen.size() == elements.size();
    }

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
        CHECK(all_distinct(std::vector<Node>{anon_null, anon_default_graph, anon_iri, anon_bn, anon_lit, iri, bn, lit}));

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

        CHECK(all_distinct(std::vector{c1, c2, n1, n2, m1, m2}));
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

        CHECK(all_distinct(std::vector{c1, c2, n1, n2, m1, m2}));
    }

    TEST_CASE("node kinds are preserved") {
        storage::reference_node_storage::UnsyncReferenceNodeStorage ns;
        util::Anonymizer anony{ns};

        auto const check = [&](Node const &orig) {
            auto const anon = anony.anonymize(orig);
            CHECK_NE(anon, orig);
            CHECK_EQ(anon.backend_handle().id().type(), orig.backend_handle().id().type());
            CHECK_EQ(anon.backend_handle().storage(), anony.node_storage());
            CHECK_EQ(anony.anonymize(orig), anon);
            return anon;
        };

        auto const iri = check(IRI::make("http://example.com#x", ns));
        auto const iri2 = check(IRI::make("http://example.com#y", ns));
        auto const bn = check(BlankNode::make("bn", ns));
        auto const bn2 = check(BlankNode::make("bn2", ns));
        auto const named_var = check(query::Variable::make_named("v", ns));
        auto const named_var2 = check(query::Variable::make_named("w", ns));
        auto const anon_var = check(query::Variable::make_anonymous("v", ns));
        auto const anon_var2 = check(query::Variable::make_anonymous("w", ns));
        auto const lit = check(Literal::make_simple("Hello World", ns));
        auto const lit2 = check(Literal::make_simple("Hello World 2", ns));

        CHECK_FALSE(named_var.as_variable().is_anonymous());
        CHECK(anon_var.as_variable().is_anonymous());
        CHECK(all_distinct(std::vector<Node>{iri, iri2, bn, bn2, named_var, named_var2, anon_var, anon_var2, lit, lit2}));
    }

    TEST_CASE("same term from different node storages") {
        storage::reference_node_storage::UnsyncReferenceNodeStorage ns;
        storage::reference_node_storage::UnsyncReferenceNodeStorage other_ns;
        util::Anonymizer anony{ns};

        // equal terms must map to the same anonymized node, regardless of their storage
        auto const check = [&](Node const &a, Node const &b) {
            CHECK_EQ(a, b);
            CHECK_NE(a.backend_handle().storage(), b.backend_handle().storage());
            auto const anon = anony.anonymize(a);
            CHECK_EQ(anony.anonymize(b), anon);
            CHECK_EQ(anon.backend_handle().storage(), anony.node_storage());
        };

        check(IRI::make("http://example.com#x", ns), IRI::make("http://example.com#x", other_ns));
        check(BlankNode::make("bn", ns), BlankNode::make("bn", other_ns));
        check(query::Variable::make_named("v", ns), query::Variable::make_named("v", other_ns));
        check(Literal::make_simple("Hello World", ns), Literal::make_simple("Hello World", other_ns));
        check(Literal::make_lang_tagged("Hello World", "en", ns), Literal::make_lang_tagged("Hello World", "en", other_ns));
    }

    TEST_CASE("datatype IRIs are kept") {
        storage::reference_node_storage::UnsyncReferenceNodeStorage ns;
        util::Anonymizer anony{ns};

        for (auto const &entry : datatypes::registry::DatatypeRegistry::registered_datatypes()) {
            if (!entry.datatype_iri.empty()) {
                IRI const iri = IRI::make(entry.datatype_iri, ns);
                CHECK_EQ(anony.anonymize(iri), iri);
            }
        }
    }

    TEST_CASE("literals") {
        using namespace datatypes;

        storage::reference_node_storage::UnsyncReferenceNodeStorage ns;
        util::Anonymizer anony{ns};
        std::unordered_set<std::string> covered_datatypes;

        // anonymized literal keeps datatype and language tag but gets a new lexical form
        auto const check_anon = [&](Literal const &orig) {
            covered_datatypes.emplace(orig.datatype().identifier());
            auto const anon = anony.anonymize(orig).as_literal();
            REQUIRE_FALSE(anon.null());
            CHECK(anon.datatype_eq(orig));
            CHECK_EQ(anon.language_tag(), orig.language_tag());
            CHECK_NE(anon.lexical_form().view(), orig.lexical_form().view());
            CHECK_EQ(anon.backend_handle().storage(), anony.node_storage());
            CHECK_EQ(anony.anonymize(orig), anon);
            return anon;
        };

        // literals without intrinsic meaning are kept as they are
        auto const check_kept = [&](Literal const &orig) {
            covered_datatypes.emplace(orig.datatype().identifier());
            CHECK_EQ(anony.anonymize(orig), orig);
            CHECK_EQ(anony.anonymize(orig), orig);
        };

        IRI const unknown_dt = IRI::make("http://example.com/dt", ns);

        CHECK(all_distinct(std::vector<Node>{
                check_anon(Literal::make_simple("Hello World", ns)),
                check_anon(Literal::make_lang_tagged("Hello World", "en", ns)),
                check_anon(Literal::make_typed<xsd::HexBinary>("0A0B", ns)),
                check_anon(Literal::make_typed<xsd::Base64Binary>("AAEC", ns)),
                check_anon(Literal::make_typed<xsd::AnyURI>("http://example.com/uri", ns)),
                check_anon(Literal::make_typed("Hello World", unknown_dt, ns)),
        }));

        check_kept(Literal::make_boolean(true, ns));
        check_kept(Literal::make_typed<xsd::Float>("1.5", ns));
        check_kept(Literal::make_typed<xsd::Double>("1.5", ns));
        check_kept(Literal::make_typed<xsd::Decimal>("1.5", ns));
        check_kept(Literal::make_typed<owl::Real>("1.5", ns));
        check_kept(Literal::make_typed<owl::Rational>("1/2", ns));
        check_kept(Literal::make_typed<xsd::Integer>("42", ns));
        check_kept(Literal::make_typed<xsd::Long>("42", ns));
        check_kept(Literal::make_typed<xsd::Int>("42", ns));
        check_kept(Literal::make_typed<xsd::Short>("42", ns));
        check_kept(Literal::make_typed<xsd::Byte>("42", ns));
        check_kept(Literal::make_typed<xsd::NonNegativeInteger>("42", ns));
        check_kept(Literal::make_typed<xsd::PositiveInteger>("42", ns));
        check_kept(Literal::make_typed<xsd::UnsignedLong>("42", ns));
        check_kept(Literal::make_typed<xsd::UnsignedInt>("42", ns));
        check_kept(Literal::make_typed<xsd::UnsignedShort>("42", ns));
        check_kept(Literal::make_typed<xsd::UnsignedByte>("42", ns));
        check_kept(Literal::make_typed<xsd::NonPositiveInteger>("-42", ns));
        check_kept(Literal::make_typed<xsd::NegativeInteger>("-42", ns));
        check_kept(Literal::make_typed<xsd::GYear>("2020", ns));
        check_kept(Literal::make_typed<xsd::GMonth>("--05", ns));
        check_kept(Literal::make_typed<xsd::GDay>("---15", ns));
        check_kept(Literal::make_typed<xsd::GYearMonth>("2020-05", ns));
        check_kept(Literal::make_typed<xsd::GMonthDay>("--05-15", ns));
        check_kept(Literal::make_typed<xsd::Date>("2020-05-15", ns));
        check_kept(Literal::make_typed<xsd::Time>("12:00:00", ns));
        check_kept(Literal::make_typed<xsd::DateTime>("2020-05-15T12:00:00", ns));
        check_kept(Literal::make_typed<xsd::DateTimeStamp>("2020-05-15T12:00:00Z", ns));
        check_kept(Literal::make_typed<xsd::Duration>("P1Y2M3DT4H", ns));
        check_kept(Literal::make_typed<xsd::DayTimeDuration>("P1DT2H", ns));
        check_kept(Literal::make_typed<xsd::YearMonthDuration>("P1Y2M", ns));

        // every registered datatype must be covered above
        for (auto const &entry : registry::DatatypeRegistry::registered_datatypes()) {
            if (!entry.datatype_iri.empty()) {
                CHECK_MESSAGE(covered_datatypes.contains(entry.datatype_iri), entry.datatype_iri);
            }
        }
    }
}
