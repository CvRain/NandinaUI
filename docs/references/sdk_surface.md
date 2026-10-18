# 源码 SDK 的公共表面：头文件分类与门禁

> **状态：政策已定（2026-10-17），门禁未实施。** 本文回答两件事：哪些头文件算**对外契约**、
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

落点：清单 `tools/header_surface.toml`（182 条，含 `level`、可选 `reason`、可选 `requires`），
脚本 `tools/check_headers.py`，注册为 `header-surface`（`integration`，实测约 16 s，8 路并行）。
用法：

```bash
tools/check_headers.py --source . --write                 # 新头补默认等级，陈旧条目清理
tools/check_headers.py --source . --build-dir buildDir    # 三条门禁全跑（CI 走这条）
tools/check_headers.py --source . --skip-compile          # 无构建目录时只跑前两条
```

实现要点：

- **编译 flag 不手写第二份**：从 `compile_commands.json` 汇总 `-I`/`-isystem`/`-D`/`-std=`，
  所以 spdlog/raylib/SDL3 等 include 路径自动正确。
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
| 删除 `make_image_decoder()` | `IPlatformWindow`（experimental public） | 改为应用服务注入 `IImageDecoder` |
| `supports_image_loading()` 收窄 + 新增 RGBA 上传查询 | `IRenderDevice`（设备实现方接口） | 原先判断"能不能显示图片"的调用方，改为分别查询解码服务、设备直载、设备 RGBA 上传三项，见 [D7](reference_draft.md#三项能力分开表达2026-10-17-定) |

项目处于 alpha、源码集成、不承诺旧入口长期兼容，因此：

- **不保留**一个仍可 override、但框架已不再调用的虚函数 —— 那是最难发现的静默失效；
- 迁移说明只表达一件事：原来由自定义平台 override 提供的解码器，改为应用服务注入；
- 库内实现、测试替身、下游样例一次改完，要求重新编译。

## 6. 不要越界声明的事

- 门禁已实现，但它**不检查签名**：internal 类型出现在公开签名里仍要靠评审（见 §4）；
- 不要把"文件在 SDK 里"当成"支持范围已定义"；
- 不要用本清单代替构建闭包校验（SDK 仍必须包含构建所需的 internal 头）；
- 提升等级是**决策**，不是 `--write` 的副作用：`--write` 只补默认值（`detail/`、`internal/` 归
  internal，其余 experimental），理由与例外必须人来写。
