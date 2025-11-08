# Components 模块 - 各种 UI 组件
set(COMPONENTS_QML_FILES
        qml/NanButton.qml
)

set(COMPONENT_CPP_FILES
        component_manager.hpp
        component_manager.cpp
        component_collection.hpp
        component_collection.cpp
        base_component.hpp
        base_component.cpp
        component_factory.hpp
        component_factory.cpp
        style_loader.hpp
        style_loader.cpp
        component_registrar.hpp
        component_registrar.cpp
)

set(COMPONENT_STYLES_FILES
        styles/NanButton.json
)

aux_source_directory(src/. SUB_COMPONENT_CPP_FILES)

# ensure generated registrar is included (it lives under src/)
# SUB_COMPONENT_CPP_FILES already collects src/*.cpp including src/nan_button_registrar.cpp

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
