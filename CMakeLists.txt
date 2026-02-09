cmake_minimum_required(VERSION 3.16)

project(Nandina VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)

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
