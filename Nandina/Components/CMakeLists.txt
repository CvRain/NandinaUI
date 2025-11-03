# Components 模块 - 各种 UI 组件
set(COMPONENTS_QML_FILES
        NanButton.qml
)

qt_add_library(NandinaComponents STATIC)
qt_add_qml_module(NandinaComponents
        URI Nandina.Components
        VERSION 1.0
        QML_FILES ${COMPONENTS_QML_FILES}
)

target_link_libraries(NandinaComponents
        PRIVATE
        NandinaCore
        NandinaTheme
        Qt6::Quick
)
