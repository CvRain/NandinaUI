set(SUBPROJECT_NAME NandinaExperimental)
set(SUBPROJECT_URI Nandina.Experimental)

set(SUBPROJECT_QML_FILES
    Controls/NanItem.qml
)

set(SUBPROJECT_QML_SINGLETON_FILES
)

qt_add_qml_module(${SUBPROJECT_NAME}
    URI ${SUBPROJECT_URI}
    VERSION 1.0
    DEPENDENCIES Nandina.Theme Nandina.Primitives
    QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
    Qt6::Quick
    Qt6::Gui
    NandinaPrimitives
    NandinaTheme
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
