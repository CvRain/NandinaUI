## Nandina Types 基础枚举与通用类型设计文档

### 1. 设计概述

- **模块名**: `nandina.foundation.nan_types`
- **命名空间**: `nandina::types`
- **目标**: 提供跨模块共享的基础类型与枚举，避免 layout/runtime/widgets 各层重复定义相同的枚举类型。
- **定位**: 本模块属于 `foundation` 层，零依赖——不依赖任何其他 nandina 模块。
- **对应 Issue**: Issue 005（Milestone M1 — Foundation 与 Runtime MVP）

#### 设计原则

1. **单一定义**：每个枚举类型只在此处定义一次，其他模块通过 `import nandina.foundation.nan_types` 使用。
2. **枚举值稳定性**：一旦发布，不应随意修改已有枚举值的含义或数值，只能向后追加新值。
3. **与 `geometry` 层分离**：`types` 仅存放枚举和轻量类型标签，不包含几何运算逻辑（几何逻辑在 `geometry` 层）。
4. **可迁移性**：部分枚举（如 `PointerButton`）从 `runtime` 迁移至此，迁移过程中两处同时保留一个版本，待下游全部切换后删除旧定义。

---

### 2. 枚举清单

| 枚举 | 底层类型 | 用途 | 消费方 |
|------|---------|------|--------|
| `Axis` | `std::uint8_t` | 主轴方向 | layout, widgets |
| `Align` | `std::uint8_t` | 交叉轴对齐 | layout |
| `Justify` | `std::uint8_t` | 主轴内容分布 | layout |
| `Direction` | `std::uint8_t` | 文本/布局方向 | layout, text |
| `Visibility` | `std::uint8_t` | 控件可见性 | widgets, runtime |
| `PointerButton` | `std::uint8_t` | 鼠标/指针按键 | runtime（从 `nandina::runtime` 迁移） |
| `KeyCode` | `std::uint16_t` | 键盘按键码 | runtime |
| `MouseCursor` | `std::uint8_t` | 鼠标光标样式 | runtime, widgets |

#### 2.1 Axis

| 枚举值 | 含义 |
|--------|------|
| `Horizontal` | 水平方向（X 轴） |
| `Vertical` | 垂直方向（Y 轴） |

**设计说明**：`Axis` 用于布局系统标识主轴方向。`Row` 容器使用 `Horizontal`，`Column` 容器使用 `Vertical`。

#### 2.2 Align

| 枚举值 | 含义 |
|--------|------|
| `Start` | 对齐到起始位置（取决于 Direction） |
| `Center` | 居中对齐 |
| `End` | 对齐到结束位置 |
| `Stretch` | 拉伸填充可用空间 |

**设计说明**：`Align` 用于布局系统中子节点在交叉轴上的对齐方式。`Stretch` 会使子节点尺寸扩展以填满交叉轴可用空间，常用于弹性容器。

#### 2.3 Justify

| 枚举值 | 含义 |
|--------|------|
| `Start` | 子节点从起始位置开始排列 |
| `Center` | 子节点居中排列 |
| `End` | 子节点从结束位置开始排列 |
| `SpaceBetween` | 首尾无间距，中间均匀分布 |
| `SpaceAround` | 每个子节点两侧间距相等 |
| `SpaceEvenly` | 所有间距（含首尾）均匀一致 |

**设计说明**：`Justify` 控制子节点在主轴上的分布方式，与 CSS Flexbox 的 `justify-content` 语义一致。

#### 2.4 Direction

| 枚举值 | 含义 |
|--------|------|
| `LeftToRight` | 从左到右（默认 LTR 布局） |
| `RightToLeft` | 从右到左（RTL 布局） |
| `TopToBottom` | 从上到下（纵向书写） |
| `BottomToTop` | 从下到上 |

**设计说明**：`Direction` 影响 `Align::Start`/`End` 的朝向以及文本渲染方向。仅保留四个基本方向，未引入 `Inherit` 值（方向由上层容器通过 layout 协议传递）。

#### 2.5 Visibility

