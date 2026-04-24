# Git 提交信息规范

> 版本：1.0
> 生效日期：2026-04-24

## 1. 提交格式

每条提交信息必须遵循以下格式：

```
<type>(<scope>): <subject>
```

**格式示例**：
```
feat(runtime): 添加 NanWindow 窗口类
docs(issue004): 优化开发文档导航页结构与内容
refactor: 使用二级命名空间按模块分类 (issue003)
test(foundation): 为 geometry 模块添加单元测试 (issue003)
```

### 1.1 组成部分

| 部分 | 说明 | 要求 |
|------|------|------|
| `<type>` | 提交类型（见 §2） | **必填**，小写字母 |
| `<scope>` | 影响范围（见 §3） | **必填**，小写字母 + 下划线 |
| `<subject>` | 提交描述 | **必填**，中文或英文，句尾不加句号 |

### 1.2 完整示例

```
feat(color): 新增 NanColor 统一颜色容器
fix(window): 修复窗口 resize 时崩溃问题
docs(architecture): 补充模块依赖关系图
refactor(runtime,app): 采用 Godot-like 继承生命周期与输入钩子
test(foundation): 为 geometry 模块添加单元测试 (issue003)
chore(build): 更新 CMake Presets 配置
style: 统一代码缩进为 4 空格
```

## 2. 提交类型（type）

| 类型 | 说明 | 示例 |
|------|------|------|
| `feat` | 新功能（feature） | `feat(foundation): 创建 nan_types 基础枚举模块` |
| `fix` | 修复 bug | `fix(window): 修复窗口关闭时内存泄漏` |
| `refactor` | 重构（既不修复 bug 也不增加功能） | `refactor(runtime): 抽取 PointerButton 至 foundation` |
| `docs` | 仅文档变更 | `docs(issue004): 优化开发文档导航页结构与内容` |
| `test` | 添加或修改测试 | `test(foundation): 为 geometry 模块添加单元测试` |
| `chore` | 构建/工具/依赖变更 | `chore(build): 启用 C++20 Modules 编译` |
| `style` | 代码样式格式化（不影响逻辑） | `style: 统一缩进为 4 空格` |
| `perf` | 性能优化 | `perf(render): 缓存 ThorVG 画布指针` |

## 3. 影响范围（scope）

### 3.1 模块范围

与项目文件系统路径对应：

| scope 值 | 对应目录 | 说明 |
|----------|---------|------|
| `foundation` | `foundation/` | 基础类型模块 |
| `runtime` | `runtime/` | 运行时核心模块 |
| `app` | `app/` | 应用程序层模块 |
| `reactive` | `reactive/` | 响应式系统模块 |
| `layout` | `layout/` | 布局引擎模块 |
| `render` | `render/` | 渲染后端模块 |
| `widgets` | `widgets/` | 控件模块 |
| `theme` | `theme/` | 主题系统模块 |
| `text` | `text/` | 文字排版模块 |
| `bindings` | `bindings/` | 脚本绑定模块 |
| `showcase` | `showcase/` | 示例演示程序 |
| `tests` | `tests/` | 测试套件 |
| `build` | `CMakeLists.txt`、`CMakePresets.json` 等 | 构建系统配置 |
| `docs` | `docs/` | 项目文档 |

### 3.2 Issue 范围

当一次提交跨越多个模块时，优先使用 `issue<N>` 作为范围：

```
docs(issue004): 优化开发文档导航页结构与内容
refactor: 使用二级命名空间按模块分类 (issue003)
```

**规则**：
- 如果变更集中在 1-2 个模块，使用模块名作为 scope。
- 如果变更涉及 3 个以上模块或重构/修复，使用 `issue<N>` 作为 scope。
- `(issue<N>)` 也可在 subject 末尾附加，用于跟踪 Issue 编号。

### 3.3 多模块范围

可使用逗号分隔多个 scope：

```
feat(log,color): 重构日志命名空间并优化颜色接口
refactor(runtime,app): 采用 Godot-like 继承生命周期与输入钩子
```

## 4. 提交信息写作指南

### 4.1 subject 写作规则

- 使用**简洁的祈使句**，描述此次提交完成的工作。
- 可使用**中文**或**英文**，保持项目一致。
- 句尾**不加句号**。
- 长度建议不超过 72 个字符。

### 4.2 body（可选）

如需补充说明，在 subject 后空一行，写入详细描述：

```
feat(foundation): 创建 nan_types 基础枚举模块

包含 8 个基础枚举类型（Axis、Visibility、Overflow、Align、
Direction、Wrap、FillRule、PointerButton），统一位于
nandina::types 命名空间下。
```

### 4.3 footer（可选）

用于引用 Issue、Breaking Changes 等元信息：

```
feat(foundation): 创建基础枚举模块

Closes #5
```

```
refactor(runtime): 迁移 PointerButton 至 foundation

BREAKING CHANGE: PointerButton 从 nandina::runtime
迁移至 nandina::types，需更新 import 路径。
```

## 5. 提交示例集

```
# 新功能
feat(window): 添加 NanWindow 类以便后续开发和测试

# 重构（Issue 跟踪）
refactor: 使用二级命名空间按模块分类 (issue003)

# 测试
test: 为 geometry 模块添加单元测试 (issue003)

# 文档 + Issue 导航
docs(issue004): 建立初始开发文档导航页

# 多模块变更
feat(log,color): 重构日志命名空间并优化颜色接口

# 跨模块重构（用 issue 范围）
refactor(issue005): 创建 foundation 基础枚举模块并统一命名空间
```

## 6. 回退与修正

- 若提交后发现格式错误，使用 `git commit --amend` 修正。
- 若已推送至远程，使用 `git commit --amend` 后 `git push --force-with-lease`。
- 禁止使用 `--force` 推送到共享分支（main），除非团队明确允许。

---

**最后更新**：2026-04-24
**维护者**：CvRain