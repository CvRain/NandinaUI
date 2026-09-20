# Button

<!-- TODO: 一句话说明这个组件是什么、解决什么问题。不要复述 API，说"人话"。 -->
<!-- 示范（可删）：触发一个操作。 -->

## 何时使用

<!-- TODO: 正向场景 2~3 条 + 反向场景。反向场景最有价值，它能防止误用。 -->

| 场景 | 建议 |
| --- | --- |
| 提交表单、确认操作、打开对话框 | 本组件 |
| <!-- TODO: 页面间跳转 --> | <!-- TODO: 用本组件的 `link` treatment，还是独立导航控件？ --> |
| 在若干互斥选项中切换 | 改用 `Tabs` / `RadioGroup` |
| 布尔开关（立即生效） | 改用 `Switch` / `Checkbox` |

## 快速开始

<!-- TODO: 最小可运行示例。优先用 builder。 -->

```cpp
auto save = ui.make<widget::Button>("保存")
                .treatment(theme::ButtonTreatment::filled)
                .on_click([] { /* TODO */ })
                .build();
```

<!-- TODO: 补一个"带 disabled 状态"或"与主题 tone 组合"的短例。 -->

## 外观与变体

两个正交维度：`ButtonTone`（语义）与 `ButtonTreatment`（视觉权重）。

**Tone**（`theme::ButtonTone`）：`primary` / `secondary` / `neutral` / `danger`

| Tone | 用途 |
| --- | --- |
| `primary` | <!-- TODO --> |
| `secondary` | <!-- TODO: 注意 secondary 是**中性次操作**色，不是品牌辅色 --> |
| `neutral` | <!-- TODO --> |
| `danger` | <!-- TODO: 危险操作（不可逆、破坏性） --> |

**Treatment**（`theme::ButtonTreatment`）：`filled` / `tonal` / `outlined` / `ghost` / `link`

| Treatment | 用途 |
| --- | --- |
| `filled` | <!-- TODO --> |
| `tonal` | <!-- TODO --> |
| `outlined` | <!-- TODO --> |
| `ghost` | <!-- TODO --> |
| `link` | <!-- TODO --> |

<!-- TODO: 给出"一屏里怎么搭配 tone × treatment"的建议（例如一屏最多一个 filled primary）。 -->

## 公开 API

| 成员 | 说明 |
| --- | --- |
| `Button(std::string text, theme::NanTheme)` | <!-- TODO --> |
| `create(...)` | <!-- TODO --> |
| `set_text(std::string)` / `text()` | <!-- TODO --> |
| `set_tone(theme::ButtonTone)` | <!-- TODO --> |
| `set_treatment(theme::ButtonTreatment)` | <!-- TODO --> |
| `set_disabled(bool)` / `disabled()` | <!-- TODO --> |
| `set_font_size(float)` / `set_font_family(...)` / `set_font_weight(int)` | <!-- TODO --> |
| `set_text_overflow(primitives::TextOverflow)` | <!-- TODO: 文本超长时的行为 --> |
| `text_node()` / `text_pipeline()` | <!-- TODO: 标注是否 recommended --> |
| `set_override(theme::ButtonRecipeRule)` | 见「主题与覆盖」 |

## 槽位与组合

<!-- TODO: Button 目前是否有槽位？若无，写"无槽位：文本内容通过 set_text()，
     复杂内容应改用其他容器组合"。 -->

## 事件与绑定

<!-- TODO: 覆盖三种路径 —— 直接 callback、reactive Event、Signal 绑定。
     注意 callback 的生命周期约定（本仓库曾因按值捕获 BuildContext 出过悬垂事故）。 -->

```cpp
// callback
// TODO
```

<!-- TODO: 若 `ui.bind(...)` 有 Button 路径，补示例；并说明在回调里应使用 request_* 延迟导航。 -->

## 键盘与指针行为

| 输入 | 行为 |
| --- | --- |
| Tab | 可获得焦点（`disabled` 时不可） |
| Enter / Space | <!-- TODO: 是否都触发 activate？以 on_input() 为准 --> |
| Escape | <!-- TODO --> |
| 方向键 | <!-- TODO --> |
| 指针按下 / 释放 | <!-- TODO: 按下视觉反馈与"在按钮外释放"的处理 --> |
| disabled 时 | <!-- TODO: 是否仍可聚焦、是否接收事件 --> |

## 无障碍语义

以 `semantics_properties()` 为准：

| 项 | 值 |
| --- | --- |
| role | `button` |
| label | 按钮文本 |
| state | `focusable`（`!disabled`）、`focused`、`disabled` |
| actions | `disabled` 时为 `none`，否则 `activate \| focus` |

<!-- TODO: 说明调用方需要做什么（例如仅有图标的按钮如何提供 label）。 -->

## 主题与覆盖

Button 消费的语义角色：<!-- TODO: 按 tone/treatment 分别列出实际引用的 ColorToken。 -->
配方圆角默认取 `ScalarToken::radius_md`（默认主题下 8px）。

| 配方字段 | 默认 | 说明 |
| --- | --- | --- |
| `container.fill` | 随 tone / treatment 变化 | <!-- TODO --> |
| `container.radius` | `radius_md` | <!-- TODO --> |
| <!-- TODO: 其余字段 --> | | |

```cpp
// TODO: set_override 示例，只覆盖关心的字段（其余跟随主题）
```

## 常见坑

- <!-- TODO: 至少 2 条真实踩过的坑。 -->
- <!-- TODO -->

## 相关组件

<!-- TODO: 只链接**已经存在**的文档页，避免死链。尚未编写页面的组件改用行内代码引用。 -->

- [`Spinner`](spinner.md) —— 异步操作进行中的反馈，常与按钮的 loading 态配合
- `Toggle` —— <!-- TODO: 页面待写 -->
- `Chip` —— <!-- TODO: 页面待写 -->

## 已知空白

- <!-- TODO: 如实列出（未实现的状态、未覆盖的能力）。没有就写"暂无"。 -->
