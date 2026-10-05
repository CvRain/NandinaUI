# 轻量字体请求的归属与兼容入口

> 当前状态：过渡头 `text/font_request.hpp` 和 `text::FontRequest` / `text::FontSlant`
> 别名已退出；使用 `theme/font_request.hpp` 中的 `theme::FontRequest` / `theme::FontSlant`。
> 下文的兼容范围和验证记录描述当时的迁移阶段，不是现行 API 承诺。

本步骤以 `0706449d65c6516198602699297604dd224fd6ec` 为基线，先确定描述边界，再移动定义。

## 类型归属

`FontRequest` 描述所需的 family、weight 与 slant，不持有已加载字体或文本引擎。
`FontSlant` 与请求一起归 `theme/font_request.hpp`，canonical 名称为
`theme::FontRequest` / `theme::FontSlant`。这与样式上下文使用描述值的职责一致。

family 继续使用 `std::optional<resource::ResourceKey>`。新头直接依赖既有 resource 值头
和标准库，保持合法的 `theme -> resource` 方向。若直接移到 foundation，会新引入
`foundation -> resource`，而 resource 已使用 foundation 工具，形成新的模块环。
本步骤不改 ResourceKey 的归属、校验或表示，也不复制一份相似但不同的请求类型。

`text/font_request.hpp` 只为原 `text::FontRequest` / `text::FontSlant` 提供同类型别名；
`text/font_family.hpp` 包含这个兼容头，原使用者仍可通过已有公开入口取得请求类型。
FontFaceSpec、FontFamilyRegistry、字体注册与 fallback 解析继续归 text。

## 值与依赖边界

保留字段顺序 `family`、`weight`、`slant`，默认值为空 family、400、normal，枚举次序
normal / italic / oblique，以及默认的成员比较。保持 aggregate、designated 与位置
初始化，不增加 weight clamp、验证、family 转换或新的解析逻辑。

`theme/style_context.hpp` 使用本层请求头和 canonical 类型，不再为字体描述包含完整
`text/font_family.hpp`。`text/text_layout.hpp` 与 `widget/internal/text_style_bridge.hpp`
使用轻量 text 兼容头。具体引擎使用者仍显式包含自己的字体头，不作全库机械改名。

`theme/style_document.hpp` 仍持有 `text::FontFaceSpec`，并通过 FontFamilyRegistry 应用
字体族、fallback 与默认值；这条 `theme -> text` 的实际引擎应用债务独立保留。
描述拆分不等于全部主题依赖收口，也不涉及 physics2d 分层或具体 renderer。

## 兼容边界

包含原 `text/font_family.hpp` 的源码保留旧名字，新轻量 `text/font_request.hpp` 也提供
这些名字。类型别名保持请求、枚举与 StyleValue 实例的身份，原注册/解析接口继续
接受旧 text 拼写的值。

类型的关联命名空间变化会影响 C++ 符号和 ADL 扩展。库与调用方都需要重新编译，
不保证旧二进制 ABI；外部手动 `struct text::FontRequest;` 或
`enum class text::FontSlant;` 前置声明与别名不兼容，应包含公开头。
依靠样式/布局值头间接获得 FontFamilyRegistry 等完整引擎定义的外部使用者应补自己的
直接 include。仓库里的实际引擎消费者已有对应 include。

## 验证范围

canonical 和 legacy 首先包含各自公开头的独立测试单元验证同一类型、字段/默认值、
aggregate 与比较行为。legacy 单元显式包含样式上下文和原 font_family 入口，验证
StyleValue 与旧注册签名互用、实际继承和 explicit 请求解析。

默认 weight 漂移应使 canonical 默认值断言失败；独立类型替代兼容别名应使身份和
接口静态断言失败；字段次序或比较改变应被具体 family / weight / slant 数值回归捕获。
执行一次默认值故障注入并按原字节恢复，保留真实 REQUIRE 失败证据。

复用既有字体、样式、文本与控件回归，串行执行普通、ASan/UBSan、no-RTTI 完整构建
和 unit；普通非 SDK integration 继续覆盖 CLI / 构建工作流。核验新增两个头在 SDK
三格式中的导出字节，并从 ZIP 导出源码编译轻量消费者，不将此 probe 称为完整
字体引擎或 SDK app 冷构建。

## 验证记录（2026-10-05）

| 检查 | 实际结果 |
| --- | --- |
| 普通、ASan/UBSan、no-RTTI 完整构建，含 showcase | 三套通过，本轮任务分别为 142 / 217 / 217，故障注入的目标构建另有记录 |
| 三套 unit suite | 各 75/75 通过 |
| canonical / legacy 请求头测试 | 每套 7 用例、30 断言通过；类型身份与接口签名另由静态断言守住 |
| 普通非 SDK integration | 6/6 通过，串行执行 |
| 默认值故障注入 | 默认 weight 400 改 500 后编译通过并在指定 REQUIRE 失败；按原字节恢复，目标 suite 重新通过 |
| 独立迁移审计 | 8 项完整 token 对比、5 个轻量 include 闭包通过，无新 foundation 反向依赖 |
| SDK 副本刷新 | 只复制 14 个改动文件与 Git 元数据，不重复制 vendor 源码树 |
| SDK 来源等价 | 13,822 个现存输入原始哈希、HEAD 与 12 项递归子模块状态全部一致 |
| SDK 三格式 | 新请求头存在且字节哈希一致，档案大小与哈希匹配 manifest |
| ZIP 冷消费者 | 仅编译导出的 resource.cpp 与轻量头，默认值、family 比较、新旧类型身份与 StyleContext 继承/explicit 解析通过 |
| 格式与空白 | 8 个改动 C++ 文件 clang-format 21 dry-run 及 git diff --check 通过 |

首次调用新目标时，旧 Meson 缓存尚未包含其名称，工具在编译和故障注入前退出。
重新配置后完成上述正式验证；保留初次诊断与成功故障证明，不将目标缓存错误算作
回归测试的失败证据。

沿用 Clang 21.1.8 / GNU libstdc++ 14 开发头、Meson 1.7.2 与 CPython 3.13.16，
编译并发为 4。上述任务数是本轮增量完整构建的数量，不是 clean build 总目标数。
没有实际 GCC、远程 CI、跨平台或真实桌面验收结论。

窗口测试关闭，clipboard 每套有一个缺少 Wayland compositor 的内部 Catch 用例跳过，
Meson 将该 unit 记为 OK。ASan/UBSan 启用，leak 检测按仓库流程关闭，physics2d 关闭。
SDK 检查仅一次导出和轻量子集冷消费者，未重跑两轮 SDK 可复现性 fixture，未覆盖
完整 SDK app/library 或具体字体引擎冷构建；消费者没有 sanitizer/no-RTTI 标志，
三套 unit 是独立证据。验证后只添加此文档结果记录，执行代码与测试保持冻结字节。
