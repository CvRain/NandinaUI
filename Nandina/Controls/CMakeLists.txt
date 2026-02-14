set(SUBPROJECT_NAME NandinaControls)
set(SUBPROJECT_URI Nandina.Controls)

set(SUBPROJECT_QML_FILES
    NanButton.qml
    NanInput.qml
    NanLabel.qml
    NanSwitch.qml
    NanCheckbox.qml
    theme_utils.js
    button_style_utils.js
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
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
