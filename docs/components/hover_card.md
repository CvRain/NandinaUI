# HoverCard

> **本文档的框架、符号表与默认值已按代码核对；带 `<!-- TODO -->` 的叙述性正文待手写。**
> 分工见 [覆盖率看板](coverage.md)。

<!-- TODO: 一句话说明这个组件是什么、解决什么问题。要点：它是"把补充信息藏在悬停后面"
     的浮层，且内容**可以交互** —— 这正是它与 Tooltip 的分界。 -->

## 何时使用

<!-- TODO: 正向场景 2~3 条 + 反向场景。反向场景里最重要的是"该用 Tooltip 的情况"。 -->

| 场景 | 建议 |
| --- | --- |
| <!-- TODO：用户头像 / 作者名 → 悬停显示资料卡与操作 --> | 本组件 |
| <!-- TODO：只需要一行只读提示、指针不需要移进去 --> | 改用 `Tooltip` |
| 点击触发、需要模态确认 | 改用 [`Dialog`](dialog.md) |
| 一组可搜索的命令 | 改用 [`CommandPalette`](command_palette.md) |

## 快速开始

```cpp
#include <nandina/widget/controls.hpp>

auto avatar = ui.make<widget::Avatar>(initials).build();

auto profile_card = ui.make<widget::HoverCard>(
                            avatar,
                            ui.column()
                                .gap(6.0F)
                                .children(
                                    ui.make<widget::Label>(display_name).build(),
                                    ui.make<widget::Label>(bio).font_size(11.0F).build(),
                                    ui.make<widget::Button>("关注").build()
                                )
                                .build()
)
                       .configure([](widget::HoverCard& card) {
                           card.set_placement(widget::internal::OverlayPlacement::bottom);
                           card.set_open_delay(0.3F);
                           card.set_close_delay(0.2F);
                       })
                       .build();
```

<!-- TODO: 说明上面这段的两个细节：拖到触发控件上的停留时长，以及为什么 close_delay
     决定了"能不能移进卡片"。 -->

## 公开 API

| 方法 | 说明 |
| --- | --- |
| `create(trigger, content, theme)` | 构造入口；`ui.make<HoverCard>(trigger, content)` 会注入主题与覆盖层服务 |
| `set_trigger(trigger)` / `trigger()` | 触发控件（单子；空指针拒绝） |
| `set_content(content)` / `content()` | 内容槽位，任意控件；内容随浮层托管，不在本节点之下 |
| `set_open_delay(seconds)` / `open_delay()` | 悬停多久后展开，默认 **0.3** 秒；负值抛 `std::invalid_argument` |
| `set_close_delay(seconds)` / `close_delay()` | 离开后多久收起，默认 **0.2** 秒；这段延迟就是"移进卡片"的窗口 |
| `set_hoverable_content(bool)` / `hoverable_content()` | 指针移入卡片是否保持展开，默认 `true` |
| `set_placement(...)` / `placement()` | 四向停靠 |
| `set_alignment(...)` / `alignment()` | 三种对齐（`start` / `center` / `end`） |
| `set_gap(float)` / `gap()` | 卡片与触发控件的间距；未设置时跟随配方的 `metrics.gap` |
| `set_viewport_padding(float)` | 卡片与窗口边缘至少保留的距离 |
| `open()` / `close()` / `toggle()` / `is_open()` | 程序化开合（自动开合之外的手动入口） |
| `set_on_open(cb)` / `set_on_close(cb)` | 开合回调 |
| `set_override(HoverCardRecipeRule)` / `resolved_style()` | 实例级样式覆盖 |

## 开合时机

<!-- TODO: 结合真实场景讲清"为什么需要两个延迟"，以及跨空隙为什么不会闪。 -->

状态机（时间全部走 `on_process`，与帧率无关）：

| 事件 | 行为 |
| --- | --- |
| 指针进入触发器 | 取消关闭计时；未打开则累计 `open_delay` 后展开 |
| 指针离开触发器 | 若指针不在卡片内，累计 `close_delay` 后收起 |
| 指针进入卡片 | 取消关闭计时 |
| 指针离开卡片 | 累计 `close_delay` 后收起 |

触发器与卡片之间有 `gap`：指针穿过去时两个区域都没命中，这一段由 `close_delay` 兜住，
所以不会闪一下。

## 定位与承载

- 卡片托管到窗口浮层，因此不会被 `ScrollView` / `Card` 之类的裁剪容器截断。
- 卡片不加遮罩、不阻断下层输入、**不抢焦点**、不进入 Tab 序列。
- 位于外层浮层之内时自动登记为 `nested_popup`，随外层一起关闭。
- 树内回退**不存在**：没有窗口浮层服务时不显示卡片（见「已知空白」）。

## 无障碍语义

| 节点 | role | 说明 |
| --- | --- | --- |
| 卡片表面 | `generic` + label「悬停卡片」 | 悬停卡片在 ARIA 里没有专用 role；它承载可交互内容，不是 tooltip |
| 卡片内容 | 由内容自己的控件暴露 | 组件不替内容决定语义 |
| 本节点 | `none` | 它只是触发控件在布局里的占位外壳 |

## 主题

`HoverCardRecipe` 只有两个字段组：

| 分组 | 字段 |
| --- | --- |
| 外壳 | `panel`（`BoxStyle`：填充 / 边框 / 圆角 / 边框宽度） |
| 度量 | `metrics.max_width`（默认 320）、`metrics.min_width`（默认 200）、`metrics.padding_x` / `padding_y`（默认 `spacing.md`）、`metrics.gap`（默认 `spacing.sm`） |

**没有文字样式字段**：卡片内容是任意控件，那些文字由内容自己的控件与配方负责，组件只画外壳、
不替内容决定排版。实例级微调用 `set_override(HoverCardRecipeRule)`。

## 已知空白

<!-- TODO: 确认这些空白是否仍是当前决定；若有取舍理由，补充说明。 -->

- **`Escape` 不关闭卡片**：卡片刻意不抢焦点，因此它收不到按键事件。要支持 Escape 就得让
  卡片可聚焦，那会与"悬停不该打断键盘操作"冲突。需要这个行为时请在触发控件或页面上处理。
- **没有浮层服务时不显示**：树内没有既能锚在触发控件旁、又不会被裁剪的位置，硬放会给出
  错位且被裁一半的结果，因此选择不显示（与 `Tooltip` 同一取舍）。
- **不自动跟随"指针已经离开窗口"**：指针移出窗口边界时按普通 leave 处理，走 `close_delay`。
- **RTL 方向键 / 停靠极性**：四向停靠尚未按书写方向镜像。
