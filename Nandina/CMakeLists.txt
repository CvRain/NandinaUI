add_subdirectory(Color)
add_subdirectory(Controls)
add_subdirectory(Core)
add_subdirectory(Theme)
add_subdirectory(Window)

set(NANDINA_MODULES
        NandinaColor
        NandinaControls
        NandinaCore
        NandinaTheme
        NandinaWindow
)

if(NANDINA_BUILD_SHARED)
        qt_add_library(Nandina SHARED)
else()
        qt_add_library(Nandina STATIC)
endif()
qt_add_qml_module(Nandina
        URI Nandina
        VERSION 1.0
        DEPENDENCIES ${NANDINA_MODULES}
)

target_link_libraries(Nandina PUBLIC
        Qt6::Quick Qt6::Gui
        ${NANDINA_MODULES}
)
