# NanTypes — 基础枚举与通用类型设计

> **关联 Issue**：Issue 005  
> **模块路径**：`nandina.foundation.nan_types`  
> **命名空间**：`nandina::types`  
> **设计状态**：已实现

---

## 1. 设计目标

为 NandinaUI 框架提供**跨模块共享的基础枚举与通用类型**，避免 layout / runtime / widgets 等各层因缺少统一枚举而重复定义，导致类型不一致、API 不兼容等问题。

### 核心原则

- **单一来源**：所有跨模块的枚举类型仅在此模块中定义一次。
- **轻量无依赖**：仅依赖 `<cstdint>`，不引入任何上层模块。
- **紧凑存储**：枚举底层类型统一使用 `std::uint8_t` 或 `std::uint16_t`。
- **明确语义**：每个枚举值使用自解释的名称。

---

## 2. 类型清单

| 枚举 | 底层类型 | 用途领域 | 说明 |
|------|----------|----------|------|
| `Axis` | `uint8_t` | Layout | 主轴方向（水平/垂直） |
| `Align` | `uint8_t` | Layout | 交叉轴对齐方式 |
| `Justify` | `uint8_t` | Layout | 主轴内容分布方式 |
| `Direction` | `uint8_t` | Layout/Text | 文本与布局方向 |
| `Visibility` | `uint8_t` | Widget | 控件可见性状态 |
| `PointerButton` | `uint8_t` | Input | 鼠标/指针按键 |
| `KeyCode` | `uint16_t` | Input | 键盘按键码 |
| `MouseCursor` | `uint8_t` | Input | 鼠标光标样式 |

---

## 3. 枚举详细设计

### 3.1 Axis — 主轴方向

```cpp
enum class Axis : uint8_t {
    Horizontal = 0,  // 水平主轴
    Vertical   = 1,  // 垂直主轴
};
```

**用途**：Row 容器的 Axis 为 Horizontal，Column 容器的 Axis 为 Vertical。

---

### 3.2 Align — 交叉轴对齐方式

```cpp
enum class Align : uint8_t {
    Start   = 0,  // 对齐到起始边
    Center  = 1,  // 居中对齐
    End     = 2,  // 对齐到结束边
    Stretch = 3,  // 拉伸填充
};
```

**对齐示例**（以 Column 为例，Axis=Vertical，交叉轴为水平方向）：
- `Start` → 子节点左对齐
- `Center` → 子节点水平居中
- `End` → 子节点右对齐
- `Stretch` → 子节点拉伸至容器宽度

---

### 3.3 Justify — 主轴方向内容分布

```cpp
enum class Justify : uint8_t {
    Start        = 0,  // 集中在起始边
    Center       = 1,  // 集中在中间
    End          = 2,  // 集中在结束边
    SpaceBetween = 3,  // 两端对齐，中间等距
    SpaceAround  = 4,  // 每个项目左右等距
    SpaceEvenly  = 5,  // 所有间距完全相等
};
```

---

### 3.4 Direction — 文本/布局方向

```cpp
enum class Direction : uint8_t {
    LeftToRight  = 0,  // 从左到右（LTR）
    RightToLeft  = 1,  // 从右到左（RTL）
    TopToBottom  = 2,  // 从上到下（TTB）
    BottomToTop  = 3,  // 从下到上（BTT）
};
```

**设计考虑**：为国际化（i18n）预留 RTL 支持。

---

### 3.5 Visibility — 控件可见性状态

```cpp
enum class Visibility : uint8_t {
    Visible   = 0,  // 正常显示，参与布局与命中测试
    Hidden    = 1,  // 隐藏，占位但不可见（仍参与布局）
    Collapsed = 2,  // 折叠，不占空间、不可见
};
```

**与 CSS/Flexbox 的对应关系**：
| NandinaUI | CSS |
|-----------|-----|
| `Visible` | `visibility: visible` |
| `Hidden`  | `visibility: hidden` |
| `Collapsed` | `display: none` |

---

