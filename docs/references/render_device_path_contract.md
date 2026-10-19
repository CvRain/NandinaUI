# 渲染设备路径契约（SDF 生产路径的前置）

> **状态：批次 3 最小实验已落地；生产 SDF 未实施。** 批次 3 会用 `SDL_GPURenderState` 打通生产 SDF。
> 在动代码前先把"两条渲染路径的边界"冻在这里，避免边写边改接口。本文写契约、验收与实测记录；
> uniform 预算见 §3，微基准与失败注入见 §8。生产接入仍按 §6 验收。

## 1. 两条路径

| 路径 | 何时使用 | 质量 | 状态 |
| --- | --- | --- | --- |
| **SDF**（`SDL_GPURenderState` 自定义 fragment shader） | 宿主拿到 SDL **GPU renderer** 且能创建 render state | 解析抗锯齿，与 raylib 同级 | **Vulkan 最小实验已通过（§8）；生产未接入** |
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
| `supports_analytic_aa()` | 当前路径是否提供解析抗锯齿 | false（CPU 几何） |
| `supports_rounded_clip()` | 是否按圆角轮廓裁剪 | false（只到矩形） |
| `supports_soft_shadows()` | 是否提供软阴影 | false |
| `render_path()` | `sdf` / `cpu` + 原因 | `cpu`（仅 CPU 几何实现） |

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
  （[依赖策略](dependency_policy.md) §7）。普通消费者不需要安装 shadercross。
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
运行不依赖源码或构建树中的着色器文件。`sdf-probe-artifacts` 检查源码/产物一致性，维护命令
`python3 tools/sdf_probe_shader.py --verify-compiler` 进一步验证固定编译器可复现。

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

缓存规则（生产接入时实现）：

- key = 64 字节参数块的**按位**内容（hash + `memcmp`；`-0.0` 与 `+0.0` 视为不同，不做归一化，
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
2. 真实页面（Label / 列表 / 按钮 / 涟漪）按[性能预算](performance_budget.md)协议采样，并与 CPU 几何路径对比；
3. 同一参数块在两条路径上的像素一致（GPU 需要显示器，归入探针或人工验收）；
4. GPU renderer 不可用或 state 创建失败时回落到 CPU 几何，报告 `render_path=cpu` 与原因。
