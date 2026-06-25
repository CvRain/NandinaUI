const std = @import("std");

pub fn build(b: *std.Build) void {
    // ── Target ────────────────────────────────────────────────────────────────
    const host_tag = b.graph.host.result.os.tag;
    const host_arch = b.graph.host.result.cpu.arch;
    const default_query = if (host_tag == .linux and host_arch == .x86_64)
        std.Target.Query{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .gnu }
    else
        std.Target.Query{};
    const target = b.standardTargetOptions(.{ .default_target = default_query });
    const optimize = b.standardOptimizeOption(.{});

    // ── vcpkg 路径 ────────────────────────────────────────────────────────────
    // 项目内置 vcpkg（./vcpkg/），安装目录为 ./vcpkg_installed/<triple t>/
    const vcpkg_triplet = if (host_tag == .windows) "x64-windows" else "x64-linux";
    const vcpkg_installed = b.pathJoin(&.{ ".", "vcpkg_installed" });
    const vcpkg_dir = b.pathJoin(&.{ vcpkg_installed, vcpkg_triplet });
    const vcpkg_include = b.pathJoin(&.{ vcpkg_dir, "include" });
    const vcpkg_lib = b.pathJoin(&.{ vcpkg_dir, "lib" });
    const vcpkg_pkgconfig = b.pathJoin(&.{ vcpkg_lib, "pkgconfig" });

    // 设置 PKG_CONFIG_PATH，使 Zig 的 linkSystemLibrary 使用 pkg-config 时
    // 能找到 vcpkg 安装的库。
    b.graph.environ_map.put("PKG_CONFIG_PATH", vcpkg_pkgconfig) catch @panic("OOM");

    // ── SDL3 ─────────────────────────────────────────────────────────────────
    const sdl_dep = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
        .preferred_linkage = .static,
        .pic = true,
    });
    const sdl_lib = sdl_dep.artifact("SDL3");

    // ── NandinaUI 库模块 ────────────────────────────────────────────────────
    const mod = b.addModule("NandinaUI", .{
        .root_source_file = b.path("src/root.zig"),
    });
    mod.resolved_target = target;
    // 通过 vcpkg pkg-config 链接字体库
    linkVcpkgModule(b, mod, vcpkg_include, vcpkg_lib);
    linkThorvgModule(b, mod, vcpkg_include, vcpkg_lib);

    // ── SDL3 后端模块 ───────────────────────────────────────────────────────
    const sdl_backend_mod = b.createModule(.{
        .root_source_file = b.path("src/runtime/backends/sdl3.zig"),
        .imports = &.{
            .{ .name = "NandinaUI", .module = mod },
        },
    });
    sdl_backend_mod.resolved_target = target;
    sdl_backend_mod.linkLibrary(sdl_lib);
    sdl_backend_mod.addIncludePath(sdl_dep.path("include"));
    sdl_backend_mod.addIncludePath(sdl_dep.path("src"));
    linkVcpkgModule(b, sdl_backend_mod, vcpkg_include, vcpkg_lib);
    linkThorvgModule(b, sdl_backend_mod, vcpkg_include, vcpkg_lib);

    // ── Zig 前端层模块 ───────────────────────────────────────────────────────
    // frontend/zig/nandina.zig：直通 Core 的「Zig 前端绑定」，收敛再导出 + 全包 App。
    const frontend_mod = b.createModule(.{
        .root_source_file = b.path("frontend/zig/nandina.zig"),
        .imports = &.{
            .{ .name = "NandinaUI", .module = mod },
            .{ .name = "sdl_backend", .module = sdl_backend_mod },
        },
    });
    frontend_mod.resolved_target = target;

    // ── 可执行目标 ───────────────────────────────────────────────────────────
    // GUI 展示程序：Zig 前端 showcase（消费 frontend/zig/nandina.zig）
    const exe = createExe(b, "NandinaUI", "showcase/zig/main.zig", mod, sdl_backend_mod, sdl_lib, sdl_dep, vcpkg_include, vcpkg_lib, host_tag, target, optimize);
    exe.root_module.addImport("nandina", frontend_mod);
    b.installArtifact(exe);

    const run_step = b.step("run", "构建并启动主程序（SDL3 可视化界面）");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);

    // ── ABI 静态库 ─────────────────────────────────────────────────────────
    // C ABI 导出层，不纳入 root.zig 聚合导出，单独编译为静态库供 C/C++ 等绑定使用。
    const abi_mod = b.createModule(.{
        .root_source_file = b.path("src/abi/nandina_abi.zig"),
        .target = target,
        .optimize = optimize,
        .imports = &.{
            .{ .name = "NandinaUI", .module = mod },
            .{ .name = "sdl_backend", .module = sdl_backend_mod },
        },
        .link_libc = true,
    });
    linkVcpkgModule(b, abi_mod, vcpkg_include, vcpkg_lib);
    linkThorvgModule(b, abi_mod, vcpkg_include, vcpkg_lib);
    // 全包窗口入口 nandina_app_* 内部用 SDL3，需链接 SDL 与字体 C 包装。
    abi_mod.linkLibrary(sdl_lib);
    abi_mod.addIncludePath(sdl_dep.path("include"));
    abi_mod.addIncludePath(sdl_dep.path("src"));
    abi_mod.addCSourceFile(.{ .file = b.path("src/text/backends/ft_glyph.c"), .flags = &.{
        b.fmt("-I{s}/freetype2", .{vcpkg_include}),
    } });
    const abi_lib = b.addLibrary(.{
        .name = "nandina_abi",
        .root_module = abi_mod,
    });
    b.installArtifact(abi_lib);

    // 安装 C ABI 头文件
    b.installFile("src/abi/nandina_abi.h", "include/nandina_abi.h");

    // ── C++ 前端 showcase ─────────────────────────────────────────────────────
    // 仅依赖 nandina_abi.h + libnandina_abi.a，演示 C++ 前端（frontend/cpp）。
    // 用 Zig 自带 clang 编译，保证单一 `zig build` 入口。
    const cpp_exe = b.addExecutable(.{
        .name = "NandinaUI-cpp",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    cpp_exe.root_module.link_libcpp = true;
    cpp_exe.root_module.addCSourceFile(.{
        .file = b.path("showcase/cpp/main.cpp"),
        .flags = &.{ "-std=c++20", "-fno-sanitize=undefined" },
    });
    cpp_exe.root_module.addIncludePath(b.path("frontend/cpp/include"));
    cpp_exe.root_module.addIncludePath(b.path("src/abi"));
    // 链接 ABI 静态库及其全部传递依赖。
    cpp_exe.root_module.linkLibrary(abi_lib);
    cpp_exe.root_module.linkLibrary(sdl_lib);
    linkVcpkgModule(b, cpp_exe.root_module, vcpkg_include, vcpkg_lib);
    linkThorvgModule(b, cpp_exe.root_module, vcpkg_include, vcpkg_lib);
    b.installArtifact(cpp_exe);

    const run_cpp_step = b.step("run-cpp", "构建并运行 C++ 前端 showcase");
    const run_cpp_cmd = b.addRunArtifact(cpp_exe);
    run_cpp_step.dependOn(&run_cpp_cmd.step);
    run_cpp_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cpp_cmd.addArgs(args);

    // ── Zig 模块测试 ────────────────────────────────────────────────────────
    const mod_tests = b.addTest(.{ .root_module = mod });
    const run_mod_tests = b.addRunArtifact(mod_tests);
    const exe_tests = b.addTest(.{ .root_module = exe.root_module });
    const run_exe_tests = b.addRunArtifact(exe_tests);
    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_mod_tests.step);
    test_step.dependOn(&run_exe_tests.step);
    // 旧命令行 showcase 已移除，其逻辑验证职责由单元测试接管
    // ABI 层测试（含“C 调用约定回调派发不崩溃”回归）。
    const abi_tests = b.addTest(.{ .root_module = abi_mod });
    const run_abi_tests = b.addRunArtifact(abi_tests);
    test_step.dependOn(&run_abi_tests.step);
}

