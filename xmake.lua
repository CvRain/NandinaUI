-- ============================================================
-- xmake.lua — NandinaUI
-- C++26 UI runtime + design-system-driven widget library
--
-- 包管理：系统包（pkg-config/cmake）优先。
--         ThorVG 使用 vcpkg 预安装产物（vcpkg_installed/）。
-- 编译：  默认 Clang，与 CMakePresets 中 linux-clang-* 一致。
--
-- 已知问题：xmake 并行编译含 C++ Module 的多 target 项目时，
--           会在多个 target 间并行编译同一模块的 BMI，导致
--           `module file .pcm not found` 竞态错误。
--           默认请使用单线程构建：xmake -j1 -y
--           或通过环境变量 XMAKE_JOBS=N 覆盖并行度。
-- ============================================================

set_project("NandinaUI")
set_version("0.1.0", {build = "%Y%m%d"})

-- ============================================================
-- 工具链
-- ============================================================
set_toolchains("clang")

-- ============================================================
-- 编译与语言标准
-- ============================================================
set_languages("c++26")
set_policy("build.c++.modules", true)
add_cxflags("-Wall", "-Wextra", "-Wpedantic")

-- ThorVG 头文件路径（所有需要 ThorVG 的模块共享）
-- 放在全局，确保模块依赖扫描阶段能找到 thorvg-1/thorvg.h
add_includedirs(path.join(os.projectdir(), "vcpkg_installed", "x64-linux", "include"))
add_defines("TVG_BUILD=1")
set_config("compile_commands", true)

-- ============================================================
-- 平台特殊处理
-- ============================================================
if is_plat("mingw") then
    add_ldflags("-Wl,--allow-multiple-definition", "-static")
end

-- ============================================================
-- 构建选项
-- ============================================================
option("build_tests")
    set_default(true)
    set_description("Build NandinaUI unit tests")
option_end()

option("build_showcase")
    set_default(true)
    set_description("Build NandinaUI showcase application")
option_end()

-- ============================================================
-- 第三方依赖
-- ============================================================

-- 系统包：xmake 会自动通过 pkg-config / cmake 发现
add_requires("spdlog")
add_requires("sdl3")
add_requires("freetype")
add_requires("harfbuzz")

if has_config("build_tests") then
    add_requires("gtest")
end

-- ThorVG：vcpkg 已预装产物位于 vcpkg_installed/x64-linux
-- 直接 add_includedirs / add_links，避免 vcpkg 从源码编译
local THORVG_BASE = path.join(os.projectdir(), "vcpkg_installed", "x64-linux")

-- ============================================================
-- 模块层（按依赖顺序：底层 → 高层）
-- ============================================================

-- ------------------------------------------------------------------
-- Foundation — 基础几何类型与通用原语
-- 依赖：spdlog
-- ------------------------------------------------------------------
target("nandina_foundation")
    set_kind("static")
    add_packages("spdlog")
    add_files("foundation/src/*.cppm", {public = true})
    add_files("foundation/src/log.cpp")

-- ------------------------------------------------------------------
-- Runtime — UI 运行时基础
-- 依赖：nandina_foundation, SDL3, ThorVG
-- ------------------------------------------------------------------
target("nandina_runtime")
    set_kind("static")
    add_deps("nandina_foundation")
    add_packages("sdl3")
    add_includedirs(path.join(THORVG_BASE, "include"))
    add_defines("TVG_BUILD=1")
    add_files("runtime/src/*.cppm", {public = true})
    add_files("runtime/src/nan_window.cpp")
    add_links(path.join(THORVG_BASE, "lib", "libthorvg-1.a"))

-- ------------------------------------------------------------------
-- Reactive — 响应式核心
-- 依赖：nandina_foundation
-- ------------------------------------------------------------------
target("nandina_reactive")
    set_kind("static")
    add_deps("nandina_foundation")
    add_files("reactive/src/*.cppm", {public = true})

