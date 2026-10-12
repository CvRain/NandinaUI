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

资源构建已有 helper 和脚手架自动化，但 `nanres` 目前仍链接完整框架，SQLite 尚无工具/后端
独立关闭选项。[D5 资源构建与部署](docs/references/resource_build_and_deployment.md) 已冻结设计，
将分批补齐工具解耦、运行时查询作用域、身份生成及声明式构建/安装定位；这些能力尚未实施。

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
| 浮层与反馈 | Dialog、AlertDialog、Popover、DropdownMenu、ContextMenu、CommandPalette、HoverCard、Tooltip、Alert |
| 交互扩展 | PointerArea、GestureArea（实验性） |
| 布局与滚动 | Column、Row、Flex、FlexItem、Wrap、Stack、Padding、Center、Expanded、Grid、ScrollView、ListView |

表中前四类共 37 个公开组件（`RadioGroup` 与 `RadioButton` 同行记录）；"布局与滚动"一栏是声明式布局原语，不计入组件数。完整的用途、成熟度与计划组件见 [组件参考](docs/components/README.md)。

当前布局以 Column、Row、Flex、Grid 等排列容器为主。场景树 anchors 已落地 scene 内核（`NodeRef<T>`、`AnchorCanvas`、`set_anchors`）与作者入口（`ui.ref<T>()`、`ui.anchor_canvas()`、builder 的 `.anchor.*` / `.parent.anchor.*` 与 `.anchors(...)`），支持父级/兄弟关系、依赖排序、批量关系切换与冲突诊断；`showcase` 的 `anchors` 页面提供侧边栏停靠切换案例。当前边界是**画布需要确定尺寸，因此 `fill` 画布无法占据排列容器的剩余空间**（显式/百分比尺寸的画布可以嵌入排列容器），把画布放在布局根再用兄弟锚点表达剩余区域即可，见 [Anchors](docs/references/anchors.md)。它与浮层锚定定位器是两项独立能力。

## 🚀 快速开始

### 环境要求

- **Linux 桌面**（Wayland | Xorg）
- **编译器**：GCC ≥ 16 或 Clang ≥ 21（需 C++26 支持）
- **构建工具**：Meson、Ninja、CMake、pkg-config、Python 3
- **系统库**：OpenGL、OpenSSL ≥ 3.0

其余第三方依赖（spdlog、raylib、FreeType、HarfBuzz、FriBidi、utf8proc、toml++、nlohmann/json、Catch2、Box2D 等）均以 Git 子模块方式随仓库提供；SQLite 以官方 amalgamation 直接随源码分发（见 `third_party/sqlite3/README.md`），不需要系统安装。

### 在 Debian / Ubuntu 上准备环境

发行版自带的编译器通常达不到上面的版本要求，所以分四步：装系统库、装合规工具链、装 Meson、再配置构建。

**1. 系统库**（X11 后端，与 CI 同源）

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  git ca-certificates curl python3 python3-pip ninja-build pkg-config cmake \
  zlib1g-dev libssl-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev
