cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(rdf4cpp REQUIRED)

add_executable(example example.cpp)

target_link_libraries(example PUBLIC
        rdf4cpp::rdf4cpp)

set_target_properties(
        example PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)
