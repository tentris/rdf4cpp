Users Guide
===========

Terms and Definitions
---------------------
We use the official definitions of `<https://www.w3.org/TR/rdf11-concepts/>`_.

Framework Basics
----------------

The main types of rdf4cpp:

 * :class:`rdf4cpp::Node`: Base class of Literal, IRI and BlankNode
 * :class:`rdf4cpp::IRI`: An IRI.
 * :class:`rdf4cpp::Literal`: A Literal containing some data.
 * :class:`rdf4cpp::BlankNode`: A Blank Node used to connect other Nodes.
 * :class:`rdf4cpp::Statement`: A basic rdf statement consisting of Subject, Predicate and Object.
 * :class:`rdf4cpp::Quad`: A Statement with added Graph.
 * :class:`rdf4cpp::Graph`: A Collection of Statements, with the ability to execute simple Queries.
 * :class:`rdf4cpp::Dataset`: A Collection of Quads, with the ability to execute simple Queries.
 * :class:`rdf4cpp::Namespace`: A namespace that makes constructing IRIs with common prefix easier.
 * :class:`rdf4cpp::query::Variable`: A Variable used in Queries.
 * :class:`rdf4cpp::query::TriplePattern`: A Query patterns of Subject, Predicate and Object.
 * :class:`rdf4cpp::query::QuadPattern`: A Query of Graph, Subject, Predicate and Object.
 * :class:`rdf4cpp::query::Solution`: A single matched Solution, with all Variables bound.

Datatypes
---------

Literal supports common rdf Datatypes and operations on them (as defined in `<https://www.w3.org/TR/sparql12-query/#SparqlOps>`_),
as well as conversions between Literal Datatypes.

You can find all supported Datatypes here:

 * :ref:`namespace_rdf4cpp__datatypes__xsd`: XSD datatypes.
 * :ref:`namespace_rdf4cpp__datatypes__rdf`: RDF datatypes (LangString).
 * :ref:`namespace_rdf4cpp__datatypes__owl`: OWL datatypes.

Parsing Files
-------------

The class :class:`rdf4cpp::parser::RDFFileParser` allows reading files containing rdf Statements and iterate over them.
Supported Formats: Turtle, TriG, N-Triples N-Quads and RDF/XML.
:class:`rdf4cpp::parser::IStreamQuadIterator` allows doing the same over arbitrary data streams.

Relaxed Parsing Mode
--------------------

The setting :var:`rdf4cpp::datatypes::registry::relaxed_parsing_mode` disables IRI validity checks and allows rdf4cpp to automatically try to correct some faulty Literals.
See the linked relaxed_parsing_mode for a full list of changes.
Currently aimed at reducing loading errors with DBPedia, more might be added in future versions.

(since rdf4cpp v0.0.24)
