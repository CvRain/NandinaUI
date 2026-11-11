# 源码 SDK 的公共表面：头文件分类与门禁

> **状态：政策与三类门禁已落地（2026-10-18）；公开签名中的内部类型仍需评审。** 本文回答两件事：哪些头文件算**对外契约**、
> 用什么机制保证这件事不被静默破坏。它不改变 [构建系统与 Modules 范围](build_system_scope.md)
> 里"CMake package 暂缓"的结论，也不改变 [D7](reference_draft.md#d7-渲染质量与后端视觉一致性)
> 的图片解码归属。

## 1. 两个必须分开的概念

| 概念 | 说明 | 谁使用 |
| --- | --- | --- |
| **构建闭包**（collection） | 编译框架与工具所需的全部源文件；当前由 `tools/sdk_bundle.py` 用 `git ls-files --recurse-submodules` 收集 | 构建、SDK 打包 |
| **公共表面**（surface） | 允许下游直接包含、并按等级承诺稳定性的头文件清单 | 应用开发者、文档、未来的安装式 SDK |

**"SDK 里带着这个文件"不等于"它是公共 API"。** 源码 SDK 要从源码构建框架和工具，内部头也可能是
构建所必需的；不能用一张"公共头白名单"去过滤掉其余文件，那会把构建闭包一起删掉。两者关联，
但不是一个过滤器。

## 2. 三档分类

2026-11-11 P4.8b 页面渐进式分节挂载（experimental，新增作者 API）：`app::PageContext` 新增
`DeferredSectionBuilder` / `supports_deferred_sections()` / `defer_section(widget::View
placeholder, builder)`；页面把已入树的占位控件注册为延迟节，Router 在首帧之后每帧最多构建
一节并在原兄弟位置替换占位，换页/销毁丢弃未执行节，builder 异常走错误处理器；无 dispatcher
的宿主同步执行。契约见 [页面与路由](page_and_router.md) §4.1、[页面加载响应性](page_load_responsiveness.md)。

2026-11-10 P4.7 合成字重/斜体回退（experimental）：新增 `nandina/text/font_synthesis.hpp`
（`FontSynthesis { bold, oblique, bold_em }`、`synthetic_font_style()`、`synthetic_bold_em` /
`synthetic_oblique_shear`；500/600/700+ 分级强度，缺 face 时生效）。`FreeTypeFontFace` 的
`rasterize*()`、`GlyphAtlas`、`HarfBuzzTextLayoutBackend` 与 `FontPipeline` 增加带默认值的
`FontSynthesis`/`FontRequest` 参数（旧调用保持源码兼容）；`page_probe` 新增
`NANDINA_PROBE_WIDTH/HEIGHT` 仅供人工视觉验收，golden 仍固定 1200×800。契约见
[合成字重/斜体回退](font_synthesis.md)。

2026-11-10 P4.6 Step 2a 语义排版 role（experimental，新增作者 API）：`theme::TypographyRole`
（display/heading/title/subtitle/body/label/caption）与 `theme::typography_role_style()` /
`typography_role_name()`；`Label` 新增 `set_typography_role()` / `typography_role()` 与 builder
`.typography_role()`。role 只携带 TextSize/Leading/weight/slant 默认值，颜色仍由组件颜色 token
决定；显式字号/行高/字重优先于 role，role 的 px/leading 经主题 `NanTypeScale` 解析。role 表暂为
固定 canonical 表，Step 2b 再主题化并让组件消费；现有页面视觉不变。另新增受保护的
`Text::apply_component_font_weight/slant()` 与 `font_request_is_explicit()` 供组件角色实现使用。
契约见 [排版尺度与单位](typography_scale.md) §5。

2026-11-10 P4.6 Step 1 排版尺度（experimental，新增作者 API）：新增
`nandina/theme/typography_scale.hpp`：`TextSize`（xs…9xl，13 档 Tailwind 兼容；C++ 标识符不能含 `-`，因此代码拼写为 `xl_2 … xl_9`）、
`Leading`（none…loose，6 档）、`NanTypeScale`（`root_base` + ratios + leading，主题可覆盖）
及纯函数 `text_size_px()` / `leading_factor()` / 名称查询。`NanTypographyTokens` 新增
`scale`；`text::TextStyle` 新增 `line_height`（0 = 后端默认，>0 = 字号乘数）；
`Label` 新增 `set_text_size()` / `set_leading()` / `set_line_height()` 与对应 builder 方法，
`primitives::Text` 新增 `set_line_height()` / `line_height()`。既有 `.font_size(px)` 保持不变
（绝对逻辑 px）；OS 内容缩放仍始终跟随，不引入 pt 或物理像素字号。契约见
[排版尺度与单位](typography_scale.md)。

2026-11-10 TextField 悬停状态（experimental，行为增强）：`theme::TextFieldVisualState` 新增
`hovered` 位，`widget::TextField::hovered()` 提供只读查询；`TextField` 在 `on_input` 处理
`mouse_enter` / `mouse_leave`（paint-only，禁用时始终为 false）。默认配方新增 hovered 规则
（`accent` 底 + `ring` 边框，focused 规则复位为页面底 + `input` 边框），`NanStyle` 的 TextField
规则 `state` 名称新增 `"hovered"`。迁移无需改动；主题作者可覆盖/移除该规则。测试标签
`[widget][text-field][hover]`、`[theme][text-field][hover]`。

2026-11-09 P4.5a 高 DPI 内容缩放（experimental，行为语义迁移）：`WindowConfig::width/height`
改为逻辑（DIP）单位；`high_dpi=false` 表示固定 1×（测试/探针/golden 用）。平台 seam 的坐标
空间定形：`IPlatformWindow::screen_size()` 返回逻辑窗口尺寸、`drawable_size()` 仍为物理
framebuffer、`display_scale()` 为物理/逻辑比、`poll_input()` 的指针坐标折回逻辑空间；
`SdlTextInputService::set_window_scale()` 由平台层维护，场景 `TextInputArea` 仍为逻辑坐标。
`app/window_metrics.hpp` 新增 `content_scale_from_display()`、`window_size_from_logical_size()`、
`logical_size_from_window_size()`。注入平台或迁移 `WindowConfig` 调用方后须重新编译；契约见
[高 DPI 与多显示器](high_dpi_and_multi_display.md)。

2026-11-05 P4.3 D3 子批（破坏性字段迁移，复核完成并随 `095336b` 提交）：
`RouteOptions::type` 改为 `nav_group`，默认从 `"default"` 改为空串；`NavSection::type`
改为 `group`，不保留旧字段或别名。显式 `.nav_group = "default"` 仍创建普通有标题组；
省略/空串全局合并为一个无标题组，组间分隔线保留。所有组按精确值比较与首次出现顺序排列，
`PageKey` 路由身份、address 显示与 typed navigation 沿用既有规则。迁移调用方后须重新编译；
正式契约见 [Page / Router](page_and_router.md)，验证见 [D3](reference_draft.md#d3-路由身份与导航分组开放到哪一层)。

2026-11-08 P4.3 D2 Shell 多方案框架准备（experimental，已随 `f821f40` 提交）：
`ThemeCatalogErrorKind` 破坏性改为区分 scheme/family 的 operation-specific 值，
`ThemeCatalogError {kind, optional<ThemeSchemeId> scheme, optional<ThemeFamilyId> family}`
替代原 `{kind, ThemeSchemeId id}`；`BuildContext` 新增作用域绑定的
`on_theme_selection_change()` 与 `on_theme_catalog_change()`。迁移调用方后须重新编译；
错误消费应使用新的主体字段与 `describe()`，不要从 family 操作伪造 scheme ID。

2026-11-08 P4.3 D2 第 4d 步 TOML 命名颜色字面量（experimental，已随 `5061b39` 提交）：
`style_document` 的颜色字面量字段新增命名字符串写法（`primary = "SteelBlue"`、参考色阶
档位与 Button/TextField 规则颜色字段），经 `foundation::nan_named_color` 解析；
`$palette.*`/`$palettes.*` 引用语义不变、失败不回退成色名；配方颜色错误保留规则种类与
0 起始下标，palette 统一校验错误也保留完整输入路径（含别名拼写）；失败不修改活动状态。

2026-11-08 P4.3 D2 第 4c 步命名颜色字面量（experimental，已随 `f383201` 提交）：新增
`foundation/named_color.hpp`：`NanNamedColor`（CSS Color 4 2026-11-03 CRD §6.1 的 148 个
命名颜色）+ `nan_named_color(NanNamedColor)` / `nan_named_color(std::string_view)` /
`nan_named_color_name()`；字符串入口 ASCII 大小写不敏感并拒绝空白/NUL/非 ASCII/未知名称，
私有 `constexpr` 表在 `.cpp`，别名保留独立身份，枚举序号不承诺持久化稳定；
枚举入口要求已声明成员。复核补齐实际枚举成员与表的对应、名称/身份唯一性与有界查询回归。

2026-11-07 P4.3 D2 第 4b 步作者颜色入口（experimental，已随 `3e2542a` 提交）：`theme.hpp` 新增
`ReferenceColorFamily`、`reference_scale()`、`reference_color()`、`reference_family_name()`、
`color_shade_name()`（primitive palette 类型化访问）；`theme_scheme.hpp` 新增
`SemanticColorOverride`、`ThemeColorError`、`canonical_color_token()`、`semantic_color()`、
`set_semantic_color()`、`apply_semantic_colors()`、`ThemeSchemeContent::from_reference()` /
`from_semantic_colors()` 与可选 `reference` 来源记录；显式 `NanColor` 覆盖与文件导入同一提交语义，
不新增原始色语义 token。文件 palette 覆盖复用 `apply_semantic_colors` 并拒绝非有限值；
规范名优先规则保持不变。C++ 批量重复角色最后一项生效；查询要求已声明的枚举值。

2026-11-09 P4.3 D2 第 4a 步（experimental，CPU 侧已随 `fef829a` 提交，后端读回复核完成）：
新增 `foundation/composition.hpp`：唯一 CPU 合成器（显式 `CompositeSpace::encoded_srgb`
策略、`nan_composite_over`/`nan_composite_background_stack`/`nan_composite_over_stack`
与 `CompositionError`）；新增 `theme/contrast_diagnostics.{hpp,cpp}`：显式
`ContrastRule`/`ContrastProfile`/`evaluate_contrast`/`ContrastDiagnostic`（三态状态、
角色路径、实测比值、所需阈值、解析后 OKLCH 与转换后 sRGB）与 `default_contrast_profile()`。
`ContrastRule::tone` 为 tone 引用（含变换操作数）提供显式上下文；缺少时报告上下文不足。
诊断 `foreground` 为可选 sample，未解析时为空；背景在首个不透明终点停止，变换路径保留
操作及操作数。`ContrastDiagnostic::config/tone` 记录计算上下文；报告注明现行通道裁剪与
浮点、无 u8 量化策略。软件 renderer、Linux/Hyprland+RADV 与 Windows 11/D3D12 离屏及默认
swapchain GPU renderer 的生产绘制路径已按 §6.8 读回，并与共享编码 sRGB 合成器及错误空间
负向对照核对（8 位 sRGB 目标；读回不可用或 HDR 目标打印 SKIP）；Linux 默认 swapchain、
其他驱动读回和注册门槛接线仍未完成。`color_token_name()` 供角色路径诊断。

2026-11-07 P4.3 D2 第 4 步（experimental，已随 `4746173` 提交）：新增
`theme/theme_observation.{hpp,cpp}`：`ThemeSelectionPresentation`（选择投影）、
`ThemeCatalogPresentation`（目录投影，`ThemeCatalogState {version, schemes, families}`）、
`ThemeChangeHub`（`ThemeChange {before, after}` 明确变化事件，仅生效身份/外观/有效快照变化）；
`BuildContext::theme_selection()/theme_catalog()/theme_changes()/on_theme_change()`；
`ReactiveScope::keep(Subscription)`。作者入口：`derive_content`/`derive_scheme`/
`validate_scheme`/`ThemeSchemeError`、`ThemeSchemeCatalog::contains_family()`；
`ThemeManager` 增加仅选择/目录变化的 staged-only 发布（不递增 revision、不广播组件）。
`ThemeObserver::prepare_theme_presentation` 改名为 `prepare_theme_publication`，同时参与有效与
staged-only 发布；自定义 observer 须迁移 override 并重新编译。`ThemeScheme` 的相等比较按
元数据与不可变内容指针，等价替换不推进 version。新状态投影的 readonly 是借用句柄，须在
其 scope 与 Graph 存活期间读取；保存 ThemeChange 的值可延长端点快照寿命。

2026-11-06 P4.3 D2 第 3 步（experimental，已复验并随 `995e6ae` 提交）：新增
`ThemeSchemeId`/`ThemeFamilyId`（`namespace:name` 规范文本、typed parse 带偏移错误、255 字节
上限）、`ThemeSchemeContent`/`ThemeSchemeCatalog`（register/replace/remove_scheme、family、
staged 失败原子提交）、`ThemeSelection`（fixed/appearance + `UnregisteredThemeSource` 匿名
来源）。`ThemeManager` 移除 legacy 写入口（register_theme/activate/register_family/
activate_family/register_theme_family/set_theme/apply），只保留目录、select/selection、
active_scheme_id 与偏好/系统外观入口；`StyleDocument::apply` 把文件主题/家族编译为
`styles:x<hex>` 规范 ID。破坏性迁移：仓库内调用点与测试经 theme_test 兼容层迁移，需重新编译。

2026-11-05 P4.3 D2 首批窄投影（experimental，已随 `8920f3a` 提交）：
新增 `theme/theme_presentation.hpp/.cpp`：`ThemePresentationState { ColorAppearance appearance;
std::uint64_t revision; }` 与作用域绑定的只读 `ThemePresentation`（`get()`/`readonly()`/`subscribe()`），
`BuildContext::theme_state()` 返回本页唯一实例；`ReactiveScope::own<T>()` 提供作用域拥有的
非响应式服务（同类唯一、随作用域先于信号销毁）。只提供读取与订阅，写入仍经 ThemeManager；
首批不含选择状态、方案身份、目录或作者颜色入口。

2026-11-06 D2 首批复核补修（已随 `8920f3a` 提交）：`ThemeObserver` 增加默认空的
`prepare_theme_presentation` 阶段与 `ThemeObserverUpdate` 临时提交/通知任务；现有观察者
仍实现原有内部广播，投影在其后统一写入，再提交 effects、最后调用外部订阅。
通知期间所有主题写入口先抛 `logic_error`，单个通知异常不阻断后续必要投递，结束后重抛首个
异常；原先依赖重入写主题的调用方须改为在通知返回后提交请求。manager 与投影的内部布局
已调整，库与调用方须一起重编译。`readonly()` 仍为借用视图，不能在页面作用域销毁后读取。

2026-11-05 P4.2c（experimental，已随 `095336b` 提交）：新增 `widget/collapsible.hpp`，
导出 Collapsible（Button trigger + retained 内容）；`ui.make<Collapsible>(text)` / Signal<bool>
重载、`.child()` / `.open()` / `.on_change()` / `.gap()`。`set_open` 静默，用户 toggle 才发事件，
trigger typed override 和 container/label visual part 沿用 Button；不新增配方。
`semantics::State` 新增可选 expanded，合并语义迁移至同一规则；现有非折叠控件该值为空。
契约与边界见 [折叠内容组件](disclosure_components.md)。

2026-11-05 P4.2b（experimental，代码复核完成、视觉验收待完成）：新增 `widget/table.hpp`，
导出 `Table` / `TableColumn` 与类型受限的 `.header_cells()` / `.row_cells()` Builder
入口。Table 使用 NanControl 场景布局，cell/row/header 语义包装属于 widget，平台无关语义
只新增 table/row/column_header/cell role；无排序、选择或数据模型，不新增主题配方，单元格
内容沿用现有 StyleContext。使用前置条件、事务性与测量契约见
[数据组件 §8](data_components_and_virtualization.md#8-p42b-静态-table布局契约)。

2026-11-04 P4.2 阶段 5 第一批（experimental，维护者复核完成）：新增
`widget/selection_model.hpp`（`SelectionMode` 枚举 + `SelectionModel<Key>` 模板：身份即键，
single/multiple、范围选择与锚点、`retain()` 数据替换保留、`changed()` 无载荷事件，纯逻辑
不依赖窗口）与 `widget/virtual_list_view.hpp`（`VirtualListView<Item, Key, NodeT>` 模板：
统一行高窗口物化、自身为垂直滚动视口、点击/键盘选择、`activated()` 事件；内部持有一个
`ForEach` 窗口区域，不继承 ForEach/Flex）。`VirtualListView` 与 `ListView` 同样不属于
`ComponentTraits` 组件清单（声明式数据列表原语），入口是直接构造 + `set_source` /
`set_model`。变高行、水平滚动、行内交互控件、无障碍语义与节点池回收不在本批；
规则与边界见 [数据组件：选择模型与虚拟化依据](data_components_and_virtualization.md)。

2026-11-03 P4.1b 视觉表面作者入口（experimental，代码已提交 `0e7fca1`，Showcase 正式视觉验收待完成）：新增
`widget/surface.hpp`，Surface 复用 Padding/BoxPainter/BoxPresentation，使用 `ui.surface()` /
`authoring::surface()` 工厂；不新增 ComponentTraits 或组件配方。`set_box_style()` / `.box_style()`
设置 token 盒子，`set_padding()` / `.padding()` 设置有限非负边距，既有 container visual path
用于实例/动画覆盖；默认零内边距，圆角不提供子内容 mask。
Card 新增 `set_content_insets()` / `content_insets()` / `clear_content_insets()`，四侧显式覆盖
与默认配方 padding 分开，`.content_insets()` 连接同一有效布局几何；默认 Card 行为不变。
Image 新增 `set_cover_anchor(PercentLength, PercentLength)` / `cover_anchor()`，默认 50/50，
范围 [0,100]，在 source_rect 可裁剪余量内定位，paint-only 且不改变缓存。builder
`.image_scale()` / `.cover_anchor()` 按 setter 能力约束。新增 inset_validation.hpp 为 internal。
依赖方向不变，圆角子树裁剪和任意节点变换均未扩展；正式边界见 D6 §18。

2026-11-03 P4.1 文本作者/绑定子批（experimental，行为调整）：`Property<T>::bind()` 现在
单源替代，`set()`（含同值）解除旧绑定；新增 `unbind()` / `is_bound()`、ReactiveScope 绑定
重载，ReadProperty 可查询 is_bound。Property 显式禁用复制/移动，守卫绑定的目标地址与令牌；
scope 清理、目标销毁或 source 求值期间重入替代均使旧 callback 失效。
`Label::bind_text(source)` 改为立即生效，新增页面 scope 重载与 `unbind_text()` /
`text_is_bound()`；`ui.make<Label>(source)` 和 `ui.bind(label, &Label::set_text, source)`
统一使用 managed 文本绑定，退出树解绑且不复活。普通直接写入后需要继续跟随源的旧调用必须
显式重新 bind；其他组件/泛型 setter 的绑定语义不在本次调整范围。
NodeBuilder 新增 `.text()` / `.bind_text()` / `.unbind_text()`、`.font()` / `.font_family()` /
`.font_weight()` / `.font_slant()` / `.text_overflow()` / `.max_lines()`，均按 setter 能力约束。
Text/Label 新增只读 `line_count()` / `overflowed()`；布局后端与字体服务未注入时，字体请求
变化也会失效并传给自定义 backend。逐字段样式 reset、完整 D6 和文本节点变换边界仍保留。
详见 [D6 §17](authoring_dsl_and_content_measurement.md#17-p41-文本作者入口与绑定替代契约2026-11-03第二个实施子批)。

2026-11-03 P4.1a 文本盒内对齐（experimental，增量）：新增 `text::TextVerticalAlign`
（top/center/bottom）、`TextStyle::vertical_align`（默认 top）、`Text::set_vertical_align()` /
`vertical_align()`，Label 继承共享实现；`NodeBuilder::text_align()` / `text_vertical_align()`
按 setter 能力约束开放，不用于复合组件内部文本。`TextStyle::approx_equals()` 比较新字段，
metrics cache 排除颜色与两轴对齐；纯绘制 `set_style` 不再标 layout 或重新整形，显式覆盖规则
保持现状。自定义 ITextLayoutBackend 须忽略颜色/对齐字段对排版结果的影响。
`Text::draw_in()` 明确使用已映射的 screen-space 盒子（修正文档误记的逻辑坐标），布局结果
保持逻辑尺寸；`draw_at()` 不应用两轴对齐，viewport scale 只应用一次。D6 其余作者能力/绑定/
测量设计仍在后续 P4.1，见 [P4.1a 契约](authoring_dsl_and_content_measurement.md#16-p41a-文本盒内对齐契约2026-11-03首个实施子批)。

2026-11-03 P3.3b2b 部分交付（experimental）：公开 scene/widget 签名未变。
SDL 服务现在把 begin 请求的 text/password/email/number 与 multiline 映射到
`SDL_StartTextInputWithProperties`；SDL host 默认声明 `SDL_IME_IMPLEMENTED_UI=composition`，
显式环境/宿主 hint 仍优先，候选列表由平台绘制。内部 SDL 服务新增可注入的启动函数用于验证
真实调用属性，仍归 internal，不构成作者 API。用途是会话开始时的快照/平台提示，不过滤提交
文本；活动会话中更改用途未增加动态刷新协议。Linux 实机结果与剩余边界见 D1 §5.5。

2026-11-02 P3.3b2a caret 输入区域（experimental，已实施）：`NanNode` 新增
`text_input_caret_rect()`（焦点节点局部逻辑矩形，允许刷新编辑区滚动），默认转发编辑目标，
`TextField`/`TextArea` 覆写以计入内边距/垂直居中/滚动。`ITextInputTarget` 的 caret 仍是
编辑目标局部坐标。`NanSceneTree::update_text_input_area(world_to_window = identity,
window = empty)` 新增可选宿主映射/窗口客户区边界，返回 `TextInputRequestResult`（原为 void）；
普通无参调用仍兼容，保存旧成员函数指针签名的调用方需调整。`NanWindow` 在布局后每帧传
viewport transform 与窗口逻辑尺寸，SDL 报告 composition + input_area 并调用
`SDL_SetTextInputArea`。软键盘/可见性反馈/遮挡仍 unsupported；实机验收见 P3.3b2b。

2026-11-02 P3.3b1 预编辑与密码用途显示（experimental，已实施）：`EditableText` 新增
`caret_at()`（布局局部点 → 提交值 caret，自动处理 composition/密码视觉映射）；
`TextInputPurpose::password` 现在把视觉文本按 grapheme 折叠为 `U+2022`，而 `value()`、剪贴板
与提交路径保持原始文本。`TextField`/`TextArea` 的指针命中改经该入口。caret 区域由 P3.3b2a 补齐；真实 IME 与
软键盘/遮挡平台验收仍归 P3.3b2b。

2026-11-01 P3.3a 文本输入会话协议（experimental，已实施）：新增
`scene/text_input_service.hpp`（`TextInputPurpose`/能力位/`TextInputSession`/
`ITextInputTarget`/`ITextInputService`/`ITextInputSessionSink`/`TextInputPlatformEvent`）。
`NanNode` 新增 `text_input_target()`；`NanSceneTree` 新增 `set_text_input_service()`、
`active_text_input_session()`、`text_input_session_active()`、`apply_text_input_event()` 与
`update_text_input_area()`；`FrameEvent` 新增 `scene::TextEditingEvent`（IME 组合更新）；
`IPlatformWindow` 新增带默认 nullptr 的 `text_input_service()`，自定义平台可继续不为文本
会话提供实现（committed text 走既有 `TextInputEvent` 路径）。P3.3a 当时只覆盖 SDL 桌面的
composition；P3.3b2a 增补 input_area。软键盘/遮挡仍未支持，真实 IME 验收见 P3.3b2b。

2026-10-31 P3.2a 焦点提示策略（experimental，已实施）：新增 `scene/focus_hint.hpp`
（`InputModality` / `FocusRequestReason` / `FocusRequestContext` / `FocusHintRequirement`）；
`NanNode` 新增虚查询 `focus_hint_requirement()`；`NanSceneTree` 新增 `input_modality()` /
`record_input_modality()`、`window_active()` / `set_window_active()`、
`focus_hint_always_visible()` / `set_focus_hint_always_visible()`、`should_show_focus_hint()`、
`focus_hint_context_visible()` 与 `set_focus(node, FocusRequestContext)`。
`FrameInput` 新增 `std::optional<bool> window_active`（平台窗口活动状态，宿主转交 scene）。
自定义宿主/平台应把 window focus 事件映射到该字段；焦点提示绘制应只消费 scene 的统一查询，
不要各自维护设备状态。证据与剩余边界见 [D1 第一阶段记录](focus_and_input.md#第一阶段实施记录2026-10-31p32a-核心批次)。

2026-10-31 P3.1c 有序输入帧（experimental，破坏性，已实施）：`app::FrameInput`
由按类型分数组改为 `std::vector<FrameEvent> events`，`FrameEvent` 是鼠标移动/按钮/滚轮、
键与提交文本的 `std::variant`。自定义平台按捕获顺序追加事件；宿主按同一顺序派发。
帧级 `pointer` / `modifiers` 保留为当前状态，不替代事件字段。逐事件 viewport 映射与
坐标/修饰键快照不变；`MouseEnter`/`MouseLeave` 仍由场景命中测试产生。证据见
[D8 P3.1c](reference_draft.md#p31c跨类型事件保序2026-10-31-已实施)。

2026-10-29 P3.1b 键身份（experimental，破坏性，已实施）：新增
`foundation/input.hpp` 的 `LogicalKey` / `PhysicalKey`。`scene::KeyEvent` 改用类型化
逻辑键构造，增加物理键参数与查询；删除 int 构造、`keycode()` 和
`widget/key_codes.hpp`。自定义平台必须分别提供可用的两种身份与 press/release/repeat；
无法映射时使用 unknown，不伪造键。`is_pressed()` 仅初次按下，可重复导航/编辑使用
`is_press_or_repeat()`；单次激活/开合/提交不消费 repeat。完整行为见
[D8 P3.1b](reference_draft.md#p31b键身份与生命周期2026-10-29-已实施)。

2026-10-29 P3.1a 输入快照（行为修复，无签名变更）：`FrameInput` 中的事件坐标与
修饰键是捕获时快照，独立 `pointer` / `modifiers` 是帧级当前状态。自定义平台仍按现有
接口填充事件；宿主将按钮/滚轮自己的坐标映射到逻辑空间，不再用帧末指针覆盖。
SDL 键事件消费原生 mod，鼠标事件消费队列修饰键状态。键身份、键生命周期和跨类型
保序未改变；实施证据见 [D8](reference_draft.md#p31a事件快照2026-10-28)。

2026-10-22 P2.1a 构建表面变更：资源运行时通过 `resource_runtime_dep` 链接，协议核心可单独使用
`resource_core_dep`；扫描/manifest/packager 属于可选 `resource_build_dep`，不再由 `nandina_dep`
传递提供 scanner 实现。直接调用构建 API 的消费者须开启 `resource_tools` 并显式链接
`resource_build_dep`。`resource_package_format.hpp` 为 experimental 共享格式协议；旧
`SQLiteBackend::application_id/schema_version` 保留为同一常量的转发，包格式不变。
`NanApplicationConfig::resource_package` 默认值与外部 nanres 接入将在 P2.1b 单独迁移。

2026-10-23 P2.2a 后端协议变更（experimental）：`IResourceBackend::find()` / `open_stream()`
改返回带 `PublishGeneration` 与 `ResourceConsistency` 的 `ResourceSnapshot` /
`ResourceStreamSnapshot`，负结果也属于某个代次；新增 `generation_exhausted` 错误码。
`ResourceManager` 八个公开入口在 P2.2a 保持裸句柄返回，只在内部解包；作用域结果
（`ResolvedResource`/`ResolvedStream`、域令牌、歧义/过期错误）按 P2.2b 迁移。自定义后端
须返回一致的结果与代次；`non_snapshot` 结果不得仅凭作用域键进入共享内容缓存。

2026-10-23 P2.2b 作用域结果（experimental，破坏性）：`ResourceManager` 八入口改为返回
`ResolvedResource` / `ResolvedStream`，携带 `ResourceDomainToken`、`ResourceMountId`、
发布代次、一致性类别、记录 ID 与句柄；旧 `ResourceLookup`/`ResourceStreamLookup` 裸句柄别名
删除，消费者改用 `.resource` / `.stream`。新增 `resolve(const ResolvedResource&)` /
`resolve(const ResolvedStream&)`，只按原作用域重解析，过期返回 `expired_scope`；裸 ID 多重
挂载命中返回 `ambiguous_resource_id`，消息列出冲突挂载。缓存身份迁移（字体/图片/流）在
P2.2c，本批只做编译级消费者迁移。

2026-10-23 P2.2c 缓存身份（experimental，破坏性）：新增 `ResourceScopeIdentity`、
`scope_identity(...)` 与 `ResourceManager::domain()`。`text::FontFaceKey` 改为
`(ResourceScopeIdentity, face_index)`；`FontLoader::invalidate(ResourceId)` 改名为
`invalidate_wide(ResourceId)`，新增 `invalidate(const ResolvedResource&)`；直接
`ResourceHandle` 加载绕过共享 face 缓存，`non_snapshot` 来源不进入共享缓存。
`render::TextureCache` 新增 `load_resolved(const ResolvedResource&, options)`；资源异步加载在
后台解析后才形成作用域缓存键，`non_snapshot` 内容可上传但不索引、不保留。文本 key 的
`load_memory` 仍只用于调用方显式提供的非资源内存。

2026-10-24 P2.2d 缓存回收（experimental，增量）：`text::FontLoaderLimits{max_entries}` 为
FontLoader 提供有界 LRU，`cached_entries()` 返回当前关联数；访问时清扫 expired 域与已释放的
weak face，淘汰不销毁调用方持有的 face。`render::TextureCache` 新增 `sweep_expired()` 与
`invalidate(const ResolvedResource&)`；`begin_frame()`/`trim()` 执行 expired 域清扫，既有
`max_retained_entries`/`max_retained_bytes` 继续负责容量淘汰。资源异步加载改为解析/解码
两阶段，缓存命中不再解码或上传；pending 仍按域 + 逻辑 key + options 合并。

2026-10-24 P2.3a 资源身份（experimental，构建侧）：`resources.toml` 的 `[[resources]]` 新增
相对 `path`，与 `key` 一起构成显式 key→路径映射；显式映射优先于路径派生 key。资源构建核心
新增 `derive_resource_key(prefix, relative)` 与 `derive_resource_id(package_id, key)`；
后者使用版本前缀 + NUL + big-endian u32 长度 + package_id + key，SHA-256 截断 128 bit。
同 key 的旧 lock 保留原 ID，新 key 使用派生 ID；同一 `(package_id, key)` 的派生 ID 跨构建、
跨平台稳定（固定向量锁定），不同 key 即使内容相同也各得独立 ID。扫描新增
`ResourceScanOptions::mapped_paths`：显式映射拥有的文件不再派生 key、不报扫描诊断；
`resource_path_identity(...)` 统一路径身份比较（Windows 上按 ASCII 折叠大小写）。
`--rewrite-lock`、构建目录迁移与碰撞注入在 P2.3b。

2026-10-24 P2.3b lock 迁移（构建侧）：`nanres scan/validate/pack` 新增 `--lock <path>`；
构建 helper 将 lock 写到 `<output>.lock.toml`，普通构建不再写源码树。旧随机 ID 的 lock 会被
检测并失败，只有 `nanres scan --rewrite-lock` 显式迁移；两个标志仅在 manifest 模式生效，
原始扫描覆盖下返回用法错误。加载 lock 遇到同 ID 不同 key 或同 key 不同 ID 会列出冲突；lock
的包 ID 与 policy 不一致时先报包不匹配。相同策略/资产的独立重建得到逐字节相同的 lock，
无变化不重写 lock 或 `resource-location.json`。

2026-10-25 P2.3c 作者常量头（experimental，构建侧）：新增 `nanres keys --header <path>`，
从 `resources.toml` 与扫描结果生成只含逻辑 key 的 `inline constexpr std::string_view`
常量头；规范 key 与 `[aliases]` 都会生成，不含内部 ID、摘要、源路径或安装位置。命名空间为
`nandina::resource_keys::pkg_<package-id>`（`.`/`-`/`_` 分别转义为大写
`DOT`/`DASH`/`UNDERSCORE`），符号为 `RESOURCE_` 加大写 key。两个 key 折叠成同一符号时命令失败并列出双方，不静默改名；
内容未变化时不重写文件。`keys` 只接受 `--header`，`--root` 等扫描覆盖和其它命令的
`--header` 均返回用法错误。该头是可选构建输入，不影响只用运行时 key 的应用。

2026-10-25 P2.4 示例应用（构建侧，非 SDK API）：`examples/lab` 随普通 UI 构建默认参与，
一个可执行文件包含登录/注册、笔记、Focus 计时器、图片预览与确认弹窗。它要求 `build_ui=true`、
SDL3 和资源工具链；资源包与生成常量头由 Meson 在链接/编译前生成。示例不是 SDK 安装内容；
lab 本身不承诺密码掩码，notebook 在 P3.3b1 后已通过 `TextInputPurpose::password` 显示掩码，
但示例仍不承诺真实认证或跨会话持久化。

2026-10-28 示例应用演进（同样是构建侧变化，非 SDK API）：`examples/lab` 已退役，由
`examples/notebook` 接替（个人笔记软件，数据层先行）。notebook 要求 `build_ui=true`、资源工具链
和 `resource_sqlite=enabled`（应用数据库与迁移包都走 SQLite），因此在关闭 SQLite 或关闭工具的
配置里用 `-Dexamples=false` 跳过；`notebook-data` 数据层测试随 `examples` 一起构建，Windows 上
可正常运行。P2.4 的 `examples/lab` 验收记录保留为历史。

| 等级 | 含义 | 稳定性承诺 | 谁可以包含 |
| --- | --- | --- | --- |
| `public` | 推荐使用的接口 | alpha 内也尽量不破坏；破坏时给迁移说明 | 应用代码、示例、下游宿主 |
| `experimental` | 允许外部使用，但接口仍会调整 | 明确标注可能变更；变更需要迁移说明 | 愿意跟进的宿主/适配器作者 |
| `internal` | 为构建与内部实现携带，不是契约 | 无 | 仅框架自身（含 probe、开发工具） |

规则：

1. **internal 头不得出现在 public/experimental 头或公开签名里**；需要暴露类型时要么前移类型，
   要么把它提升为 experimental 并说明。
2. **public/experimental 头必须能在 SDK 消费环境里独立包含**（只带自己需要的依赖）。
   仓库已有先例：`foundation/motion/spec.hpp` 承担独立包含测试，文本头退出时也保留了
   canonical 独立包含测试 —— 这条机制复用即可，不需要新造。
3. **新增框架头文件必须有分类**，没有分类直接失败（见 §4）。

## 3. 当前决策（2026-10-17）

| 头文件 | 等级 | 理由 |
| --- | --- | --- |
| `app/platform_window.hpp` | **experimental public** | 自定义宿主、平台适配器和窗口测试替身有真实外部用途；它已经是应用注入平台实现的接口。D8 会调整输入模型，所以允许变更但明确标注 |
| `render/recording_device.hpp` | **internal** | 目前服务后端取证：字符串记录格式、成本与行为都不是对外契约。外部使用者仍可用公开的 `IRenderDevice` + `NanWindow::set_render_device_wrapper()` 自己实现装饰器。确有下游诊断需求时再提升为 experimental |

把 `platform_window.hpp` 藏成 internal 并不符合实际用途（它本来就是给宿主实现方用的），
而把 `recording_device.hpp` 提升为 public 会让一个取证格式变成长期负担。两者的正确等级不同，
不能"统一处理"。

## 4. 三类门禁（**已实施 2026-10-18**）

| 门禁 | 检查什么 | 失败表现 |
| --- | --- | --- |
| 分类完整 | `nandina/**/*.hpp` 每个都在清单里恰好一条 | 新头未分类 / 陈旧条目 / 非法等级 → 失败（不是警告） |
| 不越界 | public/experimental 头不 `#include` internal 头（相对与 `<nandina/...>` 两种写法都解析） | 依赖方向倒置 → 失败 |
| 独立可包含 | 每个 public/experimental 头用一个只包含它的 TU 做 `-fsyntax-only` | 缺依赖/隐式前置 → 失败 |

落点：清单 `tools/header_surface.toml`（188 条，含 `level`、可选 `reason`、可选 `requires`），
脚本 `tools/check_headers.py`，注册为 `header-surface`（`integration`，实测约 16 s，8 路并行）。
用法：

```bash
tools/check_headers.py --source . --write                 # 新头补默认等级，陈旧条目清理
tools/check_headers.py --source . --build-dir buildDir    # 三条门禁全跑（CI 走这条）
tools/check_headers.py --source . --skip-compile          # 无构建目录时只跑前两条
```

实现要点：

- **编译 flag 不手写第二份**：从 `compile_commands.json` 汇总 `-I`/`-isystem`/`-D`/`-std=`，
  所以 spdlog/SDL3 等 include 路径自动正确。
- **可选功能用 `requires` 表达**：例如 `physics2d/physics_world2d.hpp` 标 `requires = "physics2d"`，
  该选项关闭时跳过独立可包含检查并打印跳过原因，而不是硬编码依赖的 include 路径。
- **故障注入已验证**：新增未分类头 → 失败；`button.hpp` 包含 `internal/focus_scope.hpp` → 失败并指出
  两端；破坏 `geometry.hpp` → 独立可包含失败（并级联到包含它的头）。
- **"internal 头不出现在公开签名里"仍靠评审**：脚本只能检查 include 方向，看不到类型出现在签名中；
  这一条写在这里，避免被误认为已自动化。

### 4.1 首次运行发现的真实违规与处理

`popover.hpp` / `hover_card.hpp` / `dropdown_menu.hpp` 在公开签名里使用
`internal::OverlayPlacement` / `internal::OverlayAlignment`，而定义它们的是
`nandina/widget/internal/anchored_positioner.hpp`（原分类 internal）。处理：**把该头提升为
experimental**，理由与后续动作写进清单——两个枚举是公开 widget API 的一部分，但文件仍在
`internal/` 目录下，后续应移动到 `nandina/widget/overlay_placement.hpp` 再删除这条例外。

**清单描述支持范围，`sdk_bundle.py` 描述构建闭包。** 将来提供安装式 SDK 时，由这份清单生成
公共 include 树，而不是再维护第二份"安装头列表"。

## 5. 破坏性变更的记录方式

图片批同时有两处**破坏性 API 变更**，都要登记：

| 变更 | 位置 | 迁移说明 |
| --- | --- | --- |
| 删除 `make_image_decoder()` | `IPlatformWindow`（experimental public） | 改为应用服务注入 `IImageDecoder`（2026-10-20 已删除，含两后端 override） |
| 删除 `make_raylib_image_decoder()` | `render/image_decoder.hpp`（experimental public） | 默认实现改为 `render::make_cpu_image_decoder()`（stb，后端无关） |
| `supports_image_loading()` → `supports_direct_image_loading()` | `IRenderDevice`（experimental public） | 语义收窄为"设备自己解码直载"，默认 false |
| 新增 `supports_rgba_texture_upload()` | `IRenderDevice` | SDL/raylib 均为 true；`Image` 用三项能力选择服务路径 |
| 新增 `NanApplication::image_decoder()` / `set_image_decoder()` | `app`（experimental public） | 默认注入 CPU 解码器，应用可替换；无热切换（窗口打开时快照） |
| 删除 `make_raylib_device()` 与 `raylib_device.hpp` | `render/backends`（experimental） | 使用 SDL3，或实现并注入自己的 `IPlatformWindow` / `IRenderDevice` |
| 删除 `WindowBackend::raylib` | `app/window_config.hpp`（experimental） | 改为 `automatic` / `sdl3`；`sdl3` 保留原数值 2。环境变量中的 raylib/未知值明确报错，不能静默选择另一路径 |
| 默认 SDL3 静态链接 | Meson / SDK | `sdl3_linkage=shared` 可恢复共享模式；关闭 SDL3 时无内置窗口后端，`showcase=false`，应用在 `open()` 前注入平台 |
| `NanApplicationConfig::resource_package` 默认值 | `app`（experimental public） | 默认从 `"resources.db"` 改为**空**：启动不再自动发现/打开包；需要自动开包时显式设置。迁移：依赖旧默认值的应用显式写 `"resources.db"`，或改用注入 |
| 新增 `NanApplication::mount_resource_backend()` | `app`（experimental public） | 在窗口/字体/图片首次消费前注入自定义后端；后端由应用保活 |
| `resource_tools=false` / `external_nanres` | Meson 构建表面 | 关闭本地工具时 `nandina_resource_toolchain` 仅在提供外部工具时存在；外部工具须通过 `nanres schema`（值 1）与 `--version` 握手，否则配置期失败 |
| 新增 `nanres schema` / `nanres --version` | `nanres` CLI（构建期工具） | 供外部工具握手与诊断；不属于应用运行时 API |
| `supports_image_loading()` 收窄 + 新增 RGBA 上传查询 | `IRenderDevice`（设备实现方接口） | 原先判断"能不能显示图片"的调用方，改为分别查询解码服务、设备直载、设备 RGBA 上传三项，见 [D7](reference_draft.md#三项能力分开表达2026-10-17-定) |

项目处于 alpha、源码集成、不承诺旧入口长期兼容，因此：

- **不保留**一个仍可 override、但框架已不再调用的虚函数 —— 那是最难发现的静默失效；
- 迁移说明只表达一件事：原来由自定义平台 override 提供的解码器，改为应用服务注入；
- 库内实现、测试替身、下游样例一次改完，要求重新编译。

退役后清单为 182 个头文件，其中 10 个 internal（清单随后续批次增长，当前为 198/12）。生成模板归 NandinaCLI；
框架维护 [`sdk_app` 消费 fixture](../../tests/fixtures/sdk_app/README.md)，不维护第二份模板。

## 6. 不要越界声明的事

- 门禁已实现，但它**不检查签名**：internal 类型出现在公开签名里仍要靠评审（见 §4）；
- 不要把"文件在 SDK 里"当成"支持范围已定义"；
- 不要用本清单代替构建闭包校验（SDK 仍必须包含构建所需的 internal 头）；
- 提升等级是**决策**，不是 `--write` 的副作用：`--write` 只补默认值（`detail/`、`internal/` 归
  internal，其余 experimental），理由与例外必须人来写。

2026-11-06 第 3 步复核补修（已随 `995e6ae` 提交）：文件导入采用同一 manager 提交的暂存目录/选择/
style，member ID 增加 `/families/x<family-hex>`，各文件家族成员独立；家族切换不重归属。
旧初稿的非家族 styles ID 拼写继续用于独立主题。过长别名 expected 诊断，槽位冲突显式
appearance_mismatch。manager 的目录/选择/active ID 是借用视图，任何成功写入后需重新查询；
读取跨写入的内容应持有复制值或 shared content。文件字体注册沿用主题提交后的独立流程。
