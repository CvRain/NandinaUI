# 模块依赖规则

NandinaUI 按模块组织，模块之间的方向决定了改动会扩散到哪里。只要依赖始终单向向下，底层修改就不会牵动上层，上层可以单独替换或测试，也不会出现互相等待的循环。本文记录当前仍然有效的依赖约束，以及源码里已经存在的偏离——它们按项目约定都算技术债，需要带着收口方向一起排期。

## 分层与职责

框架自底向上分为五层，共 12 个模块：

| 层 | 模块 | 职责 |
| --- | --- | --- |
| 基础层 | `foundation` | 几何、颜色、色彩空间、变换、UTF-8、JSON、日志 |
| | `reactive` | 信号图与依赖追踪，纯逻辑，不接触渲染与场景 |
| | `resource` | 资源清单、多后端与运行时管理 |
| | `physics2d` | 可选的 Box2D 物理桥（默认关闭） |
| 主题层 | `theme` | 设计令牌、内置主题、样式文档 |
| 呈现层 | `render` | 渲染设备、绘制上下文、纹理缓存与裁剪栈 |
| | `text` | 字体加载、整形、字形图集与文本绘制 |
| | `scene` | 场景树、节点、控件、画布层与帧调度 |
| | `animation` | Tween / Spring / 关键帧与动画宿主 |
| | `semantics` | 无障碍语义树 |
| 控件层 | `widget` | 布局原语、primitives 与组件库 |
| 应用层 | `app` | 窗口、Router / Page、异步作用域与入口 |

依赖只允许沿这张表**向下**：上层可以使用下层的类型，下层不得反向引用上层。同一层之间的依赖应当是必要的、单向的。

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
| `animation` 与 `scene` 互相引用 | `animation/animation_host.hpp`、`group.hpp` 依赖 `scene::NanControl`；`scene/node.cpp`、`scene_tree.cpp` 依赖 `animation::AnimationHost` | `SceneTree` 需要每帧推进动画，所以它持有动画宿主；`AnimationHost` / `Group` 又需要操作场景节点。可选：把动画宿主的所有权上移到窗口 / `app`，由应用驱动；或在 `scene` 定义最小的推进接口，由 `animation` 实现。 |
| `text` 引用 `widget::primitives` | `text/glyph_run_renderer.hpp`、`text/harfbuzz_text_backend.hpp` 引用 `widget/primitives/text_layout*` | `TextPipeline`、`ITextLayoutBackend`、`ITextLayoutRenderer` 描述的是文本布局协议，不是组件原语；应下移到 `text`（渲染器部分可留在 `render`）。 |
| `scene` 引用 `widget::primitives` | `scene/scene_tree.hpp` 使用 `widget::primitives::TextPipeline` | 与上一条同源：文本管线类型下移后，这条边自然消失。 |
| `theme` 引用 `text` | `theme/style_context.hpp`、`style_document.hpp` 引用 `text/font_family.hpp` | `theme` 只需要字体的**描述**（`FontRequest` 等），不需要文本引擎；把字体描述类型下移到 `foundation`，或在 `theme` 内联一份等价类型。 |
| `physics2d` 引用 `scene` | `physics2d/physics_world2d.hpp` 公开引用 `scene::NanNode2D` | `physics2d` 被列在基础层，却直接绑定场景节点。可选：把它在分层表里上移到呈现层旁；或让它只接受一个最小适配接口，由上层完成节点绑定。 |

修改这些位置时，如果需要新增一条向上依赖，正确做法通常是**把被引用的类型下移**，而不是让下层头文件命名上层类型；`docs/references/component_contract.md` 第 8 节对类型识别访问器也给出了同一条规则。

## 判断一次改动是否越界

- 新增的头文件引用是在往下（可以用）还是往上（需要下移或记录债务）？
- 两个模块是否开始互相引用？如果会形成环，先把共享类型抽到更低的层。
- 第三方头文件是否出现在公开头里？如果是，先用包装头隔离。
- 新类型是否让某个底层模块获得了它不该有的职责（例如 `scene` 认识组件、`reactive` 认识窗口）？

这些问题在提交前回答一次，比事后重构便宜得多。
