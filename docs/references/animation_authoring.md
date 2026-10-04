# 同一节点的组合动画作者入口

> 本文先确定本轮 API 与归属层，再推进实现。范围是 L2 节点路径、Tween 与扁平组合；
> 跨节点组、嵌套时间线、关键帧与组内 Spring 不在本轮。

## 1. 声明与触发分开

`widget::authoring::to(path, target, motion::TweenSpec)` 保存一个属性的目标与规格。
`parallel(steps...)`、`sequential(steps...)`、`stagger(interval, steps...)` 组合这些描述。
`NodeBuilder::group(spec)` 返回可复制的 `NanAnimation` 句柄；它不改属性、不安装轨道，
也不在构建语句中隐式播放。声明可以发生在挂树之前。

`NanAnimation::play()` 在挂树后显式触发，返回是否接受此次播放。节点已销毁或未挂树时
返回 `false`，不排队；重新挂树后，同一句柄使用节点当前的 SceneTree。成功调用返回
`true`，包括目标已达到、零时长与 reduced-motion 下立即完成的情况。

句柄只保存弱节点引用与纯规格。事件回调按值捕获句柄，不强持有节点，也不保存 endpoint
借用。每次播放才锁定节点、取得当前稳定的 endpoint、创建新的 clips 并提交给当前 Host。
不要在节点自己的回调里捕获 `NodeBuilder`：builder 本身强持有节点。

## 2. 初始能力与非法声明

本轮只开放 `visual::opacity`、`visual::translate`、`visual::scale`；组件也可使用这些路径。
这些 endpoint 与节点同寿命，不存在组件配方字段清除后借用失效的问题。

- 不支持的路径由编译期约束拒绝；组至少有一个步骤。
- 同一组重复属性在声明时抛 `std::invalid_argument`，三种组合规则一致。
- stagger 间隔必须有限且非负。
- 目标检查与普通 setter 一致：opacity 有限并钳制到 `[0, 1]`；scale 必须有限；
  translate 使用现有 NanPoint 的值契约。
- 非法声明没有属性、轨道或场景副作用。

## 3. 运行时与值的归属

作者规格与弱句柄归 `widget`。值插值仍归 `foundation/motion`，endpoint、clip、group 与
调度仍归 `scene`。scene 不引用 widget，foundation 不认识场景树或属性路径。

Host 在取消已有轨道前核验所有 clip：owner 必须与 group owner 相同，identity 不能重复，
不能提交缺少必要回调的 clip。拒绝非法组不能损伤已经在途的动画。

clip 在**开始时**采用声明的 Tween 行为，它同时更新 PropertyEndpoint 配置与实际
AnimatedProperty。此行为成为该属性后续普通 setter 的行为；组完成后不恢复先前策略。
声明和创建 clip 均不提前改变策略。延迟 clip 在整组完成时也会开始并采用自己的策略。
从 Spring 切换为 Tween 时先读取实际当前表现值，保持切换瞬间连续。

因此稳定 endpoint 工厂的 clip 可以借用 endpoint，以在开始时同步配置；作者句柄不保存
这种借用。低层的裸 AnimatedProperty clip 保留为独立场景接口，调用方负责属性寿命。

## 4. 仲裁、完成与重复播放

- 普通轨道被组接管时保留当前表现值，组从那里开始。
- 任一属性与旧组重叠时，旧组先**整体完成**，包括尚未开始的步骤，再安装新组。
- 普通 setter 或行为修改命中组中任一属性，同样先完成整个组。行为修改随后安装新策略，
  不能被旧组的延迟步骤覆盖。
- 属性集合互不重叠的组可以并行；一个属性同时只有一个推进者。
- 离树、Host 清空和 reduced-motion 都完成整组，不保留半组。
- reduced-motion 在播放时即完成全部目标，不等待下一帧、不留下轨道。
- 每次播放生成新的 started/elapsed 状态。已经达到全部目标时仍接受播放，但没有可见
  变化；需要再次展示同一路径时，由作者先设置起始状态。

顺序与错峰沿用现有帧调度语义，不增加跨节点 owner 注册或时间线控制设施。

## 5. 验证门槛

公开入口测试贯穿构建期声明、真实指针事件、播放、推进、表现值与几何更新。L2 组合必须
同时证明没有 measure/layout 脏位、没有实际重排，并抵抗后续布局覆盖。

其它回归覆盖：弱句柄释放、离树与换树、重复播放、普通轨道和组双向替换、互不重叠的组、
延迟属性打断、运行中与播放时 reduced-motion、非法 owner/重复属性拒绝且在途轨道不受损、
行为配置同步与 Spring 到 Tween 连续性。

每条关键回归记录可使其失败的故障注入；交付执行主套件、ASan/UBSan 与 no-RTTI。
真实桌面节奏和手感由使用者验收，自动化结果只证明逻辑、几何与生命周期。

## 6. 本轮验证记录（2026-10-05）

基线为 `475b460658d91dcb83ec461633231c4d2c0d7853`。本地 Linux 环境使用 Clang 21.1.8、
GNU libstdc++ 14 开发头文件、Meson 1.7.2 与 Python 3.13；编译并发限制为 4。
窗口后端为 X11，窗口测试关闭。本地验证不等同于已运行远程 GCC/Clang CI 矩阵。

| 验证 | 结果 |
| --- | --- |
| 普通构建（含 showcase） | 通过 |
| 主测试（不含 SDK 打包 fixture） | Meson 78/78 通过，无失败或超时 |
| SDK 打包与下游消费者 fixture | 源码一致的 ext4 副本单独运行通过，退出码 0 |
| ASan/UBSan 完整构建与 unit | 72/72 通过；按开发流程关闭 leak 检测 |
| no-RTTI 完整构建与 unit | 72/72 通过 |
| 六项动画故障注入 | 各自编译成功、触发指定断言失败；每次按字节恢复并重跑两项动画 suite 通过 |

SDK fixture 在 NTFS 源码位置因打包 I/O 超过原 600 秒限时，之后使用内容一致的 ext4
副本和 1800 秒限时单独执行。对照记录核验了基线、12 项递归 submodule 状态、全部本轮
修改文件的 SHA256 与其余源码内容；fixture 完成两轮三种格式的打包、档案 SHA256/大小比较
及下游消费者构建。因此主测试记录与 SDK 记录分开归档，不宣称一次完整 79 项运行通过。

故障注入分别移除 owner 检查、移除播放时 reduced-motion 完成、移除 translate 的
transform 脏标记、保留 endpoint 的旧 Spring 配置、让句柄强持有节点，以及从旧 Tween
策略读取 Spring 切换值。只计指定的真实断言失败，不把编译失败、崩溃或异常当作证据。
另有 CI 元数据关卡的负向回归，以及系统字体发现符号链接后无法读取的先红后绿回归。

尚待人工验收的是实际桌面窗口中的节奏和手感；跨节点组合、组内 Spring、跨平台验证
与远程 CI 运行均不在本轮验证结论内。补丁与原始日志随交付包提供，不写入仓库构建目录。
三套测试中的 clipboard suite 各有一个 Catch 用例因无桌面会话而跳过，虽然 Meson 的
suite 级计数显示全部通过；窗口 lifecycle 用例走禁用窗口测试的占位分支，physics2d 关闭。
