# 渲染设备路径契约（SDF 生产路径的前置）

> **状态：批次 3 最小实验与参数策略已落地；生产 SDF 已接入填充/描边/圆。** 其余图元与多平台产物仍待做。
> 在动代码前先把"两条渲染路径的边界"冻在这里，避免边写边改接口。本文写契约、验收与实测记录；
> uniform 预算见 §3，微基准与失败注入见 §8。生产接入仍按 §6 验收。

## 1. 两条路径

| 路径 | 何时使用 | 质量 | 状态 |
| --- | --- | --- | --- |
| **SDF**（`SDL_GPURenderState` 自定义 fragment shader） | 宿主拿到 SDL **GPU renderer** 且能创建 render state | 解析抗锯齿，与 raylib 同级 | **已接入填充/描边/圆（Vulkan 实测）；arc/线段/阴影/圆角裁剪仍走 CPU** |
| **CPU 几何**（当前实现：中心矩形 + 边条 + 圆角扇形） | 普通 `SDL_Renderer`、GPU render state 不可用、或显式关闭 | 硬边（`segments_for(r)=clamp(ceil(r*0.75),6,48)`） | 已存在，保留为基础路径 |

规则：

1. **不按图元大小在两条路径间随机切换**：切换单位是"整个设备会话"，因为着色器参数、批次 flush
   与 AA 外扩都是全局约定；逐图元混用会让同一页面出现两种视觉语义。
2. **CPU 几何路径必须始终可用**，并如实报告自己的质量（无解析 AA、圆角裁剪只到矩形）。
3. **活动路径要能被诊断出来**：报告里给出 `render_path=sdf|cpu` 及原因（renderer 类型、
   render state 创建失败的错误码）。

## 2. 能力报告

`SdlRenderDevice`（或平台层）在创建后必须能回答：

| 能力 | 含义 | 今天 |
| --- | --- | --- |
| `supports_analytic_aa()` | 当前路径是否提供解析抗锯齿 | **是**（GPU renderer 上的 SDF；默认 false 的接口方法 `analytic_antialiasing()`） |
| `supports_rounded_clip()` | 是否按圆角轮廓裁剪 | false（只到矩形） |
| `supports_soft_shadows()` | 是否提供软阴影 | false |
| `render_path()` | `sdf` / `cpu` + 原因 | `sdf`（GPU renderer）或 `cpu`；原因见 `sdf_unavailable_reason()` |

已有的 `supports_rounded_rect()` / `supports_alpha_textures()` 保持语义不变。
**新查询必须有消费者**：`supports_analytic_aa()` 至少要被一处诊断或测试读取，否则不算落地。

## 3. SDL GPU render state 的硬约束

- **uniform 传输**：`SDL_SetGPURenderStateFragmentUniforms(state, slot, data, length)` 复制数据，再经
  `SDL_PushGPUFragmentUniformData` 推送；字段遵循 **std140 布局**，不是每个 slot 只能放 16 字节。
  固定的 SDL 3.4.18 实现每阶段有 4 个 uniform slot；内部 32 KiB 分配缓冲不能当作公开的单次上传上限。
  参数集必须装进预算，装不下要移到顶点属性或纹理——这是**布局决策**，必须在接通图元前定形。
- **顶点格式**：走 `SDL_Vertex`（position + color + uv），CPU 几何路径已经在用；SDF 路径复用同一批
  顶点，只是把"形状参数"从几何细分挪到 uniform。
- **AA 外扩**：SDF 的解析 AA 需要把几何向外扩约 1 像素，否则边缘的抗锯齿会被几何本身裁掉。
- **坐标空间**：uniform 里的位置/半径必须与 fragment 坐标同一空间（设备像素），因此需要
  `logical_to_screen` 的比例作为参数或在提交前换算；分数缩放（1.25/1.5）必须测。
- **状态设置与恢复**：`SDL_SetGPURenderState(renderer, state)` 之后必须
  `SDL_SetGPURenderState(renderer, nullptr)` 才能绘制文字/纹理；漏恢复会让后续内容错乱。
