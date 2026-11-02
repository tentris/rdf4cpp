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

* Conan >= 2.28
* CMake >= 3.28
* Clang >= 21 or GCC >= 14
* Ninja
* `mold linker <https://github.com/rui314/mold>`_ (Linux only)

Dependencies
____________

    sudo apt install python3-pip
    pip3 install --user "conan"
    conan profile detect
    conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris


Compile
_______

rdf4cpp is built via the presets in :code:`CMakePresets.json`. To build and test it, run: ::

    cmake --preset dev
    cmake --build --preset dev
    ctest --preset dev

To install it to your system, run: ::

    cmake --preset release
    cmake --build --preset release
    sudo cmake --install build/release


Additional CMake config options:
________________________________

* :code:`-DBUILD_EXAMPLES=ON/OFF [default: OFF]`: Build the examples.
* :code:`-DBUILD_TESTING=ON/OFF [default: OFF]`: Build  the tests.
* :code:`-DBUILD_SHARED_LIBS=ON/OFF [default: OFF]`: Build a shared library instead of a static one.
