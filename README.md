[![Read the Docs](https://img.shields.io/readthedocs/rdf4cpp)](https://rdf4cpp.readthedocs.io/en/latest/)
[![Conan](https://img.shields.io/badge/conan-package-blue)](https://conan.dice-research.org/ui/packages/conan:%2F%2Frdf4cpp) 
[![GitHub Release](https://img.shields.io/github/v/release/rdf4cpp/rdf4cpp)](https://github.com/rdf4cpp/rdf4cpp/releases)
![GitHub License](https://img.shields.io/github/license/rdf4cpp/rdf4cpp)


# rdf4cpp

_rdf4cpp_ is a modern C++23 library providing basic RDF support.

The focus is **correctness**, **performance** and **ease-of-use** for **basic building blocks** like:

- parsing, validating and writing RDF data ([N-Triples](https://www.w3.org/TR/n-triples/), [Turtle](https://www.w3.org/TR/turtle/), [N-Quads](https://www.w3.org/TR/n-quads/), [TriG](https://www.w3.org/TR/trig/))
- parsing and validating [RDF/XML](https://www.w3.org/TR/rdf11-xml/) and [JSON-LD](https://www.w3.org/TR/json-ld11/) (json-ld-1.1 without remote contexts, see the users guide for the limitations)
- Complete and extensible literal datatypes (validation, functions, operations, subtype and promotion casting, mapping to C++ types, error handling, ...) 
- Managing RDF nodes efficiently
- Blank node scoping (e.g., for RDF datasets)

_rdf4cpp_ is **not** a **SPARQL engine** or **reasoning engine**, although it provides very basic support for triple/quad
pattern matching on RDF graphs/datasets. _rdf4cpp_ rather provides the necessary primitives to implement such engines.

We implement the following W3C standards:

- [RDF 1.1 Concepts and Abstract Syntax](https://www.w3.org/TR/rdf11-concepts/)
- [XML XSD 1.1 Part 2: Datatypes](https://www.w3.org/TR/xmlschema11-2/) (RDF related parts)
- [OWL Real and Rational](https://www.w3.org/TR/owl2-syntax/#Datatype_Maps)
- [XPath and XQuery Functions and Operators 3.1](https://www.w3.org/TR/xpath-functions-31/) (SPARQL related parts)

## JSON-LD conformance

`tests/parser/tests_JSON_LD_Parser.cpp` runs the [json-ld-streaming](https://github.com/w3c/json-ld-streaming)
test suite. The tests below are deactivated there. Every entry names the reason.

Tests of json-ld-1.0. This parser implements json-ld-1.1 only, and each of these documents is either
valid in 1.1 or needs a 1.0 processing mode:

| test | what it needs |
|---|---|
| `0118` | the generalized RDF flag, a blank node as predicate |
| `e026`, `e071` | 1.0 term semantics, the same documents are the negative tests `er43` and `er44` in 1.1 |
| `e075` | `@vocab` as a blank node identifier, deprecated in 1.1 |
| `c029` | `@propagate` reported outside a scoped context |
| `e115`, `e116` | a relative IRI as a property with `@vocab` reported |
| `ep02` | the processing mode json-ld-1.0, conflicting with `@version` |
| `er21` | `@container: @id` reported, `m001` and `m002` cover the 1.1 behavior |
| `er24`, `er32` | a list inside a list reported, `li01` and `li02` cover the 1.1 behavior |
| `er42` | a redefined keyword reported |
| `tn01` | `@type: @none` reported |

Missing features:

| test | what it needs |
|---|---|
| `c031`, `c034`, `e126`, `e127`, `e128` | fetching a context named by an IRI, see [#431](https://github.com/rdf4cpp/rdf4cpp/issues/431) |
| `so05`, `so06`, `so08`, `so09`, `so11` | `@import`, which also fetches a context by IRI |
| `e077` | an expandContext option on the parser, the document carries no context |
| `js06` to `js16`, `js19`, `js20`, `js21` | canonicalization of rdf:JSON literals after [RFC 8785](https://www.rfc-editor.org/rfc/rfc8785) |

Different behavior on purpose:

| test | what it expects |
|---|---|
| `wf01` to `wf04`, `wf07`, `e111`, `e112` | a triple with an invalid IRI dropped without a message. This parser reports a `ParsingError` for it, like the other parsers do. |
| `wf05` | a triple with an invalid language tag dropped without a message. This parser reports a `ParsingError` for it. |
| `0035` | the literal `"9.9E0"^^xsd:integer`, whose lexical form does not fit its datatype. rdf4cpp validates literals, the test above it covers the rest of the document. |
| `se01` to `se09` | the key order of the streaming profile, `@context` before `@id` and before the properties. This parser reads the whole document before it expands it, so the key order does not change its result. |

Open bug:

| test | cause |
|---|---|
| `e109` | `IRIView` takes the text before the first `:` as a scheme even when a `?` or `#` comes first, so a fragment containing `:` is reported as an invalid scheme. The fix changes IRI resolution for every parser and needs its own version bump. |

## Example

```c++
#include <iostream>
#include <rdf4cpp.hpp>

int main() {
    using namespace ::rdf4cpp;
    using namespace ::rdf4cpp::shorthands;
    using namespace ::rdf4cpp::namespaces;
    using namespace ::rdf4cpp::datatypes;

    /// 1) basic dataset, graph and RDF node usage
    // using namespaces
    FOAF foaf{};                               // common, predefined namespace
    Namespace const ex{"http://example.com/"}; // self-declared namespace

    Dataset dataset;
    // populate a named graph in the dataset
    auto &graph = dataset.graph(IRI{"http://ex.com/MyGraph"});                                                  // IRI constructor
    graph.add({"http://example.com/Bob"_iri, "http://example.com/knows"_iri, "http://example.com/Alice"_iri});  // IRI shorthand
    graph.add({ex + "Alice", foaf + "knows", ex + "Bob"});                                                      // using namespaces
    graph.add({ex + "Bob", foaf + "name", "Bob"_xsd_string}); // Literal datatype shorthand

    // serialize the dataset as N-Quads
    std::cout << "Dataset as N-Quads: \n"
              << dataset << std::endl;

    // 2) Using datatypes and arithmetics
    // typed Literal instantiation
    auto const d = Literal::make_typed_from_value<xsd::Double>(2.3); // factory function
    auto const ui = 42_xsd_uint;         // Literal datatype shorthand
    auto const dec = "42.1"_xsd_decimal; // infinite precision decimals

    // basic arithmetics with automatic result type deduction
    auto const r1 = d * dec;            // double * decimal → double
    auto const r2 = (ui + dec).round(); // round(integer + decimal) → decimal

    std::cout << "Using XSD datatypes, functions and operators: \n"
              << std::format("{} * {} = {}\n", d, dec, r1)
              << std::format("ceil({} + {}) = {}", ui, dec, r2) << std::endl;

    return 0;
}
```

## Using _rdf4cpp_

_rdf4cpp_ is consumed via Conan 2 but it is not available via [Conan Center](https://conan.io/center).
Instead, it can be found on the artifactory of the [DICE Research Group](https://dice-research.org/).

You need the package manager [conan](https://conan.io/downloads.html) installed and set up. You can add the DICE
artifactory with:

```shell
conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris
```

To use _rdf4cpp_, add it to your `conanfile.txt`:

```
[requires]
rdf4cpp/0.2.1
```

For getting started how to use rdf4cpp, check out the [examples](./examples) directory and refer to our documentation.



## Developing _rdf4cpp_

### Compile

_rdf4cpp_ uses CMake and Conan 2. To build it, run:

```shell
wget https://github.com/conan-io/cmake-conan/raw/develop2/conan_provider.cmake -O conan_provider.cmake # download conan provider
cmake -B build_dir -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake # configure and generate
cmake --build build_dir # compile
```

To install it to your system, run afterward:

```shell
cd build_dir
sudo make install
```

### Additional CMake config options:

- `-DBUILD_EXAMPLES=ON/OFF [default: OFF]`: Build the examples.
- `-DBUILD_TESTING=ON/OFF [default: OFF]`: Build the tests.
- `-DBUILD_SHARED_LIBS=ON/OFF [default: OFF]`: Build a shared library instead of a static one.


## Supported Platforms
- **Linux distributions (x86-64, AArch64)** (e.g. Ubuntu>=24.04, Fedora>=41, etc.) with:
    - GCC>=14 (libstdc++>=14; used with both GCC and Clang)
    - Clang>=19
    - glibc 2.35+ or musl 1.2.4+
- **macOS (ARM64)**: macOS Sonoma (14)+ with GCC>=14 (via Homebrew)

## Stability

### API Stability

From version 0.1 onwards (before 1.0.0), all high-level public API that the average user is expected to interact with is
considered stable.
This includes basically everything, except what is in the `rdf4cpp::storage` and `rdf4cpp::datatypes::registry`
namespaces.
Should we ever break anything in these high-level interfaces, we will bump the minor version (for example, from 0.1.0 to
0.2.0).

### ABI Stability

ABI stability is not guaranteed.

### POBR Stability

The POBR (Persisted Object Binary Representation) version tracks on-disk format stability (e.g., with allocators
like [Metall](https://github.com/LLNL/metall)).
This includes everything in `rdf4cpp::storage::identifiers` but nothing else.
The current POBR version can be retrieved via `rdf4cpp::pobr_version`.
