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

TEST_CASE("rdf xml positive tests") {
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
    SUBCASE("unicode iri 1") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/#">

  <!-- The é below is a single Unicode character #xE9 in
       Unicode Normal Form C, NFC (here encoded as
       two UTF-8 octets #C3,#A9) -->

   <rdf:Description rdf:about="http://example.org/#André">
      <eg:owes>2000</eg:owes>
   </rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://example.org/#Andr\u00E9> <http://example.org/#owes> "2000" .)";
    }
    SUBCASE("unicode iri 2") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/#">

  <!-- The %C3%A9 below corresponds to é under the standard
        %-escaping algorithm for URIs. -->

   <rdf:Description rdf:about="http://example.org/#Andr%C3%A9">
      <eg:owes>2000</eg:owes>
   </rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://example.org/#Andr%C3%A9> <http://example.org/#owes> "2000" .)";
    }
    SUBCASE("type instead of description") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:ex="http://example.org/terms#">

  <ex:Book rdf:about="http://example.org/Book">
    <ex:title>Dogs in Hats</ex:title>
  </ex:Book>

</rdf:RDF>)";
        nt = R"(<http://example.org/Book> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/terms#Book> .
<http://example.org/Book> <http://example.org/terms#title> "Dogs in Hats" .)";
    }
    SUBCASE("id 1") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
<rdf:Description rdf:ID="foo">
  <rdf:value>abc</rdf:value>
</rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://example.org/#foo> <http://www.w3.org/1999/02/22-rdf-syntax-ns#value> "abc" .)";
    }
    SUBCASE("id 2") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
<rdf:Description rdf:ID="D&#xFC;rst">
  <rdf:value>abc</rdf:value>
</rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://example.org/#D\u00FCrst> <http://www.w3.org/1999/02/22-rdf-syntax-ns#value> "abc" .)";
    }
    SUBCASE("id 3") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
<rdf:Description rdf:about="#D&#xFC;rst">
  <rdf:value>abc</rdf:value>
</rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://example.org/#D\u00FCrst> <http://www.w3.org/1999/02/22-rdf-syntax-ns#value> "abc" .)";
    }
    SUBCASE("duplicate bag entries") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
  <rdf:Bag rdf:about="http://example.org/foo">
     <rdf:_1 rdf:resource="http://example.org/a" />
     <rdf:_1 rdf:resource="http://example.org/b" />
  </rdf:Bag>
</rdf:RDF>)";
        nt = R"(<http://example.org/foo> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/1999/02/22-rdf-syntax-ns#Bag> .
<http://example.org/foo> <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> <http://example.org/a> .
<http://example.org/foo> <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> <http://example.org/b> .)";
    }
    SUBCASE("empty property 1") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:random="http://random.ioctl.org/#">

<rdf:Description rdf:about="http://random.ioctl.org/#bar">
  <random:someProperty rdf:resource="http://random.ioctl.org/#foo" />
</rdf:Description>

</rdf:RDF>)";
        nt = R"(<http://random.ioctl.org/#bar> <http://random.ioctl.org/#someProperty> <http://random.ioctl.org/#foo> .)";
    }
    SUBCASE("empty property 2") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:random="http://random.ioctl.org/#">

<rdf:Description rdf:about="http://random.ioctl.org/#bar">
  <random:someProperty />
</rdf:Description>

</rdf:RDF>)";
        nt = R"(<http://random.ioctl.org/#bar> <http://random.ioctl.org/#someProperty> "" .)";
    }
    SUBCASE("empty property 3") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:random="http://random.ioctl.org/#">

<rdf:Description rdf:about="http://random.ioctl.org/#bar">
  <random:someProperty rdf:parseType="Literal"/>
</rdf:Description>

</rdf:RDF>)";
        nt = R"(<http://random.ioctl.org/#bar> <http://random.ioctl.org/#someProperty> ""^^<http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral> .)";
    }
    SUBCASE("empty property 4") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:random="http://random.ioctl.org/#">

<rdf:Description rdf:about="http://random.ioctl.org/#bar">
  <random:someProperty rdf:parseType="Resource" />
</rdf:Description>

</rdf:RDF>)";
        nt = R"(<http://random.ioctl.org/#bar> <http://random.ioctl.org/#someProperty> _:a1 .)";
    }
    SUBCASE("empty property 13") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:random="http://random.ioctl.org/#">

<rdf:Description rdf:about="http://random.ioctl.org/#bar">
  <random:someProperty rdf:resource="http://random.ioctl.org/#foo"
        random:prop2="baz" />
</rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://random.ioctl.org/#foo> <http://random.ioctl.org/#prop2> "baz" .
<http://random.ioctl.org/#bar> <http://random.ioctl.org/#someProperty> <http://random.ioctl.org/#foo> .)";
    }
    SUBCASE("blank node identity") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/">

 <eg:node>
   <eg:property>property value</eg:property>
 </eg:node>

</rdf:RDF>)";
        nt = R"(_:j0 <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/node> .
_:j0 <http://example.org/property> "property value" .)";
    }
    SUBCASE("blank node identity 2") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/">

 <rdf:Description rdf:nodeID="a">
   <eg:property1 rdf:nodeID="a" />
 </rdf:Description>
 <rdf:Description>
   <eg:property2>
<!-- Note the rdf:nodeID="b" is redundant. -->
      <rdf:Description rdf:nodeID="b">
            <eg:property3 rdf:nodeID="a" />
      </rdf:Description>
   </eg:property2>
 </rdf:Description>
</rdf:RDF>)";
        nt = R"(_:j0A <http://example.org/property1> _:j0A .
_:j2 <http://example.org/property2> _:j1B .
_:j1B <http://example.org/property3> _:j0A .)";
    }
    SUBCASE("collection") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF
    xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
    xmlns:eg="http://example.org/eg#">

    <rdf:Description rdf:about="http://example.org/eg#eric">
        <rdf:type rdf:parseType="Resource">
            <eg:intersectionOf rdf:parseType="Collection">
                <rdf:Description rdf:about="http://example.org/eg#Person"/>
                <rdf:Description rdf:about="http://example.org/eg#Male"/>
            </eg:intersectionOf>
        </rdf:type>
    </rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://example.org/eg#eric> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> _:a0 .
_:a0 <http://example.org/eg#intersectionOf> _:a1 .
_:a1 <http://www.w3.org/1999/02/22-rdf-syntax-ns#first> <http://example.org/eg#Person> .
_:a1 <http://www.w3.org/1999/02/22-rdf-syntax-ns#rest> _:a2 .
_:a2 <http://www.w3.org/1999/02/22-rdf-syntax-ns#first> <http://example.org/eg#Male> .
_:a2 <http://www.w3.org/1999/02/22-rdf-syntax-ns#rest> <http://www.w3.org/1999/02/22-rdf-syntax-ns#nil> .)";
    }
    SUBCASE("nested reify") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
 xmlns:eg="http://example.org/"
 xml:base="http://example.com/">

 <rdf:Description rdf:about="http://example.org/a">
   <eg:prop rdf:ID="reify">
     <rdf:Description rdf:about="http://example.org/b">
       <eg:prop rdf:ID="reify2" rdf:resource="http://example.org/c"/>
     </rdf:Description>
   </eg:prop>
 </rdf:Description>
</rdf:RDF>)";
        nt = R"(
<http://example.org/a> <http://example.org/prop> <http://example.org/b> .
<http://example.com/#reify> <http://www.w3.org/1999/02/22-rdf-syntax-ns#subject> <http://example.org/a> .
<http://example.com/#reify> <http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate> <http://example.org/prop> .
<http://example.com/#reify> <http://www.w3.org/1999/02/22-rdf-syntax-ns#object> <http://example.org/b> .
<http://example.com/#reify> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement> .
<http://example.org/b> <http://example.org/prop> <http://example.org/c> .
<http://example.com/#reify2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#subject> <http://example.org/b> .
<http://example.com/#reify2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate> <http://example.org/prop> .
<http://example.com/#reify2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#object> <http://example.org/c> .
<http://example.com/#reify2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement> .)";
    }
    SUBCASE("reify target") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/">

  <rdf:Description>
    <eg:prop1  rdf:ID="reify" eg:prop2="val"></eg:prop1>
  </rdf:Description>
</rdf:RDF>)";
        nt = R"(_:j88091 <http://example.org/prop2> "val" .
_:j88090 <http://example.org/prop1> _:j88091 .
<http://example.org/#reify> <http://www.w3.org/1999/02/22-rdf-syntax-ns#subject> _:j88090 .
<http://example.org/#reify> <http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate> <http://example.org/prop1> .
<http://example.org/#reify> <http://www.w3.org/1999/02/22-rdf-syntax-ns#object> _:j88091 .
<http://example.org/#reify> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement> .)";
    }
    SUBCASE("reify collection") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF
    xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
    xmlns:eg="http://example.org/eg#"
    xml:base="http://example.com/">

    <rdf:Description rdf:about="http://example.org/eg#eric">
        <rdf:type rdf:parseType="Resource">
            <eg:intersectionOf rdf:ID="reif" rdf:parseType="Collection">
                <rdf:Description rdf:about="http://example.org/eg#Person"/>
                <rdf:Description rdf:about="http://example.org/eg#Male"/>
            </eg:intersectionOf>
        </rdf:type>
    </rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://example.org/eg#eric> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> _:a0 .
