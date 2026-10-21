# 性能预算：采样协议与候选阈值

> **状态：采样协议、release 基线与确定性门禁已实施（2026-10-20）。**
> 确定性门禁（布局 0、稳态 miss 0、binds 上限）总是生效；计时阈值按本文候选规则执行，
> 只有 `--enforce` **且**基线 `buildtype=release` 时才阻断。CI 无 GPU，页面探针进不了 CI，
> 这是发布检查/本地门禁。本文只谈**测量与判定**；具体渲染质量的取舍见
> [D7](reference_draft.md#d7-渲染质量与后端视觉一致性)。

## 1. 为什么现有数字不能直接当基线

| 问题 | 具体表现 |
| --- | --- |
| 采样条件不同 | 文档里 SDL `2.0–2.2 ms` 与 raylib `1.85 ms` 来自不同记录；更早的探测还有相反的关系 |
| 计时边界不同 | raylib 的 `EndDrawing()` 在**设备**侧结束，SDL 的 present 在**平台**侧 —— 两边的 "draw" 包含的阶段不同，不是同一种成本 |
| CPU ≠ GPU | 异步 GPU 下，CPU 提交时间不等于 GPU 执行时间；帧耗时还包含 present/等待 |

因此：**不与另一后端比，只与同一路径的上一版基线比。**

## 2. 采样协议（先冻结，再谈阈值）

- 同一机器、同一构建模式、同一页面、同一字体、同一视口与缩放、同一驱动、同一渲染路径；
- **关闭 trace、截图与其他取证插桩**（录制装饰器会按调用次数分配字符串，成本不可忽略）；
- 明确计时边界，并分别记录：CPU 场景提交、present/等待、完整帧耗时；
- 预热后再采样：建议预热 300 帧、采样 1200 帧，每个版本重复 5 次；
- 保存每次运行的 **p50 / p95** 与运行间波动；不同渲染路径**各自**建立基线。

## 3. 候选阈值（相对同路径同环境基线）

以约 2 ms 的 Label 稳态 CPU draw 为量级参考：

| 等级 | 判定 | 例：基线 p50 = 2.1 ms |
| --- | --- | --- |
| 预警 | p50 增长 > `max(10% × 基线, 0.20 ms)` | ≈ 2.31 ms |
| 阻断候选 | p50 增长 > `max(20% × 基线, 0.40 ms)` | ≈ 2.52 ms |
| 尾部阻断候选 | p95 增长 > `max(25% × 基线 p95, 0.75 ms)` | 按实测 p95 计算 |

- 名为"候选"，因为**首次超限只触发一次受控复测**；复测确认才阻断。不允许无限重跑直到出现一次通过。
- 若环境波动本身已经接近这些阈值，先改善测量环境，结论保持"报告模式"。

## 4. 与毫秒阈值无关的确定性门禁

这些比毫秒更容易稳定验证，应独立成立：

| 门禁 | 判据 |
| --- | --- |
| 稳态布局次数为零 | 静止帧不得触发布局（已有明确行为依据） |
| SDF 每帧状态切换次数 | 批次 3 实现后建立基线，并解释新增切换的来源 |
| 固定交互序列的 p95/p99 | 用固定输入序列（按下→移动→释放）测，不能用静止 Label 页代表涟漪/弹窗/导航成本 |
| 页面打开与完整重排 | 分别记录，不合并成"性能通过" |
| CPU 内存与 GPU 资源估算 | 分别记录；GPU 纹理字节不一定在 RSS 里 |

## 5. 基线更新政策

- 新路径（例如生产 SDF）可以经审核**建立自己的基线**；不能拿无 AA 的 CPU 几何成本要求高质量路径
  在所有指标上零增长。
- 基线更新必须附带：质量收益、成本差异、测量证据；**不得直接用最新结果覆盖旧值**。
- 纯重构/无行为变化的改动原则上不更新基线；确需更新时说明为什么旧基线不再可比。

## 6. 与其它文档的关系

- 像素预期的组织与审批见[视觉回归](visual_regression.md)；
- 历史测量记录在[整形成本](text_shaping_cost.md)与[布局收敛](layout_convergence_and_text_measurement.md)，
  它们是**记录**，不是当前门禁基线；
- 依赖迁移后的构建时间不在本文范围（属 [依赖策略 §6](dependency_policy.md) 的离线/SDK 交付）。

## 7. 已实施的门禁（2026-10-20）

- 检查器：`tools/check_backend_budget.py --report <fresh.json> --baseline <baseline.json> [--enforce]`；
- 基线：`probe/cross_platform/results/backend-baseline-release-2026-10-20.json`（`buildtype=release`、
  `enforce_timing=true`、8 个配置）；debug 基线保留为对照，**不允许**启用计时阻断；
- 采样工具：`tools/backend_performance.py`（可指定 `--probe`、`--interaction none|once|cycle|dialog`）；
- 确定性门禁：稳态每帧布局 pass 必须为 0；使用 SDF 的页面预热后 miss/帧必须为 0；
  binds/帧 ≤ 基线 × 1.25 + 2；
- 计时阈值：p50 超过 `max(base×1.10, base+0.20ms)` 警告、`max(base×1.20, base+0.40ms)` 阻断；
  p95 超过 `max(base×1.25, base+0.75ms)` 阻断。绝对项是给亚毫秒页面的噪声余量，因此 +30% 的
  回归在 0.45 ms 页面上不会阻断，+100% 会（实测 0.901 ms > 0.851 ms 退出 1）；
- release 实测（RADV RX 6750 GRE，1200×800）：label SDF 0.451 ms（CPU 0.481，−6.2%）、
  anchors SDF 0.019 ms（CPU 0.027，−30%）、home 0.109 ms；交互换页 p50 0.465 / p95 4.814 ms，
  Dialog 开关 0.493 / 0.651 ms。debug 构建的 label SDF 为 2.505 ms（约 5.5×），不能当基线；
- Windows 尚无性能基线（实验性平台，非当前门禁）。

## 9. Windows release 参考基线（2026-10-22）

采集环境：Windows 桌面机、`buildtype=release`、协议与 Linux 相同（300 预热 / 1200 采样 /
5 次重复轮换、1200×800 隐藏窗口）。两条路径分开采集：

- **CPU 路径**：SSH 会话（session 0 无 GPU swapchain），`NANDINA_PROBE_RENDERER=sdl2d`；
- **SDF 路径**：交互式桌面会话，SDL GPU renderer + DXIL 产物；报告的能力行确认
  `analytic_aa=1 rounded_clip=1 soft_shadows=1`（P1.5b 已在 Windows release 生效）。

报告与基线（`probe/cross_platform/results/`）：

- `backend-report-windows-release-none-2026-10-22.json` / `...-cycle-...`（CPU 原始报告）
- `backend-report-windows-release-sdf-none-2026-10-22.json` / `...-sdf-cycle-...`（SDF 原始报告）
- `backend-baseline-windows-release-2026-10-22.json`（整理后的参考基线：8 条，
  `approved=false`、`enforce_timing=false`）

| 页面 | 路径 | interaction | p50 (ms) | p95 (ms) | binds/帧 |
| --- | --- | --- | --- | --- | --- |
| home | sdf | none | 0.157 | 0.184 | 0 |
| label | sdf | none | 0.637 | 0.683 | 21 |
| anchors | sdf | none | 0.036 | 0.050 | 7 |
| home | cpu | none | 0.177 | 0.214 | 0 |
| label | cpu | none | 0.752 | 0.830 | 0 |
| anchors | cpu | none | 0.051 | 0.079 | 0 |
| label | sdf | cycle | 0.634 | 8.110 | 23.37 |
| label | cpu | cycle | 0.723 | 8.993 | 0 |

观察（仅 Windows 自身对照）：稳态下 SDF 路径在 label 上比 CPU 快约 15%（0.637 vs 0.752），
anchors 上 0.036 vs 0.051；home 两路径都约 0.16 ms（该页无 SDF 图元，binds=0）。交互 cycle 的
p95 尖峰（8.1 / 9.0 ms）来自换页，与 Linux 记录同量级，不作为质量结论。

**边界**：这些数字**只作 Windows 自身参考**，不跨平台比较、不参与计时门禁
（`approved=false`、`enforce_timing=false`）；确定性门禁（稳态布局 pass=0、miss=0、binds 上限）
可按需用该基线检查。CPU 路径运行在无 GPU 的 SSH 会话（sdl2d），SDF 路径运行在桌面会话，
两者环境不同，不能互相推断。

## 8. 与发布检查的关系

无 GPU 的 CI 只跑确定性部分之外的门禁（format/header/artifact/golden 清单）；完整预算检查在发布
检查或本地运行。把各门禁串成一条命令属于 P1.1，见[开发路线](development_roadmap.md)。
