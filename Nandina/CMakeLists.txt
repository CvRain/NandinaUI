cmake_minimum_required(VERSION 3.16)

project(Nandina LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Quick REQUIRED)
qt_standard_project_setup(REQUIRES 6.8)

add_library(NandinaLib STATIC
    Nandina.cpp
    Theme.cpp
    CatppuccinLatte.cpp
    CatppuccinFrappe.cpp
    CatppuccinMacchiato.cpp
    CatppuccinMocha.cpp
    nandinaui.cpp
)

target_include_directories(NandinaLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(NandinaLib PRIVATE Qt6::Core Qt6::Quick)

qt_add_plugin(Nandina
    CLASS_NAME NandinaPlugin
    NandinaPlugin.cpp
)

target_link_libraries(Nandina
    PRIVATE
    Qt6::Quick
    NandinaLib
)

qt_add_qml_module(Nandina
    URI Nandina
    VERSION 1.0
    PLUGIN_TARGET Nandina
    QML_FILES
        NandinaUIControls.qml
)

# Style QML files
set(style_qml_files
    Nandina/qmldir
    Nandina/Button.qml
)

install(FILES ${style_qml_files} DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/QtQuick/Controls.2/Nandina)

# qtquickcontrols2.conf
install(FILES ${CMAKE_SOURCE_DIR}/NandinaExample/qtquickcontrols2.conf DESTINATION ${CMAKE_INSTALL_PREFIX}/qml)
