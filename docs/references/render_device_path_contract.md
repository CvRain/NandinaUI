# 渲染设备路径契约（SDF 生产路径的前置）

> **状态：planned（2026-10-18 起草，未实施）。** 批次 3 会用 `SDL_GPURenderState` 打通生产 SDF。
> 在动代码前先把"两条渲染路径的边界"冻在这里，避免边写边改接口。本文只写契约与验收，
> 具体数值（uniform 上限、状态切换基线）由批次 3 的最小实验实测后回填。

## 1. 两条路径

| 路径 | 何时使用 | 质量 | 状态 |
| --- | --- | --- | --- |
| **SDF**（`SDL_GPURenderState` 自定义 fragment shader） | 宿主拿到 SDL **GPU renderer** 且能创建 render state | 解析抗锯齿，与 raylib 同级 | 待实施 |
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

- **uniform 传输**：`SDL_SetGPURenderStateFragmentUniforms(state, slot, data, length)` 的数据经
  `SDL_PushGPUFragmentUniformData` 推送，按 **16 字节对齐的 slot** 组织；上限由目标驱动决定。
  参数集必须装进预算，装不下要移到顶点属性或纹理——这是**布局决策**，必须在接通图元前定形。
- **顶点格式**：走 `SDL_Vertex`（position + color + uv），CPU 几何路径已经在用；SDF 路径复用同一批
  顶点，只是把"形状参数"从几何细分挪到 uniform。
- **AA 外扩**：SDF 的解析 AA 需要把几何向外扩约 1 像素，否则边缘的抗锯齿会被几何本身裁掉。
- **坐标空间**：uniform 里的位置/半径必须与 fragment 坐标同一空间（设备像素），因此需要
  `logical_to_screen` 的比例作为参数或在提交前换算；分数缩放（1.25/1.5）必须测。
- **状态设置与恢复**：`SDL_SetGPURenderState(renderer, state)` 之后必须
  `SDL_SetGPURenderState(renderer, nullptr)` 才能绘制文字/纹理；漏恢复会让后续内容错乱。
- **批次 flush**：每次设置/恢复都可能 flush 2D 批次。Label 页每帧上千次纹理绘制、几何与文字交错，
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