-- ------------------------------------------------------------------
-- Render — 渲染抽象层（headeronly）
-- 依赖：nandina_foundation, nandina_runtime, ThorVG
-- ------------------------------------------------------------------
target("nandina_render")
    set_kind("headeronly")
    add_deps("nandina_foundation", "nandina_runtime")
    add_includedirs(path.join(THORVG_BASE, "include"))
    add_defines("TVG_BUILD=1")

-- ------------------------------------------------------------------
-- Layout — 布局系统
-- 依赖：nandina_foundation, nandina_runtime
-- ------------------------------------------------------------------
target("nandina_layout")
    set_kind("static")
    add_deps("nandina_foundation", "nandina_runtime")
    add_files("layout/src/*.cppm", {public = true})

-- ------------------------------------------------------------------
-- Text — 文本能力
-- 依赖：nandina_foundation, FreeType, HarfBuzz, ThorVG
-- ------------------------------------------------------------------
target("nandina_text")
    set_kind("static")
    add_deps("nandina_foundation")
    add_packages("freetype", "harfbuzz")
    add_includedirs(path.join(THORVG_BASE, "include"))
    add_defines("TVG_BUILD=1")
    add_links(path.join(THORVG_BASE, "lib", "libthorvg-1.a"))
    add_files("text/src/*.cppm", {public = true})
    add_files("text/src/nan_font.cpp")

-- ------------------------------------------------------------------
-- Theme — 主题与设计系统
-- 依赖：nandina_foundation, nandina_text
-- ------------------------------------------------------------------
target("nandina_theme")
    set_kind("static")
    add_deps("nandina_foundation", "nandina_text")
    add_files("theme/src/*.cppm", {public = true})

-- ------------------------------------------------------------------
-- Widgets — 组件库
-- 依赖：runtime/reactive/layout/text/theme/ThorVG
-- ------------------------------------------------------------------
target("nandina_widgets")
    set_kind("static")
    add_deps("nandina_runtime", "nandina_reactive", "nandina_layout",
             "nandina_text", "nandina_theme")
    add_includedirs(path.join(THORVG_BASE, "include"))
    add_defines("TVG_BUILD=1")
    add_links(path.join(THORVG_BASE, "lib", "libthorvg-1.a"))
    add_files("widgets/src/*.cppm", {public = true})
    add_files("widgets/src/nan_button.cpp")

-- ------------------------------------------------------------------
-- App — 应用开发层
-- 依赖：nandina_runtime/layout/theme/widgets, ThorVG
-- ------------------------------------------------------------------
target("nandina_app")
    set_kind("static")
    add_deps("nandina_runtime", "nandina_layout", "nandina_theme", "nandina_widgets")
    add_includedirs(path.join(THORVG_BASE, "include"))
    add_defines("TVG_BUILD=1")
    add_links(path.join(THORVG_BASE, "lib", "libthorvg-1.a"))
    add_files("app/src/*.cppm", {public = true})
    add_files("app/src/nan_var_impl.cpp")
    add_files("app/src/nan_computed_var_impl.cpp")

-- ------------------------------------------------------------------
-- Bindings — 多语言绑定（headeronly）
-- ------------------------------------------------------------------
target("nandina_bindings")
    set_kind("headeronly")
    add_deps("nandina_app")

-- ============================================================
-- Showcase — 展示与验证应用
-- ============================================================
if has_config("build_showcase") then
target("nandina_showcase")
    set_kind("binary")
    add_deps("nandina_app", "nandina_layout", "nandina_widgets")
    add_includedirs(path.join(THORVG_BASE, "include"))
    add_defines("TVG_BUILD=1")
    add_links(path.join(THORVG_BASE, "lib", "libthorvg-1.a"))
    add_files("showcase/main.cpp")
    add_files("showcase/*.cppm", {public = true})
    add_files("showcase/pages/*.cppm", {public = true})
    if is_plat("windows") then
        add_ldflags("/SUBSYSTEM:WINDOWS")
    end
end

