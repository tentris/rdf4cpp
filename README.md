⚠️ This repo is work-in-progress! Before v0.1.0 all APIs are considered unstable and might be subject to change. ⚠️

# rdf4cpp

rdf4cpp aims to be a stable, modern RDF library for C++, implementing the following standards:
- [W3C XML Schema Definition Language (XSD) 1.1 Part 2: Datatypes](https://www.w3.org/TR/xmlschema11-2/)
- [XPath and XQuery Functions and Operators 3.1 ](https://www.w3.org/TR/xpath-functions-31/)
- [OWL Real and Rational](https://www.w3.org/TR/owl2-syntax/#Datatype_Maps)

Current documentation: https://rdf4cpp.readthedocs.io/en/latest/

## Usage 
check out the [examples](./examples) directory. 

### As Conan Package

Until its first stable release, rdf4cpp will not be available via Conan Center. Instead, it is available via the artifactory of the [DICE Research Group](https://dice-research.org/). 

You need the [package manager Conan](https://conan.io/downloads.html) installed and set up. You can add the DICE artifactory with:
```shell
conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris
```

To use rdf4cpp, add it to your `conanfile.txt`:
```
[requires]
rdf4cpp/0.0.53
```

Note:

If you want to include rdf4cpp without using conan, make sure you also include its dependencies exposed via the rdf4cpp API.

## Build

### Requirements

Currently, rdf4cpp builds only on linux with a C++20 compatible compiler. 
CI builds and tests rdf4cpp with gcc-{13}, clang-{17,18,19} with libstdc++-13 on ubuntu 22.04. 

### Dependencies
It is recommended to include build dependencies via conan. Set up Conan as follows on Ubuntu 22.04+:
```shell
sudo apt install python3-pip
pip3 install --user conan
conan user
conan profile new --detect default
conan profile update settings.compiler.libcxx=libstdc++11 default
conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris
```


### Compile
rdf4cpp uses CMake and conan 2. To build it, run: 
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

`-DBUILD_EXAMPLES=ON/OFF [default: OFF]`: Build the examples.

`-DBUILD_TESTING=ON/OFF [default: OFF]`: Build  the tests.

`-DBUILD_SHARED_LIBS=ON/OFF [default: OFF]`: Build a shared library instead of a static one.
