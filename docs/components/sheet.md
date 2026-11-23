# Sheet

> 🟡 实验性（experimental）组件使用手册提纲；框架、符号表与默认值已按代码核对，叙述性正文待维护者手写。
> 契约见 [Dialog 与 Sheet 契约](../references/dialog_migration.md#sheet贴边模态抽屉v1)。

<!-- TODO: 一句话说明 Sheet（贴边模态抽屉）与 Dialog 的分界：Dialog 在视口居中聚焦短决策，Sheet 从视口四边停靠承载详单检查器、设置面板或二级导航抽屉。 -->

## 何时使用

| 场景 | 建议 |
| --- | --- |
| 在不离开当前页面的前提下，从右侧/左侧/顶部/底部滑出详情检查器、编辑面板或抽屉导航 | 本组件（`Sheet`） |
| 居中展示简短表单或可关闭模态确认 | [`Dialog`](dialog.md) |
| 不可关闭、要求用户必选其一的高风险确认 | [`AlertDialog`](alert_dialog.md) |
| 锚定在某个触发控件旁边的非模态浮层 | [`Popover`](popover.md) |

## 快速开始

```cpp
#include <nandina/widget/controls.hpp>

auto sheet = ui.make<widget::Sheet>(widget::SheetSide::right, "条目详情").build();
sheet->set_description("查看并编辑当前选中条目的属性。");
sheet->set_content(ui.make<widget::Label>("主体内容区").build());
const std::weak_ptr<widget::Sheet> weak_sheet = sheet;
sheet->set_footer(
    ui.row(
          ui.make<widget::Button>("关闭").on_click([weak_sheet] {
              if (auto current = weak_sheet.lock()) {
                  current->close();
              }
          })
    )
        .build()
);

auto open_btn = ui.make<widget::Button>("打开抽屉").on_click([sheet] { sheet->open(); });
```

## 停靠方向与尺寸规则

`enum class SheetSide { top, right, bottom, left }`（默认 `SheetSide::right`）：

| 停靠方向 | 交叉轴（铺满视口） | 主轴（抽屉厚度） | 内部纵向分布 |
| --- | --- | --- | --- |
| `right` / `left` | 高度 = 视口高度 `H` | 宽度 = `min(extent(), W)`（默认 `360.0F` DIP） | `header` 置顶，`footer` 贴底，`content` 占据中间剩余可用高度 |
| `top` / `bottom` | 宽度 = 视口宽度 `W` | 高度 = 显式 `set_extent(h)`，或未设时取三槽位自然总高（不低于 `min_height`，不超过 `H`） | `header` 置顶，`footer` 贴底，`content` 居中段 |

## 公开 API

| 接口 | 说明 |
| --- | --- |
| `Sheet::create(side, theme)` / `Sheet::create(title, side, theme)` | 命令式创建入口 |
| `ui.make<Sheet>(title, content)` / `ui.make<Sheet>(side, title, content)` | 声明式入口（自动注入主题与窗口 `OverlayHost`），支持 `.open(bool)` |
| `set_side(SheetSide)` / `side()` | 切换或查询停靠边缘（非法枚举值抛 `std::invalid_argument`） |
| `set_extent(float)` / `clear_extent()` / `has_explicit_extent()` / `extent()` | 显式设置或恢复主轴厚度（非有限或 `<= 0` 抛 `std::invalid_argument`）；`extent()` 返回显式值或 recipe 的 `panel_width` 配置回退值，`top`/`bottom` 清除显式值后的实际自然高度以布局结果为准 |
| `set_title(string)` / `title()` | 内置头部标题文本（与自定义 `set_header()` 互斥，后设置者生效） |
| `set_description(string)` / `description()` | 内置头部说明文本（最多 4 行自动换行，与自定义 `set_header()` 互斥） |
| `set_header(control)` / `header()` | 自定义头部槽位（非空、未挂载、无祖先/自引用环） |
| `set_content(control)` / `content()` | 主体内容槽位 |
| `set_footer(control)` / `footer()` | 底部操作区槽位（在面板底部内边距处贴底对齐） |
| `open()` / `close()` / `set_open(bool)` / `is_open()` | 打开与关闭；`close()` 启动淡出，淡出期间 `is_open() == false` 且保持浮层挂载，动画完成后卸载 |
| `set_dismissible(bool)` / `dismissible()` | 默认为 `true`（点击遮罩或按 Escape 关闭）；`false` 时吞掉遮罩点击与 Escape 且不关闭 |
| `set_on_close(callback)` / `closed()` | 淡出完成并卸载后触发；外层保活，允许回调释放最后一个 `shared_ptr<Sheet>` |
| `set_override(DialogRecipeRule)` / `resolved_style()` | 复用 `DialogRecipe`；默认面板圆角 `panel.radius = 0.0F`（直角贴边），可由 `DialogRecipeRule::panel_radius` 等字段覆盖 |

## 焦点、嵌套浮层与无障碍

- 打开时通过 `FocusScope` 将焦点移入面板首个可聚焦控件（无可聚焦子项时由 `FocusScope` 自身兜底以保证 Escape/Tab 可达），Tab / Shift+Tab 限制在抽屉内循环；关闭卸载后恢复打开前焦点。
- 抽屉内部打开 `Select`、`DropdownMenu` 等子浮层时，自动抬升为 `OverlayLevel::nested_popup`，位于模态遮罩之上且可正常交互。
- 浮层承载时页面内的 `Sheet` 锚点保持不可见且不占父容器 `gap`；可见面板节点暴露 `semantics::Role::dialog`，`label` 取当前 `title()`。
- 无 `OverlayHost` 的单元测试或离屏树自动回退为树内模态。