| 枚举值 | 含义 |
|--------|------|
| `Visible` | 正常显示，参与布局与命中测试 |
| `Hidden` | 隐藏，占位但不可见（仍参与布局体积计算） |
| `Collapsed` | 折叠，不占空间、不可见、不参与布局 |

**设计说明**：三态可见性系统与 CSS `visibility` / `display` 概念对齐。`Hidden` 和 `Collapsed` 的区别在于是否保留布局空间：

- **典型使用场景**：
  - `Visible`：正常的可见控件
  - `Hidden`：Tab 切换中仅切换隐藏而非销毁的面板
  - `Collapsed`：条件性渲染的内容（如展开/折叠详情区）

#### 2.6 PointerButton

| 枚举值 | 含义 |
|--------|------|
| `Unknown` | 未知/未定义按键 |
| `Left` | 左键 |
| `Middle` | 中键（滚轮） |
| `Right` | 右键 |
| `X1` | 侧键 1（返回） |
| `X2` | 侧键 2（前进） |

#### 2.7 KeyCode

采用与 USB HID 键盘扫描码兼容的数值映射（基于 `hid_usage_tables` 中的 Keyboard/Keypad Page (0x07)）。

| 枚举值 | 数值 | 含义 |
|--------|------|------|
| `Unknown` | 0x00 | 未知/空按键 |
| `A`–`Z` | 0x04–0x1D | 字母键 |
| `Key0`–`Key9` | 0x27–0x30 | 数字键 |
| `Return` | 0x28 | 回车 |
| `Escape` | 0x29 | Esc |
| `Backspace` | 0x2A | 退格 |
| `Tab` | 0x2B | Tab |
| `Space` | 0x2C | 空格 |
| `Delete` | 0x4C | Delete |
| `LeftArrow` | 0x50 | 左方向键 |
| `RightArrow` | 0x4F | 右方向键 |
| `UpArrow` | 0x52 | 上方向键 |
| `DownArrow` | 0x51 | 下方向键 |
| `F1`–`F12` | 0x3A–0x45 | 功能键 |

**设计说明**：
- 使用 HID 扫描码作为枚举值，使得从平台事件（SDL/Wayland/Windows）到 `KeyCode` 的转换只需简单的值映射或直接对应。
- 当前仅包含最小常用子集，后续按需扩展（如 `Shift`, `Ctrl`, `Alt`, `CapsLock` 等修饰键）。
- 修饰键的设计尚未在此枚举中处理，后续可在 `ModifierKeys` 位掩码类型中补充。

#### 2.8 MouseCursor

| 枚举值 | 含义 | 对应系统光标名 |
|--------|------|--------------|
| `Default` | 默认箭头光标 | `arrow` / `default` |
| `Pointer` | 手型指针（可点击元素） | `pointer` / `hand` |
| `Text` | 文本选择 I 型 | `ibeam` / `text` |
| `Crosshair` | 十字准星 | `crosshair` |
| `Wait` | 等待/忙碌 | `wait` / `busy` |
| `Hand` | 抓取手势 | `grab` |
| `ResizeNS` | 垂直调整大小 | `n-resize` / `s-resize` |
| `ResizeEW` | 水平调整大小 | `e-resize` / `w-resize` |
| `ResizeNESW` | 对角线调整（↗ 方向） | `ne-resize` / `sw-resize` |
| `ResizeNWSE` | 对角线调整（↖ 方向） | `nw-resize` / `se-resize` |
| `NotAllowed` | 禁止操作 | `not-allowed` / `no-drop` |

---

### 3. 模块导出接口

```cpp
export module nandina.foundation.nan_types;

export namespace nandina::types {
    enum class Axis : std::uint8_t;
    enum class Align : std::uint8_t;
    enum class Justify : std::uint8_t;
    enum class Direction : std::uint8_t;
    enum class Visibility : std::uint8_t;
    enum class PointerButton : std::uint8_t;
    enum class KeyCode : std::uint16_t;
    enum class MouseCursor : std::uint8_t;
}
```

---

