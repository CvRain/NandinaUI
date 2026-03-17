set(SUBPROJECT_NAME NandinaFonts)
set(SUBPROJECT_URI Nandina.Fonts)

set(SUBPROJECT_CPP_FILES
        font_manager.hpp
        font_manager.cpp
)

qt_add_qml_module(${SUBPROJECT_NAME}
        URI ${SUBPROJECT_URI}
        VERSION 1.0
        SOURCES ${SUBPROJECT_CPP_FILES}
)

target_compile_definitions(${SUBPROJECT_NAME} PRIVATE NANDINA_LIBRARY)

target_link_libraries(${SUBPROJECT_NAME} PUBLIC
        Qt6::Gui
        Qt6::Quick
        NandinaUtils
)

target_include_directories(${SUBPROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ──────────────────────────────────────────────────────────────────────────────
#  Development convenience: create a symlink build/fonts → public/fonts
#  so the runtime FontManager can discover font files without copying 1 GB.
# ──────────────────────────────────────────────────────────────────────────────
set(_FONTS_SRC  "${CMAKE_SOURCE_DIR}/public/fonts")
set(_FONTS_LINK "${CMAKE_BINARY_DIR}/fonts")

if(NOT EXISTS "${_FONTS_LINK}")
    file(CREATE_LINK "${_FONTS_SRC}" "${_FONTS_LINK}" SYMBOLIC RESULT _link_result)
    if(NOT _link_result EQUAL 0)
        # Fallback: copy on systems that disallow symlinks (e.g. some CI)
        file(COPY "${_FONTS_SRC}/" DESTINATION "${_FONTS_LINK}")
    endif()
endif()
