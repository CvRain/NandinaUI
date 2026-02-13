cmake_minimum_required(VERSION 3.16)

project(Nandina VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

option(NANDINA_BUILD_SHARED "Build Nandina as shared library" OFF)
option(NANDINA_DEV_IMPORT_PATH "Set QML import paths to build dir (dev only)" ON)

find_package(Qt6 REQUIRED COMPONENTS Gui Quick)

if(PROJECT_IS_TOP_LEVEL AND NANDINA_DEV_IMPORT_PATH)
    set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(QML_IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}" CACHE PATH "QML import path" FORCE)
    set(QML2_IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}" CACHE PATH "QML2 import path" FORCE)
endif()

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

if(QT_KNOWN_POLICY_QTP0005)
    qt_policy(SET QTP0005 NEW)
endif()

# 添加子目录
add_subdirectory(Nandina)
add_subdirectory(example)
