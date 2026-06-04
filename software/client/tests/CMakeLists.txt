cmake_minimum_required(VERSION 3.16)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS Core Multimedia)

# ==================== Sherpa-ONNX 配置 ====================
set(SHERPA_ONNX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/sherpa-onnx)

# ==================== test_audio ====================
add_executable(test_audio 
    test_audio.cpp
    ../infrastructure/audio/AudioInputManager.cpp
)

target_link_libraries(test_audio
    Qt6::Core
    Qt6::Multimedia
)

target_include_directories(test_audio PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/infrastructure/audio
)

# ==================== test_wakeup ====================
add_executable(test_wakeup 
    test_wakeup.cpp
)

target_link_libraries(test_wakeup
    Qt6::Core
    ${SHERPA_ONNX_DIR}/lib/sherpa-onnx-c-api.lib
)

# 修正：添加正确的头文件路径
target_include_directories(test_wakeup PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${SHERPA_ONNX_DIR}/include          # 这里，因为 c-api.h 在 include/sherpa-onnx/c-api/ 下
)

target_link_directories(test_wakeup PRIVATE
    ${SHERPA_ONNX_DIR}/lib
)