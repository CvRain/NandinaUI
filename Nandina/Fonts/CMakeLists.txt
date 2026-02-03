set(QML_FONTS_FILE

)

set(FONT_RESOURCE_FILES
        JetBrainsMono/JetBrainsMonoNerdFont-Bold.ttf
        JetBrainsMono/JetBrainsMonoNerdFont-Italic.ttf
        JetBrainsMono/JetBrainsMonoNerdFont-Light.ttf
        JetBrainsMono/JetBrainsMonoNerdFont-Medium.ttf
        JetBrainsMono/JetBrainsMonoNerdFont-Regular.ttf
        Sarasa/Sarasa-Bold.ttc
        Sarasa/Sarasa-Italic.ttc
        Sarasa/Sarasa-Light.ttc
        Sarasa/Sarasa-Regular.ttc
)

set(CPP_FILES
        font_collection.hpp
        font_collection.cpp
)

set_source_files_properties(${QML_FONTS_FILE}
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(NandinaFontCollection STATIC)
qt_add_qml_module(NandinaFontCollection
        URI Nandina.FontCollection
        VERSION 1.0
        QML_FILES ${QML_FONTS_FILE}
        SOURCES ${CPP_FILES}
        RESOURCES ${FONT_RESOURCE_FILES}
)

target_link_libraries(NandinaFontCollection
        PRIVATE
        Qt6::Quick
        NandinaCore
)
