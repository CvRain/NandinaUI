qt_add_library(NandinaUI STATIC)

set_target_properties(NandinaUI PROPERTIES AUTOMOC ON)
target_link_libraries(NandinaUI PUBLIC Qt6::Quick)

list(APPEND COLOR_QML_FILES
        Color/CatppuccinFrappe.qml
        Color/CatppuccinLatte.qml
        Color/CatppuccinMacchiato.qml
        Color/CatppuccinMocha.qml
        Color/NandinaTheme.qml
)

list(APPEND MODULE_QML_SINGLETON_FILE
        Color/NandinaThemeType.qml
)

set_source_files_properties(${MODULE_QML_SINGLETON_FILE} PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(NandinaUIColor
        URI NandinaUI.Color
        VERSION 1.0
        QML_FILES ${COLOR_QML_FILES} ${MODULE_QML_SINGLETON_FILE}
)

list(APPEND WIDGET_QML_FILES
        Widget/NandinaWindow.qml
)

qt_add_qml_module(NandinaUIWidget
        URI NandinaUI.Widget
        VERSION 1.0
        QML_FILES ${WIDGET_QML_FILES}
)

# 链接模块到主库
target_link_libraries(NandinaUI
        PUBLIC
        Qt6::Quick
        NandinaUIColor
        NandinaUIWidget
)

# 确保模块可见性
set_target_properties(NandinaUIColor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/NandinaUI/Color
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/NandinaUI/Color
)

set_target_properties(NandinaUIWidget PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/NandinaUI/Widget
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/NandinaUI/Widget
)