/// 为模块添加 vcpkg 的 include 路径和字体库链接（通过 pkg-config）。
fn linkVcpkgModule(b: *std.Build, m: *std.Build.Module, vcpkg_include: []const u8, vcpkg_lib: []const u8) void {
    m.addIncludePath(.{ .cwd_relative = vcpkg_include });
    m.addIncludePath(.{ .cwd_relative = b.pathJoin(&.{ vcpkg_include, "freetype2" }) });
    m.addIncludePath(.{ .cwd_relative = b.pathJoin(&.{ vcpkg_include, "harfbuzz" }) });
    m.addLibraryPath(.{ .cwd_relative = vcpkg_lib });
    m.linkSystemLibrary("freetype2", .{ .use_pkg_config = .yes, .preferred_link_mode = .static, .needed = true });
    m.linkSystemLibrary("harfbuzz", .{ .use_pkg_config = .yes, .preferred_link_mode = .static, .needed = true });
}

/// 为模块添加 ThorVG 的 include 路径和库链接。
fn linkThorvgModule(b: *std.Build, m: *std.Build.Module, vcpkg_include: []const u8, vcpkg_lib: []const u8) void {
    _ = vcpkg_include;
    _ = vcpkg_lib;

    // 使用 Clang 编译的 ThorVG 共享库（兼容 Zig 的链接器）
    // 路径：build/thorvg-clang/install/
    const thorvg_clang = b.pathJoin(&.{ "build", "thorvg-clang", "install" });
    const thorvg_lib = b.pathJoin(&.{ thorvg_clang, "lib" });
    m.addIncludePath(.{ .cwd_relative = b.pathJoin(&.{ thorvg_clang, "include", "thorvg-1" }) });
    m.addLibraryPath(.{ .cwd_relative = thorvg_lib });
    m.addRPath(.{ .cwd_relative = thorvg_lib });
    // 用共享库的绝对路径直接链接，而非 `-lthorvg-1`。
    // 否则若 vcpkg 重新安装生成了 `vcpkg_installed/.../libthorvg-1.a`（gcc 静态库，
    // 依赖未被链接的 libstdc++ 符号），它会因 `-L` 搜索顺序在前而抢先匹配，
    // 导致大量 std::* 未定义符号链接错误。指定 `.so` 文件路径可彻底规避该歧义。
    m.addObjectFile(.{ .cwd_relative = b.pathJoin(&.{ thorvg_lib, "libthorvg-1.so" }) });
    // __isoc23_strtol 兼容桩已由 ft_glyph.c 提供（weak alias → strtol）
}

