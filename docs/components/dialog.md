# Dialog

Dialog 展示需要用户立即处理、并且应当暂时阻断页面其余操作的模态内容：确认操作、危险提示、必须填写的表单。

它负责四件事：铺满视口的遮罩、居中的面板、对下层输入的阻断，以及打开期间的焦点限制与关闭后的焦点恢复。这些能力由统一浮层设施提供，因此面板不会被 `ScrollView`、`Card` 之类的裁剪容器截断。

> 组件本身属于 recommended；`set_header()` / `set_footer()` 是较新的命名槽位，alpha 阶段仍可能细化。

## 不适合使用它的场景

- 只是提示一段信息、不需要用户确认：用页内 `Label` 或后续的 `Alert`；
- 需要持续显示、用户可以继续操作页面其他部分：用 `Card` 或后续的 `Sheet`；
- 需要把浮层锚在某个控件旁边：用 `Tooltip`，或后续的 `Popover`。

## 最小示例

```cpp
auto dialog = ui.make<widget::Dialog>("放弃更改？", ui.make<widget::Button>("确定")).build();

ui.make<widget::Button>("打开").on_click([dialog] { dialog->open(); });
```

`Dialog` 节点本身只是页面里的锚点：打开时面板被托管到窗口浮层，关闭时回到锚点状态。页面作者不需要创建 `OverlayHost`，组件会从 `BuildContext` 取窗口级浮层服务。

## 内容与槽位

面板有 `header` / `content` / `footer` 三个固定语义区域，缺省为空且不占高度：

```cpp
auto dialog = ui.make<widget::Dialog>("删除这条记录？").build();
dialog->set_content(ui.make<widget::Label>("删除后无法恢复。"));
dialog->set_footer(
    ui.make<widget::Row>()
        .children(ui.make<widget::Button>("取消"), ui.make<widget::Button>("删除"))
);
```

| 接口 | 说明 |
| --- | --- |
| `set_title()` / `title()` | header 槽位的文本便捷入口 |
| `set_header()` | 用任意控件取代标题文本；与 `set_title()` 互斥，后设置者生效 |
| `set_content()` | 面板主体 |
| `set_footer()` | 底部操作区，通常放按钮 |

传入的控件必须是尚未挂载的节点（`create()` / `ui.make()` 的产物）；替换槽位时旧内容会被移除。

## 打开、关闭与回调

| 接口 | 说明 |
| --- | --- |
| `open()` / `close()` | 打开与关闭；`close()` 会先播完淡出再卸载面板 |
| `is_open()` | 淡出期间即为 `false`，此时面板仍在绘制 |
| `set_dismissible()` | `false` 时 Escape 与点击遮罩都不关闭，且输入仍被吞掉 |
| `set_on_close()` | 淡出结束、面板卸载后触发一次 |

`close()` 可以从按钮回调里调用；面板被卸载时属于它的浮层句柄一并释放，不会泄漏。

## 输入、焦点与键盘

- 打开期间模态层阻断其下所有输入：下层按钮、Select、Tooltip 触发器都不响应；
- 点击面板之外的遮罩或按 Escape 关闭（`dismissible` 为 `true` 时）；
- Tab / Shift+Tab 在面板内的控件之间循环，焦点不会跑到面板之外；
- 关闭后焦点回到打开前的控件；打开前没有焦点时不残留悬空焦点；
- 面板内没有任何可聚焦控件时，焦点仍然留在浮层内部，Escape 依旧可用；
- 面板内再展开的下拉或提示（例如 `Select`）压过遮罩显示且可以正常操作。

## 主题与实例覆盖

```cpp
auto dialog = widget::Dialog::create();
dialog->set_override(theme::DialogRecipeRule {
    .panel_radius = theme::ThemeScalar::literal(4.0F),
    .metrics_panel_width = theme::ThemeScalar::literal(420.0F),
});
const auto style = dialog->resolved_style();
```

解析顺序遵循[组件公共契约](../references/component_contract.md)：DesignSystem 默认值 → recipe → 实例覆盖。遮罩取 `style.scrim`，面板外观取 `style.panel`，标题排版取 `style.title`。

## 无窗口上下文

脱离窗口的单元测试或离屏绘制场景没有 `OverlayHost`。此时 Dialog 回退为树内模态：面板留在原位置，靠 z 序压在兄弟之上、铺满父容器作为遮罩范围，交互语义与浮层托管时一致。应用层构造方式不变。

## 无障碍

`DialogPanel` 暴露 `dialog` 语义角色，标签取当前标题；两种承载方式下都是它在报告，因此辅助技术拿到的边界就是面板本身。内容槽位中的按钮等控件保持各自的语义，不会被合并。

Dialog 节点本身在浮层承载时只是页面里的锚点：它不可见、不占布局空间，也不报告语义。
