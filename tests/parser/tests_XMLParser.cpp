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

TEST_CASE("rdf") {
    // adapted from https://github.com/w3c/rdf-tests/tree/main/rdf/rdf11/rdf-xml

    std::string xml = "";
    std::string nt = "";


    SUBCASE("syntax 1 (base applies to id)") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file">

 <rdf:Description rdf:ID="frag" eg:value="v" />

</rdf:RDF>)";
        nt = R"(<http://example.org/dir/file#frag> <http://example.org/value> "v" .)";
    }
    SUBCASE("syntax 2 (base applies to resource)") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file">

 <rdf:Description rdf:about="http://example.org/foo">
   <eg:value rdf:resource="relFile" />
 </rdf:Description>

</rdf:RDF>)";
        nt = R"(<http://example.org/foo> <http://example.org/value> <http://example.org/dir/relFile> .)";
    }
    SUBCASE("syntax 3 (base applies to about)") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file">

 <eg:type rdf:about="relfile" />

</rdf:RDF>)";
        nt = R"(<http://example.org/dir/relfile> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/type> .)";
    }
    // case 4 needs reification
    SUBCASE("syntax 6 (base scoping)") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file">

 <rdf:Description rdf:ID="frag" eg:value="v" xml:base="http://example.org/file2"/>
 <eg:type rdf:about="relFile" />

</rdf:RDF>)";
        nt = R"(<http://example.org/file2#frag> <http://example.org/value> "v" .
<http://example.org/dir/relFile> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/type> .)";
    }
    SUBCASE("syntax 7 (relative resolution)") {
        xml = R"(<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file">

 <eg:type rdf:about="../relfile" />

</rdf:RDF>)";
        nt = R"(<http://example.org/relfile> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/type> .)";
    }
    SUBCASE("syntax 8 (empty local)") {
        xml = R"(<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file">

 <eg:type rdf:about="" />

</rdf:RDF>)";
        nt = R"(<http://example.org/dir/file> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/type> .)";
    }
    SUBCASE("syntax 9 (absolute path)") {
        xml = R"(<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file">

 <eg:type rdf:about="/absfile" />

</rdf:RDF>)";
        nt = R"(<http://example.org/absfile> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/type> .)";
    }
    SUBCASE("syntax 10 (absolute host)") {
        xml = R"(<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file">

 <eg:type rdf:about="//another.example.org/absfile" />

</rdf:RDF>)";
        nt = R"(<http://another.example.org/absfile> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/type> .)";
    }
    SUBCASE("syntax 11 (base without path)") {
        xml = R"(<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org">

 <eg:type rdf:about="relfile" />

</rdf:RDF>)";
        nt = R"(<http://example.org/relfile> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/type> .)";
    }
    SUBCASE("syntax 13 (base with fragment)") {
        xml = R"(<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file#frag">

 <eg:type rdf:about="" />
 <rdf:Description rdf:ID="foo" >
   <eg:value rdf:resource="relpath" />
 </rdf:Description>

</rdf:RDF>)";
        nt = R"(<http://example.org/dir/file> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/type> .
<http://example.org/dir/file#foo> <http://example.org/value> <http://example.org/dir/relpath> .)";
    }
    SUBCASE("syntax 14 (same ids)") {
        xml = R"(<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test014.rdf"
         >

 <rdf:Description xml:base="http://example.org/dir/file"
                rdf:ID="frag" eg:value="v" />
 <rdf:Description rdf:ID="frag" eg:value="v" />

</rdf:RDF>)";
        nt = R"(<http://example.org/dir/file#frag> <http://example.org/value> "v" .
<https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xmlbase/test014.rdf#frag> <http://example.org/value> "v" .)";
    }
    SUBCASE("amp") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">

  <rdf:Description rdf:about="http://example/q?abc=1&#38;def=2">
    <rdf:value>xxx</rdf:value>
  </rdf:Description>
  <rdf:Description rdf:about="http://example2/q?abc=1&amp;def=2">
    <rdf:value>xxx</rdf:value>
  </rdf:Description>

</rdf:RDF>)";
        nt = R"(<http://example/q?abc=1&def=2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#value> "xxx" .
<http://example2/q?abc=1&def=2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#value> "xxx" .)";
    }
    SUBCASE("datatypes") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/">

 <rdf:Description rdf:about="http://example.org/foo">
   <eg:bar rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">10</eg:bar>
   <eg:baz rdf:datatype="http://www.w3.org/2001/XMLSchema#integer" xml:lang="fr">10</eg:baz>
 </rdf:Description>

</rdf:RDF>)";
        nt = R"(<http://example.org/foo> <http://example.org/bar> "10"^^<http://www.w3.org/2001/XMLSchema#integer> .
<http://example.org/foo> <http://example.org/baz> "10"^^<http://www.w3.org/2001/XMLSchema#integer> .)";
    }
    SUBCASE("unicode literal") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/">
   <!-- Dürst registers himself as a creator of the Charmod WD. -->

   <rdf:Description rdf:about="http://www.w3.org/TR/2002/WD-charmod-20020220">

   <!-- The ü below is a single character #xFC in NFC
        (encoded as two UTF-8 octets #xC3 #xBC)  -->
      <eg:Creator eg:named="Dürst"/>

   </rdf:Description>
</rdf:RDF>)";
        nt = R"(_:a <http://example.org/named> "D\u00FCrst" .
<http://www.w3.org/TR/2002/WD-charmod-20020220> <http://example.org/Creator> _:a .)";
    }

    if (xml.empty()) {
        return;
    }

    std::stringstream xml_str{xml};
    XMLQuadIterator xml_iter{xml_str};

    std::stringstream nt_str{nt};
    IStreamQuadIterator nt_iter{nt_str, ParsingFlag::NTriples};

    std::map<BlankNode, BlankNode> bn_map{};
    auto check = [&bn_map](Node xml, Node nt) {
        CHECK(xml.is_blank_node() == nt.is_blank_node());
        if (nt.is_blank_node()) {
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

    while (nt_iter != std::default_sentinel) {
        REQUIRE(xml_iter != std::default_sentinel);
        if (!xml_iter->has_value()) {
            FAIL(xml_iter->error().message);
        }
        REQUIRE(nt_iter->has_value());
        check(xml_iter->value().subject() , nt_iter->value().subject());
        check(xml_iter->value().predicate(), nt_iter->value().predicate());
        check(xml_iter->value().object(), nt_iter->value().object());

        ++xml_iter;
        ++nt_iter;
    }

    REQUIRE(xml_iter == std::default_sentinel);
}
