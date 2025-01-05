![](../Image/placeholder.jpg)
<br>
# NandinaUI
**南天竹： 一套使用 Catppuccin 主题的扁平设计 QML 组件库。**
[English](../README.md)

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/releases/latest) 
[![GitHub](https://img.shields.io/github/license/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/blob/main/LICENSE)
[![GitHub all releases](https://img.shields.io/github/downloads/Nandina/NandinaUI/total?style=flat-square)](https://github.com/Nandina/NandinaUI/releases)
[![GitHub issues](https://img.shields.io/github/issues/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/issues)

## 组件预览
***正在努力更新中***

## 快速开始
1. **克隆当前项目**
   ``` bash
    git clone https://github.com/CvRain/NandinaUI.git
   ```

2. **对项目进行编译**
    
    使用任何你喜欢的方式编译本项目，但是需要确保存在如下文件：
    1. 库文件（以使用C++构建的QML组件）：
       * 在Linux上：`libNandinaUI.a`或`libNandinaUI.so`
       * 在Windows上：`NandinaUI.lib`或`NandinaUI.dll`
       * 在MacOS上：`libNandinaUI.a`或`libNandinaUI.dylib` （没用用过MacOS，不确定是否是这个文件）
  
    * 使用cmake命令行直接编译
       ``` bash
        cd NandinaUI
        mkdir build && cd build
        cmake ..
        make
        ```
    * 使用QtCreator编译
        如果使用QtCreator进行编译，项目将生成在`build/编译工具/`  
        例如这样：`NandinaUI/build/Qt_6_8_1_gcc_64-Debug/NandinaUI`

3. **如何使用**
   1. 将项目中`NandinaUI`文件夹复制到你的项目中
   2. 将编译出的lib文件复制到`NandinaUI`文件夹下
   3. 在项目的`CMakeLists.txt`中添加如下内容：
       ```cmake
       add_subdirectory(NandinaUI)
       target_link_libraries(<your_target> PRIVATE NandinaUI)
       ```
   4. 如果使用的是Qt 6.8.1及以上版本，在`CMakeLists.txt`中添加如下内容：
      ```cmake
      if (QT_KNOWN_POLICY_QTP0001)
         qt_policy(SET QTP0001 NEW)
      endif ()

      if (QT_KNOWN_POLICY_QTP0004)
         qt_policy(SET QTP0004 NEW)
      endif ()
      ```

4. **注意**
   * 确保构建的版本和使用的版本一致
   * 不要混用Realse 和 Debug 版本

5. **附录**
**项目中使用NandinaUI的CMakeLists.txt示例**
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