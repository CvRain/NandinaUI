![](Image/placeholder.jpg)
<br>
# NandinaUI
**南天竹： 一套使用 Catppuccin 主题的扁平设计 QML 组件库。**
[简体中文](./docs/README_CN.md)

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/releases/latest) 
[![GitHub](https://img.shields.io/github/license/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/blob/main/LICENSE)
[![GitHub all releases](https://img.shields.io/github/downloads/Nandina/NandinaUI/total?style=flat-square)](https://github.com/Nandina/NandinaUI/releases)
[![GitHub issues](https://img.shields.io/github/issues/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/issues)

## preview
***正在努力更新中***

## Quick start
1. **clone this repo**
   ``` bash
    git clone https://github.com/CvRain/NandinaUI.git
   ```

2. **build the project**

    Compile this project any way you like, but make sure the following files exist

    1. Library (in order to use c++ qml components):
       * In Linux：`libNandinaUI.a` or `libNandinaUI.so`
       * In Windows：`NandinaUI.lib` or `NandinaUI.dll`
       * In MacOS：`libNandinaUI.a` or `libNandinaUI.dylib` (Probably. Sorry I never used MacOS)
  
    * Use cmake to build the project  
       ``` bash
        cd NandinaUI
        mkdir build && cd build
        cmake ..
        make
        ```
    * Or use QtCreator to build the project  
        the build project will generate in `build/compile_kit/`  
        such as: `NandinaUI/build/Qt_6_8_1_gcc_64-Debug/NandinaUI`

3. **how to use**

   1. Copy the 'NandinaUI' folder from your project to your project
   2. Copy the compiled lib file to the 'NandinaUI' folder
   3. Add the following content to the 'CMakeLists.txt' of the project:
       ```cmake
       add_subdirectory(NandinaUI)
       target_link_libraries(<your_target> PRIVATE NandinaUI)
       ```
   4. If you are using Qt 6.8.1 or later, add the following in 'CMakeLists.txt' :
      ```cmake
      if (QT_KNOWN_POLICY_QTP0001)
         qt_policy(SET QTP0001 NEW)
      endif ()

      if (QT_KNOWN_POLICY_QTP0004)
         qt_policy(SET QTP0004 NEW)
      endif ()
      ```

4. **notice**
    * Make sure you build the same version as you use
    * Do not mix Realse and Debug versions

5. **tips**
**exaple: use NandinaUI in project**

```cmake
cmake_minimum_required(VERSION 3.16)

project(TestPlugin VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(NandinaUI)

if (QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif ()

if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif ()


qt_add_executable(appTestPlugin
    main.cpp
)

qt_add_qml_module(appTestPlugin
    URI TestPlugin
    VERSION 1.0
    RESOURCE_PREFIX /TestPlugin
    QML_FILES
        qml/Main.qml
)

set_target_properties(appTestPlugin PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appTestPlugin
    PRIVATE Qt6::Quick  NandinaUI
)

include(GNUInstallDirs)
install(TARGETS appTestPlugin
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

```