cmake_minimum_required(VERSION 3.16)
project(EdgeXGuardClient)

# ==================== 1. 编译选项与优化 ====================
option(BUILD_TESTS "Build test applications" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ==================== 2. 🪐 绝对大一统：锁定 MSYS2 工具链路径 ====================
set(CMAKE_PREFIX_PATH "C:/msys64/mingw64")

# COMPONENTS 包含原生解析 XML 所需的 Xml 模块以及多媒体组件
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Xml OpenGL OpenGLWidgets Multimedia Sql)

# ==================== 3. 第三方库路径重定位 ====================
set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ffmpeg)
set(HNSWLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/hnswlib)

# OpenCV 配置（100% 对齐你的 MSYS2 MinGW64 治下）
set(OPENCV_DIR "C:/msys64/mingw64")
set(OPENCV_INCLUDE_DIR ${OPENCV_DIR}/include/opencv4)
set(OPENCV_LIB_DIR ${OPENCV_DIR}/lib)
set(OPENCV_BIN_DIR ${OPENCV_DIR}/bin)

# Paho MQTT 配置
set(PAHO_MQTT_INCLUDE_DIR C:/msys64/mingw64/include)
set(PAHO_MQTT_LIB_DIR C:/msys64/mingw64/lib)
set(PAHO_MQTT_BIN_DIR C:/msys64/mingw64/bin)

# ==================== 4. 🌀 目录树复活：全自动全量深度扫描 ====================
# 使用 GLOB_RECURSE 自动钻进所有子文件夹，让 Qt Creator 左侧的代码目录树瞬间全量回魂！
file(GLOB_RECURSE AUTO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/application/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/domain/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/presentation/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

file(GLOB_RECURSE AUTO_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/application/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/domain/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/presentation/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.h"
)

# 🪐【核心修正】：加双引号并保持在同一行，彻底绝杀 CMake 的换行解析断火！
set(RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/presentation/ui/resources/resources.qrc")

# ==================== 5. 可执行文件构建 ====================
# 把全自动、指哪打哪扫描出来的全量业务代码和资源统统灌入构建
add_executable(EdgeXGuardClient
    ${AUTO_SOURCES}
    ${AUTO_HEADERS}
    ${RESOURCES}
)

# ==================== 6. 包含目录 ====================
target_include_directories(EdgeXGuardClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/application/service
    ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/repository_impl
    ${CMAKE_CURRENT_SOURCE_DIR}/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/presentation/viewmodel
    ${CMAKE_CURRENT_SOURCE_DIR}/presentation/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/presentation/ui/common
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${FFMPEG_DIR}/include
    ${HNSWLIB_DIR}
    ${PAHO_MQTT_INCLUDE_DIR}
    ${OPENCV_INCLUDE_DIR}
)

# ==================== 7. 链接库目录 ====================
target_link_directories(EdgeXGuardClient PRIVATE
    ${FFMPEG_DIR}/lib
    ${PAHO_MQTT_LIB_DIR}
    ${OPENCV_LIB_DIR}
)

# ==================== 8. 链接库总线 ====================
# ==================== 8. 链接库总线 ====================
target_link_libraries(EdgeXGuardClient PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Xml
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Multimedia
    Qt6::Sql

    ${FFMPEG_DIR}/lib/libavcodec.dll.a
    ${FFMPEG_DIR}/lib/libavformat.dll.a
    ${FFMPEG_DIR}/lib/libavutil.dll.a
    ${FFMPEG_DIR}/lib/libswscale.dll.a

    ${PAHO_MQTT_LIB_DIR}/libpaho-mqttpp3.a
    ${PAHO_MQTT_LIB_DIR}/libpaho-mqtt3a.dll.a

    ${OPENCV_LIB_DIR}/libopencv_core.dll.a
    ${OPENCV_LIB_DIR}/libopencv_imgproc.dll.a
    ${OPENCV_LIB_DIR}/libopencv_objdetect.dll.a
    ${OPENCV_LIB_DIR}/libopencv_dnn.dll.a
    ${OPENCV_LIB_DIR}/libopencv_calib3d.dll.a
    ${OPENCV_LIB_DIR}/libopencv_imgcodecs.dll.a
    ${OPENCV_LIB_DIR}/libopencv_features2d.dll.a
    ${OPENCV_LIB_DIR}/libopencv_flann.dll.a
    ${OPENCV_LIB_DIR}/libopencv_videoio.dll.a
    ${OPENCV_LIB_DIR}/libopencv_highgui.dll.a

    -lpthread
    -static-libgcc
    -static-libstdc++
)

# ==================== 10. 测试子目录（可选） ====================
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()