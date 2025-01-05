qt_add_library(NandinaUI STATIC)

set_target_properties(NandinaUI PROPERTIES AUTOMOC ON)
target_link_libraries(NandinaUI PUBLIC Qt6::Quick)

list(APPEND MODULE_QML_FILES
        Color/CatppuccinFrappe.qml
        Color/CatppuccinLatte.qml
        Color/CatppuccinMacchiato.qml
        Color/CatppuccinMocha.qml
        Color/NandianPalette.qml
        Button/NanDefaultButton.qml
)

list(APPEND MODULE_QML_SINGLETON_FILE
    Color/NandinanTheme.qml
)

set_source_files_properties(${MODULE_QML_SINGLETON_FILE} PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(NandinaUI
        URI NandinaUI
        VERSION 1.0
        RESOURCE_PREFIX NandinaUI
        QML_FILES ${MODULE_QML_FILES})
