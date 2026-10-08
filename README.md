![nandina_cover](docs/images/nandina_cover_1.jpg)

<div align="center">

# NandinaUI · 南天竹

**一个面向 Linux 桌面的 C++26 GUI 框架 —— 让编写桌面应用成为一件舒服的事。**

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0--alpha.1-blue.svg)](release-metadata.toml)
[![C++](https://img.shields.io/badge/C%2B%2B-26-00599C.svg)]()
[![CI](https://github.com/CvRain/NandinaUI/actions/workflows/ci.yml/badge.svg)](https://github.com/CvRain/NandinaUI/actions/workflows/ci.yml)

</div>

## 简介

NandinaUI（南天竹）是一个用 **C++26** 编写、基于 **Meson** 构建的原生桌面 UI 框架。它以 C++ 控件和场景树为基础，提供声明式界面构建、响应式状态、主题、动画、资源管理与页面导航，让应用代码可以用类型安全的 DSL 组织桌面界面，同时保留直接访问控件对象的能力。

项目在开发时参考了现有主流框架的设计思路，如 `Angular` 的页面调度与响应式机制、`Shadcn` 的 primitives + token 组件设计、`Slint` 的脏数据更新与文件渲染机制、`Qt` 的新信号槽机制与组件 API 设计。

> 当前处于 **alpha** 阶段，正式支持范围为 `linux-desktop-source` profile（Linux 桌面源码集成）。API 仍在快速演进；项目不承诺保留旧公开头文件或命名空间的源码兼容入口。

## 项目速览

| 项目 | 信息 |
| --- | --- |
| 定位 | 面向 Linux 桌面应用的原生 C++ UI 框架 |
| 语言 / 标准 | C++，`cpp_std=c++26` |
| 构建系统 | Meson（第三方库经 CMake subproject 适配） |
| 许可证 | MIT（Copyright © 2026 ClaudeRainer） |
| 版本 | 0.1.0-alpha.1 |

## ✨ 核心特性

- **声明式 UI DSL** —— 用 `ui.column()`、`ui.center()`、`ui.make<widget::Button>()` 组合出可读的界面树，布局、对齐、间距一链式完成。
- **响应式状态** —— `signal` / `computed` / `effect` / `property` / `batch`，状态变化自动驱动界面更新，告别手动刷新。
- **可组合组件库** —— 覆盖内容展示、输入选择、布局滚动、浮层反馈与通用指针手势，并通过统一主题和声明式构建器组合。
- **动画系统** —— 值级 Tween、Spring、关键帧与缓动曲线；节点表现层支持 opacity、translate、scale 等 L2 属性动画，同一节点的扁平 Tween 组合提供实验性作者入口。跨节点组合、组内 Spring 与关键帧作者入口尚未开放。
- **现代文本引擎** —— FreeType + HarfBuzz + FriBidi + utf8proc 组成的字形管线，支持多字体、系统字体发现、复杂文字整形与双向文本。
- **主题与设计系统** —— 三层设计令牌（primitive → semantic → component）、明暗外观（Appearance）、内置主题与样式文档。默认主题对齐 shadcn 的语义角色集，开箱即用；对比度与层级由测试守住。
- **资源系统** —— 资源清单（manifest）+ 内置/目录/内存/SQLite 四类后端，配合 `nanres` 编译器与可移植打包流程。
- **应用运行时** —— 窗口、Router/Page 导航、视口缩放、异步作用域与统一的输入/剪贴板分发。
- **可选 2D 物理** —— 基于 Box2D 3.x 的轻量物理桥（默认关闭）。
- **无障碍语义** —— 控件语义树导出，为可访问性工具铺路。

## 🧱 架构

框架按模块组织：

| 模块 | 职责 |
| --- | --- |
| `foundation` | 颜色、几何、布局约束、纯值 motion、UTF-8、JSON、日志 |
| `reactive` | 信号图、computed/effect、属性绑定 |
| `resource` | 资源清单、多后端、运行时管理 |
| `physics2d` | 可选 Box2D 物理桥（默认关闭） |
| `theme` | 设计令牌、主题管理器、样式文档 |
| `render` | 渲染设备（raylib 后端）、纹理缓存、SDF 图元 |
| `text` | 字体加载、字形图集、复杂文本整形 |
| `scene` | 场景树、节点、控件、表现层、动画宿主与帧调度 |
| `semantics` | 无障碍语义 |
| `widget` | 组件库、声明式 DSL、布局原语 |
| `app` | 窗口、Router/Page、应用入口 |

## 🧩 组件支持

| 类别 | 当前可用 |
| --- | --- |
| 内容与展示 | Label、Image、Avatar（首字母占位）、Badge、Chip、Divider、ProgressBar、Spinner、Skeleton、EmptyState、Card |
| 输入与选择 | Button、Checkbox、Switch、RadioButton/RadioGroup、Slider、TextField、TextArea、Select、Combobox、Tabs、Toggle、ToggleGroup、ButtonGroup、Breadcrumb、Pagination |
| 浮层与反馈 | Dialog、Popover、DropdownMenu、ContextMenu、Tooltip、Alert |
| 交互扩展 | PointerArea、GestureArea（实验性） |
| 布局与滚动 | Column、Row、Flex、Wrap、Stack、Padding、Center、Expanded、Grid、ScrollView、ListView |

完整的用途、成熟度与计划组件见 [组件参考](docs/components/README.md)。

当前布局以 Column、Row、Flex、Grid 等排列容器为主。场景树 anchors 已落地 scene 内核（`NodeRef<T>`、`AnchorCanvas`、`set_anchors`）与作者入口（`ui.ref<T>()`、`ui.anchor_canvas()`、builder 的 `.anchor.*` / `.parent.anchor.*` 与 `.anchors(...)`），支持父级/兄弟关系、依赖排序、批量关系切换与冲突诊断；`showcase` 的 `anchors` 页面提供侧边栏停靠切换案例。当前边界是**画布需要确定尺寸，因此 `fill` 画布无法占据排列容器的剩余空间**（显式/百分比尺寸的画布可以嵌入排列容器），把画布放在布局根再用兄弟锚点表达剩余区域即可，见 [Anchors](docs/references/anchors.md)。它与浮层锚定定位器是两项独立能力。

## 🚀 快速开始

### 环境要求

- **Linux 桌面**（Wayland | Xorg）
- **编译器**：GCC ≥ 16 或 Clang ≥ 21（需 C++26 支持）
- **构建工具**：Meson、Ninja、CMake、pkg-config、Python 3
- **系统库**：OpenGL、OpenSSL ≥ 3.0、SQLite ≥ 3.37

其余第三方依赖（spdlog、raylib、FreeType、HarfBuzz、FriBidi、utf8proc、toml++、nlohmann/json、Catch2、Box2D 等）均以 Git 子模块方式随仓库提供。

### 创建 C++ 项目
```bash
# 建立项目目录
mkdir nandina_playground
cd nandina_playground

# 初始化项目
meson init --name nandina_playground --language cpp
```

### 将本项目以源码方式导入
```bash
mkdir subprojects
cd subprojects
git clone https://github.com/CvRain/NandinaUI.git --recursive
```

### 编辑 `meson.build`
```meson
project(
  'nandina_playground',
  'cpp',
  version : '0.1',
  meson_version : '>= 1.3.0',
  default_options : ['warning_level=3', 'cpp_std=c++26'],
)

nandina = subproject(
    'NandinaUI',
    default_options: ['build_tests=false', 'physics2d=disabled'],
)
nandina_dep = nandina.get_variable('nandina_dep')

dependencies = [
    nandina_dep
]

sources = [
  'nandina_playground.cpp',
]

exe = executable(
  'nandina_playground',
  sources,
  install : true,
  dependencies : dependencies,
)

test('basic', exe)

```


### 编辑 `nandina_playground.cpp`
```cpp
#include <nandina/app/nan_application.hpp>
#include <nandina/widget/controls.hpp>

using namespace nandina;

auto main() -> int {
    return app::run(
        app::RunConfig {
            .id = "com.example.hello",
            .window = {
                .title = "Hello NandinaUI",
                .width = 640,
                .height = 420,
            },
        },
        [](const widget::BuildContext& ui) {
            return ui.center()
                .child(ui.make<widget::Label>("Hello, NandinaUI!"))
                .build();
        }
    );
}
```

### 构建
```bash
meson setup buildDir --wrap-mode=nodownload
meson compile -C buildDir
```

### 运行
```bash
./buildDir/nandina_playground
```
![运行效果](docs/images/z_hello_nandina.png)

## 📚 文档

- [入门指南](docs/getting_started/README.md)：从项目认识、创建窗口到布局、响应式状态和 Page 导航。
- [组件参考](docs/components/README.md)：组件清单、公开 API、使用方式与交互规则。
- [开发参考](docs/references/README.md)：组件契约、架构约束和维护流程。
- [构建系统与 Modules 范围](docs/references/build_system_scope.md)：当前 Meson/include 主线，以及暂缓 C++ Modules 和 CMake package 的原因。
- [组件开发路线图](docs/references/component_roadmap.md)：当前差距、依赖关系与推荐实现顺序。
- [项目进度与下一步](docs/references/project_status.md)：当前能力边界、anchors 开发阶段与验证关口。

## 🗺️ 开发计划

项目当前处于 alpha 阶段。下列已完成项是历史里程碑；近期工作的排序与验收标准以[项目进度与下一步](docs/references/project_status.md)为准。项目处于快速演进期，不承诺长期保留旧公开头或命名空间的源码兼容入口；发生破坏性 API 调整时，库内示例、文档和测试应同步迁移。

- [x] 建立声明式 UI、响应式状态、主题、动画、资源和 Page/Router 基础。
- [x] 支持 Linux Wayland 与 X11，并提供 PointerArea / GestureArea 组合式交互。
- [x] 建立组件公共契约、组件文档与开发参考目录。
- [x] 建立 OverlayHost/portal 的内容层、顶层层级和 RAII 生命周期基础。
- [x] 补齐浮层锚点定位、边界翻转和视口内收。
- [x] 补齐点击外部关闭和焦点作用域等浮层基础设施。
- [x] 将 Tooltip、Select、Dialog 迁移到统一浮层设施并保持应用层 API 兼容。
- [x] 默认主题对齐 shadcn 的语义角色集（含 `muted` / `accent` / `card` / `popover` / `destructive`），并把对比度与层级门槛固化为测试。
- [x] 补充 TextArea、Toggle、Alert、Spinner、Skeleton、EmptyState 等高频组件；新组件直接消费已有语义角色，不再新增硬编码色值。
- [x] 补齐浮层基础设施的 roving focus / typeahead 与嵌套浮层父子关闭关系。
- [x] 补齐 `AlertDialog`，收尾阶段 3 的组件清单。
- [x] 落统一 MenuItem model 与 Popover 浮层基座（锚定、外部关闭、焦点作用域，内容为任意控件），作为阶段 4 菜单族的公共依赖。
- [x] 实现 `DropdownMenu`（动作 / 勾选 / 单选条目、递归子菜单、键盘漫游、typeahead、无障碍语义），并在 playground 增加菜单演示页。
- [x] 实现 `ContextMenu`（右键 / 菜单键 / Shift+F10 在指针处打开，包装 DropdownMenu）并接入构建与契约测试。
- [x] 实现 `Combobox`（输入即筛选 + 下拉选择，自由文本可选），并在 playground 增加演示单元。
- [x] 基于该基座实现 CommandPalette 和 HoverCard。
- [x] 将纯布局约束、文本布局协议与字体请求值移到实际归属层；L2 节点表现层与同节点 Tween 组合作者入口已落地。
- [x] 清理旧 `animation::` 名称及转发头；值级动画归 `foundation/motion`，节点调度归 `scene`。
- [x] 退出旧 `scene::LayoutConstraints` 名称；布局约束统一使用 `foundation::NanLayoutConstraints`。
- [x] 退出文本布局与字体请求值的旧公开转发入口；使用 `text/text_layout*.hpp` 与 `theme/font_request.hpp`。
- [x] **Anchors 基础验证**：百分比与无界约束、隐藏排列项、ScrollView 轴向约束与 reparent 失效规则已固定为测试，并写回 reference。
- [x] **Anchors scene 内核**：类型化弱引用、显式锚定画布、父/同画布兄弟锚线、尺寸冲突诊断、依赖排序与环检测、批量关系切换；通过 `layout_to()` 交付布局结果。
- [x] **Anchors 作者入口**：`ui.ref<T>()`、builder 绑定与只读锚线门面、`ui.anchor_canvas()`、`.anchors(spec|source)` 与 `AnchorCanvas::set_child_anchors()` 批量切换均已落地，由 `tests/anchors_authoring_tests.cpp` 覆盖。
- [ ] **Anchors 集成验收**：重复布局、布局系统误用、绘制/命中/语义几何一致性已由 authoring 测试覆盖，`showcase` 的 `anchors` 页面已提供侧边栏切换案例；剩余真实窗口的人工体验验收与 playground 同类案例。
- [ ] Anchors 稳定后再评估阶段 5 复合组件及浮层打开/关闭动效；两者不作为 anchors 的隐含前置。
- [ ] 给浮层补打开/关闭过渡与缓动曲线，并把 `motion` token、`reduced_motion` 偏好接到动画侧。
- [ ] 继续打磨 `butter`（本项目自研风格）；`fluent` / `material` 需要补上各自设计语言在几何、密度与状态层上的差异，目前只有配色与圆角尺度。
- [ ] 后续完善 Table/DataTable、Accordion、Sheet 等复合组件。
- [ ] 在模块边界和跨平台构建流程稳定后，重新评估 C++ Modules 与 CMake package 支持。

路线图会随着基础设施成熟度调整；Getting Started 只采用推荐 API，实验性能力会在组件文档中明确标注。


## 外部依赖一览

| 类别 | 依赖 |
| --- | --- |
| 渲染 | raylib（GPU 后端，支持 JPG 等格式） |
| 文本 | FreeType、HarfBuzz、FriBidi、utf8proc |
| 数据 / 配置 | nlohmann/json、toml++、SQLite3 |
| 其他 | spdlog（日志）、OpenSSL（资源签名）、Box2D（可选物理）、Catch2（测试） |


## 📄 许可证

本项目以 [MIT License](LICENSE) 开源。
