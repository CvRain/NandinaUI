# Skeleton

Skeleton 在内容就绪前显示占位块：加载列表、卡片、详情时先把最终版式占住，数据到达后原地替换，避免页面跳动。

它有两种形状：`text`（文本行占位，支持多行，末行按配方比例收窄）与 `rectangle`（整块矩形占位，铺满请求到的尺寸）。组件是**纯展示**的：不可聚焦、不接收键盘或指针输入，也没有动画。

> 组件属于 recommended；动画（shimmer / 呼吸）是已知空白，当前实现为静态弱化块。

## 不适合使用它的场景

- 操作需要即时反馈、且没有可预知的最终版式：用 `Spinner`；
- 内容已经确定为空（不是加载中）：用 `EmptyState`；
- 需要显示确定进度：用 `ProgressBar`。

## 最小示例

```cpp
auto skeleton = ui.make<widget::Skeleton>().build();

// 三行文本占位
ui.make<widget::Skeleton>(widget::SkeletonVariant::text, 3);
```

`Skeleton` 无界测量时宽度取配方首选宽度（默认 240），有界时铺满约束上限；文本高度为 `行数 × 高度 + (行数 − 1) × 行距`。

## 常用构建器写法

```cpp
// 文本占位：1 行 / 3 行
ui.make<widget::Skeleton>();
ui.make<widget::Skeleton>(widget::SkeletonVariant::text, 3);

// 整块占位
ui.make<widget::Skeleton>(widget::SkeletonVariant::rectangle)
    .configure([](widget::Skeleton& skeleton) {
        skeleton.set_width(160.0F);
        skeleton.set_height(96.0F);
    });
```

| 接口 | 说明 |
| --- | --- |
| `set_variant()` / `variant()` | `text` 或 `rectangle` |
| `set_lines()` / `lines()` | 文本行数；越界值钳制到 `>= 1`，`rectangle` 忽略 |
| `set_label()` / `label()` | 无障碍标签（仅进 semantics） |

显式尺寸用基类的 `set_width()` / `set_height()`；`rectangle` 会优先尊重它，否则铺满约束。

## 状态与响应式绑定

Skeleton 只有 `normal` 一个视觉状态，没有事件与回调，也不参与响应式绑定。数据到达后由页面把 Skeleton 从树上移除、换成真实内容即可。

## 键盘、焦点与无障碍

- 不可聚焦，Tab 不会停留；没有键盘与指针行为；
- semantics 复用 `progress_bar` 角色并报告 `set_label()` 的文本，`value` 为空表示不确定进度；没有合适的新角色可以表达"骨架屏"，因此不新增角色；
- 未设置标签时辅助技术只会读到"忙碌"，不会读到百分比。

## 主题与实例覆盖

```cpp
auto skeleton = widget::Skeleton::create();
skeleton->set_override(theme::SkeletonRecipeRule {
    .surface_fill = theme::ThemeColor::token(theme::ColorToken::muted),
    .metrics_height = theme::ThemeScalar::literal(16.0F),
    .metrics_last_line_ratio = theme::ThemeScalar::literal(0.5F),
});
const auto style = skeleton->resolved_style();
```

解析顺序遵循[组件公共契约](../references/component_contract.md)：DesignSystem 默认值 → recipe → 实例覆盖。默认值只用语义角色：`surface` 取 `muted` 底色、`radius_md` 圆角；高度 / 行距 / 末行比例是字面量（没有对应尺度 token，末行比例是比例值只能字面表达）。

## 完整公开 API

| 接口 | 说明 |
| --- | --- |
| `Skeleton(theme)` / `create(theme)` | 构造；默认 `text`、1 行 |
| `set_theme()` / `theme_ref()` | 整份主题覆盖 / 当前主题视图 |
| `set_override(theme::SkeletonRecipeRule)` | 只覆盖明确指定的配方字段，系统切换后跟随新快照 |
| `visual_state()` / `resolved_style()` | 当前状态与解析后的具体样式 |
| `on_theme_changed()` / `on_draw()` / `on_measure()` | 控件协议实现 |

`rectangle` 变体下的显式尺寸、`text` 变体下的超长文本截断属于布局系统行为，见 `scene::LayoutConstraints`。
