import os
import re

from conan.tools.cmake import cmake_layout, CMake

from conan import ConanFile

from conan.tools.files import load, copy, rmdir


class Recipe(ConanFile):
    author = "https://github.com/rdf4cpp"
    url = "https://github.com/rdf4cpp/rdf4cpp"
    license = "MIT"
    description = "rdf4cpp aims to be a stable, modern RDF library for C++."
    topics = "rdf", "semantic-web", "sparql", "knowledge-graphs", "C++20"
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
        self.requires("boost/1.86.0", transitive_headers=True, libs=False)
        self.requires("expected-lite/0.8.0", transitive_headers=True)
        self.requires("re2/20230301") # using latest version that does not require abseil
        self.requires("openssl/3.3.2")
        self.requires("uni-algo/1.2.0")
        self.requires("highway/1.2.0")
        self.requires("dice-hash/0.4.11", transitive_headers=True)
        self.requires("dice-sparse-map/0.2.9", transitive_headers=True)
        self.requires("dice-template-library/1.19.0", transitive_headers=True)
        self.requires("libxml2/2.15.0")

        if self.options.with_test_deps:
            self.test_requires("doctest/2.4.11")
            self.test_requires("nanobench/4.3.11")

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
