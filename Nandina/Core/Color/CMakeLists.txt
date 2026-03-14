set(SUBPROJECT_NAME NandinaColor)
set(SUBPROJECT_URI Nandina.Color)

set(SUBPROJECT_QML_FILES

)

set(SUBPROJECT_QML_SINGLETON_FILES
)

set(SUBPROJECT_CPP_FILES
        color_schema.cpp
        color_schema.hpp
        color_factory.cpp
        color_factory.hpp
)

file(GLOB SUBPROJECT_RESOURCE_FILES CONFIGURE_DEPENDS
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        themes/*.json
)

list(SORT SUBPROJECT_RESOURCE_FILES)

qt_add_qml_module(${SUBPROJECT_NAME}
        URI ${SUBPROJECT_URI}
        VERSION 1.0
        QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
        SOURCES ${SUBPROJECT_CPP_FILES}
)

qt_add_resources(${SUBPROJECT_NAME} ${SUBPROJECT_NAME}Resources
        PREFIX /Nandina/Color
        FILES ${SUBPROJECT_RESOURCE_FILES}
)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        NandinaTypes
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
