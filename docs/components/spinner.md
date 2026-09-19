# Spinner

Spinner 展示**不定量**的进行中状态：内容正在加载、请求正在处理、页面正在初始化。它是一个持续旋转的圆环弧线，无法表达具体完成比例。

## 与 ProgressBar 的分工

| 场景 | 用哪个 |
| --- | --- |
| 已知完成比例（上传 42%、构建 8/20） | `ProgressBar` |
| 只有"进行中"，无法给出比例 | `Spinner` |
| 等待时间很长、进度可估算 | `ProgressBar`（确定进度更能降低焦虑） |
| 首次进入页面的短等待、按钮内联忙碌态 | `Spinner` |

两者都不是交互控件：不接受点击，也没有键盘行为；`Spinner` 的弧线按 `rotation_speed` 自行转动。

## 最小示例

```cpp
// 声明式（BuildContext）
auto spinner = ui.make<widget::Spinner>("正在加载").build();

// 直接构造
auto spinner = widget::Spinner::create();
spinner->set_label("正在加载");
```

`Spinner` 是非交互展示组件，可以作为任意布局容器的子节点；测量尺寸是配方 `metrics.diameter` 的正方形。

## 公开 API

| 接口 | 说明 |
| --- | --- |
| `create(theme)` / `Spinner(theme)` | 构造；缺省使用 `theme::default_theme()` |
| `set_label(std::string)` / `label()` | 无障碍标签（可选，仅进 semantics） |
| `set_disabled(bool)` / `disabled()` | 禁用；禁用时指示环按 `opacity.disabled` 淡化且**停止转动** |
| `set_override(SpinnerRecipeRule)` | 只覆盖明确指定的配方字段，主题切换后仍跟随新快照重解析 |
| `set_theme(NanTheme)` / `theme_ref()` | 以完整主题覆盖，此后不再跟随系统切换 |
| `visual_state()` / `resolved_style()` | 当前状态与解析后的具体样式（供测试 / 调试） |
| `rotation()` | 当前弧线起始角（弧度），测试用 |

`on_process(dt)` 每帧把 `dt * metrics.rotation_speed` 累加到起始角并请求重绘；`set_disabled(true)` 后不再推进。

## 主题与配方字段

配方遵循组件四步同步：`SpinnerVisualState` → `SpinnerRecipe` / `SpinnerRecipeRule` / `ResolvedSpinnerStyle` / `SpinnerRecipes` → `resolve_spinner()` → `default_spinner_recipe()`。

| 配方字段 | 默认值 | 语义 |
| --- | --- | --- |
| `indicator` | `ThemeColor::token(ColorToken::primary)` | 指示环颜色 |
| `metrics.diameter` | `literal(16.0F)`（字面量：组件几何，无语义 token） | 指示环外径 |
| `metrics.thickness` | `literal(2.0F)` | 环厚 |
| `metrics.arc_radians` | `literal(4.712389F)`（3π/2） | 弧长，`2π` 为整环 |
| `metrics.rotation_speed` | `literal(4.0F)` rad/s | 角速度 |

实例级微调：

```cpp
spinner->set_override(theme::SpinnerRecipeRule {
    .indicator = theme::ThemeColor::token(theme::ColorToken::error),
    .metrics_diameter = theme::ThemeScalar::literal(24.0F),
    .metrics_rotation_speed = theme::ThemeScalar::literal(2.0F),
});
```

`SpinnerVisualState` 目前只有 `normal` / `disabled`。禁用变换把 `indicator` 的 alpha 乘以 `opacity.disabled`，与 `ProgressBar` 一致。

**新增状态时的已知空白**：`indicator` 与四个度量字段都已在四处同步，但 `SpinnerRecipeRule.state` 尚无语义匹配的默认规则（`default_spinner_recipe()` 的 `rules` 为空）；主题作者可自行添加规则。

## 无障碍

`semantics_properties()` 返回 `Role::progress_bar`（与 `ProgressBar` 同一角色，代表"进度指示"），并在 `label` 非空时播报标签；因为是不定量，不给 `value`（没有百分比可播报）。`disabled` 状态映射到 `state.disabled`。

由于旋转是纯视觉，读屏用户只会在状态变化时收到更新，持续旋转不会产生语义噪音。

## 已知空白

- 没有 `prefers-reduced-motion` 分支：`Button` 的 ripple 会在 reduced motion 下跳到终态，`Spinner` 尚未接入，持续旋转对动效敏感用户不友好。
- 弧线用设备的分段描边绘制；半透明（如 disabled 的 `opacity.disabled`）时，相邻线段的圆头会有轻微叠加，边缘比理论值略实。设备层缺少单次调用的弧线图元，暂以分段近似。
- 不提供 `progress` 变体；需要确定进度请用 `ProgressBar`。
