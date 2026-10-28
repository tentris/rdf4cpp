import os
import re

from conan.tools.cmake import cmake_layout, CMake

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration

from conan.tools.files import load, copy, rmdir


# the botan modules rdf4cpp needs, see Literal::hash_with and util/Anonymizer.cpp
_BOTAN_MODULES = ("md5", "sha1", "sha2_32", "sha2_64", "auto_rng", "system_rng")


class Recipe(ConanFile):
    author = "https://github.com/rdf4cpp"
    url = "https://github.com/rdf4cpp/rdf4cpp"
    license = "MIT"
    description = "rdf4cpp aims to be a stable, modern RDF library for C++."
    topics = "rdf", "semantic-web", "sparql", "knowledge-graphs", "C++23"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_test_deps": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_test_deps": False,
    }
    exports = "LICENSE",
    exports_sources = "src/*", "private/*", "CMakeLists.txt", "cmake/*"
    generators = ("CMakeDeps", "CMakeToolchain")

    def requirements(self):
        # only charconv is compiled, everything else we use from boost is header-only
        boost_unused_libs = ("atomic", "chrono", "cobalt", "container", "context", "contract", "coroutine",
                             "date_time", "exception", "fiber", "filesystem", "graph", "graph_parallel",
                             "iostreams", "json", "locale", "log", "math", "mpi", "nowide", "process",
                             "program_options", "python", "random", "regex", "serialization", "stacktrace",
                             "test", "thread", "timer", "type_erasure", "url", "wave")
        self.requires("boost/1.91.0", transitive_headers=True,  # >= 1.91 because <=1.90 has a bug in charconv for __int128
                      options={f"without_{lib}": True for lib in boost_unused_libs})
        self.requires("pcre2/10.49", options={"support_jit": True,
                                              "build_pcre2_16": False,
                                              "build_pcre2_32": False,
                                              "build_pcre2grep": False,
                                              "with_zlib": False,
                                              "with_bzip2": False})
        self.requires("botan/3.13.0", options={"enable_modules": ",".join(_BOTAN_MODULES)})
        self.requires("uni-algo/1.2.0")
        self.requires("highway/1.4.0")
        self.requires("dice-hash/0.5.0", transitive_headers=True)
        self.requires("dice-sparse-map/0.3.0", transitive_headers=True)
        self.requires("dice-template-library/2.8.0", transitive_headers=True)
        self.requires("libxml2/2.15.3", options={"iconv": False, "zlib": False, "programs": False})
        self.requires("simdjson/4.6.3", options={"threads": False})
        self.requires("dragonbox/1.1.3")

        if self.options.with_test_deps:
            self.test_requires("doctest/2.4.11")
            self.test_requires("nanobench/4.3.11")

    def validate(self):
        # Conan feature resolution is not additive (e.g. Cargo, vcpkg) but depends on which package is declared as a dependency first and how that package sets them.
        # The only way to have this work properly (apart from hosting our own recipes for dependencies) is to error if our dependent hasn't set the features we need
        boost = self.dependencies["boost"].options
        if boost.header_only or boost.without_charconv:
            raise ConanInvalidConfiguration("rdf4cpp needs the compiled Boost.Charconv, "
                                            "set boost/*:header_only=False and boost/*:without_charconv=False")

        if not self.dependencies["pcre2"].options.build_pcre2_8:
            raise ConanInvalidConfiguration("rdf4cpp needs the 8-bit pcre2 library, set pcre2/*:build_pcre2_8=True")

        botan = self.dependencies["botan"].options
        enabled = set(str(botan.enable_modules).split(",")) if botan.enable_modules != None else set(_BOTAN_MODULES)
        disabled = set(str(botan.disable_modules).split(",")) if botan.disable_modules != None else set()
        missing = [m for m in _BOTAN_MODULES if m not in enabled or m in disabled]
        if missing:
            raise ConanInvalidConfiguration(f"rdf4cpp needs the botan modules {','.join(missing)}, "
                                            f"list them in botan/*:enable_modules and not in botan/*:disable_modules")

    def set_name(self):
        if not hasattr(self, 'name') or self.version is None:
            cmake_file = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.name = re.search(r"project\(\s*([a-z0-9\-]+)\s+VERSION", cmake_file).group(1)

    def set_version(self):
        if not hasattr(self, 'version') or self.version is None:
            cmake_file = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.version = re.search(r"project\([^)]*VERSION\s+(\d+\.\d+.\d+)[^)]*\)", cmake_file).group(1)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "cmake"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        copy(self, "LICENSE", src=self.recipe_folder, dst=os.path.join(self.package_folder, "licenses"))
        copy(self, os.path.join("serd", "COPYING"), src=self.build_folder, dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.libs = ["rdf4cpp"]
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_target_name", "rdf4cpp::rdf4cpp")
        self.cpp_info.set_property("cmake_file_name", "rdf4cpp")
