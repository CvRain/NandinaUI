# 开发流程

> 本文是**每轮改动都适用的循环**：先定形状、再分层落地、测试必须证明会红、最后归档原因。
> 它不是"新增组件要动哪些文件"的清单 —— 那是 [新增一个组件](adding_a_component.md)。
> 本文管的是**节奏与关卡**。

## 0. 选批次与记录状态

跨专题排期统一见[项目进度与下一步](project_status.md)。D 编号标识设计议题，P 编号标识
D4 的历史阶段；都不能直接当作下一轮任务顺序。每轮先选一个可验收批次，列出前置与非目标，
再读对应 reference 和实际消费者链路。

状态分开记录：方向已接受、精确契约已定形、实现存在、自动验收通过、指定平台实机验收通过。
文档提交只推进文档状态；历史日志不计为本轮测试，shader 生成也不计为目标设备加载。
未定行为先收敛，未具备设备的验收保留限制，不能勾选为全部完成。

纯文档轮检查相对链接/锚点、专题状态与主排期一致性、`git diff --check` 和提交范围；
不为计划变更重跑整套 C++ 构建。代码轮继续遵守下列三套验证与专项关口。

## 1. 六步循环

| 步骤 | 做什么 | 跳过的后果 |
| --- | --- | --- |
| **1 读** | 把要动的链路读通：谁调用谁、谁拥有值、哪一层依赖哪一层 | 会按"看起来该放哪"放，然后撞分层规则 |
| **2 定形状** | 先定 API 形状与**归属层**，写进 `docs/references/`（**代码前**） | 边写边改形状，测试与文档跟着返工 |
| **3 分层落地** | 自下而上：下层设施 → 节点能力 → DSL → 组件/示例 | 从上层往下改，会在下层发现缺东西 |
| **4 测试** | 每个新能力都要有**会红的**测试（第 3 节） | 加的是"通过的测试"，不是回归网 |
| **5 三套验证** | 主套件 + ASan/UBSan + no-RTTI（第 2 节的命令） | sanitizer 才是抓悬垂/越界的那一道 |
| **6 归档** | 决定进 `references`、踩坑进 `stories` | 下一个维护者只能从源码逆推 |

## 2. 验证关卡

```bash
meson setup --reconfigure buildDir          # 只改了 meson.build 时需要

meson compile -C buildDir && meson test -C buildDir

# 注意：先编译再测试，且用 && 连起来
meson compile -C buildDir-asan && \
  LSAN_OPTIONS=detect_leaks=0 meson test -C buildDir-asan --suite unit --print-errorlogs
# 精简/sanitizer 配置若用 -Dresource_tools=false，必须同时加 -Dshowcase=false：
# showcase 现在会构建资源包，没有资源工具链时配置阶段直接报错（或用
# -Dexternal_nanres=<nanres> 保留 showcase）。

meson compile -C buildDir-no-rtti && \
  meson test -C buildDir-no-rtti --suite unit --print-errorlogs

# 测试必须归入 unit（C++）或 integration（Python / 构建工作流）。
# CI 辅助脚本只依赖 Meson introspection JSON 的 suite 字段，不解析人类可读列表。
python3 tools/check_test_suites.py buildDir

clang-format -i <只格式化你改过的文件>
python3 tools/check_format.py     # 全仓格式关卡，固定 clang-format 23.1.1

# 一条命令汇总无 GPU 门禁（format/header/artifact/golden/suite/docs/whitespace）：
python3 tools/release_check.py --build-dir buildDir
# 发布检查再带上性能采样报告（需要 GPU/窗口，本地或发布机执行）：
python3 tools/release_check.py --build-dir buildDir --report <perf-report.json>

./buildDir/playground/playground --self-test      # 有窗口副作用时
./buildDir/showcase/nandina_showcase              # 视觉验收的对象是 showcase
```

**只改渲染后端 / 宿主 / 依赖时，再加跑这几条**（D4 的教训：三套套件全绿不等于后端等价）：