```

Wayland 会话还需要 `libwayland-dev libxkbcommon-dev wayland-protocols libdecor-0-dev`。默认的 `linux_window_system=auto` 在 `WAYLAND_DISPLAY` 存在时会选 Wayland，缺这些开发包会直接配置失败；暂时不想装就显式加 `-Dlinux_window_system=x11`。

`zstd` 只有 SDK 打包与全量测试套件需要，构建框架本身不需要。

**2. C++26 工具链**

| 发行版 | 仓库内最高版本 | 做法 |
| --- | --- | --- |
| Debian 13 trixie | gcc-14 / clang-19 | 都低于要求 → 用 apt.llvm.org 装 clang-21，或改用 sid |
| Debian sid | gcc-16 / clang-22 | `sudo apt-get install -y g++-16`（或 `clang-21` / `clang-22`） |
| Ubuntu 24.04 LTS | gcc-14 / clang-18 | 都低于要求 → 用 apt.llvm.org 装 clang-21 |
| Ubuntu 25.10 questing | gcc-15 / clang-21 | GCC 不够、Clang 够 → `sudo apt-get install -y clang-21` |
| Ubuntu 26.04 resolute | gcc-16 / clang-21 | `sudo apt-get install -y g++-16` |

走 LLVM 官方仓库装 Clang 21：

```bash
curl -fsSL -o llvm.sh https://apt.llvm.org/llvm.sh && chmod +x llvm.sh
sudo ./llvm.sh 21
```

Clang 链接系统自带的 libstdc++；Debian 13 与 Ubuntu 24.04 提供的是 GCC 14 的版本，与 CI 的 Clang 任务相同。若发行版更旧（libstdc++ 低于 GCC 14），需要先补一个新的 `libstdc++-*-dev`，否则会缺 C++26 库特性。

**3. Meson**

CI 固定 1.7.2。Debian 13 自带的 1.7.0 与之接近；Ubuntu 24.04 自带的 1.3.2 差距较大，建议装一份与 CI 一致的（`pip install --user` 从 Ubuntu 24.04 起会被 PEP 668 拒绝，所以用 pipx）：

```bash
sudo apt-get install -y meson                                  # 自带版本够新时
sudo apt-get install -y pipx && pipx install "meson==1.7.2"    # 或：与 CI 对齐
```

**4. 配置与构建**

版本化的编译器不是默认的 `gcc` / `clang`，要把 `CC` / `CXX` 指过去；Meson 只在首次配置时读取它们。项目同时需要 C 与 C++ 编译器，因为随源码分发的 SQLite amalgamation 是 C 源码：

```bash
CC=clang-21 CXX=clang++-21 meson setup buildDir --wrap-mode=nodownload
# GCC 组合换成：CC=gcc-16 CXX=g++-16
meson compile -C buildDir
```

把 NandinaUI 作为 subproject 引入时同理：在消费项目**首次** `meson setup` 之前设好 `CC` / `CXX`，否则 Meson 会把发行版默认编译器固化进缓存。

### 在 Fedora 上准备环境

Fedora 比 Debian/Ubuntu 省事：仓库自带的编译器通常已经够新，Meson 也无须额外安装。

**1. 系统库**

```bash
sudo dnf install -y \
  git python3 ninja-build cmake pkgconf-pkg-config \
  zlib-ng-compat-devel openssl-devel \
  libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
  mesa-libGL-devel mesa-libGLU-devel
```

Wayland 会话还需要 `wayland-devel libxkbcommon-devel wayland-protocols-devel libdecor-devel mesa-libEGL-devel`；`linux_window_system=auto` 的取舍与上面 Debian 一节相同。

注意 zlib 自 Fedora 40 起已由 zlib-ng 提供，`zlib-devel` 不再存在，开发头文件在 `zlib-ng-compat-devel`。`zstd` 同样只服务 SDK 打包与全量测试套件。

**2. 工具链与 Meson**

| Fedora | 默认 gcc / g++ | 默认 clang | 自带 Meson |
| --- | --- | --- | --- |
| 43 | 15.3（低于要求） | 21.1 ✓ | 1.8.5 |
| 44 | 16.0 ✓ | 22.1 ✓ | 1.11.2 |
| rawhide | 16.2 ✓ | 23.1 ✓ | 1.12.1 |

Fedora 44 及以上直接用默认组合：

```bash
sudo dnf install -y gcc gcc-c++ meson
```

Fedora 43 的默认 GCC 只有 15，达不到要求，但它自带 Clang 21，改用 Clang：

```bash
sudo dnf install -y clang meson
```

Fedora 另外提供带版本号的兼容包（如 `gcc15`、`clang21`）。注意包名不带连字符，与 Debian 的 `gcc-15` / `clang-21` 写法相反。

**3. 配置与构建**

```bash
# Fedora 44+：默认 gcc / g++ 已合规
meson setup buildDir --wrap-mode=nodownload

# Fedora 43：默认 GCC 15 不够，显式指向 Clang
CC=clang CXX=clang++ meson setup buildDir --wrap-mode=nodownload

