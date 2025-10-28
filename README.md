![](Image/placeholder.jpg)
<br>
# NandinaUI
**NandinaUI**: A flat design QML component library using the `Catppuccin` color scheme.

[中文](docs/README_ZH.md)

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/releases/latest)
[![GitHub](https://img.shields.io/github/license/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/blob/main/LICENSE)
[![GitHub all releases](https://img.shields.io/github/downloads/Nandina/NandinaUI/total?style=flat-square)](https://github.com/Nandina/NandinaUI/releases)
[![GitHub issues](https://img.shields.io/github/issues/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/issues)

## Component Preview
***Working hard to update***

## Quick Start

### Method 1: Import as a subproject
A. For non-git projects, assume the new project name is `TryNandina`
1. Clone this project locally under your project.
```bash
cd TryNandina
git clone https://github.com/CvRain/NandinaUI.git
```
2. Subsequent steps jump to `C. Configure CMakeLists.txt`


B. For git projects, assume the project name is `TryNandina`
1. Create a subproject and enable git
```bash
cd TryNandina
git init
```
2. Add the project as a submodule to your project
```bash
git submodule add https://github.com/CvRain/NandinaUI.git
```
3. Subsequent steps jump to `C. Configure CMakeLists.txt`

C. Configure CMakeLists.txt
1. Add subproject
```cmake
add_subdirectory(Nandina)
```

2. Add dependencies
```cmake
target_link_libraries(appTryNandina
    PRIVATE Qt6::Quick Qt6::Core Nandinaplugin
)
```

3. Complete configuration reference
```cmake
cmake_minimum_required(VERSION 3.16)

project(TryNandina VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Core)

qt_standard_project_setup(REQUIRES 6.5)

if (QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif ()

if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif ()

if (QT_KNOWN_POLICY_QTP0005)
    qt_policy(SET QTP0005 NEW)
endif ()


add_subdirectory(Nandina)

qt_add_executable(appTryNandina
    main.cpp
)

qt_add_qml_module(appTryNandina
    URI TryNandina
    VERSION 1.0
    QML_FILES
        Main.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appTryNandina PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appTryNandina
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appTryNandina
    PRIVATE Qt6::Quick Qt6::Core Nandinaplugin
)

include(GNUInstallDirs)
install(TARGETS appTryNandina
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
```

### Method 2: Use the project as a library
1. Get the project
```bash
git clone https://github.com/CvRain/NandinaUI.git
```

2. Compile the project
The output files after compilation are in the same directory as this project
```bash
cmake . -B build -DCMAKE_PREFIX_PATH=/path/to/qt/install/dir
cd build
make install
```

3. Add dependencies
Drag the output folder (Release/Nandina) into the project to be used, assuming the project name is `TryNandina` and add the following content to CMakeLists.txt
```cmake
#Add the components you need
set(NANDINA_QML_FILES
    Nandina/Window/NandinaWindow.qml
)

qt_add_library(Nandina STATIC)
qt_add_qml_module(Nandina
    URI MyPlugin
    QML_FILES
        ${NANDINA_QML_FILES}
)

target_link_libraries(appTryNandina
    PRIVATE Qt6::Quick Nandinaplugin
)
```

Complete configuration
```cmake
cmake_minimum_required(VERSION 3.16)

project(TryNandina VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(NANDINA_QML_FILES
    Nandina/Window/NandinaWindow.qml
)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appTryNandina
    main.cpp
)

qt_add_qml_module(appTryNandina
    URI TryNandina
    VERSION 1.0
    QML_FILES Main.qml
)

qt_add_library(Nandina STATIC)
qt_add_qml_module(Nandina
    URI Nandina
    QML_FILES ${NANDINA_QML_FILES}
)

set_target_properties(appTryNandina PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appTryNandina
    PRIVATE Qt6::Quick Nandinaplugin
)

include(GNUInstallDirs)
install(TARGETS appMyPluginTest
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
```

4. Edit `main.cpp` and add the following content
```cpp
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(MyPluginPlugin)
```
