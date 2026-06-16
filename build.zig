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

    // ── 可执行目标 ───────────────────────────────────────────────────────────
    const exe = createExe(b, "NandinaUI", "src/main.zig", mod, sdl_backend_mod, sdl_lib, sdl_dep, vcpkg_include, vcpkg_lib, host_tag, target, optimize);
    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);

    const showcase_exe = createExe(b, "showcase", "showcase/main.zig", mod, sdl_backend_mod, sdl_lib, sdl_dep, vcpkg_include, vcpkg_lib, host_tag, target, optimize);
    b.installArtifact(showcase_exe);

    const showcase_step = b.step("showcase", "Run the component / capability showcase");
    const showcase_run = b.addRunArtifact(showcase_exe);
    showcase_step.dependOn(&showcase_run.step);
    showcase_run.step.dependOn(b.getInstallStep());
    if (b.args) |args| showcase_run.addArgs(args);

    // ── 测试 ────────────────────────────────────────────────────────────────
    const mod_tests = b.addTest(.{ .root_module = mod });
    const run_mod_tests = b.addRunArtifact(mod_tests);
    const exe_tests = b.addTest(.{ .root_module = exe.root_module });
    const run_exe_tests = b.addRunArtifact(exe_tests);
    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_mod_tests.step);
    test_step.dependOn(&run_exe_tests.step);
    const showcase_tests = b.addTest(.{ .root_module = showcase_exe.root_module });
    const run_showcase_tests = b.addRunArtifact(showcase_tests);
    test_step.dependOn(&run_showcase_tests.step);
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
