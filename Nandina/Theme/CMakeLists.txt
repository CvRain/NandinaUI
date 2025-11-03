# Theme 模块 - 主题和配色
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/. THEME_CPP_FILES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Components/. COMPONENT_STYLE_FILES)

set(PALETTE_COLOR_FILES
        Resources/Palettes/Frappe.json
        Resources/Palettes/Latte.json
        Resources/Palettes/Macchiato.json
        Resources/Palettes/Mocha.json
)


set(COMPONENT_STYLE_SOURCES
        Resources/Styles/NanButton.json
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Components)

qt_add_library(NandinaTheme STATIC)
qt_add_qml_module(NandinaTheme
        URI Nandina.Theme
        VERSION 1.0
        SOURCES ${THEME_CPP_FILES} ${COMPONENT_STYLE_FILES}
        QML_FILES ${THEME_QML_FILES}
        RESOURCES ${PALETTE_COLOR_FILES} ${COMPONENT_STYLE_SOURCES}
)

target_include_directories(NandinaTheme PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/Components
)

target_link_libraries(NandinaTheme
        PRIVATE Qt6::Quick NandinaCore
)
