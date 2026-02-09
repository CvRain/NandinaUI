cmake_minimum_required(VERSION 3.16)

qt_add_executable(NandinaExampleApp
    main.cpp
)

qt_add_qml_module(NandinaExampleApp
    URI NandinaExample
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(NandinaExampleApp
    PRIVATE
    Qt6::Quick
)
