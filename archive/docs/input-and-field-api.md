# Input / Field API 设计

> 状态校正（2026-05）：本文档用于收口 Issue 090，固定 TextField / Field 在 widgets 与 app authoring 层的职责边界、状态模型与推荐绑定方式。本文档是后续 Issue 091 / 092 / 095 的直接依据，而不是实现完成后的补票说明。

## 目标

在进入 TextField / Field 真实实现前，先固定以下问题：

- 输入控件和表单语义容器分别负责什么
- widgets 层应暴露哪些公开状态与事件
- authoring DSL 应采用什么命名和链式风格
- 输入值与外部状态之间应如何绑定，避免后续实现反向塑形 API

本文档的目标不是一次性定义完整 forms 系统，而是为第一条表单垂直切片建立稳定契约。

## 设计原则

### 1. TextField 是 control，Field 是语义容器

- `TextField` 负责单行文本编辑、焦点、占位符、caret、键盘输入和只读/禁用语义。
- `Field` 负责 label、helper、error、required、invalid 等更高层的表单语义组织。
- `Field` 不是 primitive，也不负责文本编辑本身。

这与当前 primitive 文档中的约束保持一致：

- `Surface` 提供底面、边框、padding
- `FocusRing` 提供统一焦点可视化
- 文本能力仍主要由 `Label` / `NanFont` 吸收
- `Input` 自身只承载编辑语义，不吞掉整套表单结构语义

### 2. authoring 风格与现有 `label()` / `button()` 保持一致

新的输入控件不应退回到“只有 widgets 低层 API 可用”的双轨状态。

因此 authoring 层应继续沿用当前模式：

- 工厂函数返回 typed node
- 通过链式 setter 声明外观与行为
- 支持 `bind(ref)`
- 支持 `on_change(...)` / `on_submit(...)` 这类事件回调

### 3. 输入绑定坚持单向数据流，不引入隐式双向魔法

输入控件与 `label()` / `button()` 最大的区别在于：文本正在被用户编辑，不能简单照搬 `text(fn)` 这种 Effect 推值模式。

原因：

- 直接用 effect 把外部值每帧推回控件，容易打断 caret 和 selection
- 输入控件需要区分“程序性写入”和“用户编辑导致的变更”
- 后续如果支持 IME / preedit / selection，隐式双向绑定更容易失控

因此第一版契约应明确：

- `TextField` 对外暴露 `value(...)` 快照输入与 `on_change(...)` 事件输出
- 程序性 `set_value(...)` 默认不回发 `on_change(...)`
- 真正的双向 sugar 可以后续补充，但底层语义仍应等价于“外部状态 -> value snapshot，用户编辑 -> on_change callback”

## 组件契约

下面把 `TextField` 与 `Field` 分开定义，避免后续实现再次把输入编辑语义和表单结构语义混写到同一个控件里。

## TextField

### 职责

`TextField` 是第一版单行输入控件。它负责：

- 当前文本值
- placeholder 文本
- focus / blur
- caret 可视化
- 键盘输入、backspace、delete、submit
- `disabled` / `read_only`
- standalone `invalid` 视觉状态

`TextField` 不负责：

- label 文案
- helper / error 文案
- required indicator 的展示
- 表单级验证策略

### 视觉组合

推荐心智：

- `Surface`：输入框底面、边框、padding
- 文本能力：当前由 `Label` / `NanFont` 边界吸收
- `FocusRing`：焦点态 outline
- `TextField`：编辑状态机与事件桥接

第一版应优先复用 `NanInputStyle` 与 `NanFocusRingStyle`，而不是在控件内部重新硬编码颜色和焦点描边。

### widgets 层公开契约

建议第一版 `TextField` 至少暴露以下公开 API：

```cpp
class TextField : public Surface {
public:
    static auto create() -> Ptr;

    auto set_value(std::string value) -> TextField&;
    [[nodiscard]] auto value() const noexcept -> const std::string&;

    auto set_placeholder(std::string value) -> TextField&;
    [[nodiscard]] auto placeholder() const noexcept -> const std::string&;

    auto set_disabled(bool value) -> TextField&;
    [[nodiscard]] auto disabled() const noexcept -> bool;

    auto set_read_only(bool value) -> TextField&;
    [[nodiscard]] auto read_only() const noexcept -> bool;

    auto set_invalid(bool value) -> TextField&;
    [[nodiscard]] auto invalid() const noexcept -> bool;

    [[nodiscard]] auto focused() const noexcept -> bool;

    auto on_change(std::function<void(std::string_view)> cb) -> TextField&;
    auto on_submit(std::function<void(std::string_view)> cb) -> TextField&;

    auto focus() -> void;
    auto blur() -> void;
};
```

