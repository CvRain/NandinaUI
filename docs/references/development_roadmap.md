# 开发路线（2026-10-20 重估）

> 本文是**下一阶段 P1–P4 的排序与退出条件**。已完成批次、设计议题与历史证据仍以
> [项目进度与下一步](project_status.md) 为准；组件之间的依赖顺序见
> [组件开发路线图](component_roadmap.md)；性能和视觉的门禁细节见
> [性能预算](performance_budget.md) 与 [视觉回归](visual_regression.md)。
> 本文不承诺日期，也不把"计划"写成"已实现"。

## 1. 当前基线（截至 2026-10-20，已验收）

| 方向 | 结论 | 证据入口 |
| --- | --- | --- |
| 平台 | **Linux 正式支持**（Wayland/X11，主套件 98/98）；**Windows 实验性**（release 构建通过、`--suite unit` 80/80、`nandina doctor` 全绿、showcase 可运行）；**macOS 不支持**（无硬件，不承诺） | README“在 Windows 上构建”、路径契约 §14 |
| 渲染 | SDL3 为默认后端；生产 SDF 已接入**填充/描边/圆**，Vulkan(SPIR-V) 与 Windows D3D12(DXIL) 均通过设备级像素验收，直线 AA 数值逐位一致；其余图元仍走 CPU 几何 | 路径契约 §10、§14 |
| 视觉回归 | 6 条页面 golden 已批准（Linux，平台隔离）；比较器与审批工具就绪；Windows 基线未建立 | [视觉回归](visual_regression.md) |
| 性能 | release 基线 8 配置 + 确定性门禁（布局 0、稳态 miss 0、binds 上限）+ 候选计时阈值（`--enforce` 启用） | [性能预算](performance_budget.md) |
| 依赖与分发 | 11 个 wrap + 字体 submodule + SQLite amalgamation；Linux SDK 消费 fixture；头文件门禁 184 个（11 internal） | [依赖策略](dependency_policy.md)、[SDK 表面](sdk_surface.md) |
| 工程门禁 | format / suite / header / shader artifact / golden manifest / backend budget；CI 无 GPU | README、路径契约 §13 |

## 2. 推进原则

1. **先定行为与边界，再写实现**：需要用户判断的未定项在实施前收敛，不把实验性约定写进公开承诺。
2. **测试必须证明会红**：每批交付要说明"改坏哪里、什么会红"；没有回归证据的修复不算完成。
3. **平台结论分开记录**：Linux 的验收不能替代 Windows/DXIL；Windows 的验收不能替代 MSL；每个平台各自有证据。
4. **不静默降级**：能力缺失要写进 `sdf_unavailable_reason()`、capability 查询或文档，而不是悄悄换路径。
5. **CI 与发布门禁分工**：无 GPU 的检查（format、header、artifact、golden 清单、预算确定性门禁）进 CI；
   需要 GPU/窗口的检查（页面 golden、性能采样、设备级像素）是发布检查或本地门禁。

## 3. 阶段划分

### P1 收口与平台补齐（当前阶段）

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P1.1 | **发布检查脚本**：把 format、header、artifact、golden 清单、budget 确定性门禁串成一条 `tools/release_check.*`（本地/发布用，CI 可复用无 GPU 部分） | 各门禁已就绪 | 一条命令给出逐项结论；失败即非零退出；文档写清哪些项需要 GPU/交互桌面 |
| P1.2 | **Windows 页面 golden 候选**：`capture_backend_golden.py --platform windows` 采集 → 人工审阅 → 批准（不替换 Linux 基线） | P1 前 Windows 构建可跑（已满足） | manifest 出现 platform=windows 的 approved 条目；比较器按当前平台选择；跨平台策略文档同步 |
| P1.3 | **Windows 性能基线（可选）**：同协议在 Windows release 构建采样，作为对照而非跨平台比较 | P1.2 同前置 | 报告归档；不要求与 Linux 可比 |
| P1.4 | **CLI 端到端**：`nandina new` → `nandina build` → `nandina run` 在 Linux 与 Windows 各走一遍真实项目（验证 CreateProcessW、`.exe`、路径转义） | 无 | 两个平台各有一份完整输出；失败路径（命令不存在、构建失败）有明确退出码 |
| P1.5 | **SDF 剩余图元**：arc / 线段 / 阴影；先做 D7 批次 6 的涟漪专用圆角交集，再按用例定形通用裁剪 | 现有 SDF 参数策略与产物链 | 设备级像素验收覆盖新图元；参数 state 不重复上传；能力缺失有 reason |
| P1.6 | **图片路径**：共享 CPU 解码器、SDL RGBA 上传、删除 `IPlatformWindow::make_image_decoder()`（破坏性，按 SDK 表面记录） | 解码库来源定形 | 两后端真实 Image 消费者可用；PNG/JPEG/透明/失败输入测试；删除后无遗留 override |

