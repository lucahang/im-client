cmake_minimum_required(VERSION 3.16)
project(im-client LANGUAGES CXX)

# 消除 CMake 3.30+ 针对 FindBoost 的弃用警告
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Quick QuickWidgets QuickControls2)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)

# 设定 proto 文件
set(PROTO_FILES proto/message.proto)

# 生成 Protobuf C++ 代码
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.9)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
else()
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
    src/message_model.cpp
    src/add_contacts_dialog.cpp
    # ⚠️ 确保这里的路径与 find 命令查到的实际路径完全一致
    src/others/resources.qrc 
    
    ${PROTO_SRCS}
)

set(HEADERS
    include/mainwindow.h
    include/login_dialog.h
    include/chat_widget.h
    include/imclientwrapper.h
    include/im_client.h
    include/message_model.h
    include/add_contacts_dialog.h
)

# 已开启 CMAKE_AUTORCC ON，不再需要额外的 ${RESOURCES} 变量
add_executable(im_client ${SOURCES} ${HEADERS})

target_link_libraries(im_client PRIVATE
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickWidgets
    Qt6::QuickControls2
    Qt6::Core
    Boost::system
    protobuf::libprotobuf
    pthread
)