### 事件语义

- `on_change(...)`
  - 仅在用户编辑导致文本实际变化时触发
  - `set_value(...)` 不应默认再次触发 `on_change(...)`
  - 回调参数为最新文本快照

- `on_submit(...)`
  - 第一版只对应单行输入的 Enter 行为
  - 若 `disabled == true` 或 `read_only == true`，不触发

- `focused()`
  - 是运行时状态，不建议作为页面 authoring 的主要输入属性
  - 外部若需控制焦点，优先通过 `Ref<TextField>` + `focus()` / `blur()`

### 状态边界

- `disabled`
  - 阻断 pointer / key / text input
  - 不允许 focus ring 保持激活

- `read_only`
  - 允许 focus
  - 不接受用户编辑写入
  - 允许选择、复制与 submit 语义后续扩展

- `invalid`
  - 是视觉与语义状态输入
  - standalone `TextField` 可直接消费
  - 被 `Field` 包裹时，优先由 `Field` 统一驱动

## Field

### 职责

`Field` 是围绕输入控件的语义容器。它负责：

- label
- control slot
- helper text
- error text
- `required` 语义展示
- `invalid` / `disabled` 的上层表单语义组织

`Field` 不负责：

- caret
- 键盘输入
- 文本编辑逻辑
- 具体输入值的存储

### 第一版结构约束

第一版 `Field` 应按“单 control slot”的 wrapper 心智设计，而不是一开始就做成任意 children 的泛容器。

推荐结构：

```text
Field
├── label slot
├── control slot   <- TextField / 后续其他输入控件
├── helper slot
└── error slot
```

### widgets 层公开契约

建议第一版 `Field` 至少暴露以下能力：

```cpp
class Field : public NanWidget {
public:
    static auto create() -> Ptr;

    auto set_label(std::string value) -> Field&;
    auto set_helper_text(std::string value) -> Field&;
    auto set_error_text(std::string value) -> Field&;

    auto set_required(bool value) -> Field&;
    auto set_invalid(bool value) -> Field&;
    auto set_disabled(bool value) -> Field&;

    auto set_control(runtime::NanWidget::Ptr control) -> Field&;
};
```

### Field 与 TextField 的关系

- `Field.required(true)` 负责 required indicator 与 label/helper/error 的语义组织
- `Field.invalid(true)` 负责错误文案显隐，并将 invalid 视觉状态向内部 control 传播
- `Field.disabled(true)` 负责统一传播 disabled 语义到内部 control 和相关文案样式

也就是说：

- `required` 是表单语义，优先属于 `Field`
- `invalid` 可以由 `TextField` standalone 消费，但在 form 组合下由 `Field` 统一驱动更合理

## authoring DSL 契约

### 命名

第一版 canonical 命名使用：

- `text_field()`：对应 widgets 层 `TextField`
- `field(...)`：对应 widgets 层 `Field`

不把 `input()` 作为第一版 canonical 名称，原因是：

- 仓库后续还会出现 checkbox / radio / select 等 input family
- `TextField` 是更准确的控件语义
- 需要避免 API 过早把所有输入控件都折叠为一个模糊工厂

若后续确实需要 shadcn 风格的更短命名，可在 `text_field()` 稳定后再考虑增加 `input()` 薄别名。

### 推荐 authoring API

```cpp
auto email = text_field()
    .value("alice@example.com")
    .placeholder("Email")
    .read_only(false)
    .disabled(false)
    .invalid(false)
    .on_change([this](std::string_view value) {
        m_email = std::string{value};
    })
    .on_submit([this](std::string_view value) {
        submit_email(value);
    })
    .bind(m_email_ref);

auto email_field = field(std::move(email))
    .label("Email")
    .helper("Used for login and notifications")
    .error("Email is required")
    .required(true)
    .invalid(m_show_error)
    .bind(m_email_field_ref);
```

### TextFieldNode 建议链式接口

```cpp
text_field()
    .value("...")
    .placeholder("...")
    .disabled(false)
    .read_only(false)
    .invalid(false)
    .on_change(...)
    .on_submit(...)
    .bind(ref)
```

### FieldNode 建议链式接口

```cpp
field(text_field())
    .label("...")
    .helper("...")
    .error("...")
    .required(true)
    .invalid(false)
    .disabled(false)
    .bind(ref)
```

