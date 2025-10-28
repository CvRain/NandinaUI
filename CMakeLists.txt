cmake_minimum_required(VERSION 3.16)

project(Nandina VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
set(QT_QML_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR})

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

list(APPEND MODULE_CPP_FILES
    Nandina/Theme/palette.hpp
    Nandina/Theme/palette.cpp
    Nandina/Theme/themeManager.hpp
    Nandina/Theme/themeManager.cpp
    Nandina/Theme/baseColors.hpp
    Nandina/Theme/baseColors.cpp
)

list(APPEND MODULE_WINDOW_QML_FILE
    Nandina/Window/NandinaWindow.qml
    Nandina/Window/TitleBarButton.qml
)

list(APPEND MODULE_CONTROLS_QML_FILE
    Nandina/Controls/Button.qml
    Nandina/Controls/MyRectangle.qml
)

include_directories(Nandina)
include_directories(Nandina/Theme)

qt_add_library(Nandina STATIC)
qt_add_qml_module(Nandina
    URI Nandina
    VERSION 1.0
    SOURCES ${MODULE_CPP_FILES}
    QML_FILES ${MODULE_WINDOW_QML_FILE} ${MODULE_CONTROLS_QML_FILE}
)

set_target_properties(Nandina PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(Nandina
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(Nandina
    PRIVATE Qt6::Quick)

target_include_directories(Nandina PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    Nandina
    Nandina/Theme
)