set(SUBPROJECT_NAME NandinaColor)
set(SUBPROJECT_URI Nandina.Color)

set(SUBPROJECT_QML_FILES

)

set(SUBPROJECT_QML_SINGLETON_FILES
)

set(SUBPROJECT_CPP_FILES
        color_shema.cpp
        color_shema.hpp
        color_factory.cpp
        color_factory.hpp
)

qt_add_qml_module(${SUBPROJECT_NAME}
        URI ${SUBPROJECT_URI}
        VERSION 1.0
        QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
        SOURCES ${SUBPROJECT_CPP_FILES}
)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
        Qt6::Gui
        Qt6::Quick
        NandinaTypes
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)