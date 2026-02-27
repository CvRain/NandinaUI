set(SUBPROJECT_NAME NandinaWindow)
set(SUBPROJECT_URI Nandina.Window)

set(SUBPROJECT_QML_FILES
    NanWindow.qml
    CornerRectangle.qml
    DefaultTitleBar.qml
    ResizeHandler.qml
    TitleBarButton.qml
    WindowResizer.qml
)

set(SUBPROJECT_QML_SINGLETON_FILES
)

qt_add_qml_module(${SUBPROJECT_NAME}
    URI ${SUBPROJECT_URI}
    VERSION 1.0
    DEPENDENCIES Nandina.Theme
    QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
    Qt6::Quick
    Qt6::Gui
    NandinaTheme
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
