# AlertDialog

> **本文档的框架、符号表与默认值已按代码核对；带 `<!-- TODO -->` 的叙述性正文待手写。**

<!-- TODO: 一句话说明它与 Dialog 的分界：Dialog 是"可以关掉的模态"，AlertDialog 是
     "必须给出一个答案的模态"。 -->

## 何时使用

| 场景 | 建议 |
| --- | --- |
| <!-- TODO：删除 / 覆盖 / 退出登录等不可逆或高风险动作 --> | 本组件 |
| 需要用户填写内容后再继续 | 改用 [`Dialog`](dialog.md) |
| 内联的、不需要确认的状态提示 | 改用 [`Alert`](alert.md) |

## 快速开始

```cpp
#include <nandina/widget/controls.hpp>

auto alert = ui.make<widget::AlertDialog>(
                    "删除这个文件？",
                    "删除后无法恢复，也不会进入回收站。"
)
                  .configure([](widget::AlertDialog& dialog) {
                      dialog.set_confirm_text("删除");
                      // 破坏性动作用 danger 语气，让"确认"在视觉上不等于"默认安全"。
                      dialog.set_confirm_tone(theme::ButtonTone::danger);
                      dialog.set_on_confirm([] { /* 执行删除 */ });
                  })
                  .build();

alert->open();
```

## 与 Dialog 的关系

AlertDialog **没有自己的配方**：它内部组合一个 `Dialog`，把两件事固定下来 ——

| 固定项 | 效果 |
| --- | --- |
| `set_dismissible(false)` | 外部点击与 `Escape` 都**不会**关闭，用户必须按按钮 |
| `set_alert_semantics(true)` | 读屏 role 是 `alertdialog`（断言式），不是 `dialog` |

外观、布局、定位、焦点限制、遮罩、淡入淡出全部来自 `Dialog`，包括 `DialogRecipe`。
实例覆盖因此用 `set_override(DialogRecipeRule)`。需要可关闭的模态请直接用 `Dialog`。

## 两条输入约束

构造期与 setter 都会强制它们，违反时抛 `std::invalid_argument`：

1. **描述不能为空。** alertdialog 的读屏契约要求它有描述；只有标题时用户听到的是
   "某个标题 + 两个按钮"，不知道自己在确认什么。
2. **确认按钮文案不能为空。** 它不可关闭，如果没有确认动作，用户会被困在模态里。

取消按钮**允许**留空，表示只留确认一条出路（按钮会被摘掉，而不是显示一个空按钮）。

## 公开 API

| 方法 | 说明 |
| --- | --- |
| `create(title, description, theme)` | 构造入口；`ui.make<AlertDialog>(title, description)` 注入主题与浮层服务 |
| `set_title` / `title` | 标题（也是读屏 label） |
| `set_description` / `description` | 说明文本，必填 |
| `set_cancel_text` / `cancel_text` | 取消文案，默认「取消」；空 = 隐藏该按钮 |
| `set_confirm_text` / `confirm_text` | 确认文案，默认「确定」；空值被拒绝 |
| `set_confirm_tone` / `confirm_tone` | 确认按钮语气，默认 `ButtonTone::primary` |
| `set_on_confirm` / `set_on_cancel` | 两个动作的回调（组件随后自动关闭） |
| `set_on_close` | 关闭后触发（转发自内部 `Dialog`） |
| `open` / `close` / `is_open` | 开关；动作按钮会自动关闭 |
| `dismissible()` | 静态常量，恒为 `false`：这个约束不可配置 |
| `set_override(DialogRecipeRule)` / `resolved_style()` | 沿用 Dialog 的实例覆盖路径 |

## 无障碍语义

| 节点 | role | 说明 |
| --- | --- | --- |
| 内部面板 | `alert_dialog` | `role_name()` 输出 `"alertdialog"`；label 是标题 |
| 动作按钮 | `button` | 由 `Button` 自己暴露，label 是按钮文案 |

<!-- TODO: 补充读屏实测结果（描述是否被播报、焦点落点是否符合预期）。 -->

## 已知空白

<!-- TODO: 确认这些空白是否仍是当前决定。 -->

- **描述只铺 4 行**（内部常量 `kDescriptionMaxLines`）：超出部分被裁掉。超长说明应改用
  `Dialog` 自己排内容。
- **面板宽度沿用 `DialogRecipe`**：AlertDialog 没有独立的宽度档，需要更宽的确认面板时
  通过 `set_override` 调 `metrics_panel_width`。
