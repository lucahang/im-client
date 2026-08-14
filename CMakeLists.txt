cmake_minimum_required(VERSION 3.16)
project(im-client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)
find_package(redis++ REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/message.proto)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
    src/main.cpp
    src/im_client.cpp
    src/codec.cpp
    ${PROTO_SRCS}
)

add_executable(im_client ${SOURCES})
target_link_libraries(im_client
    Boost::system
    protobuf::libprotobuf
    spdlog::spdlog
    redis++::redis++
    Threads::Threads
)