_:a0 <http://example.org/eg#intersectionOf> _:a1 .
<http://example.com/#reif> <http://www.w3.org/1999/02/22-rdf-syntax-ns#subject> _:a0 .
<http://example.com/#reif> <http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate> <http://example.org/eg#intersectionOf> .
<http://example.com/#reif> <http://www.w3.org/1999/02/22-rdf-syntax-ns#object> _:a1 .
<http://example.com/#reif> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement> .
_:a1 <http://www.w3.org/1999/02/22-rdf-syntax-ns#first> <http://example.org/eg#Person> .
_:a1 <http://www.w3.org/1999/02/22-rdf-syntax-ns#rest> _:a2 .
_:a2 <http://www.w3.org/1999/02/22-rdf-syntax-ns#first> <http://example.org/eg#Male> .
_:a2 <http://www.w3.org/1999/02/22-rdf-syntax-ns#rest> <http://www.w3.org/1999/02/22-rdf-syntax-ns#nil> .)";
    }
    SUBCASE("reify literal") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/"
         xml:base="http://example.org/dir/file">

 <rdf:Description>
  <eg:value rdf:ID="frag">v</eg:value>
 </rdf:Description>

</rdf:RDF>)";
        nt = R"(_:j0 <http://example.org/value> "v" .
<http://example.org/dir/file#frag> <http://www.w3.org/1999/02/22-rdf-syntax-ns#subject> _:j0 .
<http://example.org/dir/file#frag> <http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate> <http://example.org/value> .
<http://example.org/dir/file#frag> <http://www.w3.org/1999/02/22-rdf-syntax-ns#object> "v" .
<http://example.org/dir/file#frag> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement> .)";
    }
    SUBCASE("lang literal") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/">

  <rdf:Description rdf:about="http://example.org/node">
     <eg:property xml:lang="fr">chat</eg:property>
  </rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://example.org/node> <http://example.org/property> "chat"@fr .)";
    }
    SUBCASE("lang literal attribute") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/">

  <rdf:Description rdf:about="http://example.org/node"
                   xml:lang="fr"
                   eg:property="chat" />

</rdf:RDF>)";
        nt = R"(<http://example.org/node> <http://example.org/property> "chat"@fr .)";
    }
    SUBCASE("simple list") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">

  <rdf:Bag>
    <rdf:li>1</rdf:li>
    <rdf:li>2</rdf:li>
  </rdf:Bag>
</rdf:RDF>)";
        nt = R"(_:bag <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/1999/02/22-rdf-syntax-ns#Bag> .
_:bag <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> "1" .
_:bag <http://www.w3.org/1999/02/22-rdf-syntax-ns#_2> "2" .)";
    }
    SUBCASE("list interference") {
        xml = R"(<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:foo="http://foo/">

  <foo:Bar>
    <rdf:_1>_1</rdf:_1>
    <rdf:li>1</rdf:li>
    <rdf:_3>_3</rdf:_3>
    <rdf:li>2</rdf:li>
  </foo:Bar>
</rdf:RDF>)";
        nt = R"(_:bag <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://foo/Bar> .
_:bag <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> "_1" .
_:bag <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> "1" .
_:bag <http://www.w3.org/1999/02/22-rdf-syntax-ns#_3> "_3" .
_:bag <http://www.w3.org/1999/02/22-rdf-syntax-ns#_2> "2" .)";
    }
    SUBCASE("list advanced") {
        xml = R"(<?xml version="1.0"?>

<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:foo="http://foo/">

  <foo:Bar>
    <rdf:li rdf:ID="e1">1</rdf:li>
    <rdf:li rdf:parseType="Literal">2</rdf:li>
    <rdf:li rdf:parseType="Resource">
      <rdf:type rdf:resource="http://foo/Bar"/>
    </rdf:li>
    <rdf:li rdf:ID="e4" foo:bar="foobar"/>
  </foo:Bar>
</rdf:RDF>)";
        nt = R"(_:bar <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://foo/Bar> .
