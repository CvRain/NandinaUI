cmake_minimum_required(VERSION 3.16)

project(NandinaUtils VERSION 0.1 LANGUAGES CXX)

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

list(APPEND NANDINA_QML_FILES

)

list(APPEND NANDINA_CPP_FILES
    NandinaFont.hpp
    NandinaFont.cpp
    NandinaType.hpp
    NandinaType.cpp
)

qt_add_qml_module(NandinaUtils
    URI Nandina.Utils
    VERSION 1.0
    QML_FILES
    ${NANDINA_QML_FILES}
    SOURCES ${NANDINA_CPP_FILES}
    IMPORT_PATH ${CMAKE_BINARY_DIR}
)

target_link_libraries(NandinaUtils
    PRIVATE
    Qt6::Quick NandinaResource)

set_target_properties(NandinaUtils PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER nandina.cvrain.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(NandinaUtils
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)


