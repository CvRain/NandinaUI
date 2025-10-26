cmake_minimum_required(VERSION 3.16)

project(NandinaUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

set(PROJECT_NAME "NandinaUI")
set(QML_PLUGIN_DESTDIR "${CMAKE_CURRENT_SOURCE_DIR}/Release")

find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)

if (QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif ()

if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif ()

if (QT_KNOWN_POLICY_QTP0005)
    qt_policy(SET QTP0005 NEW)
endif ()

set(CPP_FILES
        NandinaUI/Theme/themeManager.hpp
        NandinaUI/Theme/themeManager.cpp
        NandinaUI/Theme/palette.hpp
        NandinaUI/Theme/palette.cpp
    )

include_directories(
    NandianUI
    NandinaUI/Theme
)

qt_add_library(NandinaUI STATIC)

qt_add_qml_module(NandinaUI
        URI NandinaUI
        VERSION 1.0
        QML_FILES NandinaUI/Controls/NandinaWindow.qml
        SOURCES ${CPP_FILES}
)

set_target_properties(NandinaUI PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

target_compile_definitions(NandinaUI
        PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(NandinaUI
        PRIVATE Qt6::Quick Qt6::Core)

target_include_directories(NandinaUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/NandinaUI
    ${CMAKE_CURRENT_SOURCE_DIR}/NandinaUI/Theme)

install(DIRECTORY "${CMAKE_BINARY_DIR}/${PROJECT_NAME}" DESTINATION ${QML_PLUGIN_DESTDIR}
        FILE_PERMISSIONS
        OWNER_READ OWNER_EXECUTE OWNER_WRITE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
        DIRECTORY_PERMISSIONS
        OWNER_READ OWNER_EXECUTE OWNER_WRITE
        GROUP_READ GROUP_EXECUTE GROUP_WRITE
        WORLD_READ WORLD_EXECUTE
        FILES_MATCHING PATTERN "*")

# example project
qt_add_executable(NandinaExample Example/example.cpp)
qt_add_qml_module(NandinaExample
    URI NandinaExampleApp
    VERSION 1.0
    QML_FILES Example/Main.qml
)
target_link_libraries(NandinaExample
        PRIVATE Qt6::Quick Qt6::Core NandinaUIplugin)

target_compile_definitions(NandinaExample PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
