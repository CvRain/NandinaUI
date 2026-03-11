set(SUBPROJECT_NAME NandinaWindow)
set(SUBPROJECT_URI Nandina.Window)

set(SUBPROJECT_QML_FILES
        # ── Internal primitives ─────────────────────────────────────
        NanResizeEdge.qml # Single-edge DragHandler + cursor shape
        NanWindowResizer.qml # Overlays 8 NanResizeEdge zones
        NanTitleBarButton.qml # Minimize / maximize / close button
        NanTitleBar.qml # Default title bar (icon + title + controls)
        # ── Public ─────────────────────────────────────────────────
        NanWindow.qml # Theme-aware ApplicationWindow
)

set(SUBPROJECT_QML_SINGLETON_FILES
)

set(SUBPROJECT_CPP_FILES
)

qt_add_qml_module(${SUBPROJECT_NAME}
        URI ${SUBPROJECT_URI}
        VERSION 1.0
        DEPENDENCIES
        Nandina.Core
        Nandina.Theme
        Nandina.Controls
        QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
        SOURCES ${SUBPROJECT_CPP_FILES}
)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
        Qt6::Gui
        Qt6::Quick
        NandinaTheme
        NandinaControls
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