_:bar <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> "1" .
<http://example.org/#e1> <http://www.w3.org/1999/02/22-rdf-syntax-ns#subject> _:bar .
<http://example.org/#e1> <http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate> <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> .
<http://example.org/#e1> <http://www.w3.org/1999/02/22-rdf-syntax-ns#object> "1" .
<http://example.org/#e1> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement> .
_:bar <http://www.w3.org/1999/02/22-rdf-syntax-ns#_2> "2"^^<http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral> .
_:bar <http://www.w3.org/1999/02/22-rdf-syntax-ns#_3> _:res .
_:res <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://foo/Bar> .
_:res2 <http://foo/bar> "foobar" .
_:bar <http://www.w3.org/1999/02/22-rdf-syntax-ns#_4> _:res2 .
<http://example.org/#e4> <http://www.w3.org/1999/02/22-rdf-syntax-ns#subject> _:bar .
<http://example.org/#e4> <http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate> <http://www.w3.org/1999/02/22-rdf-syntax-ns#_4> .
<http://example.org/#e4> <http://www.w3.org/1999/02/22-rdf-syntax-ns#object> _:res2 .
<http://example.org/#e4> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement> .)";
    }
    SUBCASE("list other") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:foo="http://foo/">

  <rdf:Seq rdf:ID="e1" rdf:_3="3" rdf:value="foobar"/>
  <rdf:Alt rdf:about="#e2" rdf:_2="2" rdf:value="foobar">
    <rdf:value>barfoo</rdf:value>
  </rdf:Alt>
  <rdf:Bag />
</rdf:RDF>)";
        nt = R"(<http://example.org/#e1> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type>  <http://www.w3.org/1999/02/22-rdf-syntax-ns#Seq> .
<http://example.org/#e1> <http://www.w3.org/1999/02/22-rdf-syntax-ns#_3> "3" .
<http://example.org/#e1> <http://www.w3.org/1999/02/22-rdf-syntax-ns#value> "foobar" .
<http://example.org/#e2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type>  <http://www.w3.org/1999/02/22-rdf-syntax-ns#Alt> .
<http://example.org/#e2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#_2> "2" .
<http://example.org/#e2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#value> "foobar" .
<http://example.org/#e2> <http://www.w3.org/1999/02/22-rdf-syntax-ns#value> "barfoo" .
_:bag <http://www.w3.org/1999/02/22-rdf-syntax-ns#type>  <http://www.w3.org/1999/02/22-rdf-syntax-ns#Bag> .)";
    }
    SUBCASE("list independence") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:foo="http://foo/">

  <rdf:Description>
    <rdf:li>
      <rdf:Description>
        <rdf:li>1</rdf:li>
        <rdf:li>2</rdf:li>
      </rdf:Description>
    </rdf:li>
    <rdf:li>2</rdf:li>
  </rdf:Description>
</rdf:RDF>)";
        nt = R"(_:d1 <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> _:d2 .

_:d2 <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> "1" .
_:d2 <http://www.w3.org/1999/02/22-rdf-syntax-ns#_2> "2" .

_:d1 <http://www.w3.org/1999/02/22-rdf-syntax-ns#_2> "2" .)";
    }
    SUBCASE("list per element") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">

  <rdf:Description rdf:about="http://desc">
    <rdf:li>1</rdf:li>
  </rdf:Description>

  <rdf:Description rdf:about="http://desc">
    <rdf:li>1-again</rdf:li>
  </rdf:Description>
</rdf:RDF>)";
        nt = R"(<http://desc> <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> "1" .
<http://desc> <http://www.w3.org/1999/02/22-rdf-syntax-ns#_1> "1-again" .)";
    }
    SUBCASE("xml literal") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:eg="http://example.org/">


  <rdf:Description rdf:about="http://www.example.org/a">
    <eg:prop rdf:parseType="Literal"><br /></eg:prop>
  </rdf:Description>

</rdf:RDF>)";
        nt = R"(<http://www.example.org/a> <http://example.org/prop> "<br />"^^<http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral> .)";
    }

    if (xml.empty()) {
        return;
    }

    std::stringstream xml_str{xml};
    IStreamQuadIterator xml_iter{xml_str, ParsingFlag::RdfXml};

    std::stringstream nt_str{nt};
    IStreamQuadIterator nt_iter{nt_str, ParsingFlag::NTriples};

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

