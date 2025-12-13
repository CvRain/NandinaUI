# Nandina Icon System 使用指南

Nandina UI 的图标系统经过重构，现在基于 SVG 路径数据进行渲染。这种方式轻量、高效，并且支持动态修改颜色、线条粗细和填充色。

## 核心组件

*   **NanIconItem**: QML 组件，用于显示图标。
*   **IconManager**: 单例类，提供图标枚举定义 (`IconManager.Icons`)。

## 快速开始

### 1. 使用预定义图标

使用 `icon` 属性指定 `IconManager` 中的枚举值即可显示预定义图标。

```qml
import Nandina.Icon 1.0

NanIconItem {
    width: 24
    height: 24
    icon: IconManager.ICON_CLOSE
    color: "red" // 设置图标颜色
}
```

### 2. 使用自定义 SVG 路径

如果需要显示未注册的图标，可以直接通过 `pathData` 属性传入 SVG 路径字符串。

```qml
import Nandina.Icon 1.0

NanIconItem {
    width: 32
    height: 32
    // 传入 SVG path 的 d 属性值
    pathData: "M12 2L2 22h20L12 2z" 
    color: "blue"
    lineWidth: 2
    fillColor: "yellow"
}
```

## 属性详解

| 属性名 | 类型 | 默认值 | 说明 |
| :--- | :--- | :--- | :--- |
| `icon` | `enum` | `IconManager.ICON_NONE` | 预定义图标枚举。设置此属性会自动更新 `pathData`。 |
| `pathData` | `string` | `""` | SVG 路径数据（即 `<path d="...">` 中的内容）。 |
| `color` | `color` | `Qt.black` | 线条颜色（Stroke Color）。 |
| `lineWidth` | `real` | `1.0` | 线条粗细（Stroke Width）。 |
| `fillColor` | `color` | `Qt.transparent` | 填充颜色（Fill Color）。 |

## 如何添加新图标

如果您需要将一个图标添加到系统的预定义列表中（即通过 `IconManager` 枚举使用），请按照以下步骤操作：

### 步骤 1: 定义枚举

在 `NandinaUI/Nandina/Icon/icon_manager.hpp` 中，向 `Icons` 枚举添加新的成员。

```cpp
// icon_manager.hpp
enum class Icons {
    // ... 现有图标
    ICON_MY_NEW_ICON = 7, // 新增图标
};
```

### 步骤 2: 注册路径数据

在 `NandinaUI/Nandina/Icon/nan_icon_item.cpp` 的 `getPathForIcon` 函数中，添加对应的 `case` 分支并返回 SVG 路径字符串。

```cpp
// nan_icon_item.cpp
static QString getPathForIcon(IconManager::Icons icon) {
    switch (icon) {
        // ... 现有 case
        case IconManager::Icons::ICON_MY_NEW_ICON:
            return "M10 10 H 90 V 90 H 10 Z"; // 您的 SVG 路径
        default:
            return "";
    }
}
```

### 步骤 3: 重新编译

完成上述修改后，重新编译项目即可在 QML 中使用 `IconManager.ICON_MY_NEW_ICON`。

## 最佳实践

1.  **图标尺寸**: 默认 `NanIconItem` 的 `implicitWidth` 和 `implicitHeight` 为 24。建议 SVG 路径基于 24x24 的视口（viewBox）设计，以保证最佳显示效果。
2.  **性能**: `NanIconItem` 内部使用 `QSvgRenderer` 缓存渲染结果。频繁修改 `pathData` 会触发 SVG 解析，但修改颜色 (`color`, `fillColor`) 或线宽 (`lineWidth`) 只会触发重新渲染，开销较小。
