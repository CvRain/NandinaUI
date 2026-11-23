# Resizable

> 🟡 实验性（experimental）组件使用手册提纲；框架、符号表与默认值已按代码核对，叙述性正文待维护者手写。

<!-- TODO: 一句话说明 Resizable（双栏可调分割容器）的定位：按比例分配 first / second 两栏空间，提供指针拖拽、双击重置默认比例与键盘方向键/无障碍调节，三栏及以上工作台通过嵌套组合表达。 -->

## 何时使用

| 场景 | 建议 |
| --- | --- |
| 需要用户通过拖拽或键盘实时调整左右双栏、上下双栏，或嵌套组合成「左树 - 中表 - 右检查器」工作台布局 | 本组件（`Resizable`） |
| 固定比例或按内容自动伸缩、无需用户交互调整的线性排列 | `Row` / `Column` + `ui.expanded(flex)` |
| 静态展示一条不可拖拽的视觉分隔线 | `Divider` |

## 快速开始

```cpp
#include <nandina/widget/controls.hpp>

// 双栏水平分割（支持构造期直接传入左右两栏，并双击分隔条重置为 default_ratio）
auto workbench =
    ui.resizable(
          left_sidebar,
          ui.resizable(widget::LayoutAxis::vertical, top_list, bottom_table)
              .ratio(0.35F)
              .default_ratio(0.35F)
              .min_panes_extent(100.0F, 160.0F)
    )
        .ratio(0.22F)
        .default_ratio(0.22F)
        .min_panes_extent(180.0F, 480.0F)
        .on_ratio_changed([](float effective) {
            // 用户拖拽、双击重置或键盘调节后触发
        });
```

## 尺寸分配与交互规则

- **比例驱动**：主轴总长减去 `handle_thickness()`（默认 `8.0F` DIP）后为可用主轴空间 `available`。第一栏目标尺寸为 `available * ratio()`，并钳制在 `[min_first_extent(), available - min_second_extent()]` 范围内；第二栏获得剩余空间 `available - first_extent`。
- **窄容器回退**：当外层容器缩小至 `available < min_first_extent() + min_second_extent()` 时，按两侧最小尺寸权重比例平滑缩减，保证 `first + handle + second == total_main` 且不产生负尺寸或越界。
- **指针与键盘交互**：
  - 左键按住分隔条拖拽实时更新比例（记录按下点与分隔条起点的偏移，点击不跳变）。
  - 双击分隔条（`reset_on_double_click() == true`，默认开启）将比例重置为 `default_ratio()`（默认 `0.5F`）。
  - 聚焦分隔条后：水平分割下 `Left` / `Right` 按 `keyboard_step()`（默认 `0.05F`）递减/递增，垂直分割下 `Up` / `Down` 递减/递增；`Home` / `End` 跳至最小/最大允许比例。
- **静默程序写入与事件保活**：程序化调用 `set_ratio()` / `reset_ratio()` 静默更新布局；用户交互改变有效比例时触发 `on_ratio_changed` 与 `ratio_changed()`，触发期间持有自身保活，允许回调释放最后一个外部 `shared_ptr<Resizable>`。

## 公开 API

| 接口 | 说明 |
| --- | --- |
| `Resizable::create(axis, theme)` | 命令式创建（默认 `LayoutAxis::horizontal`） |
| `ui.resizable(first, second)` / `ui.resizable(axis, first, second)` | `BuildContext` 与 `widget::authoring` 构造简写 |
| `ui.make<Resizable>(axis)` / `ui.make<Resizable>(ratio_signal, axis)` | `ComponentTraits<Resizable>` 入口（含 `Signal<float>` 双向绑定重载） |
| `set_axis(LayoutAxis)` / `axis()` | 设置或查询分割方向（`horizontal` 左右分栏，`vertical` 上下分栏） |
| `set_first_child(control)` / `first_child()` | 第一栏（左栏或上栏）槽位；拒绝空指针、跨槽复用、已挂载节点或祖先环 |
| `set_second_child(control)` / `second_child()` | 第二栏（右栏或下栏）槽位 |
| `add(control)` / `clear_children()` | 按序填充 `first_child` 与 `second_child`（供 `.children(a, b)` 使用；超过两项抛 `std::logic_error`） |
| `set_ratio(float)` / `ratio()` / `effective_ratio()` | 设置或查询请求比例（夹紧到 `[0, 1]`）及经最小尺寸钳制后的实际生效比例 |
| `set_default_ratio(float)` / `default_ratio()` / `reset_ratio()` | 默认比例与程序化重置 |
| `set_reset_on_double_click(bool)` / `reset_on_double_click()` | 启用或禁用双击分隔条重置比例（默认 `true`） |
| `set_min_first_extent(float)` / `set_min_second_extent(float)` / `set_min_panes_extent(first, second)` | 两侧最小主轴尺寸（默认均为 `48.0F` DIP；非有限或 `< 0` 抛 `std::invalid_argument`） |
| `set_handle_thickness(float)` / `handle_thickness()` | 分隔条槽宽（默认 `8.0F` DIP；`< 1.0F` 抛 `std::invalid_argument`） |
| `set_keyboard_step(float)` / `keyboard_step()` | 键盘单步比例增量（默认 `0.05F`，范围 `(0.0F, 1.0F]`） |
| `set_disabled(bool)` / `disabled()` | 禁用时释放捕获与焦点，忽略拖拽、键盘与语义调幅 |
| `set_label(std::string)` / `label()` | 设置或查询分隔条的语义标签（默认 `Resizable divider`） |
| `set_on_ratio_changed(callback)` / `ratio_changed()` | 用户交互改变有效比例时触发的事件 |
| `first_pane_rect()` / `handle_rect()` / `second_pane_rect()` | 最近一次布局计算的两栏与分隔条局部矩形 |
| `handle()` / `focus_handle()` | 访问内部分隔条控件或请求将键盘焦点移至分隔条 |

## 焦点、主题与无障碍

- 内部子节点顺序恒为 `first_child → handle → second_child`：焦点位于组件子树内时，Tab / Shift+Tab 通过场景树级上一/下一焦点遍历，按左栏（或上栏）内部控件 → 分隔条 → 右栏（或下栏）内部控件顺序流转，并在边界继续走到组件外，不形成额外焦点陷阱；点击任一分栏内部的非聚焦区域不会误把焦点抢到分隔条上。
- 不新增独立主题配方：分隔条直接消费当前主题语义色（`border` 分隔线、`surface_variant` / `accent` 悬停与拖拽轨道高亮、`on_surface_variant` / `primary` 中央握把胶囊、`ring` 键盘焦点环），随亮暗与主题方案切换自动重绘。
- 无障碍语义：外层容器暴露 `semantics::Role::generic`，分隔条节点暴露 `semantics::Role::slider`（支持 `focus`、`increment`、`decrement`、`set_value` 动作）。
