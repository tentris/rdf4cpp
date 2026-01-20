[![Read the Docs](https://img.shields.io/readthedocs/rdf4cpp)](https://rdf4cpp.readthedocs.io/en/latest/)
[![Conan](https://img.shields.io/badge/conan-package-blue)](https://conan.dice-research.org/ui/packages/conan:%2F%2Frdf4cpp) 
[![GitHub Release](https://img.shields.io/github/v/release/rdf4cpp/rdf4cpp)](https://github.com/rdf4cpp/rdf4cpp/releases)
![GitHub License](https://img.shields.io/github/license/rdf4cpp/rdf4cpp)


# rdf4cpp

_rdf4cpp_ is a modern C++20 library providing basic RDF support.

The focus is **correctness**, **performance** and **ease-of-use** for **basic building blocks** like:

- parsing, validating and writing RDF data ([N-Triples](https://www.w3.org/TR/n-triples/), [Turtle](https://www.w3.org/TR/turtle/), [N-Quads](https://www.w3.org/TR/n-quads/), [TriG](https://www.w3.org/TR/trig/))
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
rdf4cpp/0.1.12
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

### Limits for Datatypes
By default, unlimited precision datatypes are limited in accordance with https://www.w3.org/TR/xmlschema11-2/#partial-implementation .

For `http://www.w3.org/2001/XMLSchema#integer`
(and the related: `http://www.w3.org/2001/XMLSchema#nonNegativeInteger`
`http://www.w3.org/2001/XMLSchema#positiveInteger` `http://www.w3.org/2001/XMLSchema#nonPositiveInteger`
`http://www.w3.org/2001/XMLSchema#negativeInteger`) this limit is a signed 128-bit integer
with the usual range of `[-2^127,2^127-1]`

And `http://www.w3.org/2001/XMLSchema#decimal` is composed of the following parts: `i/10^k`,
where `i` is a signed 128-bit integer (`[-2^127,2^127-1]`) and `k` is an unsigned 64-bit integer (`[0,2^64]`).

For `http://www.w3.org/2001/XMLSchema#dateTime` (and all derived types) there are 2 limits:
- represented as a time point with nanosecond precision with a 128-bit signed integer
- the year part alone in a 64-bit signed integer
Both limits are enough to cover both the current best theories of the big bang and the projected heat death of the universe.

For `http://www.w3.org/2001/XMLSchema#duration` (and all derived types), both parts have a separate signed 64-bit integer
and with it its associated limits. The seconds part of the duration supports nanosecond precision.

### Additional CMake config options:

- `-DBUILD_EXAMPLES=ON/OFF [default: OFF]`: Build the examples.
- `-DBUILD_TESTING=ON/OFF [default: OFF]`: Build the tests.
- `-DBUILD_SHARED_LIBS=ON/OFF [default: OFF]`: Build a shared library instead of a static one.


## Supported Platforms
- **Linux distributions (x86-64, aarch64)** (e.g. Ubuntu>=24.04, Fedora>=41, etc.) with:
    - gcc>=14 (libstdc++>=14; used with both GCC and Clang)
    - clang>=19* (on aarch64 clang>=20 is required)
    - glibc>=2.35 or musl>=1.2.4
- **macOS (aarch64)**: macOS Sonoma (>=14) with GCC>=14 (via Homebrew)

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
