cmake_minimum_required(VERSION 3.22)
project(rdf4cpp VERSION 0.1.3)
set(POBR_VERSION 3)  # Persisted Object Binary Representation

include(cmake/boilerplate_init.cmake)
boilerplate_init()

if (PROJECT_IS_TOP_LEVEL)
    set(CONAN_INSTALL_ARGS "${CONAN_INSTALL_ARGS};-o=boost/*:header_only=True")

    if (BUILD_TESTING)
        set(CONAN_INSTALL_ARGS "${CONAN_INSTALL_ARGS};-o=&:with_test_deps=True")
    endif ()
endif ()

# find packages
find_package(expected-lite REQUIRED)
find_package(Boost REQUIRED)
find_package(re2 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(uni-algo REQUIRED)
find_package(highway REQUIRED)
find_package(dice-hash REQUIRED)
find_package(dice-sparse-map REQUIRED)
find_package(dice-template-library REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/src/rdf4cpp/version.hpp)

set(serd_rev 15c042d29a030c78a3cc3bdf1feef98315df5208)
set(serd_source_files
        include/serd/serd.h
        src/attributes.h
        src/base64.c
        src/base64.h
        src/byte_sink.h
        src/byte_source.c
        src/byte_source.h
        src/env.c
        src/n3.c
        src/node.c
        src/node.h
        src/reader.c
        src/reader.h
        src/serd_config.h
        src/serd_internal.h
        src/stack.h
        src/string.c
        src/string_utils.h
        src/system.c
        src/system.h
        src/try.h
        src/uri.c
        src/uri_utils.h
        src/warnings.h
        src/writer.c
)
set(style_files
    .clang-format
    .clang-tidy
)

foreach(serd_source_file ${serd_source_files})
    file(DOWNLOAD "https://raw.githubusercontent.com/dice-group/serd/${serd_rev}/${serd_source_file}"
            "${CMAKE_CURRENT_BINARY_DIR}/serd/${serd_source_file}"
            TLS_VERIFY ON)
endforeach()
file(DOWNLOAD "https://raw.githubusercontent.com/dice-group/serd/${serd_rev}/COPYING"
        "${CMAKE_CURRENT_BINARY_DIR}/serd/COPYING"
        TLS_VERIFY ON)
foreach(style_file ${style_files})
    file(DOWNLOAD "https://raw.githubusercontent.com/dice-group/tentris-cpp-coding-guidelines/main/${style_file}"
            "${CMAKE_SOURCE_DIR}/${style_file}"
            TLS_VERIFY ON)
endforeach()

list(FILTER serd_source_files INCLUDE REGEX "^.+\\.c$")
list(TRANSFORM serd_source_files PREPEND "${CMAKE_CURRENT_BINARY_DIR}/serd/")

## Create the main rdf4cpp library target
add_library(rdf4cpp
        src/rdf4cpp/BlankNode.cpp
        src/rdf4cpp/ClosedNamespace.cpp
        src/rdf4cpp/Dataset.cpp
        src/rdf4cpp/Graph.cpp
        src/rdf4cpp/IRI.cpp
        src/rdf4cpp/Literal.cpp
        src/rdf4cpp/Namespace.cpp
        src/rdf4cpp/Node.cpp
        src/rdf4cpp/Quad.cpp
        src/rdf4cpp/Statement.cpp
        src/rdf4cpp/bnode_mngt/reference_backends/generator/RandomIdGenerator.cpp
        src/rdf4cpp/bnode_mngt/reference_backends/generator/IncreasingIdGenerator.cpp
        src/rdf4cpp/datatypes/registry/DatatypeRegistry.cpp
        src/rdf4cpp/datatypes/owl/Rational.cpp
        src/rdf4cpp/datatypes/owl/Real.cpp
        src/rdf4cpp/datatypes/rdf/LangString.cpp
        src/rdf4cpp/datatypes/xsd/Base64Binary.cpp
        src/rdf4cpp/datatypes/xsd/Boolean.cpp
        src/rdf4cpp/datatypes/xsd/Decimal.cpp
        src/rdf4cpp/datatypes/xsd/Double.cpp
        src/rdf4cpp/datatypes/xsd/Float.cpp
        src/rdf4cpp/datatypes/xsd/HexBinary.cpp
        src/rdf4cpp/datatypes/xsd/String.cpp
        src/rdf4cpp/datatypes/xsd/AnyURI.cpp
        src/rdf4cpp/datatypes/xsd/integers/non_negative/NonNegativeInteger.cpp
        src/rdf4cpp/datatypes/xsd/integers/non_negative/PositiveInteger.cpp
        src/rdf4cpp/datatypes/xsd/integers/non_negative/UnsignedByte.cpp
        src/rdf4cpp/datatypes/xsd/integers/non_negative/UnsignedInt.cpp
        src/rdf4cpp/datatypes/xsd/integers/non_negative/UnsignedLong.cpp
        src/rdf4cpp/datatypes/xsd/integers/non_negative/UnsignedShort.cpp
        src/rdf4cpp/datatypes/xsd/integers/non_positive/NegativeInteger.cpp
        src/rdf4cpp/datatypes/xsd/integers/non_positive/NonPositiveInteger.cpp
        src/rdf4cpp/datatypes/xsd/integers/signed/Byte.cpp
        src/rdf4cpp/datatypes/xsd/integers/signed/Int.cpp
        src/rdf4cpp/datatypes/xsd/integers/signed/Integer.cpp
        src/rdf4cpp/datatypes/xsd/integers/signed/Long.cpp
        src/rdf4cpp/datatypes/xsd/integers/signed/Short.cpp
        src/rdf4cpp/datatypes/xsd/time/Year.cpp
        src/rdf4cpp/datatypes/xsd/time/Month.cpp
        src/rdf4cpp/datatypes/xsd/time/Day.cpp
        src/rdf4cpp/datatypes/xsd/time/YearMonth.cpp
        src/rdf4cpp/datatypes/xsd/time/MonthDay.cpp
        src/rdf4cpp/datatypes/xsd/time/Date.cpp
        src/rdf4cpp/datatypes/xsd/time/Time.cpp
        src/rdf4cpp/datatypes/xsd/time/DateTime.cpp
        src/rdf4cpp/datatypes/xsd/time/DateTimeStamp.cpp
        src/rdf4cpp/datatypes/xsd/time/Duration.cpp
        src/rdf4cpp/datatypes/xsd/time/DayTimeDuration.cpp
        src/rdf4cpp/datatypes/xsd/time/YearMonthDuration.cpp
        src/rdf4cpp/namespaces/RDF.cpp
        src/rdf4cpp/parser/IStreamQuadIterator.cpp
        src/rdf4cpp/parser/RDFFileParser.cpp
        src/rdf4cpp/query/QuadPattern.cpp
        src/rdf4cpp/query/Solution.cpp
        src/rdf4cpp/query/TriplePattern.cpp
        src/rdf4cpp/query/Variable.cpp
        src/rdf4cpp/regex/Regex.cpp
        src/rdf4cpp/regex/RegexReplacer.cpp
        src/rdf4cpp/util/CharMatcher.cpp
        src/rdf4cpp/storage/NodeStorage.cpp
        src/rdf4cpp/storage/reference_node_storage/SyncReferenceNodeStorage.cpp
        src/rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.cpp
        src/rdf4cpp/storage/view/BNodeBackendView.cpp
        src/rdf4cpp/storage/view/IRIBackendView.cpp
        src/rdf4cpp/storage/view/LiteralBackendView.cpp
        src/rdf4cpp/storage/view/VariableBackendView.cpp
        src/rdf4cpp/writer/SerializationState.cpp
        src/rdf4cpp/IRIView.cpp
        src/rdf4cpp/IRIFactory.cpp
        private/rdf4cpp/parser/IStreamQuadIteratorSerdImpl.cpp
        private/rdf4cpp/regex/RegexImpl.cpp
        private/rdf4cpp/regex/RegexReplacerImpl.cpp
        ${serd_source_files}
)

add_library(rdf4cpp::rdf4cpp ALIAS rdf4cpp)

target_include_directories(
        rdf4cpp
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
        PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/private>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/serd/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/serd/src>"
)

target_link_libraries(rdf4cpp
        PUBLIC
        nonstd::expected-lite
        Boost::headers
        dice-hash::dice-hash
        dice-sparse-map::dice-sparse-map
        dice-template-library::dice-template-library
        PRIVATE
        re2::re2
        OpenSSL::Crypto
        uni-algo::uni-algo
        highway::highway
        )

set_target_properties(rdf4cpp PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON
)

if (PROJECT_IS_TOP_LEVEL)
    include(cmake/install_library.cmake)
    install_cpp_library(rdf4cpp src)
endif ()

if (PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
    message("Tests are configured to be build.")
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif ()

OPTION(BUILD_EXAMPLES "Build the examples for rdf4cpp." OFF)
if (PROJECT_IS_TOP_LEVEL AND BUILD_EXAMPLES)
    message("Examples are configured to be build.")
    add_subdirectory(examples)
endif ()
