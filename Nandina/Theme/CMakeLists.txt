# Theme 模块 - 主题和配色
set(THEME_CPP_FILES
        themeManager.hpp
        themeManager.cpp
        baseColors.hpp
        baseColors.cpp
)

set(PALETTE_COLOR_FILES
        Palettes/Frappe.json
        Palettes/Latte.json
        Palettes/Macchiato.json
        Palettes/Mocha.json
)

set(COMPONENT_COLOR_FILS
    Components/NanButton.json
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

qt_add_library(NandinaTheme STATIC)
qt_add_qml_module(NandinaTheme
        URI Nandina.Theme
        VERSION 1.0
        SOURCES ${THEME_CPP_FILES}
        QML_FILES ${THEME_QML_FILES}
        RESOURCES ${PALETTE_COLOR_FILES} ${COMPONENT_COLOR_FILS}
)

target_include_directories(NandinaTheme PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(NandinaTheme
        PRIVATE Qt6::Quick NandinaCore
)