```bash
# 1) 三个 showcase 页面各过一遍该后端（像素 + 稳态成本）
for p in home label anchors; do
  NANDINA_PROBE_PAGE=$p NANDINA_PROBE_CAPTURE=/tmp/sdl_$p.bmp \
    ./buildDir-probe/probe/cross_platform/desktop/nandina_sdl_page_probe
done

# 2) 逐图元对照：像素相同不代表图元相同（圆角画成方框时像素差异可能只有几个像素）
NANDINA_BENCH_TRACE=/tmp/raylib.txt  ./buildDir/playground/label_bench <page>
NANDINA_PROBE_TRACE=/tmp/sdl.txt NANDINA_PROBE_PAGE=<page> \
  ./buildDir-probe/probe/cross_platform/desktop/nandina_sdl_page_probe
python3 tools/compare_render_traces.py /tmp/raylib.txt /tmp/sdl.txt

# 3) deps.json 实现后再执行：换过依赖时核对实际版本、来源和路径
cat buildDir/deps.json        # 目标形态，见 dependency_policy.md §6

# 4) 新增/移动框架头文件时：分类完整性 + 包含方向 + 独立可包含
python3 tools/check_headers.py --source . --build-dir buildDir
```

### 四条硬规矩（都付过学费）

1. **不要把编译和测试拆成两条会跑旧二进制的命令。** 只 `meson test` 而不先 `meson compile`，
   跑的是上一次构建的产物 —— [004](../../stories/004-the-key-that-was-not-an-identity.md) 的
   "假绿"就是这么来的：编译失败被 `grep`/`head` 过滤成上下文行，第二条命令报了旧代码的
   "All tests passed"。**编译和测试用 `&&` 连起来。**
2. **格式化范围与改动范围一致。** 只格式化自己改过的文件 —— 把别人正在写的文件一起扫了，
   会把功能变更淹没在格式化噪声里（`coding_conventions.md` 也这么要求）。全仓已做过一次
   性归一化（单独成一个提交），之后的漂移由 CI 的 `format` 任务拦下（`python3
   tools/check_format.py`，固定 clang-format 23.1.1），所以这条规矩继续适用于日常提交，
   不需要再出现第二次"顺手全仓排一遍"。
3. **测试分组关卡使用结构化元数据。** `meson test --list` 是给人看的输出；Meson 1.7.2
   打印 `NandinaUI:unit / foundation`，不能用 ` - ` 分隔符推断有没有 suite。
   `tools/check_test_suites.py` 属于 CI 工具层，执行 `meson introspect --tests`，
   检查每条测试的 `suite` 字段是否含项目限定的 `unit` 或 `integration` 标签；无分组、
   只有其他标签、空测试列表、查询失败或无效 JSON 都必须失败，不能用 `|| true` 吞掉。
   回归测试用正常元数据与故意缺分组 / 查询失败的输入验证关卡（改掉分组判断或
   `subprocess.run(..., check=True)` 会红）。
4. **跑全量套件时不要动工作区**（对应的是"假红"）。`nandina-subproject-fixture` 会把
   `--allow-dirty` 的工作区**打包两次**并断言两次产物逐字节一致。如果你在它运行期间保存了
   任何被纳入包的文件（源码、文档、脚本），第二次打包看到的就不是同一份输入，
   它会报 `repeated_manifest["artifacts"] == manifest["artifacts"]` 失败 —— 而这和你的
   改动是否正确毫无关系。**全量套件是"先定稿、再跑"，不是"边跑边改"。**

## 3. 测试：必须证明它会红

一条新测试只有在"**故意改坏对应实现就会失败**"时才算回归网。加测试时同时记下：

> 我改坏哪一行会让它失败？

三条通用要求：

- **别重复验证已有覆盖。** 先找同类测试（例如 `visual::opacity` 的"只标 paint"已有基础测试
  在 `tests/scene_tests.cpp`），新测试要覆盖**新增的那条路径**，而不是把 setter 再测一遍。
