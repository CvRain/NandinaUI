# NandinaUI 自定义组件开发指南

本文档旨在提供一个清晰的指南，帮助开发者在 NandinaUI 组件库中创建、集成和使用自定义组件。

## 简介

NandinaUI 的核心是一个可扩展的组件系统，它允许你通过 C++ 定义组件的逻辑和属性，通过 QML 实现其视觉表现，并通过 JSON 文件来定义样式。这种分离的设计使得组件高度可定制化和可重用。

## 文件结构

一个典型的组件（以 `NanButton` 为例）包含以下部分：

- **C++ 源码**:
  - `NandinaUI/Nandina/Components/src/nan_button.hpp`: 组件的头文件，定义了属性和方法。
  - `NandinaUI/Nandina/Components/src/nan_button.cpp`: 组件的实现文件。
  - `NandinaUI/Nandina/Components/src/nan_button_registrar.hpp`: 组件注册器的头文件。
  - `NandinaUI/Nandina/Components/src/nan_button_registrar.cpp`: 组件注册器的实现文件。
- **QML 文件**:
  - `NandinaUI/Nandina/Components/qml/NanButton.qml`: 组件的 QML 实现。
- **样式文件**:
  - `NandinaUI/Nandina/Components/styles/NanButton/`: 存放该组件所有样式定义的目录。
  - `NandinaUI/Nandina/Components/styles/NanButton/filledPrimary.json`: 一个具体的样式定义文件。

---

## 创建一个新的自定义组件

让我们以创建一个名为 `MyComponent` 的新组件为例，一步步完成整个流程。

### 第 1 步：定义 C++ 组件类

首先，你需要创建 C++ 类来处理组件的逻辑。

#### `MyComponentStyle.hpp`

在 `NandinaUI/Nandina/Components/src/` 目录下创建 `my_component.hpp`。这个类通常继承自 `BaseComponent`，并定义了所有你希望从 QML 中访问的属性。

```cpp
#pragma once

#include <qqmlintegration.h>
#include "base_component.hpp"

namespace Nandina::Components {

class MyComponentStyle : public BaseComponent {
    Q_OBJECT
    QML_ELEMENT // 必须添加，以暴露给 QML

    // 定义你希望在 QML 中使用的属性
    Q_PROPERTY(QString customText READ getCustomText NOTIFY styleChanged)
    Q_PROPERTY(QString backgroundColor READ getBackgroundColor NOTIFY styleChanged)

public:
    explicit MyComponentStyle(QObject *parent = nullptr);

    // Getter 方法
    [[nodiscard]] QString getCustomText() const;
    [[nodiscard]] QString getBackgroundColor() const;

    // Setter 方法
    MyComponentStyle& setCustomText(const QString &text);
    MyComponentStyle& setBackgroundColor(const QString &color);

    void updateColor() override;

signals:
    void styleChanged();

private:
    QString m_customText;
    QString m_backgroundColor;
};

}
```

#### `MyComponentStyle.cpp`

在 `NandinaUI/Nandina/Components/src/` 目录下创建 `my_component.cpp` 来实现你的组件逻辑。

```cpp
#include "my_component.hpp"
#include "themeManager.hpp" // 如果需要主题管理

namespace Nandina::Components {

MyComponentStyle::MyComponentStyle(QObject *parent) : BaseComponent(parent) {
    // 连接到主题变化信号，如果需要的话
    connect(ThemeManager::getInstance(), &Nandina::ThemeManager::paletteChanged,
            this, &MyComponentStyle::updateColor);
}

QString MyComponentStyle::getCustomText() const {
    return m_customText;
}

QString MyComponentStyle::getBackgroundColor() const {
    return m_backgroundColor;
}

MyComponentStyle& MyComponentStyle::setCustomText(const QString &text) {
    if (m_customText != text) {
        m_customText = text;
        emit styleChanged();
    }
    return *this;
}

MyComponentStyle& MyComponentStyle::setBackgroundColor(const QString &color) {
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        emit styleChanged();
    }
    return *this;
}

void MyComponentStyle::updateColor() {
    // 在这里处理主题变化时的颜色更新逻辑
    emit styleChanged();
}

}
```

