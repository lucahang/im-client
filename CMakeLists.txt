cmake_minimum_required(VERSION 3.16)
project(im-client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)
# find_package(redis++ REQUIRED)   # 若不需要则注释

# 设定 proto 文件
set(PROTO_FILES proto/message.proto)

# 生成 C++ 代码（兼容性写法）
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.9)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
else()
    # 更通用的方式
    include(CMakeParseArguments)
    protobuf_generate(TARGET im_client LANGUAGE cpp OUT_VAR PROTO_SRCS SOURCES ${PROTO_FILES})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})  # 存放生成的 .pb.h

set(SOURCES
    src/main.cpp
    src/im_client.cpp
    src/imclientwrapper.cpp
    src/login_dialog.cpp
    src/mainwindow.cpp
    src/chat_widget.cpp
    ${PROTO_SRCS}
)
set(HEADERS
    include/mainwindow.h
    include/login_dialog.h
    include/chat_widget.h
    include/imclientwrapper.h
    include/im_client.h
)

add_executable(im_client ${SOURCES} ${HEADERS})

target_link_libraries(im_client
    Qt6::Widgets
    Qt6::Core
    Boost::system
    protobuf::libprotobuf
    # redis++   # 若需要则取消注释
    pthread
)