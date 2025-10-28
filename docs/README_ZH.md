![](../Image/placeholder.jpg)
<br>
# NandinaUI
**南天竹： 一套使用`Catppuccin`配色方案的扁平设计QML组件库。**
**NandinaUI** A flat design QML component library based on `Catppuccin` color scheme.


[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/releases/latest)
[![GitHub](https://img.shields.io/github/license/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/blob/main/LICENSE)
[![GitHub all releases](https://img.shields.io/github/downloads/Nandina/NandinaUI/total?style=flat-square)](https://github.com/Nandina/NandinaUI/releases)
[![GitHub issues](https://img.shields.io/github/issues/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/issues)

## 组件预览
***正在努力更新中***

## 快速开始

### 方式一：作为子项目引入
A. 非git项目，假设新建的项目名为`TryNandina`
1. 在您的项目下将本项目clone到本地。
```bash
cd TryNandina
git clone https://github.com/CvRain/NandinaUI.git
```
2. 后续步骤跳转到`C. 配置CMakeLists.txt`


B. git项目，假设项目名为`TryNandina`
1. 创建一个子项目，并启用git
```bash
cd TryNandina
git init
```
2. 将项目作为子项目添加到您的项目中
```bash
git submodule add https://github.com/CvRain/NandinaUI.git
```
3. 后续步骤跳转到`C. 配置CMakeLists.txt`

C. 配置CMakeLists.txt
1. 添加子项目
```cmake
add_subdirectory(Nandina)
```

2. 添加依赖项
```cmake
target_link_libraries(appTryNandina
    PRIVATE Qt6::Quick Qt6::Core Nandinaplugin
)
```

3. 完整配置参考
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

### 方式二：将项目作为库使用
1. 获取项目
```bash
git clone https://github.com/CvRain/NandinaUI.git
```

2. 编译项目
编译后输出文件在本项目同级目录下
```bash
cmake . -B build -DCMAKE_PREFIX_PATH=/path/to/qt/install/dir
cd build
make install
```

3. 添加依赖项
将输出的文件夹(Release/Nandina)拖入待使用的项目中，假设项目名称为`TryNandina`并在CMakeLists.txt中添加如下内容
```cmake
#加入你需要的组件
set(NANDINA_QML_FILES
    Nandina/Window/NandinaWindow.qml
)

qt_add_library(Nandina STATIC)
qt_add_qml_module(Nandina
    URI MyPlugin
    QML_FILES
        ${NANDINA_QML_FILES}/
)

target_link_libraries(appTryNandina
    PRIVATE Qt6::Quick Nandinaplugin
)
```

完整配置
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

4. 编辑`main.cpp`，添加如下内容
```cpp
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(MyPluginPlugin)
```
