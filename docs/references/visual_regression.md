# 视觉回归：golden、容差与更新流程

> **状态：候选层与 6 条 approved 基线已交付（2026-10-20，仅限 Linux 单机；debug 与 release 渲染逐像素一致）。** 设备/页面 golden 的候选图、
> manifest、比较器与审批工具在 [路径契约](render_device_path_contract.md#12-页面-golden-候选层2026-10-19) §12；
> 谁能改 golden"。第一批落地对象是 [D7](reference_draft.md#d7-渲染质量与后端视觉一致性) 的
> 混合修复：按钮按下、涟漪中间态、Dialog 遮罩。

## 1. 为什么不能"每个后端存一张自己画出来的图"

那会把后端现存缺陷固化成预期：SDL 此前缺几何混合，如果 golden 直接取当时输出，"白底白字"就变成
"通过"。所以预期分两层：

| 验证层 | 预期来源 | 用途 | 典型对象 |
| --- | --- | --- | --- |
| 基础正确性 | **数学预期或独立参考值**（不取自任何后端） | 判断结果**是否对** | 半透明图元在已知背景上的合成色、平坦内部填充、裁剪边界 |
| 页面与固定交互帧 | **审核过的 golden** | 判断视觉**是否出现非预期变化** | 整页、按下态、涟漪中间态、Dialog 遮罩 |

第一层优先、且必须存在；没有第一层，"golden 通过"只能证明"和上次一样"。

## 2. 目录与清单

目标组织（不要求第一批就建全，先建实际用到的）：

```text
tests/golden/backend/
    manifest.toml        # 每个场景的采样条件、比较规则与特殊容差原因
    shared/              # 两条渲染路径都适用的场景
    sdl-basic/           # SDL CPU 几何路径
    sdl-sdf/             # SDL 生产 SDF 路径（批次 3 之后）
    raylib/              # 对照路径，仅用于已接受的可解释差异
```

- 放在 `tests/` 是因为 golden 是**可执行测试的预期结果**；`docs/images/` 是展示与历史证据，
  `playground/` 不承担正式回归依赖。
- **共享优先**；只有两条路径存在已接受、可解释的差异时才按路径分存。
- **不按 GPU 型号新增 golden**——那掩盖可移植性问题；驱动差异用容差与允许超限像素比例表达。

## 3. manifest 记录什么

比较器**只执行规则**，规则写在同目录的 `manifest.toml`：

| 类别 | 字段（示意） |
| --- | --- |
| 采样条件 | 场景名、视口尺寸、缩放、主题/外观、固定字体、动画采样时刻（帧号 + 固定 dt） |
| 渲染路径 | 后端、设备实现（CPU 几何 / SDF）、必要的生成环境信息 |
| 比较规则 | 比较区域（可多个矩形）、逐通道误差上限、允许超限像素比例 |
| 特殊容差 | 为什么这条场景比默认宽（例如字体栅格化、AA 边界），必须写原因 |

**禁止只看全图平均误差**：Dialog 遮罩、按钮边缘这类局部错误很容易被大面积背景稀释。混合测试的
平坦内部区域用独立预期 + 很小的舍入容差；字体与 AA 边界另设规则，不能共用一组数字。

具体数值在正常样本上定形，然后用**故障注入**验证它确实会失败（例如删掉混合设置那行、把状态层
alpha 改成 1）。

## 4. 更新流程

| 步骤 | 谁 | 规则 |
| --- | --- | --- |
| 比较 | CI / 测试 | **只比较，绝不自动接受新图**；失败时产出差异图与指标 |
| 再生 | 开发者或 AI | 执行显式再生命令，产出**候选图 + 差异图 + 指标**，不直接覆盖 |
| 批准 | 维护者 | 明确认可视觉变化后才替换受版本控制的 golden |
| 改容差 | 维护者 | 与改 golden 同等审批：写清原因，不能作为"让 CI 变绿"的手段 |
| 纯基础设施改动 | 维护者 | 原则上**不改** golden；确需改时必须解释预期视觉为何变化 |

当前由作者主导视觉体验，"作者审核候选结果、工具负责再生与比较"已经足够，不需要先做权限系统。

## 5. 第一批（D7 混合修复）：已落地

**测试**：`tests/sdl_visual_tests.cpp`，4 个用例 / 150 条断言（2026-10-17 覆盖补强后实跑），注册为 `unit` 套件里的
`sdl-visual` 目标（C++ 测试按仓库既有约定归 `unit`；`integration` 保留给 Python 与构建工作流）。

**为什么能确定性**：用 `SDL_CreateSoftwareRenderer(surface)` —— 不需要窗口、显示器或 GPU 驱动，
像素只由绘制调用与合成决定；期望值是**解析合成色**（8 位逐通道 `src*a + dst*(1-a)`，容差 ±2），
不取任何后端当前输出，因此没有 golden、也不受驱动差异影响。

| 用例 | 断言 |
| --- | --- |
| 半透明圆角矩形压在已知背景上 | 中心、四角扇形内部及矩形/扇形拼接线两侧 == `over(bg, color@128)`；圆角外部仍等于背景；且中心不同于"原色不透明"；避开圆弧栅格边界 |
| 按下态 filled 按钮 | 中心 == `over(over(bg, fill), pressed_state_layer)`；前提断言状态层 alpha ∈ (0,255)；且不等于"状态层不透明" |
| 涟漪中间态（固定 dt 推进到 progress≈0.5） | 中心 == `over(over(over(bg, fill), state), ripple@alpha*(1-p))` |
| 对话框遮罩 | 实际 OverlayHost + Dialog 打开、布局、固定 8×0.1s 推进状态机和动画、树绘制；确认 (10,10) 在实际面板外，打开后 == `over(bg, scrim)`，打开前和关闭后 == 背景；前提断言 scrim alpha ∈ (0,255) 及预期确实不同于背景 |

**历史故障注入**：删掉设备构造里的 `SDL_SetRenderDrawBlendMode(BLEND)` → 原 4 个用例全部变红；
还原后全绿。原 Dialog 用例仅直接绘制配方色，不证明实际 Dialog 消费链路。

**本次覆盖补强的故障注入**：在 `Dialog::scrim_style` 将 fill alpha 设为 0（模拟漏画），
实际 Dialog 用例红通道得到 32 而预期 16；设为 1（丢失半透明）得到 0 而预期 16，均失败。
`SdlRenderDevice::draw_fan` 提前返回时，圆角内部 (46,36) 得到 32 而预期 116，失败，
此前中心断言仍通过。所有注入均已恢复源码，重新编译后 4 用例 / 150 断言通过。
定稿后的主全量、ASan/UBSan unit、no-RTTI unit 验证结果以本轮交付报告和各构建目录日志为准。

**历史 GPU 路径的同一修复证据**（探针，真实窗口 + GPU renderer；本次未重跑，软件测试结果不覆盖或重新确认这些历史 GPU 数字）：

| 场景 | 修复前 | 修复后 |
| --- | --- | --- |
| Dialog 遮罩下的整页亮度（mean） | 5.9（近黑，看不到背后内容） | 125.0（可透视） |
| 侧边栏选中项最暗行均值 / 行带标准差 | 156.9 / 32.1（白块，文字不可见） | 74.7 / 88.0（深色填充 + 白字） |

![遮罩修复前后](../images/probe/blend-fix-scrim.png)

![选中项修复前后](../images/probe/blend-fix-nav.png)

**仍未做的部分**：页面级"审核过的 golden"这一层。当前基础正确性层已经把"对错"钉住，页面级 golden
要等 §3 的 manifest 与 §4 的审批流程真正建起来；在那之前不提交 golden，避免把当前输出固化成预期。

**harness 规则已同步到探针**：`page_probe` 的读回移到了 `SDL_RenderPresent` 之前（超过 300 行前的
实践是 present 之后再读，曾经读到空帧）。

## 6. 与其它文档的关系

- 采样协议与性能阈值见[性能预算](performance_budget.md)（测量口径不同，不要混用同一份数字）；
- 后端能力差异与降级路径见[平台与后端契约](platform_backend_contract.md) §3；
- 逐图元 trace 只证明调用序列一致，**不证明像素正确**；trace 与像素各自证明自己的边界。

## 7. 跨平台 golden 策略（2026-10-20）

**基线按平台隔离**：manifest 每条 entry 带 `platform`（`linux` / `windows` / `macos`），键为
`(page, backend, platform)`；比较器默认选择**当前平台**的条目，`--platform` 仅用于显式分析。
`golden-manifest` 校验字段合法、无重复、approved 必须有审阅元数据。

**不跨平台比较像素**。字体栅格化（hinting/抗锯齿）、AA、DPI、像素格式与合成顺序都可能不同；
Linux 图不能当 Windows/macOS 的预期。跨平台只比较**结构性**结果（页面结构、元素存在、
逐图元 trace 调用序列），像素基线各平台独立维护，容差也各自设定（Linux 当前为逐通道 ±3、
变更像素比例 ≤0.1%）。

**当前状态**：

| 平台 | 状态 | 说明 |
| --- | --- | --- |
| Linux | **6 条 approved**（2026-10-20） | 1200×800、30 帧、隐藏窗口、单机；debug 与 release 采集逐像素一致，候选图 380 KB |
| Windows | 未开始（低优先级） | 构建说明见 README“在 Windows 上构建”；有可用机器时按下面的步骤落地 |
| macOS | **不支持** | 没有硬件与测试环境，不提供构建/运行承诺；不复制 Linux 基线充数 |

**新平台落地步骤**（Windows 可用时）：

1. 按 README 构建 `nandina_sdl_page_probe`；
2. `python3 tools/capture_backend_golden.py --platform windows --pages home,label,anchors --modes sdf,cpu`；
3. `python3 tools/compare_backend_golden.py --platform windows --page label --backend sdf`
   （首轮与自己的候选比较应为 0 差异，确认采集确定性）；
4. 人工审阅候选（可另存缩略图/差异图）后
   `python3 tools/approve_backend_golden.py --entries all --reviewer <name> --note ...`；
5. 提交候选图与 manifest；**不要**复制 Linux 图或修改 `platform` 来“复用”基线。

**CI**：无 GPU/显示，像素比较不进 CI；`golden-manifest`（含 `platform` 校验）仍在 CI。
设备级测试（坐标矩阵、真实字形、AA 面积积分、产物格式）是跨平台契约；页面 golden 是
平台相关观感基线——两者分开维护，互不替代。