/// 创建可执行目标，集成 SDL3 + vcpkg 字体库。
fn createExe(
    b: *std.Build,
    name: []const u8,
    root_src: []const u8,
    mod: *std.Build.Module,
    sdl_backend_mod: *std.Build.Module,
    sdl_lib: *std.Build.Step.Compile,
    sdl_dep: *std.Build.Dependency,
    vcpkg_include: []const u8,
    vcpkg_lib: []const u8,
    host_tag: std.Target.Os.Tag,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) *std.Build.Step.Compile {
    const exe = b.addExecutable(.{
        .name = name,
        .root_module = b.createModule(.{
            .root_source_file = b.path(root_src),
            .target = target,
            .optimize = optimize,
            .imports = &.{
                .{ .name = "NandinaUI", .module = mod },
                .{ .name = "sdl_backend", .module = sdl_backend_mod },
            },
        }),
    });
    exe.root_module.linkLibrary(sdl_lib);
    exe.root_module.addIncludePath(sdl_dep.path("include"));
    exe.root_module.addIncludePath(sdl_dep.path("src"));
    // vcpkg 路径与字体库链接
    linkVcpkgModule(b, exe.root_module, vcpkg_include, vcpkg_lib);
    linkThorvgModule(b, exe.root_module, vcpkg_include, vcpkg_lib);
    // FreeType glyph 渲染 C 包装器
    exe.root_module.addCSourceFile(.{ .file = b.path("src/text/backends/ft_glyph.c"), .flags = &.{
        b.fmt("-I{s}/freetype2", .{vcpkg_include}),
    } });
    // Windows 特有系统库
    if (host_tag == .windows) {
        exe.root_module.linkSystemLibrary("dwrite", .{});
        exe.root_module.linkSystemLibrary("rpcrt4", .{});
        exe.root_module.linkSystemLibrary("ole32", .{});
        exe.root_module.linkSystemLibrary("usp10", .{});
        exe.root_module.linkSystemLibrary("ws2_32", .{});
        exe.root_module.linkSystemLibrary("shlwapi", .{});
        exe.root_module.linkSystemLibrary("gdi32", .{});
    }
    return exe;
}
