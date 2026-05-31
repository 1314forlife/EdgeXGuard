cmake_minimum_required(VERSION 3.16)
project(EdgeXGuardClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network OpenGL OpenGLWidgets)

# ==================== 第三方库配置 ====================
set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ffmpeg)
set(HNSWLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/hnswlib) # 【★新增】向量索引库路径

set(PAHO_MQTT_INCLUDE_DIR C:/msys64/mingw64/include)
set(PAHO_MQTT_LIB_DIR C:/msys64/mingw64/lib)

# ============= ONVIF 配置 ==============
set(ONVIF_GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/network/onvif/generated)
set(ONVIF_STDSOAP2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/network/onvif/stdsoap2)

# ONVIF 源文件（如果文件存在）
if(EXISTS ${ONVIF_GENERATED_DIR})
    file(GLOB ONVIF_SOURCES ${ONVIF_GENERATED_DIR}/*.cpp)
    file(GLOB ONVIF_HEADERS ${ONVIF_GENERATED_DIR}/*.h ${ONVIF_GENERATED_DIR}/*.nsmap)
    list(APPEND SOURCES ${ONVIF_SOURCES})
    list(APPEND HEADERS ${ONVIF_HEADERS})
endif()

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
    presentation/ui/modules/device/DeviceTree.cpp
    presentation/ui/modules/device/DeviceConfig.cpp
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

    # 【★新增】基础设施层 - 双擎仓储与分布式驱动打桩实现
    infrastructure/repository_impl/FaceRepositoryImpl.cpp
    infrastructure/repository_impl/LinuxPwmServoRepositoryImpl.cpp
    infrastructure/repository_impl/MqttAlarmRepositoryImpl.cpp
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
    presentation/ui/modules/device/DeviceTree.h
    presentation/ui/modules/device/DeviceConfig.h
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

    # 【★新增】领域层接口与核心模型骨架
    domain/model/detection/FaceEntity.h
    domain/repository/IFaceRepository.h
    domain/repository/IDoorAccessRepository.h
    domain/repository/IAlarmRepository.h

    # 【★新增】Core多媒体模块 - GStreamer策略模式骨架
    core/decode/decoder_strategy.h
    core/decode/cpu_software_strategy.h
    core/decode/rkmpp_hardware_strategy.h
    core/player/gst_player_context.h

    # 【★新增】基础设施层头文件
    infrastructure/repository_impl/FaceRepositoryImpl.h
    infrastructure/repository_impl/LinuxPwmServoRepositoryImpl.h
    infrastructure/repository_impl/MqttAlarmRepositoryImpl.h
)

# ==================== 资源文件 ====================
set(RESOURCES
    presentation/ui/resources/resources.qrc
)

# ==================== 可执行文件 ====================
add_executable(EdgeXGuardClient ${SOURCES} ${HEADERS} ${RESOURCES}
    presentation/ui/modules/monitor/MonitoringPage.h
    presentation/ui/modules/monitor/MonitoringPage.cpp
    core/common/logger.h
    core/common/logger.cpp

    # 【★新增】为了方便在IDE中查看和管理，把上下文实现类放这里
    core/player/gst_player_context.cpp
    presentation/ui/modules/analytics/AnalyticsPage.cpp presentation/ui/modules/analytics/AnalyticsPage.h
    presentation/viewmodel/face_database/FaceDatabaseViewModel.h
    presentation/viewmodel/face_database/FaceDatabaseViewModel.cpp
    presentation/ui/modules/face_database/FaceDatabasePage.h
    presentation/ui/modules/face_database/FaceDatabasePage.cpp
    presentation/viewmodel/perimeter_rules/PerimeterRulesViewModel.h
    presentation/viewmodel/perimeter_rules/PerimeterRulesViewModel.cpp
    presentation/viewmodel/perimeter_rules/PerimeterRulesPage.h
    presentation/viewmodel/perimeter_rules/PerimeterRulesPage.cpp
    presentation/ui/modules/perimeter_rules/PerimeterRulesPage.h
    presentation/ui/modules/perimeter_rules/PerimeterRulesPage.cpp
    infrastructure/network/mqtt/mqtt_network_service.cpp infrastructure/network/mqtt/mqtt_network_service.h
    infrastructure/network/onvif/onvif_client.h
    infrastructure/network/onvif/onvif_client.cpp
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
    ${HNSWLIB_DIR} # 【★新增】让整个工程在编译时能成功包含并认出 hnswlib 相关的头文件
    ${PAHO_MQTT_INCLUDE_DIR}
    ${ONVIF_GENERATED_DIR}
    ${ONVIF_STDSOAP2_DIR}
)

# ==================== 链接库 ====================
target_link_libraries(EdgeXGuardClient
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    ${FFMPEG_DIR}/lib/libavcodec.dll.a
    ${FFMPEG_DIR}/lib/libavformat.dll.a
    ${FFMPEG_DIR}/lib/libavutil.dll.a
    ${FFMPEG_DIR}/lib/libswscale.dll.a
    ${PAHO_MQTT_LIB_DIR}/libpaho-mqttpp3.a
    ${PAHO_MQTT_LIB_DIR}/libpaho-mqtt3a.dll.a
    libgsoap++.a
)