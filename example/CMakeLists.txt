cmake_minimum_required(VERSION 3.16)

set(SUBPROJECT_QML_FILES
        Main.qml
        pages/PageSpec.qml
        pages/SettingPage.qml
        pages/ThemePage.qml
        pages/ColorPalettePage.qml
        pages/SurfacePressablePanelPage.qml
        pages/CardPage.qml
        pages/ButtonPage.qml
        pages/SideBarPage.qml
        pages/BadgePage.qml
)

set(SUBPROJECT_QML_SINGLETON_FILES
        PageRegistration.qml
)

set_source_files_properties(${SUBPROJECT_QML_SINGLETON_FILES}
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_executable(NandinaExampleApp
        MANUAL_FINALIZATION
        main.cpp
)

qt_add_qml_module(NandinaExampleApp
        URI NandinaExample
        VERSION 1.0
        QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}

)

target_link_libraries(NandinaExampleApp
        PRIVATE
        Qt6::Quick
        Nandina
        NandinaTheme
        NandinaTokens
)

qt_import_qml_plugins(NandinaExampleApp)
qt_finalize_executable(NandinaExampleApp)
