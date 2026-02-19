set(SUBPROJECT_NAME NandinaTokens)
set(SUBPROJECT_URI Nandina.Tokens)

set(SUBPROJECT_QML_FILES

)

set(SUBPROJECT_QML_SINGLETON_FILES
)

set(SUBPROJECT_CPP_FILES
    nan_spacing.hpp
    nan_typography.hpp
    nan_typography.cpp
)

qt_add_qml_module(${SUBPROJECT_NAME}
    URI ${SUBPROJECT_URI}
    VERSION 1.0
    QML_FILES ${SUBPROJECT_QML_FILES} ${SUBPROJECT_QML_SINGLETON_FILES}
    SOURCES ${SUBPROJECT_CPP_FILES}
)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
    Qt6::Quick
    NandinaCore
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
