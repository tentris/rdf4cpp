# Conan host profile that strips the dependencies of rdf4cpp down to what is actually used.
#
# Conan feature resolution is not additive (e.g. Cargo, vcpkg) but depends on which package is declared as a dependency first and how that package sets them.
# The only way to have this work properly (apart from hosting our own recipes for dependencies) is to error if our dependent hasn't set the features we need.
# We do this in libraries that actually link dependencies (e.g. validate() in tentris-lib's conanfile.py). Hypertrie is header only so does not need this.
# If hypertrie or a dependent needs a feature then define it here.

[options]
# boost: compiled libraries that are linked somewhere in the tentris stack
# (conan profiles only allow comments on their own line)
# charconv: rdf4cpp
boost/*:without_charconv=False
# serialization: tentris-storage links Boost::serialization directly
boost/*:without_serialization=False

# boost: everything else is unused or used header-only (e.g. container's flat_map, graph's write_graphviz, process v1)
boost/*:without_atomic=True
boost/*:without_chrono=True
boost/*:without_cobalt=True
boost/*:without_container=True
boost/*:without_context=True
boost/*:without_contract=True
boost/*:without_coroutine=True
boost/*:without_date_time=True
boost/*:without_exception=True
boost/*:without_fiber=True
boost/*:without_filesystem=True
boost/*:without_graph=True
boost/*:without_graph_parallel=True
boost/*:without_iostreams=True
boost/*:without_json=True
boost/*:without_locale=True
boost/*:without_log=True
boost/*:without_math=True
boost/*:without_mpi=True
boost/*:without_nowide=True
boost/*:without_process=True
boost/*:without_program_options=True
boost/*:without_python=True
boost/*:without_random=True
boost/*:without_regex=True
boost/*:without_stacktrace=True
boost/*:without_test=True
boost/*:without_thread=True
boost/*:without_timer=True
boost/*:without_type_erasure=True
boost/*:without_url=True
boost/*:without_wave=True

# boost: optional dependencies, all only used by libraries disabled above
boost/*:bzip2=False
boost/*:lzma=False
boost/*:zlib=False
boost/*:zstd=False
boost/*:numa=False
boost/*:multithreading=False
boost/*:with_stacktrace_backtrace=False

# pcre2: rdf4cpp needs the 8-bit library with JIT, nothing else
pcre2/*:support_jit=True
pcre2/*:build_pcre2_8=True
pcre2/*:build_pcre2_16=False
pcre2/*:build_pcre2_32=False
pcre2/*:build_pcre2grep=False
pcre2/*:with_zlib=False
pcre2/*:with_bzip2=False

# botan: only the hashes and RNGs rdf4cpp uses (Literal::hash_with, util/Anonymizer.cpp)
botan/*:enable_modules=md5,sha1,sha2_32,sha2_64,auto_rng,system_rng

# libxml2: rdf4cpp only parses from memory
libxml2/*:iconv=False
libxml2/*:zlib=False
libxml2/*:programs=False

# simdjson: rdf4cpp parses single-threaded
simdjson/*:threads=False

[conf]
# pcre2: its CMakeLists looks for BZip2 and ZLIB even when they are disabled above, and a broken system config
# (e.g. Fedora's zlib-ng-compat-devel without -static) then fails the build
pcre2/*:tools.cmake.cmaketoolchain:extra_variables={"CMAKE_DISABLE_FIND_PACKAGE_BZip2": True, "CMAKE_DISABLE_FIND_PACKAGE_ZLIB": True}
