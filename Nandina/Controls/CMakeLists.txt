set(SUBPROJECT_NAME NandinaControls)
set(SUBPROJECT_URI Nandina.Controls)

set(SUBPROJECT_QML_FILES
        # ── Primitives ──────────────────────────────────────────────
        NanPressable/NanPressable.qml # Pure interaction: hover/press/click, no visuals
        NanSurface/NanSurface.qml # Theme-aware Rectangle (color variant + shade system)
        # ── Containers ─────────────────────────────────────────────
        NanPanel/NanPanel.qml # Titled, padded panel container
        NanCard/NanCard.qml # Structured card: image / header / content / footer + presets
        # ── Navigation ─────────────────────────────────────────────
        NanSideBar/NanSideBar.qml # Main sidebar container (icon/offcanvas/none collapsible)
        NanSideBar/NanSideBarGroup.qml # Labelled, optionally collapsible section
        NanSideBar/NanSideBarItem.qml # Navigation item with icon, badge, sub-items
        NanSideBar/NanSideBarTrigger.qml # Toggle button for collapsing / expanding
)

set(SUBPROJECT_QML_SINGLETON_FILES
)

set(SUBPROJECT_CPP_FILES
        nandina_controls_module.cpp
)

qt_add_qml_module(${SUBPROJECT_NAME}
        URI ${SUBPROJECT_URI}
        VERSION 1.0
        DEPENDENCIES
        Nandina.Color
        Nandina.Primitives
        Nandina.Types
        Nandina.Theme
        QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
        SOURCES ${SUBPROJECT_CPP_FILES}
)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
        Qt6::Gui
        Qt6::Quick
        NandinaTheme
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
