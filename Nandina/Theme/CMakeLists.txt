# Theme 模块 - 主题和配色
set(THEME_CPP_FILES
        palette.hpp
        palette.cpp
        themeManager.hpp
        themeManager.cpp
        baseColors.hpp
        baseColors.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

qt_add_library(NandinaTheme STATIC)
qt_add_qml_module(NandinaTheme
        URI Nandina.Theme
        VERSION 1.0
        SOURCES ${THEME_CPP_FILES}
        QML_FILES ${THEME_QML_FILES}
)

target_include_directories(NandinaTheme PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(NandinaTheme
        PRIVATE Qt6::Quick
)
