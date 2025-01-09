cmake_minimum_required(VERSION 3.16)

project(NandinaExample VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(appNandinaExample
    main.cpp
)

set(QML_IMPORT_PATH ${CMAKE_BINARY_DIR} CACHE STRING "" FORCE)

qt_add_qml_module(appNandinaExample
    URI NandinaExample
    VERSION 1.0
    QML_FILES
        Main.qml
    IMPORT_PATH ${CMAKE_BINARY_DIR}
)

set_target_properties(appNandinaExample PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.cvrain.appNandinaExample
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appNandinaExample
    PUBLIC
        Nandina
        NandinaButton
        NandinaWidget
        NandinaColor
        Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appNandinaExample
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