- **批次 flush**：固定版本的 `SDL_SetGPURenderState` 本身只设置状态指针；不同状态会阻止命令合批。
  修改已被入队命令引用的 state 的 uniform 则会触发 `FlushRenderCommandsIfGPURenderStateNeeded`。
  因此必须分别统计状态设置和 uniform 更新，不能把 flush 成本全部归因于设置/恢复。Label 页几何与文字交错，
  因此**每帧状态切换次数**是必须报告的指标（见[性能预算](performance_budget.md)），不能只看总帧时间。

## 4. 参数布局（待最小实验定形）

raylib 的 SDF 用一个 `uMode` 管 6 种图元（fill / outline / segment / clipped_circle / shadow / arc）。
移植到 SDL 至少需要：

| 参数 | 说明 |
| --- | --- |
| mode | 图元种类（整数/半整数编码） |
| 图元几何 | 矩形（中心 + 半尺寸）、半径、描边宽度等 |
| edge / clip | AA 宽度、圆角裁剪的矩形与半径（若走 shader 内裁） |
| color / alpha | 颜色与不透明度（可走顶点色，省 uniform） |

要求：**同帧不同尺寸、圆角、颜色、描边交错**时参数互不污染；退出自定义状态后文字/纹理仍正确。
最小实验的失败注入固定两条：**故意漏更新一次参数**、**故意漏一次状态恢复**，都必须变红。

本轮实验边界：独立 Vulkan 探针使用一个 slot、64 字节（4 个 float4）的候选布局，另以
4 KiB 布局读取末尾哨兵验证大块数据确实到达 shader。16 KiB 实验失败：上传返回成功，但尾部读取
未产生预期颜色；固定版本 Vulkan 实现的描述符 `range = MAX_UBO_SECTION_SIZE = 4096`
（`subprojects/SDL3/src/gpu/vulkan/SDL_gpu_vulkan.c:71` 定义，5314/5433/8686 处作为
`bufferInfos[].range` 使用），因此超过 4 KiB 的部分不会被着色器读到。
因此当前 Vulkan 路径按 **每 slot 不超过 4 KiB** 设计，不拿 GPU 硬件上限或 32 KiB 内部缓冲大小代替它。
测试覆盖填充/描边、透明叠加、1/1.25/1.5 缩放、上传纹理与 SDL 字形绘制；生产字体管线另验。
计时分别比较原生、固定 uniform、逐次更新 uniform、只切换状态、交错并更新，记录 CPU 提交和 present，
不把异步 CPU 计时称为 GPU 执行时间。探针产物嵌入可执行文件；生产三格式产物另批交付。
参数提交策略（逐次上传 vs 不可变 state 缓存）已由 §9 的数据决定。

## 5. 产物与分发

- 着色器产物（SPIR-V / DXIL / MSL）由源生成并受控提交；维护关卡核对"源哈希 + 工具版本 + 产物"一致
  （[依赖策略](dependency_policy.md) §7）。生产 shader 的 SPIR-V 已嵌入 libnandina
  （`nandina/render/backends/sdf_rounded_spirv.hpp`，登记在 `tools/shader_artifacts.toml`），
  普通构建不需要 glslc/spirv-val；`tools/shader_artifacts.py` 默认校验哈希，`--verify-compiler`
  用固定工具链再生。DXIL/MSL 尚未交付。
