# 组件 Primitives 设计

> 状态：已校正
> 来源：改写自旧 C++ `archive/docs/widget-primitives.md`，primitive/control 边界在 Zig 重写中继续有效。

## 目的

widgets 方向不是堆更多「能用的控件」，而是先建立一套可组合、可复用、可被未来样式层统一消费的 primitive 体系。
这条路线对齐 shadcn 的 primitives / composition，以及 Angular 的 signal / effect / component 心智。

核心问题：一个控件里的代码，哪些属于结构、哪些属于交互状态、哪些属于文本能力、哪些属于焦点可视化、哪些该留给样式层？

## 术语

- **Primitive**：可复用低层积木，负责单一维度能力（结构容器、交互状态机、文本渲染、焦点可视化、布局组合），重点是「能被多个 control 复用且边界稳定」。
- **Control**：面向页面作者的真实控件，组合多个 primitive，对外暴露更高层语义 API。
- **Token**：设计系统的原子值与语义值来源，只提供值，不处理布局/事件/绘制。
- **样式层**：把 primitive/control 的语义状态、theme token、视觉属性解析连起来，不重新发明 primitive。

## Primitive 地图

### Surface（结构与视觉容器）
- 负责：背景色、圆角、描边、padding、把内容区约束收缩到 child、参与 measure→layout 主线。
- 不负责：交互状态机、事件语义、文本测量绘制、focus ring 显示策略。
- 一句话：负责「内容放在哪里、底面长什么样」，不负责「现在是什么交互状态」。

### Pressable（纯交互状态）
- 负责：管理 hovered / pressed / focused / disabled 状态，响应 pointer/focus 事件，提供 click/press/release/hover/leave 回调。
- 不负责：视觉绘制、文本、focus ring 绘制、业务语义（submit/destructive/primary）。
- 一句话：负责「用户正在怎样与控件交互」，不负责「控件看起来变成什么样」。

### Text（文本能力）
- 负责：UTF-8 文本内容、字体族/字重/字号/颜色、单/多行/overflow/line height、真实测量与绘制、对齐。
- 不负责：点击行为、状态机、背景/边框、表单字段级语义（required/error/hint）。
- 一句话：文本能力的独立边界；Label 是建立在 Text 之上的语义包装。

### FocusRing（焦点可视化）
- 负责：焦点描边绘制，支持 active / color / width / offset / corner radius，默认样式从 theme 读取。
- 不负责：判断是否应获得焦点、键盘导航、tab 顺序、focus policy、其它交互状态。
- 一句话：负责「焦点怎么被看见」，不负责「焦点从哪里来」。

## Control = primitive 组合

```
Control = 语义 API + primitive 组合 + 语义状态到样式的映射
```

- **Button** = Surface（底面/padding/圆角/描边）+ Text（文本）+ Pressable（交互状态）+ FocusRing（焦点反馈）+ 自身语义（variant/size/loading/destructive/click）。
- **Input** = Surface + Text（含 placeholder）+ FocusRing + 编辑语义（selection/caret/IME/validation）。
- **Card / Panel** = Surface + Text/Label；Panel 是中性内容面板，Card 承载 title/content/header/footer 语义。Card/Panel 默认不塞 Pressable/FocusRing，除非确实可点击/可聚焦。

## 何时该复用 primitive

出现以下任一情况，优先复用已有 primitive 而非控件内硬写：
- 这段逻辑未来至少会被第二个控件复用。
- 描述的是结构、状态机、文本或焦点这类稳定维度。
- 已开始依赖 theme token 或组件状态。
- 与业务语义无关，但与视觉/交互底层机制有关。

## 何时不该新增 primitive

- 只在单个控件出现一次，无复用迹象。
- 本质是某 control 的高层语义，而非低层能力（如 `ButtonVariant::destructive`）。
- 只是几个 setter 的简单打包，没形成稳定边界。

## 责任分层

```
token/theme 提供值
primitive 提供能力边界
control 提供语义 API
style layer 负责把状态和值映射到具体外观
```
