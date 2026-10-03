# 开发流程

> 本文是**每轮改动都适用的循环**：先定形状、再分层落地、测试必须证明会红、最后归档原因。
> 它不是"新增组件要动哪些文件"的清单 —— 那是 [新增一个组件](adding_a_component.md)。
> 本文管的是**节奏与关卡**。

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

meson compile -C buildDir-no-rtti && \
  meson test -C buildDir-no-rtti --suite unit --print-errorlogs

# 测试必须声明 suite，否则静默失去 sanitizer 覆盖
meson test -C buildDir --list | grep -v -- ' - '

clang-format -i <只格式化你改过的文件>

./buildDir/playground/playground --self-test      # 有窗口副作用时
./buildDir/showcase/nandina_showcase              # 视觉验收的对象是 showcase
```

### 两条硬规矩（都付过学费）

1. **不要把编译和测试拆成两条会跑旧二进制的命令。** 只 `meson test` 而不先 `meson compile`，
   跑的是上一次构建的产物 —— [004](../stories/004-the-key-that-was-not-an-identity.md) 的
   "假绿"就是这么来的：编译失败被 `grep`/`head` 过滤成上下文行，第二条命令报了旧代码的
   "All tests passed"。**编译和测试用 `&&` 连起来。**
2. **格式化范围与改动范围一致。** 只格式化自己改过的文件 —— 把别人正在写的文件一起扫了，
   会把功能变更淹没在格式化噪声里（`coding_conventions.md` 也这么要求）。

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
| **捕获生命周期** | 存起来的 lambda（`computed` / `bind` / `on_*`）里有没有 `[&]` 抓局部量或 `this`（[006](../stories/006-the-animation-that-could-not-fire.md) 的 bug 就是它） |
| **值的唯一来源** | 有没有引入第二个"当前值"；`PropertyEndpoint` 会不会永久遮罩配方字段 |
| **测试是否会红** | 要求指出"故意改坏哪一行会让它失败" |
| **静默失效** | 有没有"设置成功但不生效"的新路径 —— 这个项目最恨这一类 |
| **文档同步** | 设计文档的状态、组件文档的字段表、两处索引 |

## 6. 一个贯穿性问题

每造一个机制都问：**预期消费者是谁，他够得到吗？**

这个项目已经三次栽在同一个形状上：`Group` 与 `Keyframes` 造好了但没有部件层句柄、
painter 家族写好了但停在 `primitives/` 里当不了应用依赖、
`NanNode2D::set_local_opacity` 存在却不可动画。
