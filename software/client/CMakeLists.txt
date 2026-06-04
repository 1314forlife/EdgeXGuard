cmake_minimum_required(VERSION 3.16)
project(EdgeXGuardClient)

# ==================== 测试选项 ====================
option(BUILD_TESTS "Build test applications" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ==================== 📦 重新锁定你原本的 MinGW Qt6 路径 ====================
set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/mingw_64")

# 【核心修正】拿掉原先找不到的特殊组件，加入 Qt 原生解析 ONVIF XML 所需的 Xml 模块
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Xml OpenGL OpenGLWidgets Multimedia)

# ==================== 第三方库配置 ====================
set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ffmpeg)
set(HNSWLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/hnswlib)

# 【无缝衔接】你的 msys64/mingw64 库在 MinGW 模式下将 100% 稳定运行，再无符号冲突
set(PAHO_MQTT_INCLUDE_DIR C:/msys64/mingw64/include)
set(PAHO_MQTT_LIB_DIR C:/msys64/mingw64/lib)

# gSOAP 和原本自动生成的几万行 ONVIF 代码全部抛弃，这里干干净净！

# ==================== Sherpa-ONNX 配置 ====================
#set(SHERPA_ONNX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/sherpa-onnx)


# ==================== 源文件 ====================
set(SOURCES
    main.cpp
    # 原有业务代码
    domain/model/alert/AlertInfo.cpp
    infrastructure/repository_impl/SensorRepository.cpp
    infrastructure/repository_impl/DeviceRepository.cpp
    infrastructure/repository_impl/AlertRepository.cpp
    infrastructure/repository_impl/DetectionRepository.cpp
    application/service/sensor/SensorService.cpp
    application/service/device/DeviceService.cpp
    application/service/alert/AlertService.cpp
    application/service/detection/DetectionService.cpp
    presentation/viewmodel/sensor/SensorViewModel.cpp
    presentation/viewmodel/device/DeviceViewModel.cpp
    presentation/viewmodel/alert/AlertViewModel.cpp
    presentation/viewmodel/detection/DetectionViewModel.cpp
    presentation/ui/main_window/MainWindow.cpp
    presentation/ui/main_window/SideBar.cpp
    presentation/ui/main_window/TopBar.cpp
    presentation/ui/modules/dashboard/DashboardPage.cpp
    presentation/ui/modules/monitor/VideoWidget.cpp
    presentation/ui/modules/monitor/VideoGrid.cpp
    presentation/ui/modules/monitor/PTZControl.cpp
    presentation/ui/modules/analytics/DetectionOverlay.cpp
    presentation/ui/modules/alert/AlertList.cpp
    presentation/ui/modules/alert/AlertRule.cpp
    presentation/ui/modules/settings/SettingsDialog.cpp
    presentation/ui/common/ChartWidget.cpp
    presentation/ui/common/TimelineWidget.cpp
    # 原有 core 模块
    core/decode/demuxer.cpp
    core/decode/video_decoder.cpp
    core/player/rtmp_player.cpp
    core/render/opengl_renderer.cpp

    # 基础设施层
    infrastructure/repository_impl/FaceRepositoryImpl.cpp
    infrastructure/repository_impl/LinuxPwmServoRepositoryImpl.cpp
    infrastructure/repository_impl/MqttAlarmRepositoryImpl.cpp

    # 网络层 —— 💡 此时这里的 onvif_client.cpp 已经换成了咱们原生手搓的、绝对不死的版本
    infrastructure/network/mqtt/mqtt_network_service.cpp
    infrastructure/network/onvif/onvif_client.cpp

    # 表现层 - 设备管理
    presentation/ui/modules/device/DeviceManagementWidget.cpp

    # 表现层 - 其他
    presentation/ui/modules/monitor/MonitoringPage.cpp
    core/common/logger.cpp
    core/player/gst_player_context.cpp
    presentation/ui/modules/analytics/AnalyticsPage.cpp
    presentation/viewmodel/face_database/FaceDatabaseViewModel.cpp
    presentation/ui/modules/face_database/FaceDatabasePage.cpp
    presentation/viewmodel/perimeter_rules/PerimeterRulesViewModel.cpp
    presentation/viewmodel/perimeter_rules/PerimeterRulesPage.cpp
    presentation/ui/modules/perimeter_rules/PerimeterRulesPage.cpp
)

