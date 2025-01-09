cmake_minimum_required(VERSION 3.16)

project(NandinaColor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(Qt6 COMPONENTS Quick REQUIRED)
qt_standard_project_setup(REQUIRES 6.8)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

# 添加头文件和源文件
set(CPP_HEADERS
    CatppuccinFrappe.hpp
    CatppuccinLatte.hpp
    CatppuccinMacchiato.hpp
    CatppuccinMocha.hpp
    CatppuccinTheme.hpp
    NandinaStyle.hpp)

set(CPP_SOURCES
    CatppuccinFrappe.cpp
    CatppuccinLatte.cpp
    CatppuccinMacchiato.cpp
    CatppuccinMocha.cpp
    CatppuccinTheme.cpp
    NandinaStyle.cpp)

qt_add_qml_module(NandinaColor
    URI Nandina.Color
    VERSION 1.0
    QML_FILES
        TestRectangle.qml
    SOURCES 
        ${CPP_HEADERS}
        ${CPP_SOURCES}
    IMPORT_PATH ${CMAKE_BINARY_DIR}
)

target_link_libraries(NandinaColor
    PRIVATE 
        Qt6::Quick)
        
set_target_properties(NandinaColor PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER nandina.cvrain.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(NandinaColor
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)