- 嵌入方式（编译进二进制 vs 随资源包）与安装布局在批次 3 定形；**不能假设构建树路径在安装后仍可用**。
- 共享库分发（当前 vendored SDL3 是共享库）与着色器产物是两件事，分别验证。
  便携目录布局、RUNPATH 与"CMake 子项目不进 Meson install 计划"的实测见
  [依赖策略 §2.6](dependency_policy.md#26-共享库的运行时分发2026-10-18-实测)。

## 6. 批次 3 的验收（不替代 D7）

1. 同帧多参数：不同尺寸/圆角/颜色/描边 + 文字 + 上传纹理交错，参数与状态正确；
2. 分数缩放（1.0/1.25/1.5）、小半径、细描边、半透明叠加通过；
3. 静止与交互像素按[视觉回归](visual_regression.md)对照（基础正确性用解析预期）；
4. 逐图元 trace 保持不变（证明改动仍在设备层之下）；
5. 报告每帧状态切换次数、总帧时间、内存；新路径经审核建立自己的性能基线；
6. 基础路径（CPU 几何）独立验收：能力报告为 false、观感与今天一致。

## 7. 与其它文档的关系

- 缺陷与观感问题：[D7](reference_draft.md#d7-渲染质量与后端视觉一致性)；
- 像素预期与 golden 审批：[视觉回归](visual_regression.md)；
- 采样协议与阈值：[性能预算](performance_budget.md)；
- 后端能力差异表：[平台与后端契约](platform_backend_contract.md) §3。

## 8. 最小实验记录（2026-10-18）

受追踪的实现为 `probe/cross_platform/desktop/sdf_state_probe.cpp`，Meson 在启用 SDL 且构建测试时
生成 `tests/sdf_state_probe`。`sdf-state-probe` 是 integration harness，先验证正常帧，再分别启动
两种故障注入；没有桌面显示时以退出码 77 明确跳过，`--require-gpu` 则不得跳过。驱动或 shader
创建失败不伪装成通过。探针直接消费 SDL，不改变生产 `IRenderDevice`，不产生页面 golden。

### 像素与参数

- AMD Ryzen 5 7500F / Radeon RX 6750 GRE 12GB / RADV（Mesa 26.2.5-arch1.1）/ Vulkan，vendored SDL 3.4.18。
- 一个 slot，64 字节：中心/半尺寸、半径/半描边宽度/模式/哨兵索引、颜色乘数、尾部哨兵。
  本轮只验证填充和描边；segment、clipped_circle、shadow、arc 的生产参数布局仍需定形。
- 1.0、1.25、1.5 缩放全部通过：不同位置/尺寸/半径/颜色、先前入队图元在后续更新后仍正确、
  中空描边、小圆角与 1 逻辑像素细描边、透明叠加、RGBA 纹理四个区域、文字状态恢复。
- 平坦区域使用独立 alpha 合成预期，直边 AA 使用一维解析覆盖率，逐通道容差 ±2。
  文字用 SDL 内建字形纹理，与同帧恢复默认状态后的参考文本逐像素比较，并要求参考确有前景。
  **不替代 Nandina 的 HarfBuzz/字形图集与真实页面验收**，曲线 AA 的全面误差界也留到生产阶段。
- 4 KiB 上传读取第 255 个 float4 的非单位乘数，像素通过。16 KiB 尝试的最后 float4 未通过；
  正式回归只执行受支持的 4 KiB，不把越界读取结果当作跨驱动稳定的反例。
- 漏第二次 uniform 更新：描边采样得到 `(32,48,64)` 而预期 `(27,173,54)`，失败。
  漏状态恢复：上传纹理采样得到 `(32,48,64)` 而预期 `(116,44,72)`，失败。
- 额外捕获：若描边在 `abs(distance)` **之后**求 `fwidth`，1.5 缩放时跨轮廓中心的导数可能为零，
  细线覆盖率错误。实验 shader 改为先对原始有符号距离求导，再构造描边距离；同一解析断言覆盖它。

### 着色器交付边界

`sdf_state.frag.hlsl` 由 glslc 2026.4 生成两个 SPIR-V 变体，独立 SPIRV-Tools v2026.4 校验。
`sdf_state_spirv.hpp` 嵌入源码 SHA-256、编译器信息及产物 SHA-256；普通构建不执行 shader 编译器，
运行不依赖源码或构建树中的着色器文件。`shader-artifacts` 检查源码/产物一致性，维护命令
`python3 tools/shader_artifacts.py --verify-compiler sdf_state` 进一步验证固定编译器可复现。

这里只交付 **Vulkan 实验产物**。生产 SPIR-V/DXIL/MSL、真实 Image/文字消费者、CPU 降级与驱动逃生舱、
三页 trace/像素/性能、内存和 GPU 资源测量仍是批次 3 的剩余工作。

### 状态与 uniform 成本

原始 25 次结果：[Vulkan 微基准记录](../../probe/cross_platform/results/sdf-state-vulkan-2026-10-18.txt)。
探针单独以 clang 23.1.3、`-std=c++26 -O2 -DNDEBUG` 编译，链接 `buildDir/subprojects/SDL3` 中同一份
**debug 构建的 vendored SDL3**；窗口 540×360、scale=1、关闭 vsync、每帧 512 次几何提交。
每模式预热 300 帧后采样 1200 帧，重复 5 次并轮换模式顺序。CPU submit 包含 clear、几何入队和
显式 `SDL_FlushRenderer`；present 单独计时；completed 包括 present 后等待 GPU idle。
该隔离协议避免前一帧积压，却不能代表正常多帧流水线或纯 GPU 时间，也不是 release Label 基线。

复现本轮混合构建条件（Linux，先构建主目录的 vendored SDL3）：

```bash
clang++ -std=c++26 -O2 -DNDEBUG -Isubprojects/SDL3/include \
  probe/cross_platform/desktop/sdf_state_probe.cpp \
  -LbuildDir/subprojects/SDL3 -lSDL3_shared \
  '-Wl,-rpath,$ORIGIN/../subprojects/SDL3' -o buildDir/tests/sdf_state_probe_bench
./buildDir/tests/sdf_state_probe_bench --require-gpu --benchmark
```

下表的 p50/p95 分别取五次运行对应分位数的中位数，不把五次样本混成一个分布；单位 ms：

| 模式 | 每帧 state 设置 / uniform 更新 | submit p50 | submit p95 | completed p50 |
| --- | --- | --- | --- | --- |
| native：默认 shader | 1 / 0 | 0.0959 | 0.1072 | 0.4117 |
| fixed：SDF 固定参数 | 2 / 1 | 0.0958 | 0.1087 | 0.4751 |
| update：同一 state 逐次改参数 | 2 / 257 | 0.9282 | 1.2278 | 4.0412 |
| switch：SDF/默认 shader 交错，参数固定 | 514 / 1 | 0.4831 | 0.5140 | 1.4520 |
| interleaved：交错且逐次改参数 | 514 / 257 | 1.1702 | 1.4766 | 4.2991 |

设置次数是 **API 调用次数**，包括首尾设置，不等于驱动实际 pipeline bind 或 GPU draw call 数。
update 每两次几何提交改变一次半径；两种交错模式每组各提交一个 SDF quad 和一个默认纹理 quad。
native/fixed/update 使用同一 shader 连续提交两次。它们用于分析提交策略，并非相同视觉质量的后端竞赛。

五次 submit p50 范围：native 0.0926–0.0967、fixed 0.0957–0.0964、update 0.9240–0.9298、
switch 0.4788–0.4839、interleaved 1.1675–1.1707。没有启用性能阻断。

**下一步约束**：不能把“复用一个可变 state、每图元改 uniform”无条件当作高效批处理。
第二轮实验（§9）已比较逐次上传、不可变 state 缓存与交错模式，并据此定下策略；
生产缓存容量仍待真实页面校准，本记录不因微基准直接扩张公开接口。

### 验证与批次 2 复核

本轮定稿后主套件 **94/94**，ASan/UBSan unit **81/81**，no-RTTI unit **81/81**。
新增 GPU 专项也分别在 sanitizer（系统 SDL3，沿用既有例外）和 no-RTTI（vendored SDL3）构建下通过。
无显示退出 77、源码变动导致产物检查失败均已验证；固定 glslc 再生与 spirv-val、固定
clang-format 23.1.1、suite 门禁、文档相对链接/锚点和 diff 检查通过。

批次 2 复核发现 `physics` CI 在 wrap 迁移后仍无预热就执行 `nodownload`，本轮补上与常规任务
相同的缓存和预热。未声称已远端执行 GitHub Actions，也未重试此前受网络限制的全部首次下载。

## 9. 参数提交策略（2026-10-19 决定）

### 9.1 候选策略的实测

同一探针新增三种模式，仍是 512 次几何提交/帧、预热 300/采样 1200/重复 5 次轮换顺序；
每帧 8 个参数块、每块 64 次提交（runs 形态，模拟列表行/按钮组），另有一个逐图元替换的
交错形态（cached-alternate）。原始数据见
[策略基准记录](../../probe/cross_platform/results/sdf-state-vulkan-2026-10-19.txt)。

| 模式 | 每帧 state 绑定 / uniform 上传 | submit p50 (ms) | submit p95 (ms) |
| --- | --- | --- | --- |
| native：默认 shader | 1 / 0 | 0.0967 | 0.1217 |
| fixed：一次上传后固定 | 2 / 1 | 0.0979 | 0.1206 |
| update：绑定后逐次上传 | 2 / 257 | 0.9408 | 1.2459 |
| switch：SDF 与默认 shader 逐次交替 | 514 / 1 | 0.4871 | 0.7159 |
| interleaved：交替且逐次上传 | 514 / 257 | 1.1951 | 1.5186 |
| **dedup-runs：绑定后仅在上传变化时上传** | 2 / **8** | **0.1185** | 0.1476 |
| **cached-runs：不可变 state，每组绑定一次** | 257 / 0 | **0.1119** | 0.1361 |
| **cached-alternate：不可变 state，逐图元绑定** | 257 / 0 | **0.2395** | 0.3060 |

- 创建并播种一个不可变 state（`SDL_CreateGPURenderState` + 一次 uniform 上传）：
  **0.21 µs**（32 次平均）。
- 绑定不同参数块的 state ≈ 0.6 µs 的边际成本；绑定相同 shader 的 state 很便宜。
- 与默认 shader 的每次切换 ≈ 0.76 µs，并会打断 2D 批次（这就是文字/纹理交错无法避免的代价）。

### 9.2 决定：不可变参数 state + 有界缓存，按"参数块连续段"绑定

理由：

1. **runs 下不付性能代价**：0.1119 vs 0.1185 ms，与"仅在上传变化时上传"等同，远低于逐次上传；
2. **交错块下快约 4×**：0.2395 vs 0.9408 ms，而真实页面同时存在共享参数的连续图元与交替块；
3. **结构性消除"忘记上传"缺陷**：参数块在 state 创建时固定，后续图元无法污染它。最小实验
   的第二条注入（漏一次 uniform 更新）正是这类缺陷，接入后由所有权约束排除，而不是靠测试兜底；
4. **创建成本低**（0.21 µs），缓存未命中可以直接创建，不需要为"避免创建"引入复杂的复用逻辑。

缓存规则（已实现）：

- key = 参数块的**按位**内容（探针 64 字节，当前生产填充/描边/圆为 32 字节；`-0.0` 与 `+0.0` 视为不同，不做归一化，
  以免掩盖上游不一致）；
- 容量有界（初定 256，LRU 淘汰并销毁 state），**跨帧保留**（按钮/列表行参数稳定，命中率高）；
  设备重建或上下文丢失时整体清空；
- 每帧计数 `binds` / `hits` / `misses` 并进入性能报告；
- **不做跨图元重排**：画家顺序与半透明合成不可交换，只允许把"连续且同块"的图元视为一段；
- 文字/纹理/图片绘制仍绑定默认 state，这部分切换由场景顺序决定，不在设备层重排。

### 9.3 为什么不选其它方案

| 方案 | 否决理由 |
| --- | --- |
| 单可变 state + 逐次上传 | 交错块慢 4×，且保留"忘记上传"缺陷类；runs 下并不比缓存快 |
| 顶点属性携带参数 | `SDL_RenderGeometry` 的 `SDL_Vertex` 只有 position/color/uv，`SDL_GPURenderState` 不允许自定义顶点布局；把参数编码进 uv 会与真实纹理采样冲突 |
| 参数纹理/存储缓冲 + 顶点索引 | 同样受顶点布局限制；要绕过 `SDL_RenderGeometry` 自建 GPU 管线，超出 `SDL_GPURenderState` 的契约 |
| 全局按 state 重排批次 | 破坏画家顺序与半透明合成（涟漪叠在按钮上会翻转结果） |

### 9.4 生产接入的验收

1. 每帧 `binds` ≈ 参数块连续段数 + 文字/纹理段数；稳定页面上 `misses` 应趋近 0；
   `SdlRenderDevice::sdf_diagnostics()` 提供 `binds/hits/misses/cached/capacity`；
2. 真实页面（Label / 列表 / 按钮 / 涟漪）按[性能预算](performance_budget.md)协议采样，并与 CPU 几何路径对比；
3. 同一参数块在两条路径上的像素一致（GPU 需要显示器，归入探针或人工验收）；
4. GPU renderer 不可用或 state 创建失败时回落到 CPU 几何，报告 `render_path=cpu` 与原因。

## 10. 生产接入审查（2026-10-19）

当前生产覆盖填充、描边与圆。审查修复非等比缩放回落及帧结束未恢复默认 state、
viewport/逻辑呈现偏移未进入 uniform、uniform 上传失败的 state 泄漏与运行期降级，
以及缓存分配失败时的所有权清理。SDL 3.4.18 销毁 state 时会 flush 引用它的命令队列，
正常 LRU 淘汰不要求额外延迟销毁；代价是超容量流量可能触发 flush，256 的容量仍待页面校准。

生产设备回归增加三档缩放下的非零 viewport、logical presentation、非等比回落、
帧结束后的默认绘制及 300 参数块淘汰。能力查询在非等比缩放下返回 false；
运行期 state 创建/上传失败后关闭本设备的 SDF 路径并保存原因。
当前诊断计数是设备生命周期累计值，逐帧报告须取差值；`binds` 只计非空绑定，
不含恢复默认 state，尚不能代替契约要求的完整切换/上传性能报告。

产物默认门禁新增工具链元数据核对，`--emit` 同样要求固定工具版本；负例覆盖源码、
产物和工具链元数据变化。SPIR-V 仍是唯一生产产物。

本轮验证：主套件 97/97（无跳过）、no-RTTI unit 82/82；ASan/UBSan 首轮并行时
字体计时断言 537.7 ms 超出 500 ms，串行重跑 unit 82/82。三配置生产 GPU 专项通过；
实验两条故障注入均在指定像素断言失败。两个 shader 固定工具再生与 spirv-val、
clang-format 23.1.1、suite/header 门禁通过。无显示默认退出 77、强制 GPU 退出 1。

页面专项发现并修复探针启用 trace 后访问已 move 的 unique_ptr，以及重复开帧丢失
clear 记录的问题。修复后 home/label/anchors 在 GPU renderer 各运行 30 帧并捕获 BMP，
与 raylib 稳态调用名字序列分别为 678/2581/95 次，完全一致。截图未建立或审批 golden，
采样期间有并行构建，本轮计时不作为性能基线。

### 10.1 逃生舱、失败注入与逐帧统计（2026-10-19 补齐）

- **驱动逃生舱**：`NANDINA_SDL_SDF=off|0|false` 在构造时禁用 SDF 并记录
  `disabled by NANDINA_SDL_SDF`；消费者据此退化到 CPU 几何。设备测试验证该路径的能力查询
  为 false、CPU 像素正确。
- **运行期失败注入**（仅测试）：`NANDINA_SDL_SDF_TEST_FAIL_AFTER=<n>` 让前 n 次 state 创建
  成功后注入一次创建失败；设备随后关闭本设备的 SDF 路径、记录原因并让当次图元由 CPU 画出。
  生产路径不设置该变量。
- **逐帧统计**：`SdfDiagnostics::binds` 是生命周期累计值（不含解绑）；`frame_binds` /
  `frame_unbinds` 在 `begin_frame` 清零，切换次数取两者之和。设备测试断言第二帧
  `frame_binds=3`、`frame_unbinds=1`，因此可以据此做契约要求的"每帧状态切换次数"报告。
- **纹理/文字交错**：设备测试在两次 SDF 绘制之间插入 alpha 纹理（与字形图集、图片共用
  `draw_texture_region`），断言纹理像素、掩码为零处、以及之后的 SDF 仍正确。
- **未决（viewport / 逻辑呈现）**：原点公式按 SDL 的 `UpdatePixelViewport()` 推导
  （`floor(viewport*scale + logical_offset)`），但本轮在**离屏 target** 上做"小矩形 + 偏移内/外"
  强断言时，观测到的落点与公式不符（`SDL_GetRenderViewport` 报告 (13,9)，图元却仍在逻辑原点），
  且 `SDL_RenderFillRect`/`SDL_RenderGeometry` 在最小 SDL-only 实验里的偏移与裁剪行为也不一致。
  该强断言已撤回，viewport/逻辑呈现分支**尚未被真实窗口路径独立验证**；生产宿主当前不设置
  viewport 或逻辑呈现，因此日常渲染不受影响。启用 HiDPI/letterbox 前必须用真实窗口重做这个实验，
  再决定是否保留原点项。

### 10.2 viewport 取证修正（2026-10-19）

`tests/sdf_device_test.cpp` 增加 60 组小矩形强断言：GPU 窗口 backbuffer 与离屏 target
各 18 组，software 窗口与 surface 各 12 组；分别走 SDL-only FillRect、Geometry 与生产
SDF（后者仅 GPU），覆盖 1/1.25/1.5 render scale、letterbox 开关及 viewport (13,9)。
本机真实桌面专项实跑全部通过，不再仅用大矩形内部的重叠区域证明偏移。

同帧先保持 viewport 读回，再 flush、禁用 logical presentation、重置 scale/viewport 后
读完整目标。前者图元落在返回 surface 的局部坐标，后者落在绝对偏移位置；同时断言旧原点
与图元外侧仍是背景。原因是固定 SDL 的 `SDL_RenderReadPixels` 从
`renderer->view->pixel_viewport` 开始读回（`SDL_render.c`），返回 surface 的原点不是
目标原点。此前“viewport 已设置而图元仍在逻辑原点”的证据混淆了这两个坐标系。
矩阵使用 60x40 viewport，确保最高有效缩放下读回范围仍在目标内；software renderer
不会像 GPU 路径一样容忍该实验原先越出目标的读回范围。

因此保留生产参数公式 `floor(viewport * effective_scale + logical_offset)`。
实际故障注入去掉 uniform 中的 `+ origin_x/y` 后，新增的 12 组 SDF 偏移内断言全部失败，
例如 (19,15) 得到背景 (32,48,64) 而不是 (255,255,255)，SDL-only 对照仍通过。
恢复实现并重新编译后专项通过。没有修改 SDL、删除失败断言或放宽像素容差。
这轮记录仅表示坐标专项通过，不替代剩余验收或全套验证。

剩余验收：**真实字形图集**（而非通用 alpha 纹理）的交错像素断言、完整 AA 误差界、
真实页面像素/性能/内存基线、GPU 失败注入的更多故障类型、DXIL/MSL。
探针的故障注入与历史页面证据不能替代这些生产验收。

### 10.3 产物格式选择（2026-10-19）

生产 shader 现在嵌入 SPIR-V、DXIL、MSL 三份产物（`tools/shader_artifacts.toml` 的
`cross_formats`，经固定版本 SDL_shadercross 生成并校验容器签名/哈希）。设备按
`SDL_GetGPUDeviceDriver()` 选择：`vulkan → SPIR-V`、`d3d12 → DXIL`、`metal → MSL`；
其它驱动记录原因并回落 CPU。`sdf_shader_format()` 报告实际格式，设备测试在 Vulkan 上断言
`spirv`，逃生舱/注入失败时为空。

边界：**平台必须用同一格式创建 GPU device**，否则 `SDL_CreateGPUShader` 失败（原因进
`sdf_unavailable_reason()`）。Linux 只能实机验证 Vulkan；DXIL/MSL 只做到"生成 + 静态校验 +
按驱动选择的代码路径"，Windows D3D12 与 macOS Metal 的目标设备加载验收仍未做。

## 11. 页面性能 harness（2026-10-19）

`tools/backend_performance.py` 驱动受追踪的页面探针（`probe/cross_platform/desktop/page_probe.cpp`），
固定 1200×800、`dt=1/60`，按协议每配置预热 300 帧、采样 1200 帧、重复 5 次（轮换顺序），
对每次取 p50/p95 再取重复中位数。SDF 与 CPU 几何分别采样：CPU 侧用 `NANDINA_SDL_SDF=off`
走同一设备的 CPU 路径，因此差异只来自绘制路径而不是宿主。

探针新增：逐帧 CSV（total/layout/draw、实际布局 pass 数、`frame_binds`/`frame_unbinds`、
累计 hits/misses、缓存条目数）、`NANDINA_PROBE_STRESS=<n>`（页面之后画 n 个不同参数块的
圆角矩形，用于缓存压力）、`resources:` 汇总（`ru_maxrss` 与设备持有的 RGBA 纹理字节）。
`SdlDeviceStats::texture_bytes` 只统计本设备创建的贴图，不是 GPU 总内存。

结果（RADV RX 6750 GRE、本机 debug 构建、2026-10-19，
原始数据 `probe/cross_platform/results/backend-perf-2026-10-19.json`）：

| page | 模式 | p50 (ms) | p95 (ms) | binds/帧 | misses/帧 | hits/帧 | cache 条目 | max RSS (KB) | 纹理字节 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| home | SDF | 0.602 | 0.626 | 0 | 0 | 0 | 0 | 63960 | 786432 |
| home | CPU | 0.587 | 0.608 | 0 | 0 | 0 | 0 | 63708 | 786432 |
| label | SDF | 2.550 | 2.615 | 21 | 0 | 21 | 42 | 82980 | 2097152 |
| label | CPU | 2.614 | 2.677 | 0 | 0 | 0 | 0 | 82924 | 2097152 |
| anchors | SDF | 0.111 | 0.136 | 7 | 0 | 7 | 14 | 62956 | 524288 |
| anchors | CPU | 0.157 | 0.190 | 0 | 0 | 0 | 0 | 62772 | 524288 |

读法：

- **label**：SDF 比 CPU 快约 2.4%，每帧 21 次绑定、42 个缓存条目、稳定后 **0 miss/帧**
  （全部命中，跨帧保留生效）；p95 只比 p50 高约 2.5%。
- **anchors**：SDF 快约 29%（CPU 扇形细分被解析图元替代），7 次绑定/帧。
- **home**：两模式都在 0.6 ms 量级，差异约 2.6%（在噪声内）；该页 **0 次 SDF 绑定** ——
  它没有走矩形/圆角/圆图元，主要是文字与纹理，因此两种模式几乎一样。
- RSS 与纹理字节两模式一致（同一图集/窗口），说明差异来自绘制路径而不是资源。
- 这不是基线：单机、debug 构建、采样期间可能有并行负载；阈值未启用。用
  `--stress 300` 时 300 个不同参数块超过 256 容量会出现 LRU 抖动（每帧全 miss），这正是
  压力场景要观察的，报告会如实给出。

剩余验收：**页面 golden**（候选图 + manifest + 差异图 + 比较器，维护者批准前不作基线）、
DXIL/MSL 目标设备加载（Windows D3D12 / macOS Metal）、直线 AA 的独立面积积分、
交互/压力场景的正式阈值。

保留 §10.2 已有的 viewport 取证，重新运行强断言；实际字形测试先走冷图集的 SDF 交错帧，
再用已上传的图集绘制参考帧，并明确验证上传和扩容，避免先画参考把待测上传路径预热掉。
AA 同时使用直线解析预期与独立的 64×64 面积积分，按边缘像素报告误差，不用整图平均稀释。

页面验收新增受追踪 harness：固定 1200×800、dt=1/60，home/label/anchors 稳态、按钮 press/release
与 Dialog 周期交互、Label 加 300 个不同参数块的压力场景。CPU 几何与 SDF 分别采样；预热 300 帧、
采样 1200 帧、重复 5 次，记录每次 p50/p95、实际布局次数、state 绑定/解绑、cache hit/miss、
进程 RSS 和设备持有的 RGBA 纹理字节。RSS 不是 GPU 总内存；参数缓存 payload 也不等于驱动分配。

页面 golden 先交付候选图、采样 manifest、差异图与比较器；维护者批准之前不作为已接受基线。
DXIL/MSL 本轮尝试生成并接入按设备格式选择；生成/静态检查与 Windows D3D12、macOS Metal
目标设备验收分开，当前 Linux 不能声称后二者实机通过。
