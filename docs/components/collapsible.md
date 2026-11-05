# Collapsible

> 🟡 P4.2c experimental 使用手册提纲；接口与状态行为已按代码核对。正文和 Showcase 页面由维护者手写。

## 核对表

| 项目 | 当前接口/行为 |
| --- | --- |
| 创建 | `ui.make<Collapsible>(text)` / `Collapsible::create(text,theme)`；非空标签 |
| 双向绑定 | `ui.make<Collapsible>(Signal<bool>&, text)`；页面作用域清理后停止同步 |
| 内容 | `.child(builder_or_control)` / `set_child()` / `clear_child()`；单个脱离树、无锚点控件 |
| 状态 | `.open(bool)` / `set_open(bool)` 静默同步；`open()` 查询；默认 false |
| 用户操作 | `toggle()`、Button 点击/Enter/Space、trigger 语义 activate |
| 事件 | `.on_change(handler)` / `set_on_change` 与 `open_changed()`；只有用户变化发通知 |
| 禁用 | `set_disabled` 只限制 trigger/toggle；不冻结程序 set_open，不禁用展开内容 |
| 布局 | `.gap(float)` / `set_gap`，默认 0；内容不可见或不存在时无 gap；自然高度，默认矩形裁剪 |
| 样式 | 复用 Button ghost/start 对齐；`set_trigger_override(ButtonRecipeRule)`；container/label visual part |
| 生命周期 | 收起隐藏内部宿主，内容仍在树中，保留自身 visible 和绑定；收起前在内容内的焦点恢复 trigger，capture 释放 |
| 语义 | generic 容器 + button trigger（expanded 状态）+ 已展开内容；隐藏内容不参与语义/Tab/命中 |
| 边界 | 无 Accordion 分组、方向键漫游、自定义触发器、过渡动画和销毁型折叠策略；视觉验收尚未完成 |

## 最小结构与内容

<!-- TODO: 维护者补充可编译的 BuildContext / NodeBuilder 用法。 -->

## 事件、绑定与焦点

<!-- TODO: 维护者说明静默 setter、用户事件、scope 清理与内容焦点恢复。 -->

## 主题与组合

<!-- TODO: 维护者补充 typed override、visual part、ScrollView 组合及 Showcase 验收。 -->

契约与回归见 [折叠内容组件](../references/disclosure_components.md)。
