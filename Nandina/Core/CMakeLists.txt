# Core 模块 - 枚举、常量、类型定义
set(CORE_QML_FILES

)

list(APPEND MODULE_TYPES_CPP_FILES
        Types/nanButton.hpp
        Types/nanButton.cpp)

include_directories(Types)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

qt_add_library(NandinaCore STATIC)

qt_add_qml_module(NandinaCore
        URI Nandina.Core
        VERSION 1.0
        QML_FILES ${CORE_QML_FILES}
        SOURCES ${MODULE_TYPES_CPP_FILES}
)

target_link_libraries(NandinaCore
        PRIVATE Qt6::Quick

)

target_include_directories(NandinaCore PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/Types
)
