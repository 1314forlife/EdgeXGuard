cmake_minimum_required(VERSION 3.10)
project(EdgeXGuardServer)

# ----------------------------------------------------------------------------
# 1. 强制 C++17 标准与平台级极致性能优化
# ----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# RK3588 (Cortex-A76) 极致性能优化
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mcpu=cortex-a76+crypto+fp+simd -ffast-math -ftree-vectorize -pipe")

message(STATUS "🚀 构建模式: ${CMAKE_BUILD_TYPE}")

# ----------------------------------------------------------------------------
# 2. 依赖查找 (GStreamer / OpenCV)
# ----------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED 
    gstreamer-1.0 
    gstreamer-app-1.0 
    gobject-2.0 
    glib-2.0
)

pkg_check_modules(OPENCV REQUIRED opencv4)

# ----------------------------------------------------------------------------
# 3. RKNN 配置
# ----------------------------------------------------------------------------
set(RKNN_INCLUDE_DIR "/usr/include")
set(RKNN_LIBRARY "/usr/lib/aarch64-linux-gnu/librknnrt.so")

# ----------------------------------------------------------------------------
# 4. 源文件
# ----------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/face_database.cpp
    src/gst_pipeline.cpp
    # rknn_inference.cpp 暂时不用，先注释掉
    # src/rknn_inference.cpp
    src/gb28181/sip_client.cpp
)

# ----------------------------------------------------------------------------
# 5. 构建
# ----------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gb28181
    ${GST_INCLUDE_DIRS}
    ${OPENCV_INCLUDE_DIRS}
    ${RKNN_INCLUDE_DIR}
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${GST_LIBRARY_DIRS}
    ${OPENCV_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GST_LIBRARIES}
    ${OPENCV_LIBRARIES}
    opencv_face          # 添加 LBPH 人脸识别模块
    ${RKNN_LIBRARY}
    /usr/local/lib/libosip2.so      # 🆕
    /usr/local/lib/libeXosip2.so    # 🆕
    pthread
    dl
)