meson compile -C buildDir
```

与 Debian 一节同理：项目同时需要 C 与 C++ 编译器（随源码分发的 SQLite amalgamation 是 C 源码），作为 subproject 引入时 `CC` / `CXX` 要在消费项目**首次** `meson setup` 之前设好。

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
- [D5 资源构建、部署与运行时定位](docs/references/resource_build_and_deployment.md)：已冻结、未实施的资源模块边界、作用域契约与分批验收计划。
- [D6 作者 DSL、文本能力与内容测量](docs/references/authoring_dsl_and_content_measurement.md)：待审阅设计；先定 Label 作者能力与有限宽度测量，再决定布局语法糖、border 和 Section 的实现。

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
- [x] **D1 设计定案**：[统一输入、焦点与文本编辑](docs/references/focus_and_input.md) 已成为实施依据，取代旧 D1 工作稿；按交互意图区分指向定位、焦点导航和编辑输入，不按设备简单开关焦点环。
- [x] **D5 设计冻结**：资源语义统一由 `resources.toml` 声明；解耦、作用域、身份分批，保留包记录格式，安装规则与运行时定位同源。
- [x] **D6 设计归档**：[作者 DSL、文本能力与内容测量](docs/references/authoring_dsl_and_content_measurement.md) 已汇总作者反馈与 Slint/QML 对照；当前待审阅，不表示 API/布局改动已经实施。
- [ ] **D6 第一实施批（待审阅确认）**：先确认 Label 作者能力表、共同属性规则和有限宽度自然长高的测量契约，再以 showcase Label 页面及 `ComponentInfoTable` 验收。
- [ ] **D6 后续（依赖首批验收）**：根据实际重复模式加入布局 factory 子项重载与共享边框入口，再设计 ScrollView/布局/viewport 明确的 Section 组合；迁移旧写法，不要求保留快速演进期的旧 DSL。
- [ ] **D5 解耦首批**：nanres 脱离完整 UI，增加工具/SQLite 后端独立开关、关闭组合 CI 与依赖闭包门禁；保持既有 ID、包格式和扫描语义。
- [ ] **D5 后续批次**：先补管理器八个查询入口的作用域、后端发布代次与缓存隔离，再独立推进身份生成、声明式构建及部署定位；不等待全部后续事项即可交付解耦。
- [ ] **D1 第一阶段（下一步）**：鼠标/键盘焦点核心、统一提示策略、文本控件例外与焦点恢复；随后分阶段接文本输入会话、软键盘和触摸/笔/手柄。设计定案不表示这些能力已经实现。
- [ ] **Anchors 集成验收**：重复布局、布局系统误用、绘制/命中/语义几何一致性已由 authoring 测试覆盖，`showcase` 的 `anchors` 页面已提供侧边栏切换案例；剩余真实窗口的人工体验验收与 playground 同类案例。
- [ ] Anchors 稳定后再评估阶段 5 复合组件及浮层打开/关闭动效；两者不作为 anchors 的隐含前置。
- [ ] 给浮层补打开/关闭过渡与缓动曲线，并把 `motion` token、`reduced_motion` 偏好接到动画侧。
- [ ] 继续打磨 `butter`（本项目自研风格）；`fluent` / `material` 需要补上各自设计语言在几何、密度与状态层上的差异，目前只有配色与圆角尺度。
- [ ] 后续完善 Table/DataTable、Accordion、Sheet 等复合组件。
- [ ] 在模块边界和跨平台构建流程稳定后，重新评估 C++ Modules 与 CMake package 支持。

路线图会随着基础设施成熟度调整；Getting Started 只采用推荐 API，实验性能力会在组件文档中明确标注。

### 🧭 待拍板的设计决策

下面三项分别有代码现状或平台能力依据，但**仍处于讨论/候选阶段**；边界与待决问题见[待决设计草案](docs/references/reference_draft.md)（D2–D4）。D1 已定案，见上文[统一输入、焦点与文本编辑](docs/references/focus_and_input.md)。

D2 的完整讨论见[主题方案、作者入口与状态订阅](docs/references/theme_authoring_and_state.md)。
D5 已另行冻结，其分批计划见上文；设计冻结不改变当前平台支持范围或 D1 的下一步安排。

- [ ] **主题变化的订阅**：`ThemeManager` 只有 `revision()` 和给 `SceneTree` 用的观察者，外壳拿不到"外观变了"，因此 footer 里的亮/暗开关只能写、不能跟随外部变化。过程见 [story 010](stories/010-the-theme-signal-that-cannot-be-subscribed.md)。
- [ ] **D3 路由元数据**：真实路由身份已是页面类型；已接受将 `type` 改名为 `nav_group`，空值表示未分组且不画标题，非空值兼作标题；无旧字段兼容，未分组 route 按空键全局合并在首个未分组项的位置。实现与测试清单见[待决设计草案](docs/references/reference_draft.md) D3 节及 [story 011](stories/011-the-route-type-that-is-a-string.md)。
- [ ] **D4 跨平台平台层与渲染路线**：首要目标是 Linux、Windows、Web 应用级交付；SDL 3.4.0 起 GPU Renderer 有自定义 fragment shader 路径，但 WebGL 与 SDL_GPU 能力不能混为一谈。P1 还须验证 shadercross 等构建机工具的锁版/产物链、SDF/字体 atlas/规范键码回归；layer-shell host 行为契约留在未来阶段。见[待决设计草案](docs/references/reference_draft.md) D4 节。


## 外部依赖一览

| 类别 | 依赖 |
| --- | --- |
| 渲染 | raylib（GPU 后端，支持 JPG 等格式） |
| 文本 | FreeType、HarfBuzz、FriBidi、utf8proc |
| 数据 / 配置 | nlohmann/json、toml++、SQLite（vendored amalgamation，见 `third_party/sqlite3/`） |
| 其他 | spdlog（日志）、OpenSSL（资源签名）、Box2D（可选物理）、Catch2（测试） |


## 📄 许可证

本项目以 [MIT License](LICENSE) 开源。
