cmake_minimum_required(VERSION 3.20)

project(APP_Teacher)

set(TARGET_OUTPUT_NAME "APP")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#导入ffmpeg的库函数
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

find_package(Qt6 COMPONENTS Core Quick Qml Core5Compat Graphs REQUIRED)
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

qt_add_resources(RESOURCES resources/resources.qrc)

qt_add_executable(${PROJECT_NAME}
    main.cpp ${RESOURCES} resources/win_res.rc
)

qt_policy(SET QTP0004 NEW)

target_include_directories( ${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rtmp
)

qt_add_qml_module(${PROJECT_NAME}
    URI TeacherApp
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    RESOURCE_PREFIX "/"
    SOURCES
    src/QmlWatcher.h src/QmlWatcher.cpp
    src/MinMaxQueue.h src/MinMaxQueue.cpp
    src/services/WristbandData.h
    src/services/WristbandClientHandler.h src/services/WristbandClientHandler.cpp
    src/services/WristbandListener.h src/services/WristbandListener.cpp
    src/rtmp/framequeue.h src/rtmp/framequeue.cpp
    src/rtmp/H264Decoder.h src/rtmp/H264Decoder.cpp
    src/rtmp/mediabase.h
    src/rtmp/packetqueue.h src/rtmp/packetqueue.cpp
    src/rtmp/PullWork.h src/rtmp/PullWork.cpp
    src/rtmp/RTMPBase.h src/rtmp/RTMPBase.cpp
    src/rtmp/RTMPPlayer.h src/rtmp/RTMPPlayer.cpp
    src/rtmp/TimesUtil.h
    src/rtmp/VideoDecodeLoop.h src/rtmp/VideoDecodeLoop.cpp
    src/services/videoListener.h src/services/videoListener.cpp
    src/rtmp/Video_View.h src/rtmp/Video_View.cpp
    src/rtmp/VideoPaintedItem.h
    QML_FILES
    qml/App.qml
    qml/Controls/MyIconButton.qml
    qml/Controls/MyPage.qml
    qml/Controls/NumberInputer.qml
    qml/Controls/Charts/MultiLineChart.qml
    qml/Controls/Charts/MyDateTimeAxis.qml
    qml/Controls/Charts/MyGraphsTheme.qml
    qml/Controls/Charts/MyValueAxis.qml
    qml/Controls/Charts/SingleLineChart.qml
    qml/Controls/Charts/util.js
    qml/Pages/HomePage.qml
    qml/Pages/OfflinePage.qml
    qml/Pages/RealTimePage.qml
    qml/Pages/SettingPage.qml
    qml/Pages/StoragePage.qml
    qml/Views/WristbandView.qml
    qml/Pages/videoreadtime.qml
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${TARGET_OUTPUT_NAME}
    #    WIN32_EXECUTABLE TRUE
)
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}  # 添加FFmpeg头文件路径
    ${librtmp_INCLUDE_DIRS}
    ${x264_INCLUDE_DIRS}    # 添加x264头文件路径
)
#target_include_directories(${PROJECT_NAME}
#    PRIVATE DelegateUI/include
#)
#
#target_link_directories(${PROJECT_NAME}
#    PRIVATE DelegateUI/lib
#)

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE QT_QML_DEBUG
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Core Qt6::Quick Qt6::Qml Qt6::Core5Compat Qt6::Graphs DelegateUI
    ${FFMPEG_LIBRARIES}  # 添加FFmpeg库链接
    PkgConfig::librtmp
    PkgConfig::x264      # 添加x264库链接
    ${PLATFORM_LIBS}     # 添加平台特定库（Winsock2）
)