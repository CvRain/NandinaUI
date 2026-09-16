# 构建系统与 Modules 范围

本文记录 NandinaUI 当前对 C++20 Modules 与 CMake 的支持边界。它们不是被否定的方向，而是在公共 API、模块依赖和跨平台构建流程稳定之前暂缓进入主线。

## 当前主线

NandinaUI 当前使用 Meson + Ninja 构建，公共源码以普通头文件和实现文件分发。应用通过 Meson subproject 引入框架，框架自身负责构建第三方依赖适配层。

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

在这些规则尚未稳定前，维护两套完整的一等构建入口会让问题分散到两个构建图中，也会增加 playground、示例和 CI 的验证矩阵。当前优先保证 Meson 源码分发和 Wayland / X11 两个窗口后端的行为，CMake 消费支持进入后续里程碑。

## 重新评估条件

满足以下条件后，再重新评估 Modules 和 CMake：

- 模块依赖规则中的已知偏离明显减少，公共头文件边界稳定；
- Select、Dialog 及菜单类浮层完成统一设施迁移，应用层 API 进入较稳定阶段；
- 至少有一套 Linux 的 Clang/Ninja 与一套 GCC/Ninja 构建验证；
- CMake package 的 target、依赖和安装布局可以写成稳定文档并纳入 CI。

在此之前，相关需求应记录为 planned，而不是通过临时脚本模拟成 supported。
