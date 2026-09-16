# Dialog 迁移契约

Dialog 是统一浮层设施迁移的最后一个阶段 2 组件，用于验证**模态**场景：遮罩阻断、Escape 与点击外部关闭、焦点限制，以及关闭后的焦点恢复。迁移目标是替换内部的呈现与焦点实现，不改变现有应用层构造方式。

## 公共 API

以下接口保持兼容：

- `Dialog::create(theme)` 与 `ComponentTraits<Dialog>::make(ui, title, content)`；
- `set_title()` / `title()`；
- `set_content()`；
- `open()` / `close()` / `is_open()`；
- `set_dismissible()` / `dismissible()`；
- `set_on_close()`；
- 主题覆盖、文本管线与语义接口。

新增命名槽位（[组件公共契约](component_contract.md) 第 4 节）：

- `set_header(std::shared_ptr<NanControl>)`、`set_footer(std::shared_ptr<NanControl>)` 与已有的 `set_content()` 构成三个固定语义区域；
- `set_title(std::string)` 保留为 header 槽位的文本便捷入口：标题文本与 `set_header()` 互斥，后设置者生效；
- 槽位缺省表现为空，不占用高度；header 与 content 之间、content 与 footer 之间由 recipe 的 `gap` 分隔。

Dialog 仍然是组件而非页面：页面作者不需要直接创建 `OverlayHost`，组件从所属窗口的 `BuildContext` 获取窗口级浮层服务。

## 内部职责

Dialog 自身退回为「面板内容 + 淡入淡出状态机」，把以下职责交给浮层设施：

```text
OverlayHost / overlay layer
└── DismissLayer            模态阻断 + 点击外部 / Escape → on_close
    └── FocusScope          初始焦点 + Tab 循环 + 卸载时焦点恢复
        └── DialogPanel     header / content / footer
```

| 现有实现 | 迁移后 |
| --- | --- |
| `z_index_hint()` 提升绘制序 | overlay layer 的层级顺序 |
| `contains_point()` 恒真吞掉遮罩命中 | `OverlayOptions::block_below` |
| `global_bounds()` 撑满父容器 | overlay surface 的视口布局 |
| `trap_focus()` 自研 Tab 循环 | `FocusScope` |
| Escape / 面板外点击判定 | `DismissLayer` |
| `open()` 中的 `request_focus()` | `FocusScope::on_ready()` 的初始焦点 |
| 关闭后的焦点恢复（缺失） | `FocusScope::on_exit_tree()` |

因此 `z_index_hint()`、`contains_point()`、`is_focusable()` 三个 override 以及 `trap_focus()` 会被移除；它们不是应用层接口，对应断言改为对浮层行为的断言。

### 焦点落点与 Escape 的可达性

`DismissLayer` 与 `FocusScope` 都通过 `on_input_capture` 生效，而按键的派发路径是「从当前焦点节点沿祖先链向上冒泡」。因此**只有焦点位于 FocusScope 内部时，Escape 才可能到达 DismissLayer**。

现状有一个必须补上的缺口：`NanSceneTree::focus_first_within()` 在作用域内没有可聚焦控件时返回 `false` 并且不改动焦点，焦点会留在浮层之外，Escape 与 Tab 都进不了浮层。典型场景是只有标题和一行说明、没有任何控件的纯提示对话框；现有实现靠 `is_focusable() == active()` 让 Dialog 自己成为焦点来规避它。

因此 `FocusScope` 需要保证焦点一定落在自己内部：

- 优先聚焦内部第一个可聚焦控件；
- 内部没有可聚焦控件时，回退为把焦点落在作用域自身，使 Escape 与 Tab 仍然沿浮层路径派发。

注意兜底不能简单地把 `FocusScope::is_focusable()` 改成恒真：`_collect_focusable_nodes()` 会把作用域自身排在子节点之前，那样 Tab 会停在不可见的容器上而不是第一个控件。兜底应由 `on_ready()` 在 `focus_first_within()` 找不到控件时显式执行，具体形态在实现时确定。

这属于共享设施的能力补齐，在 Dialog 迁移内一并完成。

### 浮层层级

