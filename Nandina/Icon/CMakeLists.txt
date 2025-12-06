aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/. ICON_CPP_FILES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Impl ICON_IMPL_FILES)

qt_add_library(NandinaIcon STATIC)
qt_add_qml_module(NandinaIcon
        URI Nandina.Icon
        VERSION 1.0
        SOURCES ${ICON_CPP_FILES} ${ICON_IMPL_FILES}
)

target_include_directories(NandinaIcon PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(NandinaIcon
        PRIVATE Qt6::Quick NandinaCore
)