- **组合断言优于单一断言。** 例如"动画不触发重排"要同时看：脏标记里没有 measure/layout，
  **并且**一个计数型 layout root 的 `measure`/`on_layout` 调用次数没有增加。只看脏标记会
  漏掉调度器行为，只看调用次数又不够精确。
- **覆盖抵抗性而不只是覆盖正向路径。** 例如表现层变换要测"布局覆盖抵抗"：布局 → 设变换 →
  **再执行一次相同布局** → 变换仍有效且布局矩形未变。

## 4. 交付审核时带什么

1. **diff 或 commit 哈希**（不用贴全文）；
2. **跑过的命令与输出**，特别是三套验证；
3. **第 2 步那几条"归属层"结论**（形状、存储、依赖方向）；
4. **拿不准的地方**，哪怕只是"感觉有点丑"；
5. **每条新测试"改坏哪里会让它红"**。

## 5. 审核会重点看什么

| 关注点 | 具体查什么 |
| --- | --- |
| **分层** | 新类型/新标签有没有让下层向上依赖；有没有设施该下移到它真正依赖的那一层 |
| **代价级别** | 写这个属性标了什么脏标记；声称为 L2 的实现在动画期间有没有偷偷触发重排 |
| **捕获生命周期** | 存起来的 lambda（`computed` / `bind` / `on_*`）里有没有 `[&]` 抓局部量或 `this`（[006](../../stories/006-the-animation-that-could-not-fire.md) 的 bug 就是它） |
| **值的唯一来源** | 有没有引入第二个"当前值"；`PropertyEndpoint` 会不会永久遮罩配方字段 |
| **测试是否会红** | 要求指出"故意改坏哪一行会让它失败" |
| **静默失效** | 有没有"设置成功但不生效"的新路径 —— 这个项目最恨这一类 |
| **文档同步** | 设计文档的状态、组件文档的字段表、两处索引 |

## 6. 一个贯穿性问题

每造一个机制都问：**预期消费者是谁，他够得到吗？**

这个项目已经三次栽在同一个形状上：`Group` 与 `Keyframes` 造好了但没有部件层句柄、
painter 家族写好了但停在 `primitives/` 里当不了应用依赖、
`NanNode2D::set_local_opacity` 存在却不可动画。

## 7. 动后端/host 时的宿主契约清单（D4 用血换的）

一个"最小宿主"漏掉下面任何一条，症状都会伪装成**框架缺陷**。真实宿主（`NanWindow`）的顺序是：

| 阶段 | 必须做什么 | 漏掉的表现 |
| --- | --- | --- |
| input | 平台翻译输入 → `tree.dispatch_*` | 点了没反应 |
| tasks | **`dispatcher().drain()`** | 回调执行了但路由不换 —— "按了没反应" |
| process | `tree.process(dt)` | 状态机停在中间态 |
| tree_commit | `flush_tree_mutations()` / `flush_deferred_deletes()` | 新挂载节点这一帧不参与渲染；跨帧堆积会崩 |
| physics | `tree.physics_step(dt)` | 物理组件不动 |
| reactive | `deferred_effects.commit()` | effect 跨帧延迟 |
| animation | **`tree.advance_animations(dt)`** | 动画停在起点：浮层"已打开但不可见"（opacity=0）、hover 过渡不动 |
| layout → semantics → render | 宿主开/关帧边界并 `clear()`，再 `tree.render(ctx)` | 用 `tree.draw(device)` 会**再开一次**帧边界（探测里踩过：每帧两对 begin/end 与两次 clear） |

推论：**探测程序不是宿主，不能拿它当行为基准**；它要么照抄上表，要么明确记录自己偏离在哪一条
（[跨平台后端探测](cross_platform_backend_probe.md) §2.3.4/§2.3.6 就是这种"记录偏离"的例子）。

## 8. 已知陷阱清单（新后端/新平台首先检查）

1. **接口里"默认实现会静默降级"的入口**：`draw_rounded_rect_outline`（默认退化成方框描边）、
   `draw_circle_clipped_rounded_rect`（默认画**未裁剪**的圆）、`draw_rounded_rect_shadow`（默认 no-op）。
   新后端必须逐个覆写或让能力查询如实返回 false，并**每个都配一条会红的测试**。
