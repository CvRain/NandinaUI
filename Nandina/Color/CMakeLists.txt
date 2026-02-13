set(SUBPROJECT_NAME NandinaColor)
set(SUBPROJECT_URI Nandina.Color)

set(SUBPROJECT_QML_FILES

)

set(SUBPROJECT_QML_SINGLETON_FILES
)

set(SUBPROJECT_CPP_FILES
        color_schema.hpp
        color_atla.hpp
        color_atla.cpp
)

qt_add_qml_module(${SUBPROJECT_NAME}
        URI ${SUBPROJECT_URI}
        VERSION 1.0
        DEPENDENCIES Nandina.Core
        QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
        SOURCES ${SUBPROJECT_CPP_FILES}
)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
        Qt6::Gui
        Qt6::Quick
        NandinaCore
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/.
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
