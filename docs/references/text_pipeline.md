# 文本布局协议的归属与兼容入口

本步骤以 `d7acce93a002e8933a270242baf29d11756febc7` 为基线，先确定协议边界，再迁移实现。
纯布局约束已归 foundation，文本协议不再需要 Control 定义。

## 归属

`TextStyle`、`TextLayoutInput/Line/Result`、caret 值与查询、`TextOverflow` / `TextAffinity`、
对齐及 overhang 辅助函数归 `text/text_layout.hpp/.cpp`。`TextAlign` 继续使用现有 theme 枚举，
不在本步骤改写主题类型。

`ITextLayoutBackend`、`ITextLayoutRenderer`、`TextPipeline` 与 deterministic fallback 归
`text/text_layout_backend.hpp/.cpp`。渲染接口继续只前置声明 `render::DrawContext`；具体
`GlyphRunRenderer` 留在 text，保持已有 text 到 render 的实现依赖方向。

这些是文本协议和值，不是节点或控件。text 的 HarfBuzz、glyph renderer、FontPipeline，
scene 的 node/tree 与 app 的 NanWindow 使用 canonical text 名称；widget 组件通过旧头的
别名继续使用相同类型，不作全库组件机械改名。

## 行为边界

本步骤只改变归属、include 与名称解析，不改变布局或 caret 算法、style 比较、fallback
选择、最大行数与 overflow 规则。`TextPipeline{}` 继续绑定进程寿命的 deterministic backend，
renderer 默认为空。旧名字与新名字必须指向同一 backend singleton，而不是复制一个默认实例。

默认函数的实现也随协议下移，避免 text 的默认 pipeline 为取函数地址而返回依赖 widget。
scene 仅前置声明 `text::TextPipeline`；旧 `struct widget::primitives::TextPipeline;` 必须同步
移除，否则与兼容类型别名冲突。NanWindow 的声明、存储与定义一起更新。

## 源码兼容与构建

原 `widget/primitives/text_layout.hpp` 与 `text_layout_backend.hpp` 保留为兼容头，重导出
原公开类型与函数。嵌套 `TextLayoutLine::Glyph` 随同类型别名保留，不创建相似但不同的类型。
原两个 `.cpp` 的实现移到 canonical text 路径，由 Meson 只编译新实现一次。

兼容保证覆盖包含原公开头的使用者。依靠下层头间接获得 widget 名称、手动前置声明旧
widget struct，或手写构建引用原实现路径的外部代码，需要改用兼容头或新的实现路径。
具名类型归属变化会改变 C++ 符号；库和调用方都需要重新编译，不提供旧二进制 ABI 兼容。
类型的关联命名空间也随之变化；外部依赖旧 widget 命名空间进行 ADL 查找的扩展需要调整，
不能仅凭兼容别名推断这类扩展保持原有查找结果。

canonical 与 legacy 公开头用不同测试单元独立 include，核验所有别名、默认 pipeline、
自定义 backend / renderer 的继承签名与实际布局调用。现有文本、caret、fallback、控件与
场景回归继续作为行为证据，执行普通、ASan/UBSan 与 no-RTTI 构建和 unit suite。

## 依赖收口

完成后应消除 `text -> widget::primitives` 与 `scene -> widget::primitives` 的文本协议引用。
新 text 头只使用 foundation、resource/text 的字体描述及既有 theme 对齐值；不重新引入
scene 或 widget 作为共享值的来源。

主题字体描述目前仍由 text 提供，`theme -> text` 的既有债务独立保留；physics2d 分层和
具体 renderer 归 render 的选择也不属于本步骤。不宣称全部模块依赖已经无环。

## 验证记录（2026-10-05）

| 检查 | 实际结果 |
| --- | --- |
| 普通、ASan/UBSan、no-RTTI 完整构建，含 showcase | 三套通过，本轮各完成 210 项构建任务 |
| 三套 unit suite | 各 74/74 通过 |
| canonical / legacy 独立公开头测试 | 每套 4 用例、39 断言通过 |
| 普通非 SDK integration | 6/6 通过，串行执行，包含 CLI 模板构建 |
| 迁移审计 | 16 组完整 token 流按约定改名后相同，无意外算法变化；下层头闭包不含 widget |
| 故障注入 | 宽度系数漂移、旧工厂 wrapper 丢失函数身份均编译成功并在指定 REQUIRE 失败；字节恢复后目标 suite 重新通过 |
| SDK 来源 | 13,817 个现存输入原始哈希、HEAD 和 12 项递归子模块状态一致，两个旧实现的删除确认通过 |
| 三格式 SDK 档案 | 新 canonical 四文件存在且哈希匹配，旧两份 `.cpp` 均直接核验缺失，档案大小/哈希与 manifest 一致 |
| ZIP 冷消费者 | 六个 Nandina 源文件及同 SDK 的 vendored utf8proc 冷编译、运行通过；新旧默认 backend 相同，CAT 布局和 caret 数值符合预期 |
| 格式与空白 | 20 个本轮 C++ 文件 clang-format 21 dry-run 通过，git diff --check 通过 |

工具链沿用 Clang 21.1.8、GNU libstdc++ 14 开发头、Meson 1.7.2、CPython 3.13.16。
这是增量完整构建的实际任务数，不是全项目 clean build 的总目标数；没有实际 GCC 运行结论。

窗口测试关闭，clipboard 每套有一个依赖 Wayland compositor 的 Catch 用例跳过；
Meson 将对应 unit 记为 OK。ASan/UBSan 启用，leak 检测按仓库开发流程关闭，physics2d 关闭。
未执行远程 CI、其他平台或真实桌面手感验收。

SDK 检查为一次导出和协议子集的冷消费者，不等同于两轮 SDK 可复现性 fixture 或完整
SDK app/library 冷构建。冷消费者未覆盖具体字体引擎、scene/window 或 GPU renderer，
也未为这个消费者开启 sanitizer/no-RTTI；三配置 unit 验证是独立证据。
验证结束后只添加此文档结果记录，执行代码与测试保持冻结时的内容。
