set(SUBPROJECT_NAME NandinaPrimitives)
set(SUBPROJECT_URI Nandina.Primitives)

set(SUBPROJECT_QML_FILES
    BaseControl.qml
    Pressable.qml
    Surface.qml
    Indicator.qml
    FormFieldBehavior.qml
)

qt_add_qml_module(${SUBPROJECT_NAME}
    URI ${SUBPROJECT_URI}
    VERSION 1.0
    DEPENDENCIES Nandina.Theme Nandina.Tokens
    QML_FILES ${SUBPROJECT_QML_FILES}
)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
    Qt6::Quick
    Qt6::Gui
    NandinaTheme
    NandinaTokens
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
