# 模块依赖规则

NandinaUI 按模块组织，模块之间的方向决定了改动会扩散到哪里。只要依赖始终单向向下，底层修改就不会牵动上层，上层可以单独替换或测试，也不会出现互相等待的循环。本文记录当前仍然有效的依赖约束，以及源码里已经存在的偏离——它们按项目约定都算技术债，需要带着收口方向一起排期。

## 分层与职责

框架自底向上分为五层，共 11 个模块：

| 层 | 模块 | 职责 |
| --- | --- | --- |
| 基础层 | `foundation` | 几何、纯布局约束、颜色、色彩空间、变换、纯值 motion、UTF-8、JSON、日志 |
| | `reactive` | 信号图与依赖追踪，纯逻辑，不接触渲染与场景 |
| | `resource` | 资源清单、多后端与运行时管理 |
| | `physics2d` | 可选的 Box2D 物理桥（默认关闭） |
| 主题层 | `theme` | 设计令牌、内置主题、样式文档 |
| 呈现层 | `render` | 渲染设备、绘制上下文、纹理缓存与裁剪栈 |
| | `text` | 文本布局协议、字体加载、整形、字形图集与文本绘制 |
| | `scene` | 场景树、节点、控件、锚定布局内核、画布层、动画宿主与帧调度 |
| | `semantics` | 无障碍语义树 |
| 控件层 | `widget` | 布局原语、primitives 与组件库 |
| 应用层 | `app` | 窗口、Router / Page、异步作用域与入口 |

依赖只允许沿这张表**向下**：上层可以使用下层的类型，下层不得反向引用上层。同一层之间的依赖应当是必要的、单向的。

> 原 `animation` 层的值实现已下移到 `foundation/motion/`，调度已下移到 `scene/`；
> 旧命名空间和转发头现已退出。声明式规格由 `foundation/motion/spec.hpp` 定义，
> `motion::TweenSpec`、`tween()`、`spring()` 与缓动常量只保留一个公开归属。
> 迁移与验证边界见[项目进度与下一步](project_status.md)。

## 约束

1. **禁止向上依赖**：底层模块不得引用高层模块的类型。例如 `scene` 不应引用 `widget`，`theme` 不应引用 `widget` / `app`。
2. **禁止循环依赖**：若 A 依赖 B，则 B 及其所有子依赖都不得反过来依赖 A。
3. **`foundation` 没有上游依赖**，只提供与框架无关的基础类型。
4. **`reactive` 保持纯逻辑**：它不依赖 `render` / `scene` / `widget`，因此可以完全脱离窗口做单元测试。
5. **第三方库只出现在实现单元**：raylib、SQLite、toml++、spdlog 等只被 `.cpp` 或它们自己的包装头引用，不进入面向上层的接口头文件。`foundation/json.hpp` 是 nlohmann/json 的包装，属于有意暴露的接口。
6. **每层可单测**：纯逻辑层应有不依赖窗口的 Catch2 测试；新增公共 API 时同步补测试。

违反上述方向的提交按项目约定视为**技术债**，需要在进入下一个里程碑前收口。如果因为阶段安排暂时保留，必须在文档或提交里写清原因与收口方向，不能默认接受。

## 当前偏离

下面这些边与上面的约束不一致。它们不是设计意图，而是分阶段推进过程中留下的债务：

| 偏离 | 位置 | 收口方向 |
| --- | --- | --- |
| `theme` 引用 `text` | `theme/style_document.hpp` 的 `text::FontFaceSpec` 与 `FontFamilyRegistry` 应用接口 | StyleContext 所需的请求描述已归 theme；StyleDocument 仍实际注册字体族、fallback 与默认值。后续需拆出上层应用适配器或最小注册接口，不能仅移动 FontRequest 就标记整条债务解决。 |
| `physics2d` 引用 `scene` | `physics2d/physics_world2d.hpp` 公开引用 `scene::NanNode2D` | `physics2d` 被列在基础层，却直接绑定场景节点。可选：把它在分层表里上移到呈现层旁；或让它只接受一个最小适配接口，由上层完成节点绑定。 |

修改这些位置时，如果需要新增一条向上依赖，正确做法通常是**把被引用的类型下移**，而不是让下层头文件命名上层类型；`docs/references/component_contract.md` 第 8 节对类型识别访问器也给出了同一条规则。

## 文本管线迁移的前置：纯布局约束

在移动 TextPipeline 协议之前，先将与节点无关的四个布局边界值抽到
`foundation::NanLayoutConstraints`（`foundation/layout_constraints.hpp`）。
迁移期 `scene/control.hpp` 曾保留 `scene::LayoutConstraints` 别名；当前已决定退出，公开签名
与调用方统一使用 foundation 拼写。`TextLayoutInput::constraints` 直接使用 foundation
类型，因此 `widget/primitives/text_layout.hpp` 不再仅为约束值包含整个 Control。

