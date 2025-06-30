cmake_minimum_required(VERSION 3.20)

project(APP_Student VERSION 0.1)

set(TARGET_OUTPUT_NAME "Student APP")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 添加FFmpeg依赖
find_package(FFMPEG REQUIRED COMPONENTS
    avcodec
    avdevice
    avformat
    avutil
    swscale
)

# 查找PkgConfig工具
find_package(PkgConfig REQUIRED)
# 查找librtmp包
pkg_check_modules(librtmp REQUIRED IMPORTED_TARGET librtmp)
# 查找x264包
pkg_check_modules(x264 REQUIRED IMPORTED_TARGET x264)

find_package(Qt6 COMPONENTS Core Widgets Network Mqtt Multimedia MultimediaWidgets REQUIRED)
qt_standard_project_setup(REQUIRES 6.5)

# ------------------ 新增Winsock2支持 ------------------
# 检查操作系统平台
if(WIN32)
    # Windows平台配置
    message(STATUS "Configuring for Windows platform")

    # 添加Winsock2头文件路径
    include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/src)

    # 添加Winsock2库
    set(PLATFORM_LIBS ws2_32)

    # 定义_WINSOCK_DEPRECATED_NO_WARNINGS以避免警告
    add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
else()
    # 非Windows平台配置
    message(STATUS "Configuring for non-Windows platform")
    set(PLATFORM_LIBS "")
endif()
# ------------------------------------------------------

set(H_FILES
    src/services/EEGDataReceiver.h
    src/services/WristbandServer.h
    src/services/WristbandHandler.h
    src/services/MqttPublisher.h
    src/services/DataQueue.h
    src/services/SettingsManager.h
    src/windows/MainWindow.h
    src/components/BarCard.h
    src/components/BaseWidget.h
    src/components/IPv4Edit.h
    src/components/LogEdit.h
    src/components/StatusIndicator.h
    src/views/MqttPublishView.h
    src/views/SettingView.h
    src/views/LogView.h
    src/views/CameraView.h
    src/services/VideoFromCamera.h
    src/services/rtmpbase.h
    src/services/mediabase.h
    src/services/H264Encoder.h
    src/services/NaluLoop.h
    src/services/pushwork.h
    src/services/RTMPPusher.h
    src/services/semaphore.h
    src/services/avtimebase.h
)

set(CPP_FILES
    main.cpp
    src/services/EEGDataReceiver.cpp
    src/services/WristbandServer.cpp
    src/services/WristbandHandler.cpp
    src/services/MqttPublisher.cpp
    src/services/SettingsManager.cpp
    src/windows/MainWindow.cpp
    src/components/BarCard.cpp
    src/components/BaseWidget.cpp
    src/components/IPv4Edit.cpp
    src/components/LogEdit.cpp
    src/components/StatusIndicator.cpp
    src/views/MqttPublishView.cpp
    src/views/SettingView.cpp
    src/views/LogView.cpp
    src/views/CameraView.cpp
    src/services/VideoFromCamera.cpp
    src/services/rtmpbase.cpp
    src/services/H264Encoder.cpp
    src/services/NaluLoop.cpp
    src/services/pushwork.cpp
    src/services/RTMPPusher.cpp
    src/services/mediabase.cpp
)

qt_add_resources(RESOURCES resources/resources.qrc)

qt_add_executable(${PROJECT_NAME}
    ${H_FILES} ${CPP_FILES} ${RESOURCES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}  # 添加FFmpeg头文件路径
    ${librtmp_INCLUDE_DIRS}
    ${x264_INCLUDE_DIRS}    # 添加x264头文件路径
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${TARGET_OUTPUT_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/MacOS/Info.plist.in"
    MACOSX_BUNDLE TRUE
    #    WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Mqtt
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    common
    ${FFMPEG_LIBRARIES}  # 添加FFmpeg库链接
    PkgConfig::librtmp
    PkgConfig::x264      # 添加x264库链接
    ${PLATFORM_LIBS}     # 添加平台特定库（Winsock2）
)