# 构建系统与 Modules 范围

本文记录 NandinaUI 当前对 C++20 Modules 与 CMake 的支持边界。它们不是被否定的方向，而是在公共 API、模块依赖和跨平台构建流程稳定之前暂缓进入主线。

## 当前主线

NandinaUI 当前使用 Meson + Ninja 构建，公共源码以普通头文件和实现文件分发。应用通过 Meson subproject 引入框架，框架自身负责构建第三方依赖适配层。**第三方依赖的获取方式（目标为 `wrap-git` + 默认 vendored 的三态开关 + `min`/`pin` 版本规则，尚未实施）由[第三方依赖策略](dependency_policy.md)规定**；那条策略说的是"Meson 侧依赖从哪来"，不改变本文"CMake 消费端暂缓"的结论，未来 CMake 消费适配也须复用同一份来源/版本声明。

实施顺序以[项目进度与下一步](project_status.md)为准：SDL3 单项 wrap/CI/离线 SDK 先交付，
其余依赖逐项迁移；生产 SDF 不等待全部迁移。CMake wrap 的 dependency 回退机制与 target 映射
按[依赖策略](dependency_policy.md)先实测，不引入未经验证的通用解析器。

这里的“支持 CMake”需要区分两件事：仓库可以在第三方依赖的构建过程中调用 CMake，但当前还没有提供可被 CMake `find_package()` 或 `add_subdirectory()` 直接消费的稳定 NandinaUI package target。

## 为什么暂不迁移 Modules

C++20 Modules 在 Clang、GCC 和较新的 MSVC 中已经可以用于实验，但它仍然要求编译器与构建系统共同完成依赖扫描、BMI 生成、缓存失效和构建排序。它不是把 `#include` 替换为 `import` 就能保持原有构建流程的机械重命名。

当前项目暂缓迁移有几个具体原因：

1. `foundation`、`scene`、`text`、`widget` 等边界仍在收敛，源码中还有已经记录的跨层依赖债务；过早导出模块会把临时边界固化成公共契约。
2. Raylib、FreeType、HarfBuzz、FriBidi、SQLite、Catch2 等第三方库仍以头文件或传统链接方式接入，不能假定它们拥有可移植的模块接口。
3. 每个受支持的工具链都要各自验证 BMI 与标准库模块行为，缓存不能跨工具链复用；当前支持面是 Linux 上的 GCC 与 Clang，Windows 所用的 clang-cl + MSVC STL 尚未纳入支持承诺。
4. Modules 可以减少重复解析，但首次构建仍要生成 BMI；对于 playground，先复用已经构建好的 NandinaUI 库通常比迁移全部源码更直接、更稳定。

因此，当前主线继续使用 include，并把 Modules 留在独立实验目标中。未来如果要验证，优先从依赖少、边界稳定的 `foundation` 小模块开始，不影响主构建和应用教程。

## 为什么暂不提供 CMake package

CMake 支持不仅是增加一个 `CMakeLists.txt`。稳定的消费者体验还需要定义：

- 导出的 targets、include 路径、编译特性和平台后端选项；
- 第三方依赖的传递关系、静态/动态链接策略和安装布局；
- Linux 上不同编译器与窗口后端组合下的资源、窗口和测试行为；
- 与 Meson subproject 相同的版本约束、可选功能和 CI 覆盖。

在这些规则尚未稳定前，维护两套完整的一等构建入口会让问题分散到两个构建图中，也会增加 playground、示例和 CI 的验证矩阵。其中"第三方依赖的传递关系"这一条现在有了具体答案（[依赖策略](dependency_policy.md)：每个依赖一份权威来源），但**只覆盖 Meson 侧**；CMake 侧一旦启动，必须复用同一份来源声明，而不是另开一套依赖获取。当前优先保证 Meson 源码分发和 Wayland / X11 两个窗口后端的行为，CMake 消费支持进入后续里程碑。

## 重新评估条件

[D5 资源构建与部署](resource_build_and_deployment.md) 已冻结设计、尚未实施。它先在 Meson
收口工具解耦、可选依赖和资源目标关系；资源语义仅由 `resources.toml` 声明，未来 CMake 适配
复用同一构建核心和声明，不另设资源列表/key/别名 DSL。该设计不表示当前已经提供 CMake target。

工具开/运行时 SQLite 开/vendored 是首批默认目标；新的关闭组合必须有 CI 实际构建与消费者
验证。纯 nanres 路径要在未提供 UI/窗口依赖的环境中配置构建成功，不能只检查最终链接符号。
运行时作用域和生成身份分别交付，不阻塞物理解耦。安装规则与定位同源、主机工具与目标程序
分离的验收按 D5 §12 执行，不能以 Linux 本机通过替代交叉编译或新平台支持。

满足以下条件后，再重新评估 Modules 和 CMake：

- 模块依赖规则中的已知偏离明显减少，公共头文件边界稳定；
- Select、Dialog 及菜单类浮层完成统一设施迁移，应用层 API 进入较稳定阶段；
- 至少有一套 Linux 的 Clang/Ninja 与一套 GCC/Ninja 构建验证；
- CMake package 的 target、依赖和安装布局可以写成稳定文档并纳入 CI。

在此之前，相关需求应记录为 planned，而不是通过临时脚本模拟成 supported。
