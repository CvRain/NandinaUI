# Popover

Popover 把一块补充内容锚定在触发控件旁边。它适合放简短说明、筛选器或少量操作：内容浮在页面之上，但不会像 Dialog 那样阻断整个页面。

Popover 只负责触发、定位、关闭和焦点作用域，内容可以是任意 `NanControl`。页面作者不需要创建或管理 `OverlayHost`。

## 何时使用

| 场景 | 建议 |
| --- | --- |
| 点击按钮后展示少量表单或补充操作 | `Popover` |
| 展示带条目语义和键盘漫游的操作列表 | `DropdownMenu` |
| 必须阻断页面、要求用户立即处理 | `Dialog` |
| 只显示一句悬停提示 | `Tooltip` |

## 快速开始

```cpp
auto trigger = ui.make<widget::Button>("筛选").build();
auto content = ui.column()
    .gap(8.0F)
    .children(
        ui.make<widget::Label>("筛选条件"),
        ui.make<widget::Checkbox>("只看未完成")
    )
    .build();

auto popover = ui.make<widget::Popover>(trigger, content).build();
```

触发控件收到鼠标单击或 Enter/Space 激活时自动切换浮层。`open()`、`close()` 和 `toggle()` 仍可用于程序控制；这些操作都是幂等的。

## 内容与定位

Popover 有两个单子槽位：

| 槽位 | 接口 | 说明 |
| --- | --- | --- |
| 触发器 | `set_trigger()` / `trigger()` | 留在页面布局中，也决定组件自身尺寸和锚点 |
| 浮层内容 | `set_content()` / `content()` | 打开时托管到窗口浮层，关闭后保留以便再次打开 |

触发器不能为空；替换触发器或内容时，旧节点会从当前承载位置安全移除。

定位默认使用 `bottom + start`：

| 接口 | 默认 | 说明 |
| --- | --- | --- |
| `set_placement()` | `bottom` | `top` / `bottom` / `left` / `right` |
| `set_alignment()` | `start` | `start` / `center` / `end` |
| `set_gap()` | `8.0F` | 浮层与触发器之间的非负逻辑距离 |
| `set_viewport_padding()` | `8.0F` | 浮层与视口边缘保留的最小距离 |

定位器会在首选方向空间不足时翻转，并把最终矩形收进视口。窗口缩放、触发器滚动或重新布局后，已打开的浮层会重新定位。

## 打开、关闭与事件

| 接口 | 说明 |
| --- | --- |
| `open()` / `close()` / `toggle()` | 控制展开状态 |
| `is_open()` | 当前是否处于展开状态 |
| `set_dismissible(bool)` | 控制是否允许 Escape 和点击外部关闭，默认 `true` |
| `set_on_open()` | 从关闭变为打开时调用一次 |
| `set_on_close()` | 正常关闭完成时调用一次 |

组件从场景树卸载时会直接释放浮层，不执行用户关闭回调，也不会把焦点恢复到已经离树的触发器。

## 键盘、指针与焦点

| 输入 | 行为 |
| --- | --- |
| 单击触发器 | 打开或关闭 |
| Enter / Space（触发器聚焦） | 打开或关闭 |
| 单击面板外部 | `dismissible == true` 时关闭 |
| Escape | `dismissible == true` 时关闭 |
| Tab / Shift+Tab | 在浮层内容的可聚焦控件之间循环 |
| 禁用触发器 | 不响应鼠标或键盘激活 |

打开后焦点进入浮层内容，关闭后回到触发器。Popover 是非模态浮层，`block_below` 始终为 `false`；页面其他区域仍可命中，但点击浮层外部通常会先关闭它。

## 主题与实例覆盖

Popover 面板使用 `popover`、`border` 语义色和共享间距、边框、圆角 token。实例级覆盖使用 `PopoverRecipeRule`：

```cpp
popover->set_override(theme::PopoverRecipeRule {
    .panel_radius = theme::ThemeScalar::literal(4.0F),
    .metrics_padding_x = theme::ThemeScalar::literal(16.0F),
});
```

可覆盖字段包括面板填充、边框、边框宽度、圆角，以及水平/垂直内边距、配方间距和最小高度。`set_gap()` 控制锚点距离；配方里的 `metrics_gap` 是主题默认度量，两者不要混为同一层 API。

## 无障碍

Popover 节点报告通用容器角色，`value` 为 `expanded` 或 `collapsed`，展开状态同时映射为布尔状态。触发器与浮层内容保留各自语义；内容中的按钮、输入框等仍由它们自己暴露动作。

## 无窗口上下文

没有窗口级浮层服务时，Popover 回退到原场景树中显示。此路径保证内容显示、焦点和程序化关闭，但无法得到完整窗口视口，因此不保证全屏范围的点击外部关闭。应用页面通过 `ui.make<Popover>()` 构造时会自动获得窗口服务。

## 已知空白

- 尚未提供打开/关闭动画，也未接入 `reduced_motion`；
- 当前语义模型没有专门的 expanded 状态，暂以现有布尔状态表达；
- 树内回退模式不能完整模拟窗口级外部点击。
