Getting Started
===============

.. highlight:: bash

Usage
-----

.. literalinclude:: ../../examples/getting_started.cpp
    :language: cpp

Installation
------------
Until its first stable release, rdf4cpp will not be available via Conan Center. Instead, it is available via the artifactory of the `DICE Research Group <https://dice-research.org/>`_.

You need the package manager `Conan <https://conan.io/downloads.html>`_ version 1 installed and set up. You can add the DICE artifactory with: ::

    conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris


To use rdf4cpp, add it to your :code:`conanfile.txt`:

.. parsed-literal::

    [requires]
    rdf4cpp/\ |release|

.. note::

    If you want to include rdf4cpp without using conan, make sure you also include its dependencies exposed via the rdf4cpp API.

Build
-----

Requirements
____________

Currently, rdf4cpp builds only on linux with a C++20 compatible compiler.
CI builds and tests rdf4cpp with gcc-{13}, clang-{15,16} with libstdc++-13 on ubuntu 22.04.

Dependencies
____________

It is recommended to include build dependencies via conan. Set up Conan as follows on Ubuntu 22.04+: ::

    sudo apt install python3-pip
    pip3 install --user "conan<2"
    conan user
    conan profile new --detect default
    conan profile update settings.compiler.libcxx=libstdc++13 default
    conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris


Compile
_______

rdf4cpp uses CMake. To build it, run: ::

    cmake -B build_dir # configure and generate
    cmake --build build_dir # compile


To install it to your system, run afterward: ::

    sudo make install


Additional CMake config options:
________________________________

* :code:`-DBUILD_EXAMPLES=ON/OFF [default: OFF]`: Build the examples.
* :code:`-DBUILD_TESTING=ON/OFF [default: OFF]`: Build  the tests.
* :code:`-DBUILD_SHARED_LIBS=ON/OFF [default: OFF]`: Build a shared library instead of a static one.
* :code:`-DRDF4CPP_USE_UNLIMITED_DATATYPES=ON`: If defined, uses unlimited datatypes for integer and decimal types.
