# rdf4cpp

rdf4cpp aims to be a stable, modern C++ library for parsing, storing, transforming, and serializing RDF data.
Importantly, rdf4cpp is **not** a SPARQL query processor (it only provides very basic triple pattern matching); 
but a SPARQL processor could be built based on rdf4cpp.

We implement the following standards:
- [W3C XML Schema Definition Language (XSD) 1.1 Part 2: Datatypes](https://www.w3.org/TR/xmlschema11-2/)
- [OWL Real and Rational](https://www.w3.org/TR/owl2-syntax/#Datatype_Maps)
- [XPath and XQuery Functions and Operators 3.1 ](https://www.w3.org/TR/xpath-functions-31/) (partially)

Current documentation: https://rdf4cpp.readthedocs.io/en/latest/

## Usage
rdf4cpp is consumed via `conan 2`, but it is not available via [Conan Center](https://conan.io/center).
Instead, it can be found on the artifactory of the [DICE Research Group](https://dice-research.org/). 

You need the package manager [conan](https://conan.io/downloads.html) installed and set up. You can add the DICE artifactory with:
```shell
conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris
```

To use rdf4cpp, add it to your `conanfile.txt`:
```
[requires]
rdf4cpp/0.1.0
```

To get started, check out the [examples](./examples) directory.

## Build
### Requirements

Currently, rdf4cpp builds only on linux with a C++20 compatible compiler. 
CI builds and tests rdf4cpp with `gcc-13` and `clang-{17,18,19}` with `libstdc++-13` on `ubuntu-22.04`. 

### Compile
rdf4cpp uses CMake and Conan 2. To build it, run: 
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


## Stability
### API Stability
From version 0.1 onwards (before 1.0.0), all high-level public API that the average user is expected to interact with is considered stable.
This includes basically everything, except what is in the `rdf4cpp::storage` and `rdf4cpp::datatypes::registry` namespaces.
Should we ever break anything in these high-level interfaces, we will bump the minor version (for example, from 0.1.0 to 0.2.0).

### ABI Stability
We do not promise any ABI stability.

### POBR Stability
In rdf4cpp we track an additional version, the Persisted Object Binary Representation (POBR) version.
POBR stability includes everything meant to be persistable to disk (e.g., with a persistent allocator like [Metall](https://github.com/LLNL/metall)).
This includes everything in `rdf4cpp::storage::identifiers` but nothing else.
The current POBR version can be retrieved via `rdf4cpp::pobr_version`.
