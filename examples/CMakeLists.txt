set(CMAKE_CXX_STANDARD 23)

add_executable(graphs_and_datasets_example graphs_and_datasets.cpp)
target_link_libraries(graphs_and_datasets_example PRIVATE
        rdf4cpp::rdf4cpp
        )

add_executable(literal_datatypes_example literal_datatypes.cpp)
target_link_libraries(literal_datatypes_example PRIVATE
        rdf4cpp::rdf4cpp
        )

add_executable(rdf_nodes_example rdf_nodes.cpp)
target_link_libraries(rdf_nodes_example PRIVATE
        rdf4cpp::rdf4cpp
        )

add_executable(namespaces_example namespaces.cpp)
target_link_libraries(namespaces_example PRIVATE
        rdf4cpp::rdf4cpp
        )

add_executable(blank_node_example blank_node_management.cpp)
target_link_libraries(blank_node_example PRIVATE
        rdf4cpp::rdf4cpp
        )

add_executable(RDFFileParser_example RDFFileParser_example.cpp)
target_link_libraries(RDFFileParser_example PRIVATE
        rdf4cpp::rdf4cpp
        )
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/RDFFileParser_simple.ttl" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(getting_started getting_started.cpp)
target_link_libraries(getting_started PRIVATE
        rdf4cpp::rdf4cpp
)

add_executable(rdf_format_checker rdf_format_checker.cpp)
target_link_libraries(rdf_format_checker PRIVATE
        rdf4cpp::rdf4cpp
)
