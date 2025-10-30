# Window 模块 - 窗口相关组件
set(WINDOW_QML_FILES
        NandinaWindow.qml
        ResizeHandler.qml
        TitleBar.qml
        TitleBarButton.qml
        WindowResizer.qml
)

qt_add_library(NandinaWindow STATIC)
qt_add_qml_module(NandinaWindow
        URI Nandina.Window
        VERSION 1.0
        QML_FILES ${WINDOW_QML_FILES}
)

target_link_libraries(NandinaWindow
        PRIVATE
        NandinaCore
        NandinaTheme
        Qt6::Quick
)