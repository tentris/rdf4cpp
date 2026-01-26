#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

#include <iostream>
#include <filesystem>
#include <curl/curl.h>

using namespace rdf4cpp;
using namespace rdf4cpp::parser;

TEST_CASE("sanity test") {
    std::stringstream str{R"(<?xml version="1.0"?><rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:rdfdata="http://www.w3.org/1999/02/22-rdf-syntax-ns#data" xmlns:ex="https://www.example.com/">
    <rdf:Description rdf:about="https://www.example.com" rdf:type="https://www.example2.com/type">
        <ex:title>example</ex:title>
        <ex:cost rdf:datatype="http://www.w3.org/2001/XMLSchema#int">42</ex:cost>
        <ex:cost rdf:datatype="http://www.w3.org/2001/XMLSchema#int">not a number</ex:cost>
        <ex:author rdf:resource="https://www.example2.com"/>
        <ex:author rdf:resource="htt?ps://example"/>
        <ex:released rdfdata:type="http://www.w3.org/2001/XMLSchema#boolean">true</ex:released>
        <ex:recommended>
            <rdf:Description rdf:about="https://www.other_example.com">
                <ex:title>other example</ex:title>
            </rdf:Description>
        </ex:recommended>
        <ex:recommended>
            <rdf:Description>
                <ex:title>blank example</ex:title>
            </rdf:Description>
        </ex:recommended>
        <ex:recommended>
            <rdf:Description>
                <ex:title>blank example 2</ex:title>
            </rdf:Description>
        </ex:recommended>
        <ex:coll rdf:parseType="Collection"/>
        <ex:a rdf:parseType="Literal"><a> <b> </b> </a></ex:a>
        <ex:a rdf:parseType="Literal">abc<a> <b> </b> </a>def</ex:a>
    </rdf:Description>
</rdf:RDF>)"};

    IStreamQuadIterator it{str, ParsingFlag::RdfXml};
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#type"));
    CHECK(it->value().object() == IRI::make("https://www.example2.com/type"));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/title"));
    CHECK(it->value().object() == Literal::make_simple("example"));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/cost"));
    CHECK(it->value().object() == Literal::make_typed_from_value<datatypes::xsd::Int>(42));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(!it->has_value());
    CHECK(it->error().error_type == ParsingError::Type::BadLiteral);
    CHECK(it->error().message == "http://www.w3.org/2001/XMLSchema#int parsing error: found n, invalid for datatype");
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/author"));
    CHECK(it->value().object() == IRI::make("https://www.example2.com"));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(!it->has_value());
    CHECK(it->error().error_type == ParsingError::Type::BadIri);
    CHECK(it->error().message == "htt?ps://example: InvalidScheme");
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/released"));
    CHECK(it->value().object() == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/recommended"));
    CHECK(it->value().object() == IRI::make("https://www.other_example.com"));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.other_example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/title"));
    CHECK(it->value().object() == Literal::make_simple("other example"));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/recommended"));
    CHECK(it->value().object().is_blank_node());
    auto bn = it->value().object();
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == bn);
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/title"));
    CHECK(it->value().object() == Literal::make_simple("blank example"));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/recommended"));
    CHECK(it->value().object().is_blank_node());
    CHECK(it->value().object() != bn);
    auto bn2 = it->value().object();
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == bn2);
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/title"));
    CHECK(it->value().object() == Literal::make_simple("blank example 2"));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/coll"));
    CHECK(it->value().object() == IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#nil"));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/a"));
    CHECK(it->value().object() == Literal::make_typed("<a> <b> </b> </a>", IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral")));
    ++it;
    CHECK(it != std::default_sentinel);
    CHECK(it->has_value());
    CHECK(it->value().subject() == IRI::make("https://www.example.com"));
    CHECK(it->value().predicate() == IRI::make("https://www.example.com/a"));
    CHECK(it->value().object() == Literal::make_typed("abc<a> <b> </b> </a>def", IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral")));
    ++it;
    CHECK(it == std::default_sentinel);
}

void xml_test_positive(std::string xml_str, std::string nt_str, std::string_view base_iri) {
    CAPTURE(base_iri);

    IStreamQuadIterator::state_type state{};
    CHECK(state.iri_factory.set_base(base_iri) == IRIFactoryError::Ok);
    std::stringstream xml{std::move(xml_str)};
    IStreamQuadIterator xml_iter{xml, ParsingFlag::RdfXml, &state};
    std::vector<query::QuadPattern> xml_results;

    std::stringstream nt{std::move(nt_str)};
    IStreamQuadIterator nt_iter{nt, ParsingFlag::NTriples};
    std::vector<query::QuadPattern> nt_results;

    static constexpr auto read_iter_to = [](IStreamQuadIterator& i, std::vector<query::QuadPattern>& r) {
        while (i != std::default_sentinel) {
            if (!i->has_value()) {
                FAIL(i->error().message);
            }
            r.emplace_back(i->value());
            ++i;
        }
    };
    read_iter_to(xml_iter, xml_results);
    read_iter_to(nt_iter, nt_results);

    REQUIRE(xml_results.size() == nt_results.size());

    static constexpr auto num_blanks = [](const query::QuadPattern& p) {
        size_t n = 0;
        if (p.subject().is_blank_node()) {
            ++n;
        }
        if (p.predicate().is_blank_node()) {
            ++n;
        }
        if (p.object().is_blank_node()) {
            ++n;
        }
        return n;
    };
    static constexpr auto sort = [](std::vector<query::QuadPattern>& v) {
        std::sort(v.begin(), v.end(), [](const query::QuadPattern& a, const query::QuadPattern& b) {
            auto a_bl = num_blanks(a);
            auto b_bl = num_blanks(b);
            if (a_bl != b_bl) {
                return std::less{}(a_bl, b_bl);
            }
            if (a.subject() != b.subject() && !a.subject().is_blank_node() && !b.subject().is_blank_node()) {
                return std::less{}(a.subject(), b.subject());
            }
            if (a.predicate() != b.predicate() && !a.predicate().is_blank_node() && !b.predicate().is_blank_node()) {
                return std::less{}(a.predicate(), b.predicate());
            }
            if (!a.object().is_blank_node() && !b.object().is_blank_node()) {
                return std::less{}(a.object(), b.object());
            }
            if (a.subject() != b.subject()) {
                return std::less{}(a.subject(), b.subject());
            }
            if (a.predicate() != b.predicate()) {
                return std::less{}(a.predicate(), b.predicate());
            }
            return std::less{}(a.object(), b.object());
        });
    };
    sort(xml_results);
    sort(nt_results);

    std::map<BlankNode, BlankNode> bn_map{};
    auto check = [&bn_map](Node xml, Node nt) {
        if (nt.is_blank_node() && xml.is_blank_node()) {
            auto i = bn_map.find(nt.as_blank_node());
            if (i != bn_map.end()) {
                CHECK(xml.as_blank_node() == i->second.as_blank_node());
            }
            else {
                bn_map[nt.as_blank_node()] = xml.as_blank_node();
            }
        }
        else {
            CHECK(xml == nt);
        }
    };

    for (size_t i = 0; i < nt_results.size(); ++i) {
        check(xml_results.at(i).subject(), nt_results.at(i).subject());
        check(xml_results.at(i).predicate(), nt_results.at(i).predicate());
        check(xml_results.at(i).object(), nt_results.at(i).object());
    }
}

void xml_test_negative(std::string xml_str, std::string_view base_iri) {
    CAPTURE(base_iri);


    std::stringstream xml{std::move(xml_str)};
    IStreamQuadIterator xml_iter{xml, ParsingFlag::RdfXml};

    bool had_error = false;
    while (xml_iter != std::default_sentinel) {
        if (xml_iter->has_value()) {
            ++xml_iter;
            continue;
        }
        had_error = true;
        ++xml_iter;
    }
    CHECK(had_error == true);
}


// adopted from https://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c/9786295#9786295
static size_t write_callback(void const *contents, size_t size, size_t nmemb, void *userp) {
    static_cast<std::string *>(userp)->append(static_cast<char const *>(contents), size * nmemb);
    return size * nmemb;
}

std::string remote_test_file_to_str(std::string const &file_name) {
    CURL *curl = nullptr;
    CURLcode curl_res;
    auto const url = std::format("https://raw.githubusercontent.com/w3c/rdf-tests/refs/heads/main/rdf/rdf11/rdf-xml/{}", file_name);
    std::string file_contents_as_str;
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file_contents_as_str);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // for https
        curl_res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    REQUIRE_EQ(curl_res, CURLE_OK);
    return file_contents_as_str;
}

TEST_CASE("test cases from rdf-tests") {
    // positive tests
    xml_test_positive(remote_test_file_to_str("amp-in-url/test001.rdf"), remote_test_file_to_str("amp-in-url/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/amp-in-url/test001.rdf");
    xml_test_positive(remote_test_file_to_str("datatypes/test001.rdf"), remote_test_file_to_str("datatypes/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/datatypes/test001.rdf");
    // xml_test_positive(remote_test_file_to_str("datatypes/test002.rdf"), remote_test_file_to_str("datatypes/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/datatypes/test002.rdf"); // invalid integer
    xml_test_positive(remote_test_file_to_str("rdf-charmod-literals/test001.rdf"), remote_test_file_to_str("rdf-charmod-literals/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-charmod-literals/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-charmod-uris/test001.rdf"), remote_test_file_to_str("rdf-charmod-uris/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-charmod-uris/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-charmod-uris/test002.rdf"), remote_test_file_to_str("rdf-charmod-uris/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-charmod-uris/test002.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-containers-syntax-vs-schema/test001.rdf"), remote_test_file_to_str("rdf-containers-syntax-vs-schema/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-containers-syntax-vs-schema/test002.rdf"), remote_test_file_to_str("rdf-containers-syntax-vs-schema/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/test002.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-containers-syntax-vs-schema/test003.rdf"), remote_test_file_to_str("rdf-containers-syntax-vs-schema/test003.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/test003.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-containers-syntax-vs-schema/test004.rdf"), remote_test_file_to_str("rdf-containers-syntax-vs-schema/test004.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/test004.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-containers-syntax-vs-schema/test006.rdf"), remote_test_file_to_str("rdf-containers-syntax-vs-schema/test006.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/test006.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-containers-syntax-vs-schema/test007.rdf"), remote_test_file_to_str("rdf-containers-syntax-vs-schema/test007.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/test007.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-containers-syntax-vs-schema/test008.rdf"), remote_test_file_to_str("rdf-containers-syntax-vs-schema/test008.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/test008.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-element-not-mandatory/test001.rdf"), remote_test_file_to_str("rdf-element-not-mandatory/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-element-not-mandatory/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-node-element/test001.rdf"), remote_test_file_to_str("rdf-node-element/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-node-element/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0001.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0001.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0003.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0003.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0003.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0004.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0004.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0004.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0005.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0005.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0005.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0006.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0006.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0006.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0009.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0009.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0009.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0010.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0010.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0010.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0011.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0011.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0011.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0012.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0012.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0012.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0013.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0013.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0013.rdf");
    xml_test_positive(remote_test_file_to_str("rdf-ns-prefix-confusion/test0014.rdf"), remote_test_file_to_str("rdf-ns-prefix-confusion/test0014.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-ns-prefix-confusion/test0014.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-difference-between-ID-and-about/test1.rdf"), remote_test_file_to_str("rdfms-difference-between-ID-and-about/test1.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-difference-between-ID-and-about/test1.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-difference-between-ID-and-about/test2.rdf"), remote_test_file_to_str("rdfms-difference-between-ID-and-about/test2.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-difference-between-ID-and-about/test2.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-difference-between-ID-and-about/test3.rdf"), remote_test_file_to_str("rdfms-difference-between-ID-and-about/test3.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-difference-between-ID-and-about/test3.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-duplicate-member-props/test001.rdf"), remote_test_file_to_str("rdfms-duplicate-member-props/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-duplicate-member-props/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test001.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test002.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test002.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test004.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test004.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test004.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test005.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test005.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test005.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test006.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test006.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test006.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test007.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test007.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test007.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test008.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test008.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test008.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test010.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test010.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test010.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test011.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test011.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test011.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test012.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test012.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test012.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test013.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test013.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test013.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test014.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test014.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test014.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test015.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test015.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test015.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test016.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test016.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test016.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-empty-property-elements/test017.rdf"), remote_test_file_to_str("rdfms-empty-property-elements/test017.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/test017.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-identity-anon-resources/test001.rdf"), remote_test_file_to_str("rdfms-identity-anon-resources/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-identity-anon-resources/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-identity-anon-resources/test002.rdf"), remote_test_file_to_str("rdfms-identity-anon-resources/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-identity-anon-resources/test002.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-identity-anon-resources/test003.rdf"), remote_test_file_to_str("rdfms-identity-anon-resources/test003.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-identity-anon-resources/test003.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-identity-anon-resources/test004.rdf"), remote_test_file_to_str("rdfms-identity-anon-resources/test004.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-identity-anon-resources/test004.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-identity-anon-resources/test005.rdf"), remote_test_file_to_str("rdfms-identity-anon-resources/test005.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-identity-anon-resources/test005.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-not-id-and-resource-attr/test001.rdf"), remote_test_file_to_str("rdfms-not-id-and-resource-attr/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-not-id-and-resource-attr/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-not-id-and-resource-attr/test002.rdf"), remote_test_file_to_str("rdfms-not-id-and-resource-attr/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-not-id-and-resource-attr/test002.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-not-id-and-resource-attr/test004.rdf"), remote_test_file_to_str("rdfms-not-id-and-resource-attr/test004.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-not-id-and-resource-attr/test004.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-not-id-and-resource-attr/test005.rdf"), remote_test_file_to_str("rdfms-not-id-and-resource-attr/test005.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-not-id-and-resource-attr/test005.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-para196/test001.rdf"), remote_test_file_to_str("rdfms-para196/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-para196/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-001.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-002.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-002.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-003.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-003.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-003.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-004.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-004.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-004.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-005.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-005.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-005.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-006.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-006.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-006.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-007.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-007.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-007.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-008.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-008.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-008.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-009.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-009.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-009.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-010.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-010.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-010.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-011.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-011.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-011.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-012.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-012.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-012.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-013.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-013.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-013.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-014.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-014.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-014.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-015.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-015.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-015.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-016.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-016.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-016.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-017.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-017.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-017.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-018.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-018.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-018.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-019.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-019.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-019.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-020.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-020.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-020.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-021.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-021.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-021.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-022.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-022.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-022.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-023.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-023.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-023.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-024.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-024.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-024.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-025.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-025.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-025.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-026.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-026.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-026.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-027.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-027.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-027.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-028.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-028.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-028.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-029.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-029.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-029.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-030.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-030.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-030.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-031.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-031.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-031.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-032.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-032.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-032.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-033.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-033.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-033.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-034.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-034.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-034.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-035.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-035.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-035.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-036.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-036.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-036.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/test-037.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/test-037.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/test-037.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/warn-001.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/warn-001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/warn-001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/warn-002.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/warn-002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/warn-002.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-rdf-names-use/warn-003.rdf"), remote_test_file_to_str("rdfms-rdf-names-use/warn-003.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/warn-003.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-reification-required/test001.rdf"), remote_test_file_to_str("rdfms-reification-required/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-reification-required/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-reification-required/test002.rdf"), remote_test_file_to_str("rdfms-reification-required/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-reification-required/test002.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-seq-representation/test001.rdf"), remote_test_file_to_str("rdfms-seq-representation/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-seq-representation/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-seq-representation/test002.rdf"), remote_test_file_to_str("rdfms-seq-representation/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-seq-representation/test002.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-syntax-incomplete/test001.rdf"), remote_test_file_to_str("rdfms-syntax-incomplete/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-syntax-incomplete/test002.rdf"), remote_test_file_to_str("rdfms-syntax-incomplete/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/test002.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-syntax-incomplete/test003.rdf"), remote_test_file_to_str("rdfms-syntax-incomplete/test003.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/test003.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-syntax-incomplete/test004.rdf"), remote_test_file_to_str("rdfms-syntax-incomplete/test004.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/test004.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-uri-substructure/test001.rdf"), remote_test_file_to_str("rdfms-uri-substructure/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-uri-substructure/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-xmllang/test003.rdf"), remote_test_file_to_str("rdfms-xmllang/test003.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-xmllang/test003.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-xmllang/test004.rdf"), remote_test_file_to_str("rdfms-xmllang/test004.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-xmllang/test004.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-xmllang/test005.rdf"), remote_test_file_to_str("rdfms-xmllang/test005.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-xmllang/test005.rdf");
    xml_test_positive(remote_test_file_to_str("rdfms-xmllang/test006.rdf"), remote_test_file_to_str("rdfms-xmllang/test006.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-xmllang/test006.rdf");
    xml_test_positive(remote_test_file_to_str("rdfs-domain-and-range/test001.rdf"), remote_test_file_to_str("rdfs-domain-and-range/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfs-domain-and-range/test001.rdf");
    xml_test_positive(remote_test_file_to_str("rdfs-domain-and-range/test002.rdf"), remote_test_file_to_str("rdfs-domain-and-range/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfs-domain-and-range/test002.rdf");
    xml_test_positive(remote_test_file_to_str("unrecognised-xml-attributes/test001.rdf"), remote_test_file_to_str("unrecognised-xml-attributes/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/unrecognised-xml-attributes/test001.rdf");
    xml_test_positive(remote_test_file_to_str("unrecognised-xml-attributes/test002.rdf"), remote_test_file_to_str("unrecognised-xml-attributes/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/unrecognised-xml-attributes/test002.rdf");
    // xml_test_positive(remote_test_file_to_str("xml-canon/test001.rdf"), remote_test_file_to_str("xml-canon/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xml-canon/test001.rdf"); // XMLLiteral is not exactly as the spec defines
    // xml_test_positive(remote_test_file_to_str("xml-canon/test002.rdf"), remote_test_file_to_str("xml-canon/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xml-canon/test002.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test001.rdf"), remote_test_file_to_str("xmlbase/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test001.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test002.rdf"), remote_test_file_to_str("xmlbase/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test002.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test003.rdf"), remote_test_file_to_str("xmlbase/test003.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test003.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test004.rdf"), remote_test_file_to_str("xmlbase/test004.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test004.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test006.rdf"), remote_test_file_to_str("xmlbase/test006.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test006.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test007.rdf"), remote_test_file_to_str("xmlbase/test007.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test007.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test008.rdf"), remote_test_file_to_str("xmlbase/test008.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test008.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test009.rdf"), remote_test_file_to_str("xmlbase/test009.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test009.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test010.rdf"), remote_test_file_to_str("xmlbase/test010.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test010.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test011.rdf"), remote_test_file_to_str("xmlbase/test011.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test011.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test013.rdf"), remote_test_file_to_str("xmlbase/test013.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test013.rdf");
    xml_test_positive(remote_test_file_to_str("xmlbase/test014.rdf"), remote_test_file_to_str("xmlbase/test014.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test014.rdf");
    // negative tests
    xml_test_negative(remote_test_file_to_str("rdf-containers-syntax-vs-schema/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/error001.rdf");
    xml_test_negative(remote_test_file_to_str("rdf-containers-syntax-vs-schema/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/error002.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-abouteach/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-abouteach/error001.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-abouteach/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-abouteach/error002.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-difference-between-ID-and-about/error1.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-difference-between-ID-and-about/error1.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-empty-property-elements/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/error001.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-empty-property-elements/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/error002.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error001.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error002.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error003.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error003.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error004.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error004.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error005.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error005.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error006.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error006.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error007.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error007.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-001.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-002.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-003.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-003.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-004.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-004.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-005.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-005.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-006.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-006.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-007.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-007.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-008.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-008.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-009.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-009.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-010.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-010.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-011.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-011.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-012.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-012.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-013.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-013.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-014.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-014.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-015.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-015.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-016.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-016.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-017.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-017.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-018.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-018.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-019.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-019.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-020.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-020.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error001.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error002.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error003.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error003.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error004.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error004.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error005.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error005.rdf");
    xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error006.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error006.rdf");
}