## 绑定模型

### 推荐原则

输入绑定的推荐原则是：

- 外部状态拥有业务值
- `TextField` 接收一个当前值快照
- 用户编辑通过 `on_change(...)` 向外发出新值
- 需要程序性控制时，通过 `Ref<TextField>` 进行 focus / blur / set_value 等命令式操作

### `Ref<TextField>` 的角色

`Ref<TextField>` 用于：

- 手动 focus / blur
- 提交成功后清空输入
- 表单重置时主动推入新值
- 调试或测试中读取当前值

`Ref<TextField>` 不应替代 `on_change(...)` 成为主要数据流。

### `signals` 的角色

第一版推荐只暴露两条外发信号：

- `on_change(std::string_view)`
- `on_submit(std::string_view)`

后续若确实出现需求，再考虑补 `on_focus` / `on_blur` / `on_escape` 等更细分事件。

### `Var` 的推荐绑定方式

当前仓库的 `Var<T>` 已用于页面级响应式 authoring，但当前公开接口里尚未形成稳定的 `Var<std::string>` 使用面，因此 090 不把“字符串双向绑定已稳定”写成既成事实。

因此分两层给出建议：

#### 当前兼容建议

在 `Var<std::string>` 尚未成为已验证主路径之前，推荐：

- 页面层用普通 `std::string` 或等价模型保存表单值
- `TextField` 通过 `.value(...)` 接收初始快照
- 通过 `.on_change(...)` 把编辑结果写回页面状态
- 需要外部主动同步控件时，通过 `Ref<TextField>` 调 `set_value(...)`

#### 目标推荐语义

当字符串状态模型进入主线后，推荐的高层 sugar 应保持与上面相同的单向语义，本质上等价于：

```cpp
text_field()
    .value(m_name())
    .on_change([&](std::string_view value) {
        m_name = std::string{value};
    });
```

如果后续引入 `.bind_value(...)` / `.model(...)` 之类的糖，它也应只是上述模式的封装，而不是新的隐藏状态机。

### 明确不推荐的方式

第一版不推荐把输入值设计为通用的 `value(fn)` Effect 推送接口，原因是这会：

- 让外部 effect 与用户输入互相争抢 source of truth
- 更容易引入 caret 跳动和 selection 丢失
- 在后续 IME / preedit 支持时扩大实现复杂度

## 与 theme / style layer 的关系

### TextField

第一版应直接消费：

- `NanInputStyle`：背景、边框、padding、placeholder 文本样式
- `NanFocusRingStyle`：焦点 outline

其中：

- `invalid` 应优先通过 input style 的语义扩展或 resolver 进入视觉层
- 不建议在 `TextField` 内直接散落多组 RGB 常量

### Field

第一版可复用：

- `NanLabelStyle`：label / helper / error 文案
- 未来独立 `FieldStyle`：若 label-helper-error 的 spacing / gap / error slot 表达开始稳定，再从 `Field` 中提取出来

## 非目标

Issue 090 只固定 API 契约，不要求在本阶段直接完成以下能力：

- 多行 `TextArea`
- selection range 公开 API
- IME preedit 细节
- password reveal / masking
- 表单验证框架
- 泛化的 form store / schema system

这些能力应在 `TextField` / `Field` 第一版走通后，再按真实需求继续扩展。

## 对后续 Issue 的约束

### Issue 091

- 必须实现 `TextField` 的单行编辑链路
- 不应把 label / helper / error 直接塞进 `TextField`

### Issue 092

- `Field` 必须以 control slot 为中心，而不是重新复制输入逻辑
- `required` / `invalid` 的表单展示逻辑应优先落在 `Field`

### Issue 095

- authoring 工厂应使用 `text_field()` 作为 canonical API
- 不应直接把输入控件降级成“只有 widgets 层命令式接口可用”的特殊例外
- 若引入 value binding sugar，必须保持本文档中的单向数据流语义

## 当前结论

Issue 090 的最终收口口径如下：

- `TextField` 是单行可编辑 control，负责编辑状态机与输入事件
- `Field` 是围绕 control slot 的语义容器，负责 label/helper/error/required/invalid 的组织
- authoring 层 canonical API 使用 `text_field()` 与 `field(...)`
- 输入绑定坚持“value snapshot + on_change output + Ref 命令式辅助”的单向模型
- 后续即使补 `Var<std::string>` 或更高层 `model(...)` sugar，也不得破坏上述基础契约