本步骤只迁移 `loose()`、`tight()`、`constrain()` 和 `deflated()`，保留四个字段的顺序、
默认值与现有数值行为。有限/无限上界、反向上下界、非有限上界的既有处理，以及四向
inset 缩减后下限归零，均用具体数值回归覆盖；不增加约束校验或节点职责。
新 foundation 头只依赖 foundation 几何值和标准库，以独立 include 的测试单元守住边界。

该迁移保持源码类型别名兼容，但具名类型的命名空间变化会改变相关 C++ 符号，例如
`NanControl::measure_layout` 的参数类型。因此库和调用方都需要重新编译；本步骤不提供
旧已编译二进制的 ABI 兼容保证。

这是纯约束类型的前置收口。在该步骤完成时，`text -> widget::primitives` 与
`scene -> widget::primitives` 两条 TextPipeline 依赖尚待协议迁移；删除一个 Control
include 本身不足以完成收口。后续协议迁移见下一节。主题字体描述与 physics2d 分层
不属于纯约束步骤。

本步骤已完成（2026-10-05）。`Text` 原语显式包含自己的 Control 基类定义，不再依靠
文本布局值头的传递 include。验证使用既有 Clang 21.1.8 / GNU libstdc++ 14 头文件环境：

| 验证 | 结果 |
| --- | --- |
| 普通、ASan/UBSan、no-RTTI 完整构建（含 showcase） | 三套通过 |
| 三套构建的 unit suite | 各 73/73 通过 |
| 新约束值与源码兼容测试 | 每套 12 个用例、92 项断言通过 |
| SDK 来源核验 | 13,812 个导出输入原始哈希、HEAD、12 项递归 submodule 状态一致 |
| 三格式 SDK 导出 | 新 `.hpp` / `.cpp` 均存在，档案内文件哈希与源码一致 |
| 导出后的纯值消费者 | 仅使用 ZIP 中的约束与 geometry 文件编译、运行通过 |

SDK 检查为单次打包与纯 foundation 消费者，不等同于重跑全部 SDK 可复现性 fixture。
窗口测试关闭、clipboard 的一个内部用例跳过、ASan leak 检测关闭及未运行远程 CI 的
限制与上一轮验证相同。随后只更新此验证记录，执行代码保持冻结时的内容。

## 文本布局协议收口

以纯约束提交 `d7acce93a002e8933a270242baf29d11756febc7` 为基线，文本值、caret 查询、
`ITextLayoutBackend`、`ITextLayoutRenderer`、`TextPipeline` 与 deterministic backend
已迁入 `text/text_layout*`。text 适配器、scene node/tree 与 NanWindow 使用 canonical
text 类型；迁移期 `widget/primitives/text_layout*.hpp` 曾重导出相同类型与函数，
现已退出。Meson 只编译新实现一次，原两份 widget `.cpp` 已删除。

因此上述两条 TextPipeline 上行引用已消除。设计、旧头的源码兼容边界、具名类型
迁移后的 ABI 重编译要求与验证记录见 [文本布局协议](text_pipeline.md)。
`theme -> text` 的字体描述债务与 physics2d 分层债务继续保留；既有 `TextAlign` 对
theme 枚举的引用也未改变。本步骤不宣称整个依赖图无环。

## 轻量字体请求拆分

以文本协议提交 `0706449d65c6516198602699297604dd224fd6ec` 为基线，
FontRequest / FontSlant 的唯一类型定义归 `theme/font_request.hpp`；迁移期 text
曾通过同类型别名保留兼容入口，现已退出。StyleContext 不再引用字体引擎，TextLayout 与内部文本样式桥也
只包含轻量请求头。family 保持 ResourceKey，合法依赖为 `theme -> resource`；
直接放入 foundation 会新增反向边，故本步骤不作这项迁移。

设计、值语义、ABI 与 include 兼容边界及验证记录见 [轻量字体请求](font_request.md)。
这只完成描述值的拆分，StyleDocument 的实际文本引擎应用仍列在当前偏离表中。

## 判断一次改动是否越界

- 新增的头文件引用是在往下（可以用）还是往上（需要下移或记录债务）？
- 两个模块是否开始互相引用？如果会形成环，先把共享类型抽到更低的层。
- 第三方头文件是否出现在公开头里？如果是，先用包装头隔离。
- 新类型是否让某个底层模块获得了它不该有的职责（例如 `scene` 认识组件、`reactive` 认识窗口）？

这些问题在提交前回答一次，比事后重构便宜得多。
