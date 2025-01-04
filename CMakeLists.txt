cmake_minimum_required(VERSION 3.16)

project(NandinaUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR})

find_package(Qt6 COMPONENTS Quick REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

if (QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif ()

if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif ()


set(QML_BUTTON Button/NanDefaultButton.qml)

set(QML_COLOR
    Color/CatppuccinLatte.qml
    Color/CatppuccinFrappe.qml
    Color/CatppuccinMacchiato.qml
    Color/CatppuccinMocha.qml)


qt_add_library(NandinaUI SHARED
    ${QML_BUTTON}
    ${QML_COLOR})

qt_add_qml_module(NandinaUI
    URI NandinaUI
    VERSION 1.0
    QML_FILES NandinaControls.qml ${QML_BUTTON} ${QML_COLOR}
)

set_target_properties(NandinaUI PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER nandinaui.cvrain.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    QML_IMPORT_PATH ${CMAKE_CURRENT_SOURCE_DIR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(NandinaUI
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(NandinaUI
    PRIVATE Qt6::Quick)

target_include_directories(NandinaUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

