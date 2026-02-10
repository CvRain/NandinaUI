set(SUBPROJECT_NAME NandinaColor)

set(SUBPROJECT_QML_FILES

)

set(SUBPROJECT_QML_SINGLETON_FILES
)

set(SUBPROJECT_CPP_FILES
        color_schema.hpp
)

qt_add_qml_module(${SUBPROJECT_NAME}
        URI Nandina.Color
        VERSION 1.0
        QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
        SOURCES ${SUBPROJECT_CPP_FILES}
)

target_link_directories(${SUBPROJECT_NAME} PUBLIC
        Qt6::Quick
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)