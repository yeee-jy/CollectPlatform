cmake_minimum_required(VERSION 3.20)

if (NOT APP_VERSION)
    set(APP_VERSION "0.1.1")
endif()

project(CoApp VERSION ${APP_VERSION} LANGUAGES CXX)

set(TARGET_OUTPUT_NAME "Collection App")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 COMPONENTS Core Widgets Svg Network Mqtt Multimedia LinguistTools REQUIRED)
qt_standard_project_setup(REQUIRES 6.5 I18N_TRANSLATED_LANGUAGES zh_CN)

# ===== FFmpeg =====
# ===== FFmpeg（vcpkg 模块模式）=====
find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
    libavdevice
)

# ===== librtmp（通过 pkg-config）=====
find_package(PkgConfig REQUIRED)
pkg_check_modules(librtmp REQUIRED IMPORTED_TARGET librtmp)
set(LIBRTMP_LIBS PkgConfig::librtmp)
# ===== END =====

set(H_FILES
    src/views/settings/CameraSettingPanel.h
    src/views/settings/EEGSettingPanel.h
    src/views/settings/BandSettingPanel.h
    src/views/settings/MqttSettingPanel.h
    src/views/settings/InfoPanel.h
    src/views/MainWindow.h
    src/views/DeviceView.h
    src/views/SettingView.h
    src/views/EEGView.h
    src/views/BandView.h
    src/views/CameraView.h
    src/components/BaseWidget.h
    src/components/BarCard.h
    src/components/IPv4Edit.h
    src/components/LogBox.h
    src/components/RecordingIndicator.h
    src/components/SvgIconHelper.h
    src/services/log.h
    src/services/DataPipe.h
    src/services/DataSerializer.h
    src/services/CameraService.h
    src/services/BandServer.h
    src/services/EEGReceiver.h
    src/services/MqttPublisher.h
    src/services/CoSettingsMgr.h
    src/components/VideoRealTime/avtimebase.h
    src/components/VideoRealTime/H264Encoder.h
    src/components/VideoRealTime/mediabase.h
    src/components/VideoRealTime/NaluLoop.h
    src/components/VideoRealTime/pushwork.h
    src/components/VideoRealTime/semaphore.h
    src/components/VideoRealTime/rtmpbase.h
    src/components/VideoRealTime/RTMPPusher.h
    src/components/VideoRealTime/VideoFromCamera.h
)

set(CPP_FILES
    main.cpp
    src/views/settings/CameraSettingPanel.cpp
    src/views/settings/EEGSettingPanel.cpp
    src/views/settings/BandSettingPanel.cpp
    src/views/settings/MqttSettingPanel.cpp
    src/views/settings/InfoPanel.cpp
    src/views/MainWindow.cpp
    src/views/DeviceView.cpp
    src/views/SettingView.cpp
    src/views/EEGView.cpp
    src/views/BandView.cpp
    src/views/CameraView.cpp
    src/components/BaseWidget.cpp
    src/components/BarCard.cpp
    src/components/IPv4Edit.cpp
    src/components/LogBox.cpp
    src/components/RecordingIndicator.cpp
    src/components/SvgIconHelper.cpp
    src/services/log.cpp
    src/services/DataPipe.cpp
    src/services/DataSerializer.cpp
    src/services/CameraService.cpp
    src/services/BandServer.cpp
    src/services/EEGReceiver.cpp
    src/services/MqttPublisher.cpp
    src/services/CoSettingsMgr.cpp
    src/components/VideoRealTime/H264Encoder.cpp
    src/components/VideoRealTime/mediabase.cpp
    src/components/VideoRealTime/NaluLoop.cpp
    src/components/VideoRealTime/pushwork.cpp
    src/components/VideoRealTime/rtmpbase.cpp
    src/components/VideoRealTime/RTMPPusher.cpp
    src/components/VideoRealTime/VideoFromCamera.cpp
)

qt_add_resources(RESOURCES resources/resources.qrc)

qt_add_executable(${PROJECT_NAME}
    ${H_FILES} ${CPP_FILES} ${RESOURCES}
)


if (Qt6_VERSION VERSION_GREATER_EQUAL "6.9.0")
    qt_add_translations(${PROJECT_NAME}
        TS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/i18n
    )
else ()
    qt_add_translations(${PROJECT_NAME}
        TS_FILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/i18n
    )
endif ()

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${APP_VERSION}"
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

set(APP_PACKAGE_DIR ${APP_OUTPUT_DIRECTORY}/coapp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${TARGET_OUTPUT_NAME}
    RUNTIME_OUTPUT_DIRECTORY ${APP_PACKAGE_DIR}
    MACOSX_BUNDLE_GUI_IDENTIFIER "edu.perlab.CoApp"
    MACOSX_BUNDLE_COPYRIGHT "Copyright©2025 CCNU PerLab. All rights reserved."
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/macOS/Info.plist.in"
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Core Qt6::Widgets Qt6::Svg Qt6::Network Qt6::Mqtt Qt6::Multimedia PlatformCommon
    PkgConfig::FFMPEG
    ${LIBRTMP_LIBS}
)

if (WIN32)
    find_program(QT_DEPLOY_QT NAMES windeployqt)
    set(QT_DEPLOY_ARGS
        ${APP_PACKAGE_DIR}/${TARGET_OUTPUT_NAME}.exe
        --translations en,zh_CN
        --translationdir i18n
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:--debug>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>:--release>
    )
elseif(APPLE)
    find_program(QT_DEPLOY_QT NAMES macdeployqt)
    set(QT_DEPLOY_ARGS
        ${APP_PACKAGE_DIR}/${TARGET_OUTPUT_NAME}.app
    )
endif()

if (QT_DEPLOY_QT)
    add_custom_target(Deploy_${PROJECT_NAME} ALL
        COMMAND ${QT_DEPLOY_QT} ${QT_DEPLOY_ARGS}
        COMMENT "Deploying dependencies for ${PROJECT_NAME} after build ..."
        WORKING_DIRECTORY ${APP_PACKAGE_DIR}
        DEPENDS ${PROJECT_NAME}
    )
endif()
