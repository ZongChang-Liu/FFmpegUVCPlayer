cmake_minimum_required(VERSION 3.30)
project(FFmpegPlayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(ELAWIDGETTOOLS_PREFIX_DIR "${PROJECT_SOURCE_DIR}/3rdparty/ElaWidgetTools")
set(SPDLOG_PREFIX_DIR "${PROJECT_SOURCE_DIR}/3rdparty/spdlog")
set(TINYXML2_PREFIX_DIR "${PROJECT_SOURCE_DIR}/3rdparty/tinyxml2")
set(QT_PREFIX_DIR "C:/Qt5/Qt5.15.2/5.15.2/msvc2019_64")
set(FFmpeg_PREFIX_DIR "C:/ffmpeg6.1.1")

set(CMAKE_PREFIX_PATH   "${ELAWIDGETTOOLS_PREFIX_DIR}/lib/cmake"
                        "${SPDLOG_PREFIX_DIR}/lib/cmake"
                        "${TINYXML2_PREFIX_DIR}/lib/cmake"
                        "${QT_PREFIX_DIR}/lib/cmake")
set(QT_MODULES
        Core
        Gui
        Widgets
)
find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS ${QT_MODULES})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_MODULES})
message(STATUS "Qt version: ${Qt${QT_VERSION_MAJOR}_VERSION}")
find_package(ElaWidgetTools REQUIRED)
find_package(spdlog REQUIRED)
find_package(tinyxml2 REQUIRED)

file(GLOB_RECURSE HEADERS "include/*.h")
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
include_directories(${PROJECT_NAME} "src")

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_big_resources(RESOURCES resources.qrc ${ELAWIDGETTOOLS_PREFIX_DIR}/include/ElaWidgetTools.qrc)
else ()
    qt5_add_big_resources(RESOURCES resources.qrc ${ELAWIDGETTOOLS_PREFIX_DIR}/include/ElaWidgetTools.qrc)
endif ()

add_executable(FFmpegPlayer logo.rc main.cpp MyMainWindows.cpp MyMainWindows.h ${HEADERS} ${SOURCES} ${RESOURCES})

foreach (QT_MODULE ${QT_MODULES})
    target_link_libraries(${PROJECT_NAME} PUBLIC Qt${QT_VERSION_MAJOR}::${QT_MODULE})
endforeach (QT_MODULE)
target_include_directories(${PROJECT_NAME} PUBLIC
        ${ELAWIDGETTOOLS_INCLUDE_DIRS}
        ${FFmpeg_PREFIX_DIR}/include
        ${SPDLOG_PREFIX_DIR}/include
)

file(GLOB_RECURSE FFMPEG_LIBS "${FFmpeg_PREFIX_DIR}/lib/*.lib")
target_link_directories(${PROJECT_NAME} PUBLIC ${ELAWIDGETTOOLS_LIBRARY_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${FFmpeg_PREFIX_DIR}/lib)

target_link_libraries(${PROJECT_NAME} PUBLIC
        ${FFMPEG_LIBS}
        ElaWidgetTools
        spdlog::spdlog_header_only
        tinyxml2::tinyxml2
)

if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ELAWIDGETTOOLS_PREFIX_DIR}/bin/ElaWidgetTools.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    file(GLOB_RECURSE FFMPEG_DLLS "${FFmpeg_PREFIX_DIR}/bin/*.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FFMPEG_DLLS}
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
endif ()
