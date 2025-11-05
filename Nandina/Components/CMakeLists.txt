# Components 模块 - 各种 UI 组件
set(COMPONENTS_QML_FILES
        #NanButton.qml
)

set(COMPONENT_CPP_FILES
        component_manager.hpp
        component_manager.cpp
        base_component.hpp
        base_component.cpp
)

set(COMPONENT_STYLES_FILES
        styles/NanButton.json
)

aux_source_directory(src/. SUB_COMPONENT_CPP_FILES)

qt_add_library(NandinaComponents STATIC)
qt_add_qml_module(NandinaComponents
        URI Nandina.Components
        VERSION 1.0
        QML_FILES ${COMPONENTS_QML_FILES}
        SOURCES ${COMPONENT_CPP_FILES} ${SUB_COMPONENT_CPP_FILES}
        RESOURCES ${COMPONENT_STYLES_FILES}
)

target_include_directories(NandinaTheme PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(NandinaComponents
        PRIVATE
        NandinaCore
        NandinaTheme
        Qt6::Quick
)
