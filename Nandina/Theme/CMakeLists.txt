set(SUBPROJECT_NAME NandinaTheme)
set(SUBPROJECT_URI Nandina.Theme)

set(SUBPROJECT_QML_FILES

)

set(SUBPROJECT_QML_SINGLETON_FILES
)

set(SUBPROJECT_CPP_FILES
        theme_manager.hpp
        theme_manager.cpp
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
        NandinaColor
        NandinaPrimitives
        NandinaTokens
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(${SUBPROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Color
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Primitives
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Tokens
)