### P2 应用闭环（资源与工具）

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P2.1 | **D5 批次 1 解耦**：资源工具脱离完整 UI；默认工具开/后端开/vendored；新增独立关闭开关 | 无 | 无 UI 依赖环境可构建工具；关闭组合真实运行；依赖闭包门禁 |
| P2.2 | **D5 作用域**：管理器八入口补作用域、后端发布代次、缓存失效 | P2.1 | 跨挂载/manager 冲突、ID 歧义、别名、并发发布、过期引用、缓存回收 |
| P2.3 | **D5 身份**：key 映射、生成算法、作者引用 | P2.2 | 显式 key 跨重命名、重复内容独立记录、碰撞与重建行为 |
| P2.4 | **真实示例应用**：导航、表单、图片、滚动、弹窗贯穿；fixture 与脚本入库 | 组件与资源接口稳定 | 干净检出可构建运行；showcase 保留组件验收职责 |

### P3 输入与焦点

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P3.1 | **D8 输入基础**：先修“宿主用帧级字段覆盖每事件坐标/修饰键”，再定形物理/逻辑键、事件快照、release/repeat/unknown、保序分发 | 无 | 同帧“在 A 按下、移到 B”派发到 A；事件时修饰键；US/非 US 布局；跨后端规范事件对照 |
| P3.2 | **D1 第一阶段**：统一焦点提示策略（paint-only 失效、程序化恢复、窗口失活/激活） | P3.1 事件语义 | 同节点模态变化、两种 roving 模型、文本能力矩阵 |
| P3.3 | **D1 文本会话**：注入式 `ITextInputService`、IME/软键盘、会话生命周期 | P3.1 | 真实 IME 分别记录平台状态；不另造第二套 host |

### P4 作者与组件

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P4.1 | **D6 首批**：Label 作者能力、绑定覆盖、有限宽度自然长高与缓存契约 | 详细 API 审阅 | Label 页与 ComponentInfoTable 验收；纯绘制变动不无谓整形 |
| P4.2 | **阶段 5 数据组件**：布局/选择/虚拟化依据 | P4.1 与布局稳定 | 复合数据组件有虚拟化与选择回归 |
| P4.3 | **D2 主题状态订阅**、**D3 nav_group** | D2 通知与生命周期定形；D3 无跨专题前置 | Shell 无重复事实来源；空组/合并/顺序/typed navigation 回归 |
| P4.4 | **视觉质量扩展**：页面 golden 覆盖面（更多页面/组件状态）、混合叠层验证 | P1.1 | 新页面有 approved 基线或明确不纳入的理由 |

## 4. 建议的近期切入顺序

1. **P1.1 发布检查脚本**（一天内可完成，立刻把已有门禁变成可重复流程，风险最低）。
2. **P1.4 CLI 端到端**（验证 Windows 实验性路径的真实可用性，产出可复制的证据）。
3. **P1.5 SDF arc/线段**（继续补渲染主线，依赖最少）。
4. **P1.2 Windows golden 候选**（有机器时再做；不急，不阻塞主线）。
5. P2/P3/P4 按应用需求插入；D8 与 D1 是交互体验的下一道大门，值得在组件扩张前完成。

## 5. 非目标（明确不做，或只保留探测）

- **macOS**：没有硬件与测试环境，不提供构建/运行承诺；MSL 产物仅静态校验。
- **Web**：探测保留，不作为应用交付路径。
- **C++ Modules / CMake package**：暂缓，条件见[构建系统与 Modules 范围](build_system_scope.md)。
- **OS 包 / 安装器 / 后端拆库**：长期项，当前没有应用安装路径。
- **跨平台像素基线共用**：不比较不同平台像素，只比较结构/调用序列。

## 6. 风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| CI 无 GPU，渲染回归只能靠发布检查 | 回归发现晚 | P1.1 把确定性门禁自动化；发布检查清单化 |
| Windows 无持续构建环境 | 实验性路径可能再次腐化 | 每次改动涉及跨平台代码时本地跑一次 Windows 构建 + unit；DXIL 验收记录在案 |
| golden 平台隔离带来维护成本 | 新平台基线缺失 | 按需建立（P1.2），不追求所有平台同步 |
| 文档数字随代码漂移 | 结论过期 | 门禁输出为准；本目录更新时同步索引与状态行 |