Select 与 Tooltip 使用 `present()` 的默认层级，模态内容必须位于它们之上。组件不应直接写 CanvasLayer 的 order 数值（[浮层架构](overlay_architecture.md) 的叠放规则），因此 `OverlayOptions` 的 `order` 改为语义层级：

```cpp
enum class OverlayLevel : int {
    popup = 0,   // 提示、下拉、菜单
    modal = 100, // 模态对话框：高于 popup，并阻断其下所有输入
};
```

Dialog 以 `{.level = OverlayLevel::modal, .block_below = true}` 呈现。Select 与 Tooltip 取默认值，行为不变。

### 关闭动画与 portal 生命周期

现有 `close()` 是「启动淡出 → 动画结束后隐藏并触发 `on_close`」，`is_open()` 在淡出期间即为 `false`。迁移后 `OverlayHandle` 必须晚于淡出释放，否则面板会在半透明状态下消失：

- `close()`：`is_open()` 变 `false`，启动淡出，**保持挂载**；
- 淡出完成：释放 `OverlayHandle` 并触发 `on_close`；
- `open()`：在已有挂载时幂等；淡出未完成时再次 `open()` 复用同一份挂载并反向淡入；
- `on_exit_tree()` 与析构同样释放 `OverlayHandle`。

### dismissible 语义

`DismissLayer` 的回调统一走 `RequestClose` 判定：

- `dismissible() == true`：关闭，与现有行为一致；
- `dismissible() == false`：**不关闭，但吞掉该次 Escape / 点击**。模态对话框不应把按键泄漏给下层，这比现状（`return false` 放行 Escape）更严格，属于有意变更。

## 验收条件

- Dialog 位于 `ScrollView`、`Card` 等裁剪容器内时，遮罩与面板覆盖整个视口而非父容器；这是当前实现无法满足的核心缺陷；
- `block_below` 阻断其下所有输入：打开期间下层按钮、Select、Tooltip 触发器都不响应；
- Dialog 内再开 Select（浮层套浮层）时，Tab 在 Dialog 的 `FocusScope` 内循环，关闭 Select 后焦点回到 Dialog 内的原控件；
- Escape 与点击面板外经 `DismissLayer` 关闭；`dismissible == false` 时两者都不关闭；
- Tab / Shift+Tab 在 Dialog 内的控件间循环，首尾相接；打开期间焦点不会落到浮层之外；
- 只含标题与说明（内部无可聚焦控件）的 Dialog 打开后，焦点仍位于浮层内部，Escape 可以关闭它；
- 关闭后焦点回到打开前的控件；打开前无焦点时回到 `nullptr`，不残留悬空焦点；
- 淡出期间遮罩、面板与内容持续可见，`is_open()` 为 `false`；淡出完成后才卸载并触发 `on_close`；
- 打开动画、`set_override()` 的配方覆盖、语义 role/label 与亮暗切换保持现有测试语义；
- 注入的窗口 `OverlayHost` 先于 Dialog 销毁时，弱服务引用安全失效；
- **无窗口、无 OverlayHost 的 detached 上下文仍以树内模态方式工作**：现有 `tests/dialog_tests.cpp` 的 10 个用例不依赖浮层即可通过。

## 实现状态

契约待评审，尚未开始实现。计划顺序：

1. `scene`：`OverlayOptions::order` → `level`（`OverlayLevel`），Select / Tooltip 取默认值；
2. `widget::internal`：`FocusScope` 补上「内部无可聚焦控件时焦点仍留在作用域内」的兜底；
3. `widget::internal`：抽出 `DialogPanel`，承载 header / content / footer 与淡入淡出；
4. `widget::Dialog`：改为 portal 托管，移除 `z_index_hint()` / `contains_point()` / `is_focusable()` / `trap_focus()`，保留 detached 回退；
5. `ComponentTraits<Dialog>` 注入浮层服务，`create()` 回退到最近的祖先 `OverlayHost`；
6. 测试：现有 10 个用例保持语义，新增裁剪容器、模态阻断、嵌套浮层焦点、焦点恢复、无控件焦点兜底与 `dismissible` 用例；
7. `docs/components/` 补 Dialog 使用参考。
