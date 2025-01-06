cmake_minimum_required(VERSION 3.16)

project(NandinaUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)

if (QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif ()

if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif ()


# list(APPEND MODULE_QML_FILES
#         NandinaUI/Color/CatppuccinFrappe.qml
#         NandinaUI/Color/CatppuccinLatte.qml
#         NandinaUI/Color/CatppuccinMacchiato.qml
#         NandinaUI/Color/CatppuccinMocha.qml
#         NandinaUI/Color/NandianPalette.qml
#         NandinaUI/Button/NanDefaultButton.qml
# )

# list(APPEND MODULE_QML_SINGLETON_FILE
#     NandinaUI/Color/NandinanTheme.qml
# )

# set_source_files_properties(${MODULE_QML_SINGLETON_FILE} PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

add_subdirectory(NandinaUI)

target_include_directories(NandinaUI 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
)

target_include_directories(NandinaUI PUBLIC NandinaUI/Color)
target_include_directories(NandinaUI PUBLIC NandinaUI/Widget)


set_target_properties(NandinaUI PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER nandinaui.cvrain.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

target_compile_definitions(NandinaUI
        PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(NandinaUI
        PRIVATE Qt6::Quick)

target_include_directories(NandinaUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(ExampleProject NandianExample/main.cpp)
qt_add_qml_module(ExampleProject
        URI ExampleProjectApp
        VERSION 1.0
        RESOURCE_PREFIX /
        QML_FILES NandianExample/Main.qml
        IMPORT_PATH ${CMAKE_BINARY_DIR}
)
target_link_libraries(ExampleProject PUBLIC Qt6::Quick)
target_link_libraries(ExampleProject PRIVATE
        NandinaUI
        NandinaUIColor
        NandinaUIWidget
)

target_compile_definitions(ExampleProject PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

set(QML_IMPORT_PATH ${CMAKE_BINARY_DIR} CACHE STRING "" FORCE)
set(QML_IMPORT_PATH ${QML_IMPORT_PATH} ${CMAKE_BINARY_DIR}/qml CACHE STRING "" FORCE)