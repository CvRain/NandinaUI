cmake_minimum_required(VERSION 3.16)

qt_add_executable(NandinaExampleApp
        MANUAL_FINALIZATION
        main.cpp
)

qt_add_qml_module(NandinaExampleApp
        URI NandinaExample
        VERSION 1.0
        QML_FILES
        Main.qml
        CustomTheme.qml
        WindowDemoPage.qml
        ButtonDemoPage.qml
        InputDemoPage.qml
        LabelDemoPage.qml
        SwitchDemoPage.qml
        CheckboxDemoPage.qml
)

target_link_libraries(NandinaExampleApp
        PRIVATE
        Qt6::Quick
        Nandina
        NandinaWindow
        NandinaColor
        NandinaCore
        NandinaTheme
)

qt_import_qml_plugins(NandinaExampleApp)
qt_finalize_executable(NandinaExampleApp)