2. **能力查询缺一个消费者就等于没有**：`supports_*()` 只有在组件真的据此改走降级路径时才算落地。
   也不要**用一个布尔值概括多件独立的事**（例如"能不能显示图片"其实由解码、直载、上传三项组成）：
   拆开分别报告，否则排查时只能看到"不可用"，看不到缺哪一项。
3. **半透明 + 混合**：几何与纹理的混合模式是两套 API（例如 SDL 的 `SDL_SetRenderDrawBlendMode` 与
   `SDL_SetTextureBlendMode`）。只设一个，症状是"某个交互看起来怪"，而静止帧的像素对照**完全看不到**。
   → 状态类观感（hover/pressed/ripple/遮罩）必须有专门的截图回归，不能只对静止帧。
4. **关闭/销毁后的语义**：`should_close()` 在窗口销毁后必须报真（raylib 的 `WindowShouldClose()` 语义），
   否则主循环会 tick 已销毁的窗口并读到 0×0 尺寸。
5. **未知输入不要伪造**：键码映射不到时**不发事件**并计数，不要退化成"发一个相近的键"。

## 9. 后端与输入的专项验收

- 混合测试有已知背景与颜色的独立预期；raylib 对照之外，固定时间采样按钮按下、涟漪和遮罩。
- SDF 测试同帧不同参数与文字/纹理交错；检查 uniforms、状态恢复、alpha 和裁剪。
  故意漏一次更新/恢复应失败；探测单一形状不替代生产路径。
- 像素容差在比较前声明，固定字体/尺寸/缩放/动画时刻；按区域和交互状态记录，
  不把全图平均差掩盖的局部缺陷当作通过。trace、像素和实际行为各证明自己的边界。
- 像素回归的 harness 自身要满足三条：**测试自绘背景**（不依赖宿主 clear 色，否则一个宿主选择就能
  造出"后端差异"）、**读回在 present 之前**（present 后读回可能拿到空帧）、**注册为 integration
  测试并纳入 golden 管理**（否则它只是手工命令，会腐烂）。C++ 测试注册进 `unit` 套件、
  脚本/构建工作流进 `integration`（文件收集规则见 `tests/meson.build` 顶部注释）。
  目录、manifest 与审批流程见
  [视觉回归](visual_regression.md)。
- 状态切换也是成本：自定义 render state / 着色器切换会 flush 2D 批次，要统计每帧切换次数，
  不能只看总帧时间。采样协议、p50/p95 口径与候选阈值见[性能预算](performance_budget.md)；
  在稳定性实验完成前只报告、不阻断。
- shader 分别记录生成、静态校验、目标设备加载/绘制；spirv-val 只验证 SPIR-V。
  普通回退 Renderer 的能力和效果独立验证。
- 输入测试覆盖事件时修饰键、物理/逻辑身份、release/repeat 和跨类型顺序；
  模拟映射通过不等于非 US 布局或 IME 实机通过。

## 10. 依赖变更的规矩

新增或替换第三方依赖时，**先读 [第三方依赖策略](dependency_policy.md)**，并遵守：

- 每个依赖只有**一份权威来源/版本声明**；system 与固定源码是显式来源选择，不允许两份独立版本表；
- 核对上游构建方法与实际 target；CMake wrap 的 provide 映射必须实测，不以旧文“不能 fallback”为前提；
- 版本写 min 与 pin 的版本/完整 commit，不写浮动分支；来源默认 vendored，auto 明确为系统优先；
- 单项迁移时同步解析报告、SDK 输入收集/许可证/manifest 和下游消费，不等全部迁完再修分发；
- nodownload 配合断网验证；wrap-git 保留固定源码检出，不能只保存 packagecache；
- 显式启用 SDL 的 CI 证明 SDL 路径，auto 无后端构建的成功不算；关闭功能不查找或下载专属依赖；
- source/feature 开关、报告和 SDK 改造未落地前保持 planned，下一轮先做最小实验。
