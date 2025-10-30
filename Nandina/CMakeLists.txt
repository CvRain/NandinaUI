# 主模块 - 聚合所有子模块
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 添加子模块
add_subdirectory(Core)
add_subdirectory(Theme)
add_subdirectory(Window)
add_subdirectory(Components)

qt_add_library(Nandina STATIC)
qt_add_qml_module(Nandina
        URI Nandina
        VERSION 1.0
        DEPENDENCIES
        NandinaCore
        NandinaTheme
        NandinaWindow
        NandinaComponents
)

target_link_libraries(Nandina
        PRIVATE
        NandinaCore
        NandinaTheme
        NandinaWindow
        NandinaComponents
        Qt6::Quick
)