# Accordion

> 🟡 批 7 experimental 使用手册提纲；接口与状态行为已按代码核对。正文和 Showcase 页面由维护者手写。

## 核对表

| 项目 | 当前接口/行为 |
| --- | --- |
| 创建 | `ui.make<Accordion>()` / `Accordion::create(theme)`；默认 `single` |
| Section | `set_sections(vector<AccordionSection>)` / `clear_sections()`；id 非空唯一、title 非空、content 非空/已脱离树/未设置 anchors、不得与 Accordion 形成祖先/后代环、内容不得重复 |
| 状态 | `single`（互斥、允许全部收起）/ `multiple`；`set_open` / `toggle` / `is_open` / `open_ids` / `set_open_ids` / `clear_open` |
| 事件 | `open_changed()` / `set_on_open_changed()`；用户动作（触发器点击/Enter/Space、`toggle`）触发，程序化 setter 静默 |
| 键盘 | Up/Down/Home/End 在 section 触发器间移动焦点，跳过 disabled；`focus_section(id)` 提供窄程序化聚焦入口；Enter/Space 由触发器 Button 切换 |
| 禁用 | `set_section_disabled` 只限制触发器与用户 toggle；不阻止程序化 `set_open` |
| 布局 | 纵向堆叠；`set_gap` 有限非负；自然高度、默认矩形裁剪 |
| 样式 | 无新配方：section 复用 Collapsible/Button；分隔线由作者用 Surface/Divider 或自定义节点组合 |
| 语义 | generic 容器 + `set_label`；每个触发器是 button + expanded（沿用 Collapsible） |
| 边界 | 无动画、无 model/delegate 驱动、无多列、无 typeahead、无 ARIA controls/labelledby |

`section_node()` / `trigger_node()` 是只读查询；状态与描述更新使用 Accordion setter，
聚焦使用 `focus_section(id)`，不会暴露可替换触发器回调的可写节点。

## 最小结构

```cpp
auto accordion = ui.make<nandina::widget::Accordion>().build();
accordion->set_sections({
    {.id = "general", .title = "通用", .content = ui.make<Label>("内容 A").build()},
    {.id = "advanced", .title = "高级", .content = ui.make<Label>("内容 B").build(),
     .disabled = true},
});
ui.connect(accordion->open_changed(), [accordion] {
    // 用户动作后读取 aggregate 状态。
    (void)accordion->open_ids();
});
```

## 事件、绑定与焦点

<!-- TODO: 维护者说明静默 setter、用户事件与页面 Signal 组合。 -->

## 主题与组合

<!-- TODO: 维护者补充分隔线组合、Surface 外框与 Showcase 验收。 -->

契约与回归见 [折叠内容组件](../references/disclosure_components.md) §7。
