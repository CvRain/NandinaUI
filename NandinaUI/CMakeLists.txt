qt_add_library(NandinaUI STATIC)

set_target_properties(NandinaUI PROPERTIES AUTOMOC ON)
target_link_libraries(NandinaUI PUBLIC Qt6::Quick)

list(APPEND MODULE_QML_FILES
        Color/CatppuccinFrappe.qml
        Color/CatppuccinLatte.qml
        Color/CatppuccinMacchiato.qml
        Color/CatppuccinMocha.qml
        Button/NanDefaultButton.qml
)

list(APPEND MODULE_QML_RESOURCES
        qmldir
        Color/qmldir
        Button/qmldir)

qt_add_qml_module(NandinaUI
        URI NandinaUI
        VERSION 1.0
        RESOURCE_PREFIX /NandinaUI
        RESOURCES ${MODULE_QML_RESOURCES}
        QML_FILES ${MODULE_QML_FILES})