set(SUBPROJECT_NAME NandinaTokens)
set(SUBPROJECT_URI Nandina.Tokens)

set(SUBPROJECT_QML_FILES

)

set(SUBPROJECT_QML_SINGLETON_FILES

)

set(SUBPROJECT_CPP_FILES
        nan_tokens.hpp
        nan_tokens.cpp
)

qt_add_qml_module(${SUBPROJECT_NAME}
        URI ${SUBPROJECT_URI}
        VERSION 1.0
        DEPENDENCIES
        Nandina.Types
        QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
        SOURCES ${SUBPROJECT_CPP_FILES}
)

target_compile_definitions(${SUBPROJECT_NAME} PRIVATE NANDINA_LIBRARY)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
        Qt6::Gui
        Qt6::Quick
        NandinaUtils
        NandinaColor
        NandinaPrimitives
        NandinaTypes
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(${SUBPROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../Color
        ${CMAKE_CURRENT_SOURCE_DIR}/../Primitives
)