# ==================== 头文件 ====================
set(HEADERS
    # 原有业务代码
    domain/model/alert/AlertInfo.h
    domain/model/detection/DetectionInfo.h
    domain/model/device/DeviceInfo.h
    domain/model/sensor/SensorData.h
    domain/repository/IAlertRepository.h
    domain/repository/IDetectionRepository.h
    domain/repository/IDeviceRepository.h
    domain/repository/ISensorRepository.h
    infrastructure/repository_impl/SensorRepository.h
    infrastructure/repository_impl/DeviceRepository.h
    infrastructure/repository_impl/AlertRepository.h
    infrastructure/repository_impl/DetectionRepository.h
    application/service/sensor/SensorService.h
    application/service/device/DeviceService.h
    application/service/alert/AlertService.h
    application/service/detection/DetectionService.h
    presentation/viewmodel/sensor/SensorViewModel.h
    presentation/viewmodel/device/DeviceViewModel.h
    presentation/viewmodel/alert/AlertViewModel.h
    presentation/viewmodel/detection/DetectionViewModel.h
    presentation/ui/main_window/MainWindow.h
    presentation/ui/main_window/SideBar.h
    presentation/ui/main_window/TopBar.h
    presentation/ui/modules/dashboard/DashboardPage.h
    presentation/ui/modules/monitor/VideoWidget.h
    presentation/ui/modules/monitor/VideoGrid.h
    presentation/ui/modules/monitor/PTZControl.h
    presentation/ui/modules/analytics/DetectionOverlay.h
    presentation/ui/modules/alert/AlertList.h
    presentation/ui/modules/alert/AlertRule.h
    presentation/ui/modules/settings/SettingsDialog.h
    presentation/ui/common/ChartWidget.h
    presentation/ui/common/TimelineWidget.h
    # 原有 core 模块
    core/common/packet.h
    core/common/frame.h
    core/thread/ring_queue.h
    core/decode/demuxer.h
    core/decode/video_decoder.h
    core/player/rtmp_player.h
    core/render/opengl_renderer.h

    # 领域层接口
    domain/model/detection/FaceEntity.h
    domain/repository/IFaceRepository.h
    domain/repository/IDoorAccessRepository.h
    domain/repository/IAlarmRepository.h

    # Core多媒体模块
    core/decode/decoder_strategy.h
    core/decode/cpu_software_strategy.h
    core/decode/rkmpp_hardware_strategy.h
    core/player/gst_player_context.h

    # 基础设施层头文件
    infrastructure/repository_impl/FaceRepositoryImpl.h
    infrastructure/repository_impl/LinuxPwmServoRepositoryImpl.h
    infrastructure/repository_impl/MqttAlarmRepositoryImpl.h

    # 网络层
    infrastructure/network/mqtt/mqtt_network_service.h
    infrastructure/network/onvif/onvif_client.h

    # 表现层头文件
    presentation/ui/modules/device/DeviceManagementWidget.h
    presentation/ui/modules/monitor/MonitoringPage.h
    core/common/logger.h
    presentation/ui/modules/analytics/AnalyticsPage.h
    presentation/viewmodel/face_database/FaceDatabaseViewModel.h
    presentation/ui/modules/face_database/FaceDatabasePage.h
    presentation/viewmodel/perimeter_rules/PerimeterRulesViewModel.h
    presentation/viewmodel/perimeter_rules/PerimeterRulesPage.h
    presentation/ui/modules/perimeter_rules/PerimeterRulesPage.h
)

# ==================== 资源文件 ====================
set(RESOURCES
    presentation/ui/resources/resources.qrc
)

# ==================== 可执行文件 ====================
add_executable(EdgeXGuardClient ${SOURCES} ${HEADERS} ${RESOURCES}
    # application/service/voice/VoiceWakeupService.h
    # application/service/voice/VoiceWakeupService.cpp
    # application/service/voice/VoiceCommandService.h
    # application/service/voice/VoiceCommandService.cpp
    # application/service/voice/VoiceprintManager.h
    # domain/model/voice/VoiceprintModel.h
    # domain/model/voice/VoiceCommandMapping.h
    # domain/repository/IVoiceprintRepository.h
    # infrastructure/asr/SherpaOnnxEngine.h
    # infrastructure/asr/SherpaOnnxEngine.cpp
    # infrastructure/asr/ASRConstants.h
    # infrastructure/repository_impl/VoiceprintFileRepository.h
    # core/voice/VoiceCommandMapper.h
    # core/voice/VoiceCommandMapper.cpp
    # infrastructure/audio/AudioInputManager.h
    # infrastructure/audio/AudioInputManager.cpp
    # infrastructure/audio/AudioOutputManager.h
    # infrastructure/audio/AudioOutputManager.cpp
)

# ==================== 包含目录 ====================
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
    # ${SHERPA_ONNX_DIR}/include
)

# target_link_directories(EdgeXGuardClient PRIVATE
#     ${SHERPA_ONNX_DIR}/lib
# )

# ==================== 链接库 ====================
target_link_libraries(EdgeXGuardClient PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Xml
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Multimedia
    ${FFMPEG_DIR}/lib/libavcodec.dll.a
    ${FFMPEG_DIR}/lib/libavformat.dll.a
    ${FFMPEG_DIR}/lib/libavutil.dll.a
    ${FFMPEG_DIR}/lib/libswscale.dll.a
    ${PAHO_MQTT_LIB_DIR}/libpaho-mqttpp3.a
    ${PAHO_MQTT_LIB_DIR}/libpaho-mqtt3a.dll.a
    # ${SHERPA_ONNX_DIR}/lib/sherpa-onnx-c-api.lib
    -lpthread
    -static-libgcc -static-libstdc++
)

# ==================== 测试子目录（可选） ====================
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()