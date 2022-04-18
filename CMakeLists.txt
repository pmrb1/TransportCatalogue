cmake_minimum_required(VERSION 3.14)
project(transport_catalogue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
        transport_catalogue.proto
        map_renderer.proto
        svg.proto
        transport_router.proto
        graph.proto
        )

add_executable(transport_catalogue
        main.cpp
        domain.cpp
        geo.cpp
        json.cpp
        json_builder.cpp
        json_reader.cpp
        map_renderer.cpp
        request_handler.cpp
        serialization.cpp
        svg.cpp
        transport_catalogue.cpp
        transport_router.cpp
        ${PROTO_SRCS}
        ${PROTO_HDRS})

target_link_libraries(transport_catalogue ${Protobuf_LIBRARIES})
