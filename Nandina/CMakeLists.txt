set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(Color)
add_subdirectory(Core)
add_subdirectory(Theme)

set(NANDINA_MODULES
        NandinaColor
        NandinaCore
        NandinaTheme
)

qt_add_library(Nandina STATIC)
qt_add_qml_module(Nandina
        URI Nandina
        VERSION 1.0
        DEPENDENCIES ${NANDINA_MODULES}
)

target_link_libraries(Nandina PUBLIC
        Qt6::Quick Qt6::Gui
        ${NANDINA_MODULES}
)