-- ============================================================
-- Tests — 单元测试
-- ============================================================
if has_config("build_tests") then
    function _add_nandina_test(name, sources, deps, modules)
        target("test_" .. name)
            set_kind("binary")
            add_packages("gtest")
            add_links("gtest", "gtest_main")
            add_deps(table.unpack(deps))
            add_files(sources)
            if modules then
                add_files(modules, {public = true})
            end
    end

    _add_nandina_test("foundation_log",
        {"tests/foundation/test_log.cpp"}, {"nandina_foundation"})
    _add_nandina_test("foundation_color",
        {"tests/foundation/test_color.cpp"}, {"nandina_foundation"})
    _add_nandina_test("foundation_geometry",
        {"tests/foundation/test_geometry.cpp"}, {"nandina_foundation"})
    _add_nandina_test("foundation_constraints",
        {"tests/foundation/test_constraints.cpp"}, {"nandina_foundation"})
    _add_nandina_test("runtime_event",
        {"tests/runtime/test_event.cpp"}, {"nandina_runtime"})
    _add_nandina_test("runtime_bounds_hit_test",
        {"tests/runtime/test_bounds_hit_test.cpp"}, {"nandina_runtime"})
    _add_nandina_test("reactive_core",
        {"tests/reactive/test_reactive.cpp"}, {"nandina_reactive"})
    _add_nandina_test("animation",
        {"tests/reactive/test_animation.cpp"}, {"nandina_reactive"})
    _add_nandina_test("layout_core",
        {"tests/layout/test_layout_core.cpp"}, {"nandina_layout"})
    _add_nandina_test("positioned",
        {"tests/layout/test_positioned.cpp"}, {"nandina_layout"})
    _add_nandina_test("theme_types",
        {"tests/theme/test_theme_types.cpp"}, {"nandina_theme"})
    _add_nandina_test("theme_manager",
        {"tests/theme/test_theme_manager.cpp"}, {"nandina_theme"})
    _add_nandina_test("text_nan_font",
        {"tests/text/test_nan_font.cpp"}, {"nandina_text"})
    _add_nandina_test("app_authoring",
        {"tests/app/test_app_authoring.cpp"}, {"nandina_app"})
    _add_nandina_test("app_authoring_qol",
        {"tests/app/test_app_authoring_qol.cpp"}, {"nandina_app"})
    _add_nandina_test("app_navigation",
        {"tests/app/test_app_navigation.cpp"}, {"nandina_app"})
    _add_nandina_test("app_var",
        {"tests/app/test_app_var.cpp"}, {"nandina_app"})
    _add_nandina_test("widgets_button",
        {"tests/widgets/test_widgets_button.cpp"}, {"nandina_widgets"})
    _add_nandina_test("widgets_checkbox",
        {"tests/widgets/test_widgets_checkbox.cpp"}, {"nandina_widgets"})
    _add_nandina_test("widgets_label",
        {"tests/widgets/test_widgets_label.cpp"}, {"nandina_widgets"})
    _add_nandina_test("widgets_text",
        {"tests/widgets/test_widgets_text.cpp"}, {"nandina_widgets"})
    _add_nandina_test("widgets_tag",
        {"tests/widgets/test_widgets_tag.cpp"}, {"nandina_widgets"})
    _add_nandina_test("widgets_panel",
        {"tests/widgets/test_widgets_panel.cpp"}, {"nandina_widgets"})
    _add_nandina_test("widgets_field",
        {"tests/widgets/test_widgets_field.cpp"}, {"nandina_widgets"})
    _add_nandina_test("widgets_text_field",
        {"tests/widgets/test_widgets_text_field.cpp"}, {"nandina_widgets"})
    _add_nandina_test("widgets_progressbar",
        {"tests/widgets/test_widgets_progressbar.cpp"}, {"nandina_widgets"})
    _add_nandina_test("showcase_layout",
        {"tests/showcase/test_showcase_layout.cpp"},
        {"nandina_app", "nandina_layout", "nandina_widgets", "nandina_text"},
        {"showcase/nan_showcase.cppm", "showcase/main_page.cppm",
         "showcase/pages/sandbox_page.cppm", "showcase/pages/button.cppm",
         "showcase/pages/checkbox.cppm", "showcase/pages/forms_page.cppm"})
end