### 3.6 PointerButton — 鼠标/指针按键

```cpp
enum class PointerButton : uint8_t {
    Unknown = 0,  // 未知按键
    Left    = 1,  // 左键
    Middle  = 2,  // 中键（滚轮按下）
    Right   = 3,  // 右键
    X1      = 4,  // 后退键
    X2      = 5,  // 前进键
};
```

**迁移历史**：从 `nandina::runtime` 迁移至此，Issue 005。

---

### 3.7 KeyCode — 键盘按键码

```cpp
enum class KeyCode : uint16_t {
    Unknown = 0,
    // 字母键 A-Z（值参考 USB HID 规范）
    A = 4, B, C, D, E, F, G, H, I, J,
    K, L, M, N, O, P, Q, R, S, T,
    U, V, W, X, Y, Z,
    // 数字键 0-9
    Key0 = 39, Key1, Key2, Key3, Key4,
    Key5, Key6, Key7, Key8, Key9,
    // 功能键 F1-F12
    F1 = 58, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,
    // 控制键
    Return = 40,  Escape = 41,
    Backspace = 42,  Tab = 43,
    Space = 44,  Delete = 76,
    // 方向键
    LeftArrow = 80,  RightArrow = 79,
    UpArrow = 82,    DownArrow = 81,
};
```

**设计原则**：
- 使用 USB HID 规范值以兼容各平台
- 只定义最小常用子集，后续可按需扩展
- 使用 `uint16_t` 以容纳更多按键码

---

### 3.8 MouseCursor — 鼠标光标样式

```cpp
enum class MouseCursor : uint8_t {
    Default    = 0,   // 默认箭头
    Pointer    = 1,   // 手型指针（可点击）
    Text       = 2,   // 文本选择（I 形）
    Crosshair  = 3,   // 十字准星
    Wait       = 4,   // 等待（沙漏）
    Hand       = 5,   // 抓取手型
    ResizeNS   = 6,   // 南北缩放
    ResizeEW   = 7,   // 东西缩放
    ResizeNESW = 8,   // 东北-西南缩放
    ResizeNWSE = 9,   // 西北-东南缩放
    NotAllowed = 10,  // 不允许操作
};
```

---

## 4. 命名空间与模块关系

```
模块：nandina.foundation.nan_types
命名空间：nandina::types
使用方式：import nandina.foundation.nan_types;
```

**消费方示例**：

```cpp
// layout 模块使用
import nandina.foundation.nan_types;
using nandina::types::Axis;
using nandina::types::Align;
using nandina::types::Justify;

// runtime 模块使用
using nandina::types::PointerButton;
using nandina::types::KeyCode;
using nandina::types::MouseCursor;
using nandina::types::Visibility;
```

---

## 5. 依赖关系

```
nandina.foundation.nan_types
  └── (仅 C++ 标准库: <cstdint>)
```

没有任何 NandinaUI 模块依赖，是最底层的基础模块之一。

---

## 6. 与类似框架的对比

| 类型 | NandinaUI | Flutter | Qt/QML | CSS Flexbox |
|------|-----------|---------|--------|-------------|
| 方向 | `Axis` | `Axis` | `Qt::Orientation` | `flex-direction` |
| 对齐 | `Align` | `CrossAxisAlignment` | `Qt::AlignmentFlag` | `align-items` |
| 分布 | `Justify` | `MainAxisAlignment` | — | `justify-content` |
| 可见性 | `Visibility` | `Opacity`+显隐 | `QWidget::setVisible` | `visibility`/`display` |
| 方向性 | `Direction` | `Directionality` | `Qt::LayoutDirection` | `direction` |

---

## 7. 后续扩展

- **HitTestMode**：控件命中测试模式（自身命中/子节点命中/两者皆可）
- **CursorStyle** 扩展：随平台渲染后端增加更多光标样式
- **LayoutUnit**：布局单位枚举（px / pct / fr / auto）

---

> **设计文档版本**：v1.0  
> **更新日期**：2026-04-24  
> **维护者**：CvRain