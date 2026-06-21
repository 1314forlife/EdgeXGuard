# GB28181 子模块
cmake_minimum_required(VERSION 3.10)
project(gb28181_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

# GStreamer 依赖
pkg_check_modules(GST REQUIRED gstreamer-1.0)

# SIP 库路径（保持不变）
set(OSIP_INCLUDE_DIRS "/usr/local/include")
set(OSIP_LIBRARIES "/usr/local/lib/libosip2.so")
set(OSIPPARSER_LIBRARIES "/usr/local/lib/libosipparser2.so")
set(EXOSIP_LIBRARIES "/usr/local/lib/libeXosip2.so")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 源文件列表（包含所有模块）
set(SOURCES
    # 主模块
    src/sip_client.cpp
    src/sip_register.cpp
    src/sip_event.cpp
    src/sip_keepalive.cpp
    src/sdp_parser.cpp
    src/rtp_streamer.cpp
)

# 测试程序
add_executable(sip_test sip_test.cpp ${SOURCES})

target_include_directories(sip_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OSIP_INCLUDE_DIRS}
    ${GST_INCLUDE_DIRS}
)

target_link_libraries(sip_test PRIVATE
    ${OSIP_LIBRARIES}
    ${OSIPPARSER_LIBRARIES}
    ${EXOSIP_LIBRARIES}
    ${GST_LIBRARIES}
    pthread
    dl
)

# 可选：编译选项
target_compile_options(sip_test PRIVATE -Wall -Wextra)