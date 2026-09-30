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


Limits for Datatypes
++++++++++++++++++++
By default, unlimited precision datatypes are limited in accordance with https://www.w3.org/TR/xmlschema11-2/#partial-implementation .

For :code:`http://www.w3.org/2001/XMLSchema#integer`
(and the related: :code:`http://www.w3.org/2001/XMLSchema#nonNegativeInteger`
:code:`http://www.w3.org/2001/XMLSchema#positiveInteger` :code:`http://www.w3.org/2001/XMLSchema#nonPositiveInteger`
:code:`http://www.w3.org/2001/XMLSchema#negativeInteger`) this limit is a signed 128-bit integer
with the usual range of :code:`[-2^127,2^127-1]`

And :code:`http://www.w3.org/2001/XMLSchema#decimal` is composed of the following parts: :code:`i/10^k`,
where :code:`i` is a signed 128-bit integer (:code:`[-2^127,2^127-1]`) and :code:`k` is an unsigned 32-bit integer (:code:`[0,2^32-1]`).

For :code:`http://www.w3.org/2001/XMLSchema#dateTime` (and all derived types) there are 2 limits:

* represented as a time point with nanosecond precision with a 128-bit signed integer
* the year part alone in a 64-bit signed integer

Both limits are enough to cover both the current best theories of the big bang and the projected heat death of the universe.

For :code:`http://www.w3.org/2001/XMLSchema#duration` (and all derived types), both parts have a separate signed 64-bit integer
and with it its associated limits. The seconds part of the duration supports nanosecond precision.


Parsing Files
-------------

The class :class:`rdf4cpp::parser::RDFFileParser` allows reading files containing rdf Statements and iterate over them.
Supported Formats: Turtle, TriG, N-Triples N-Quads, RDF/XML and JSON-LD.
:class:`rdf4cpp::parser::IStreamQuadIterator` allows doing the same over arbitrary data streams.

JSON-LD Limitations
___________________

The JSON-LD parser implements the processing mode json-ld-1.1 of `<https://www.w3.org/TR/json-ld11-api/>`_.
The following parts are missing:

 * A context given as an IRI, as ``@context`` or via ``@import``, is only fetched through the callback ``request_url`` of :class:`rdf4cpp::parser::ParsingState`. The default callback fetches nothing, so both are reported as a parsing error.
 * The processing mode json-ld-1.0 is not offered. A document that is valid in json-ld-1.1 and only invalid in json-ld-1.0 is parsed, not reported. This covers a redefined keyword, ``@propagate`` outside a scoped context, ``@type: @none``, ``@container: @id``, a list inside a list, and a relative IRI used as a property together with ``@vocab``.
 * Literals of the datatype rdf:JSON keep the JSON of the document. They are not canonicalized as `<https://www.rfc-editor.org/rfc/rfc8785>`_ describes.
 * Parsing errors carry no line and no column, both are reported as 0.

The parser reads the whole document before it expands it. The key order rules of the streaming
profile therefore do not apply to it: a document that names ``@context`` after a property is parsed.

Relaxed Parsing Mode
--------------------

The setting :var:`rdf4cpp::datatypes::registry::relaxed_parsing_mode` disables IRI validity checks and allows rdf4cpp to automatically try to correct some faulty Literals.
See the linked relaxed_parsing_mode for a full list of changes.
Currently aimed at reducing loading errors with DBPedia, more might be added in future versions.

(since rdf4cpp v0.0.24)
