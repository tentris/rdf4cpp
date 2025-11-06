#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "rdf4cpp/parser/XMLParser.hpp"


#include <doctest/doctest.h>

#include <rdf4cpp.hpp>
#include <rdf4cpp/storage/reference_node_storage/SyncReferenceNodeStorage.hpp>

#include <iostream>


using namespace rdf4cpp;
using namespace rdf4cpp::parser;

TEST_CASE("sanity test") {
    std::stringstream str{R"(<?xml version="1.0"?><rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:rdfdata="http://www.w3.org/1999/02/22-rdf-syntax-ns#data" xmlns:ex="https://www.example.com/">
    <rdf:Description rdf:about="https://www.example.com">
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
    </rdf:Description>
</rdf:RDF>)"};

    XMLQuadIterator it{str};
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
    CHECK(it == std::default_sentinel);
}
