# 配置与开发体验

> 本文规定应用/组件作者**怎么配置控件**、**回调写成什么形状**、以及**冲突时谁赢**。
> 与 [组件公共契约](component_contract.md) 的分工：契约规定组件"必须满足什么"，
> 本文规定"作者侧"的 API 形状与优先级规则。
>
> 起因是一轮使用反馈里的四条抱怨：`&` 捕获会崩、组件缺基础配置函数、字号到底谁说了算、
> 写完回调不知道参数该塞什么。四条的共同点是**作者天天要碰但规则没写下来**。

## 1. 回调分两类，规则完全不同

这是本文最重要的一条。API 里有两类 lambda，**语法完全一样**，但生命周期规则相反：

| 类别 | 例子 | 何时调用 | `&` 捕获局部量 |
| --- | --- | --- | --- |
| **立刻调用** | `.configure(...)`、`materialize` 的转换器 | 构建语句当场 | ✅ 安全 |
| **存起来以后调用** | `on_*`、`ui.bind` 的 setter、`ui.computed` | 之后（事件 / 变更 / 每帧） | ❌ **悬垂** |

用户无法从签名分辨这两类，这是当前最贵的一处 DX 缺口。

### 1.1 `guarded()` 的能力边界（必须写进头文件）

`NodeBuilder::on_*` 会用 `guarded()` 包一层，但**它保护的只是宿主作用域**：

```cpp
return [lifetime = callback_lifetime_, handler = ...](auto&&... args) mutable {
    if (lifetime.has_value() && lifetime->expired()) { return; }  // ← 只查作用域令牌
    std::invoke(handler, ...);                                    // ← 然后照常解引用捕获
};
```

所以它**不检查 handler 自己捕获了什么**。`[&local]` 仍然悬垂，而且崩溃发生在
`std::invoke` 内部 —— 令牌检查救不了。

> **规则**：`on_*` / `bind` / `computed` 里**不要用 `[&]` 抓局部量或 `this`**。
> 需要按值就写出来。注册期比创建它的作用域活得久，`[&]` 让"抓了一个活不了那么久的
> 东西"和"抓了一个作用域持有的信号"看起来一模一样。

## 2. 回调类型：concept 判断事件，模板保住零开销

**不用 `std::function`。** 它是类型擦除：丢掉内联、多一次间接调用、还可能分配。
`Handler` 模板是资产，不是负担 —— 问题只在**错误信息里出现的是 `Handler` 这个占位符**。

### 2.1 两类回调用两种手段

| 类别 | 手段 | 为什么 |
| --- | --- | --- |
| 立刻调用 | **`std::function_ref`（C++26）** | 具名签名出现在错误里，**零分配、间接最小**。是"可发现性"与"效率"之间的那个中间点 |
| 存起来以后调用 | **具名 concept + 模板** | 需要所有权 → 不能是引用语义；模板保住零开销 |

项目已经在用 `std::move_only_function`（`set_shell`），所以语言版本与习惯都到位。

### 2.2 concept 判断的是**接受哪个事件**，不是"是不是 invocable"

```cpp
template<typename F>
concept PointerDownHandler =
    std::invocable<F, const scene::MouseButtonEvent&> || std::invocable<F>;
```

入口用**一次 `if constexpr` 派发**（等价于为每个事件类型写特化，但只有一处）：

```cpp
if constexpr (std::invocable<F, const MouseButtonEvent&>) { handler(event); }
else { handler(); }
```

收益：**"接受哪些签名"只有一处声明，而那一处就是文档** —— IDE 悬停到 concept 上
就能看到全部合法形态。用户于是可以靠三样东西学会：固定文档 + 写完函数的原型提示 +
concept 注释。

### 2.3 扩展点：兜底的是**派发点**，不是 concept

```cpp
template<typename F, typename Event>
struct HandlerDispatch {
    static void invoke(F& handler, const Event& event) { handler(event); }
};
```

用户为自己的事件类型特化它即可。这是"想要别的形状就自己扩展"的落点。

### 2.4 诊断信息分层，不要指望 `static_assert` 替 concept 干活

`requires` 只能证明"这个参数不合法"，**说不出合法的是什么**。所以：

| 手段 | 作用 | 时机 |
| --- | --- | --- |
| **具名 concept + 注释** | 说明接受哪些签名 | 写的时候（IDE 悬停）—— 最早 |
| **`= delete("理由")`（C++26）** | 对**明确拒绝**的形态给一句人话，而不是掉进 concept 失败 | 编译期 |
| **contracts（C++26）** | 表达前置条件（如 handler 非空） | 编译期/运行前 |
| `static_assert` | 只在**兜底失败**那一层加"接受 X / Y / ()，或为你的类型特化 `HandlerDispatch`" | 编译期（最后一道） |

## 3. 组件的公开 API 表面

**问题**：`Label` 自己只声明了 `set_color_token`，其余 `set_font_size` / `set_font` /
`set_style` / `set_overflow` / `set_max_lines` / `set_align` 全部来自**公有继承
`primitives::Text`**。所以不是"缺函数"，而是**查不到** —— 用户被迫去读 primitive 才知道能调什么。

**这与契约第 9 节直接冲突**："primitive 和 internal 类型不应无意间成为入门示例的依赖"。

> **规则**：组件必须**显式声明自己承诺的那部分配置入口**（哪怕只是 `using` + 注释），
> 把继承来的其余部分挡在文档与入门示例之外。DSL（`.font_size(...)`）能表达的，
> 组件上必须有等价且可查的入口 —— **"基础功能等价"是要求，不是巧合**。

## 4. 样式优先级：批量派生 vs 实例显式

**问题**：既在 `Label` 上 `.font_size()`、又在 `.configure()` 里 `set_font_size()`、
还在 `set_style(TextStyle{...})` 里给了字号 —— 到底谁赢？

现状机制是 **`font_size_explicit_` 粘性标志 + 值上的"最后写赢"**：

| 写入点 | 是否置标记 | 效果 |
| --- | --- | --- |
| `Text::set_font_size` | ✅ 置位 | 从此**赢过**样式上下文与组件的配方应用 |
| 样式上下文应用 | — | **仅当未置位时**才生效 |
| 组件的配方应用 | — | 已置位则**直接返回** |
| `Text::set_style(TextStyle)` | ❌ **不置位** | 改变值，但不改"谁赢" |

**决定：保留这个行为，但把规则明确成两条**（其余是推论）：

> 1. **`set_style(TextStyle)` 是批量、派生来源**（配方 / 样式上下文 / 组件的
>    `apply_text_style`），**不置实例标记**；
> 2. **单独的字段 setter（`set_font_size` / `set_color` …）是实例级显式意图**，
>    置标记，并从此刻起赢过一切派生来源。

颜色同构（`color_explicit_` 与 `apply_component_color` 是同一套）。

**两点必须写进文档，否则会被当成 bug**：

- 标记是**粘性**的：一旦显式设置过，之后换主题、换 tone、组件重解析都不会再改这个字段，
  直到 `clear_font_size()` / `clear_explicit_color()`。
- `set_style` 之后标记仍是上一次的状态 —— 所以它的准确含义是"**有过实例级显式设置**"，
  而不是"当前这个值是显式的"。

**为什么不反过来（让 `set_style` 也置标记）**：那会让组件应用配方时把自己标成"显式"，
从而**封死**后续的样式上下文与主题应用 —— 是更坏的错误。