TEST_CASE("rdf xml negative tests") {
    // adapted from https://github.com/w3c/rdf-tests/tree/main/rdf/rdf11/rdf-xml
    std::string xml = "";
    std::vector<std::pair<ParsingError::Type, std::string_view>> expected_msg{};
    bool ignore_some_triples = false;

    SUBCASE("resource + parse type") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:random="http://random.ioctl.org/#">

<rdf:Description rdf:about="http://random.ioctl.org/#bar">
  <random:someProperty rdf:parseType="Literal"
    rdf:resource="http://random.ioctl.org/#foo" />
</rdf:Description>
</rdf:RDF>)";
        expected_msg.emplace_back(ParsingError::Type::BadSyntax, "expected only one of rdf:parseType, rdf:nodeID and rdf:resource");
        ignore_some_triples = true;
    }
    SUBCASE("implicit bn + parse type") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:random="http://random.ioctl.org/#">

<rdf:Description rdf:about="http://random.ioctl.org/#bar">
  <random:someProperty random:prop2="baz" rdf:parseType="Literal" />
</rdf:Description>
</rdf:RDF>)";
        expected_msg.emplace_back(ParsingError::Type::BadSyntax, "expected only one of rdf:parseType, rdf:nodeID and rdf:resource");
        ignore_some_triples = true;
    }
    SUBCASE("multiple ids") {
        xml = R"(<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
<rdf:Description rdf:ID="foo">
  <rdf:value>abc</rdf:value>
</rdf:Description>
<rdf:Description rdf:ID="foo">
  <rdf:value>abc</rdf:value>
</rdf:Description>
</rdf:RDF>)";
        expected_msg.emplace_back(ParsingError::Type::BadIri, "<http://example.org/#foo>: is already used as a rdf:ID");
        ignore_some_triples = true;
    }

    if (xml.empty()) {
        return;
    }

    std::stringstream xml_str{xml};
    IStreamQuadIterator xml_iter{xml_str, ParsingFlag::RdfXml};

    while (xml_iter != std::default_sentinel) {
        if (!ignore_some_triples) {
            REQUIRE(!xml_iter->has_value());
        } else if (xml_iter->has_value()) {
            ++xml_iter;
            continue;
        }
        REQUIRE(!expected_msg.empty());
        CHECK(xml_iter->error().error_type == expected_msg.back().first);
        CHECK(xml_iter->error().message == expected_msg.back().second);
        expected_msg.pop_back();
        ++xml_iter;
    }
    REQUIRE(expected_msg.empty());
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
    // xml_test_positive(remote_test_file_to_str("datatypes/test002.rdf"), remote_test_file_to_str("datatypes/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/datatypes/test002.rdf");
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
    // xml_test_positive(remote_test_file_to_str("rdf-element-not-mandatory/test001.rdf"), remote_test_file_to_str("rdf-element-not-mandatory/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-element-not-mandatory/test001.rdf");
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
    // xml_test_positive(remote_test_file_to_str("unrecognised-xml-attributes/test001.rdf"), remote_test_file_to_str("unrecognised-xml-attributes/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/unrecognised-xml-attributes/test001.rdf");
    // xml_test_positive(remote_test_file_to_str("unrecognised-xml-attributes/test002.rdf"), remote_test_file_to_str("unrecognised-xml-attributes/test002.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/unrecognised-xml-attributes/test002.rdf");
    // xml_test_positive(remote_test_file_to_str("xml-canon/test001.rdf"), remote_test_file_to_str("xml-canon/test001.nt"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/xml-canon/test001.rdf");
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
    // xml_test_negative(remote_test_file_to_str("rdf-containers-syntax-vs-schema/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/error001.rdf");
    // xml_test_negative(remote_test_file_to_str("rdf-containers-syntax-vs-schema/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdf-containers-syntax-vs-schema/error002.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-abouteach/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-abouteach/error001.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-abouteach/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-abouteach/error002.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-difference-between-ID-and-about/error1.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-difference-between-ID-and-about/error1.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-empty-property-elements/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/error001.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-empty-property-elements/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-empty-property-elements/error002.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error001.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error002.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error003.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error003.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error004.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error004.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error005.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error005.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error006.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error006.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-id/error007.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-id/error007.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-001.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-002.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-003.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-003.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-004.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-004.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-005.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-005.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-006.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-006.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-007.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-007.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-008.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-008.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-009.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-009.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-010.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-010.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-011.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-011.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-012.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-012.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-013.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-013.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-014.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-014.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-015.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-015.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-016.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-016.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-017.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-017.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-018.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-018.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-019.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-019.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-rdf-names-use/error-020.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-rdf-names-use/error-020.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error001.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error001.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error002.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error002.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error003.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error003.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error004.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error004.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error005.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error005.rdf");
    // xml_test_negative(remote_test_file_to_str("rdfms-syntax-incomplete/error006.rdf"), "https://w3c.github.io/rdf-tests/rdf/rdf11/rdf-xml/rdfms-syntax-incomplete/error006.rdf");
}