### 第 2 步：创建组件注册器

为了确保你的组件能被 QML 引擎找到，你需要显式地注册它。

#### `my_component_registrar.hpp`

在 `NandinaUI/Nandina/Components/src/` 目录下创建 `my_component_registrar.hpp`。

```cpp
#pragma once

namespace Nandina::Components {
    void registerMyComponentFactory();
}
```

#### `my_component_registrar.cpp`

在 `NandinaUI/Nandina/Components/src/` 目录下创建 `my_component_registrar.cpp`。

```cpp
#include "my_component_registrar.hpp"
#include "my_component.hpp"
#include "../component_factory.hpp"
#include "../style_loader.hpp"

namespace Nandina::Components {

void registerMyComponentFactory() {
    static bool registered = false;
    if (!registered) {
        ComponentFactoryRegistry::instance().registerFactory(
            QStringLiteral("MyComponent"), // 组件名称，与 QML 文件名一致
            [](ComponentManager *manager, const QJsonDocument &doc) {
                StyleLoader<MyComponentStyle>::load(manager, doc);
            }
        );
        registered = true;
    }
}

}
```

### 第 3 步：在主程序中调用注册器

打开你的主应用程序入口文件（例如 `main.cpp`），在 `main` 函数的开头调用刚刚创建的注册函数。

```cpp
// main.cpp
#include "NandinaUI/Nandina/Components/src/my_component_registrar.hpp"
// ... 其他 includes

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    // 在应用启动时注册你的组件
    Nandina::Components::registerMyComponentFactory();
    // 如果有其他组件，也在这里注册
    Nandina::Components::registerNanButtonFactory();


    // ... 应用的其余部分
    QQmlApplicationEngine engine;
    engine.loadFromModule("TryNandina", "Main");

    return app.exec();
}
```

### 第 4 步：创建 QML 文件

在 `NandinaUI/Nandina/Components/qml/` 目录下创建 `MyComponent.qml` 文件。文件名必须与你在注册器中使用的名称 (`MyComponent`) 一致。

```qml
// MyComponent.qml
import QtQuick
import Nandina.Core

NanComponent {
    id: root

    // 这里的 style 对象就是你在 C++ 中定义的 MyComponentStyle
    property var style: factory.get("MyComponent", styleName)

    // 使用 C++ 中定义的属性
    text: style.customText

    background: Rectangle {
        color: style.backgroundColor
    }
}
```

### 第 5 步：定义组件样式

1.  在 `NandinaUI/Nandina/Components/styles/` 目录下创建一个新目录，名称与组件名一致：`MyComponent`。
2.  在该目录中创建一个 JSON 文件来定义一个样式，例如 `default.json`。

```json5
// NandinaUI/Nandina/Components/styles/MyComponent/default.json
{
    "default": {
        "customText": "Hello from MyComponent!",
        "backgroundColor": "#4CAF50"
    },
    "hover": {
        "backgroundColor": "#45a049"
    }
}
```

### 第 6 步：更新 CMakeLists.txt

最后，将新创建的 C++ 文件添加到 `NandinaUI/Nandina/Components/CMakeLists.txt` 中，以便它们能够被编译。

打开 `NandinaUI/Nandina/Components/CMakeLists.txt` 并将你的新文件添加到 `SOURCES` 列表中：

```cmake
# ...
set(SOURCES
    # ... 已有的文件
    src/my_component.cpp
    src/my_component_registrar.cpp
)
# ...
```

---

## 如何在应用中使用你的组件

完成以上步骤后，你就可以在你的 QML 应用中像使用其他任何 NandinaUI 组件一样使用 `MyComponent` 了。

```qml
// 在你的应用 QML 文件中 (e.g., Main.qml)
import QtQuick
import Nandina.Components // 导入组件库

// ...
MyComponent {
    styleName: "default" // 指定你希望使用的样式
}
// ...
```

恭喜！你已经成功创建并集成了一个全新的自定义组件。

