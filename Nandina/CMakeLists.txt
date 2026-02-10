set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(Color)

qt_add_library(Nandina STATIC)
qt_add_qml_module(Nandina
        URI Nandina
        VERSION 1.0
        DEPENDENCIES NandinaColor
)

target_link_libraries(Nandina PUBLIC
    Qt6::Quick NandinaColor
)