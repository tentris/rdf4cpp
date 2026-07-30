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
Supported Formats: Turtle, TriG, N-Triples N-Quads, RDF/XML and JSON-LD.
:class:`rdf4cpp::parser::IStreamQuadIterator` allows doing the same over arbitrary data streams.

JSON-LD Limitations
___________________

The JSON-LD parser implements the processing mode json-ld-1.1 of `<https://www.w3.org/TR/json-ld11-api/>`_.
The following parts are missing:

 * A context given as an IRI is not fetched, neither as ``@context`` nor via ``@import``. Both are reported as a parsing error.
 * The processing mode json-ld-1.0 is not offered. Documents that are only invalid in 1.0, for example a document redefining a keyword, are parsed instead of reported.
 * Literals of the datatype rdf:JSON keep the JSON of the document. They are not canonicalized as `<https://www.rfc-editor.org/rfc/rfc8785>`_ describes.
 * Parsing errors carry no line and no column, both are reported as 0.
 * Three checks of the specification are missing. A list inside a list, a relative IRI used as a property together with ``@vocab``, and some invalid container mappings are parsed instead of reported.

The parser reads the whole document before it expands it. The key order rules of the streaming
profile therefore do not apply to it: a document that names ``@context`` after a property is parsed.

Relaxed Parsing Mode
--------------------

The setting :var:`rdf4cpp::datatypes::registry::relaxed_parsing_mode` disables IRI validity checks and allows rdf4cpp to automatically try to correct some faulty Literals.
See the linked relaxed_parsing_mode for a full list of changes.
Currently aimed at reducing loading errors with DBPedia, more might be added in future versions.

(since rdf4cpp v0.0.24)