### 4. 迁移计划

#### 4.1 PointerButton 迁移（`runtime` → `foundation`）

当前 `PointerButton` 定义在 `nandina::runtime` 命名空间的 `nan_window.cppm` 中。Issue 005 要求将其迁移至 `nandina::types`。

**迁移步骤**：
1. ✅ 在 `nandina::types` 中定义 `PointerButton`（已完成）
2. 在 `nan_window.cppm` 中添加 `import nandina.foundation.nan_types;`
3. 在 `nan_window.cppm` 中添加兼容性 using 声明：
   ```cpp
   using nandina::types::PointerButton;
   ```
4. 后续将所有使用 `nandina::runtime::PointerButton` 的地方切换为 `nandina::types::PointerButton`
5. 待所有下游消费方切换完毕后，删除 `nan_window.cppm` 中 `PointerButton` 的原始定义

#### 4.2 KeyCode 与 MouseCursor 接入

这两个枚举当前 `runtime` 中尚无完整定义，直接以 `foundation` 定义为准。

**接入步骤**：
1. ✅ 在 `nandina::types` 中完成定义（已完成）
2. 在 `nan_window.cppm` 中导入并使用 `nandina::types::KeyCode` 和 `nandina::types::MouseCursor`
3. `KeyEvent` 中的 `key_code` 字段保持 `std::int32_t` 不变（与平台事件映射兼容），但新增 `as_key_code()` 转换函数

---

### 5. 后续扩展建议

| 类型 | 优先级 | 说明 |
|------|--------|------|
| `Overflow` | P1 | 内容溢出行为（Visible / Hidden / Clip / Scroll） |
| `Wrap` | P1 | 换行模式（NoWrap / Wrap / WrapReverse） |
| `OverflowWrap` | P2 | 文本溢出的处理方式 |
| `WhiteSpace` | P2 | 空白符处理策略 |
| `VerticalAlign` | P2 | 行内垂直对齐 |
| `TextAlign` | P2 | 文本水平对齐 |
| `FontWeight` | P2 | 字体粗细枚举 |
| `FontStyle` | P2 | 字体风格 |
| `ModifierKeys` | P2 | 修饰键位掩码（Ctrl/Shift/Alt/Meta） |

这些扩展类型将在后续 Issue（如 layout/text/theme 模块）中按需引入，与本模块保持一致命名规范。

---

### 6. 使用示例

```cpp
import nandina.foundation.nan_types;

using nandina::types::Axis;
using nandina::types::Align;
using nandina::types::Justify;
using nandina::types::Direction;
using nandina::types::Visibility;
using nandina::types::PointerButton;
using nandina::types::KeyCode;
using nandina::types::MouseCursor;

// layout 模块使用
class RowLayout {
    Axis direction = Axis::Horizontal;
    Align cross_align = Align::Center;
    Justify main_justify = Justify::SpaceBetween;
};

// widget 可见性控制
struct WidgetState {
    Visibility visibility = Visibility::Visible;
};

// 事件处理
void handle_pointer_down(PointerButton button) {
    if (button == PointerButton::Left) {
        // 处理左键点击
    }
}

void handle_key_event(KeyCode code) {
    if (code == KeyCode::Escape) {
        // 关闭弹窗
    }
}
```

---

### 7. 与几何类型的边界说明

`nandina::types` 与 `nandina::geometry` 的分工如下：

| 模块 | 包含内容 | 示例 |
|------|---------|------|
| `nandina::types` | 枚举与轻量类型标签 | `Axis`, `Align`, `Visibility`, `PointerButton` |
| `nandina::geometry` | 几何数据结构与运算 | `NanPoint`, `NanSize`, `NanRect`, `NanInsets` |

**边界规则**：
- 枚举类型始终放在 `types` 层，即使该枚举主要被 geometry 使用（如几何运算中的 `Alignment` 枚举）。
- 如果某个枚举与几何运算紧密耦合且只在 geometry 内部使用，可在 geometry 模块中定义但必须通过 `nandina::geometry` 命名空间导出。