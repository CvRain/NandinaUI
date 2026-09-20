# Slider

`Slider` 在连续或离散的数值区间里选一个值：音量、缩放比例、阈值、透明度。它自带数值语义（`Role::slider`），可用方向键、Home / End 与指针拖动调节。

## 与其它输入的分工

| 场景 | 用哪个 |
| --- | --- |
| 在较宽区间内连续调节、希望"随手拖到大概位置" | `Slider` |
| 在少量互斥选项里选择（低 / 中 / 高） | `RadioGroup` |
| 精确输入一个数字（金额、端口号） | `TextField` |
| 二值开关 | `Switch` / `Checkbox` |

`Slider` 没有上下方向（垂直滑块）与双拇指区间变体，也不显示刻度；需要这些请提需求或自绘。

## 最小示例

```cpp
// 声明式（BuildContext），绑定 float signal
auto& scale = ui.signal<float>(1.0F);
auto slider = ui.make<widget::Slider>(scale, "Scale", 0.5F, 2.0F, 0.1F).build();

// 直接构造
auto slider = widget::Slider::create("Volume", 40.0F, 0.0F, 100.0F, 1.0F);
slider->set_on_change([](float value) { /* ... */ });
```

值在每次变化时被规范到 `[minimum, maximum]` 并对齐到 `step` 的整数倍。`set_range()` 要求 `minimum < maximum`，`set_step()` 要求有限且为正，否则抛 `std::invalid_argument`。

## 公开 API

| 接口 | 说明 |
| --- | --- |
| `Slider(label, value, minimum, maximum, step, theme)` / `create(...)` | 构造；默认 `0.0F / 0.0F / 1.0F / 0.01F` |
| `set_value()` / `value()` | 当前值（规范后） |
| `set_range()` / `minimum()` / `maximum()` | 区间 |
| `set_step()` / `step()` | 步长 |
| `set_label()` / `label()` | 无障碍标签 |
| `set_disabled()` / `disabled()` | 禁用：清除 hover / 拖动 / 焦点并释放指针捕获 |
| `set_on_change(std::function<void(float)>)` | 值变化回调（规范后且真正变化时才触发） |
| `value_changed()` | 值变化的响应式事件 |
| `set_show_value_label()` / `show_value_label()` | 是否在轨道上方显示数值标签（默认关闭） |
| `value_label_text()` | 值标签的当前数字文本（未开启时也随 `value` 更新） |
| `set_text_pipeline()` / `text_pipeline()` / `apply_font_context()` | 值标签文本管线 |
| `set_override(SliderRecipeRule)` | 类型化字段覆盖，系统切换后跟随新快照重解析 |
| `set_theme(NanTheme)` / `theme_ref()` | 整份主题覆盖（不再跟随系统） |
| `visual_state()` / `resolved_style()` | 当前视觉状态与解析后的具体样式 |
| `is_focusable()` / `on_input()` / `on_draw()` / `on_measure()` | 控件协议实现 |

测量：优先宽度来自 `metrics.preferred_width`（默认 240），最小高度来自 `metrics.min_height`（默认 32）；开启值标签后高度额外增加标签高度 + 4。

## 键盘、指针与无障碍

- **键盘**（需先获得焦点）：`←` / `↓` 减一个 `step`，`→` / `↑` 加一个 `step`，`Home` 跳到最小值，`End` 跳到最大值。滚轮不处理。
- **指针**：在轨道上按下即把值设为指针位置对应的值并开始拖动；拖动期间通过指针捕获持续更新，即使指针移出控件边界仍跟随；松开释放捕获。
- **焦点**：`focus_enter` 置为可聚焦，`focus_leave` 取消焦点并结束拖动。
- **semantics**：`Role::slider`，`value` 为数字文本，`hint` 为 `"min to max"`，动作集为 `focus | set_value | increment | decrement`（禁用时为空）。

## 主题与配方字段

配方遵循组件四步同步：`SliderVisualState` → `SliderRecipe` / `SliderRecipeRule` / `ResolvedSliderStyle` / `SliderRecipes` → `resolve_slider()` → `default_slider_recipe()`。`SliderVisualState` 为 `normal` / `hovered` / `dragging` / `focused` / `disabled`（优先级：disabled > dragging > hovered > focused）。

| 配方字段 | 默认值 | 语义 |
| --- | --- | --- |
| `inactive_track.box.fill` | `token(muted)` | 未填充轨道底色 |
| `active_track.box.fill` | `token(primary)` | 已填充轨道底色 |
| `track_thickness` | `literal(4.0F)` | 轨道厚度 |
| `thumb_fill` | `token(background)` | 拇指填充色 |
| `thumb_radius` | `literal(9.0F)`（dragging 11 / hovered 10） | **拇指圆的像素半径** |
| `thumb_border` | `token(primary)` | 拇指描边环颜色 |
| `thumb_border_width` | `token(border_thin)` | 拇指环线宽；`0` 或全透明时跳过环 |
| `focus_ring_color` | `token(ring)` | 焦点环颜色 |
| `focus_ring_width` | `literal(0.0F)`（focused 规则用 `token(border_focus_ring)` 开启） | 焦点环线宽 |
| `metrics.min_height` / `metrics.preferred_width` | `literal(32.0F)` / `literal(240.0F)` | 最小高度 / 首选宽度 |

> **`thumb_radius` 是像素长度，不是圆角半径。** 它与其他 `BoxStyle.radius` 字段的"圆角"语义不同，直接作为 `draw_circle()` 的半径使用。不要填 `radius_full`（"胶囊"圆角档，解析为 9999）——虽然绘制与命中都会把半径夹紧到 `min(宽, 高) / 2` 兜底，不会越出控件，但会退化成"拇指直径等于控件高度"的错误造型。拇指的 `box.radius` 现在不再承载像素半径。

拇指按 shadcn 风格绘制为**填充圆 + 描边环**：环用「半边长等于半径」的正方形圆角描边表达（该正方形在圆角等于半边长时即一个圆），描边落在半径内侧，因此不会越出已夹紧的拇指范围。

实例级微调：

```cpp
slider->set_override(theme::SliderRecipeRule {
    .thumb_fill = theme::ThemeColor::token(theme::ColorToken::background),
    .thumb_radius = theme::ThemeScalar::literal(12.0F),
    .thumb_border = theme::ThemeColor::token(theme::ColorToken::primary),
    .thumb_border_width = theme::ThemeScalar::literal(2.0F),
});
```

## 已知空白

- `thumb_radius` 没有对应的尺度 token（语义上就是组件几何），只能用 `literal`；这是刻意的。
- 聚焦且同时 hover 时解析到 `hovered` 状态，而 hovered 规则不设置焦点环宽度，因此**鼠标停在已聚焦的滑块上时焦点环不可见**。
- 无垂直方向、无双拇指区间、无刻度/吸附点、无 `prefers-reduced-motion` 相关行为（拖动本身无动画）。
- 值标签只能显示原始数字，不支持单位、小数位或自定义格式化；`semantics` 的 `value` 同样是无格式数字。
- 滚轮不调节数值。
