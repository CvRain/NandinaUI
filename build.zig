const std = @import("std");

pub fn build(b: *std.Build) void {
    // 默认 target。若系统 GCC >= 16，其 crt1.o 的 .sframe 节与 Zig LLD 不兼容，
    // 此处将默认 Linux x86_64 降级到 glibc 2.35 以绕过该问题。
    const host_tag = b.graph.host.result.os.tag;
    const host_arch = b.graph.host.result.cpu.arch;
    const default_query = if (host_tag == .linux and host_arch == .x86_64)
        std.Target.Query{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .gnu, .os_version_min = .{ .semver = std.SemanticVersion{ .major = 2, .minor = 35, .patch = 0 } } }
    else
        std.Target.Query{};
    const target = b.standardTargetOptions(.{ .default_target = default_query });
    const optimize = b.standardOptimizeOption(.{});

    // ── 第三方 C / C++ 依赖路径 ──────────────────────────────────────────────
    // 在 Linux 上使用系统路径；在 Windows 上使用 MSYS2 UCRT64 提供的库。
    const is_windows = host_tag == .windows;
    const sys_include = if (is_windows) "C:/msys64/ucrt64/include" else "/usr/include";
    const sys_lib = if (is_windows) "C:/msys64/ucrt64/lib" else "/usr/lib";
    const ft_include = if (is_windows) "C:/msys64/ucrt64/include/freetype2" else "/usr/include/freetype2";
    const hb_include = if (is_windows) "C:/msys64/ucrt64/include/harfbuzz" else "/usr/include/harfbuzz";
    const ft_cflag = if (is_windows) "-IC:/msys64/ucrt64/include/freetype2" else "-I/usr/include/freetype2";

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
    mod.addIncludePath(.{ .cwd_relative = ft_include });
    mod.addIncludePath(.{ .cwd_relative = hb_include });
    mod.addIncludePath(.{ .cwd_relative = sys_include });
    mod.addLibraryPath(.{ .cwd_relative = sys_lib });
    mod.linkSystemLibrary("freetype", .{ .search_strategy = .paths_first, .needed = true });
    mod.linkSystemLibrary("harfbuzz", .{ .search_strategy = .paths_first, .needed = true });

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
    sdl_backend_mod.addLibraryPath(.{ .cwd_relative = sys_lib });
    sdl_backend_mod.linkSystemLibrary("freetype", .{ .search_strategy = .paths_first, .needed = true });
    sdl_backend_mod.linkSystemLibrary("harfbuzz", .{ .search_strategy = .paths_first, .needed = true });

    // ── FreeType / HarfBuzz 传递依赖（MSYS2 / Linux 系统库）─────────────────
    const vcpkg_deps = [_][]const u8{
        "freetype",     "harfbuzz",
        "libpng16",     "zlib",
        "bz2",          "brotlidec",
        "brotlicommon", "graphite2",
        "glib-2.0",     "intl",
        "pcre2-8",
    };

    // ── 可执行目标：NandinaUI ─────────────────────────────────────────────────
    const exe = createExe(b, "NandinaUI", "src/main.zig", mod, sdl_backend_mod, sdl_lib, sdl_dep, ft_include, hb_include, sys_include, sys_lib, &vcpkg_deps, ft_cflag, is_windows, target, optimize);
    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    // ── 可执行目标：Showcase ─────────────────────────────────────────────────
    const showcase_exe = createExe(b, "showcase", "showcase/main.zig", mod, sdl_backend_mod, sdl_lib, sdl_dep, ft_include, hb_include, sys_include, sys_lib, &vcpkg_deps, ft_cflag, is_windows, target, optimize);
    b.installArtifact(showcase_exe);

    const showcase_step = b.step("showcase", "Run the component / capability showcase");
    const showcase_run = b.addRunArtifact(showcase_exe);
    showcase_step.dependOn(&showcase_run.step);
    showcase_run.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        showcase_run.addArgs(args);
    }

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

fn createExe(
    b: *std.Build,
    name: []const u8,
    root_src: []const u8,
    mod: *std.Build.Module,
    sdl_backend_mod: *std.Build.Module,
    sdl_lib: *std.Build.Step.Compile,
    sdl_dep: *std.Build.Dependency,
    ft_include: []const u8,
    hb_include: []const u8,
    sys_include: []const u8,
    sys_lib: []const u8,
    deps: []const []const u8,
    ft_cflag: []const u8,
    is_windows: bool,
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
    exe.root_module.addIncludePath(.{ .cwd_relative = ft_include });
    exe.root_module.addIncludePath(.{ .cwd_relative = hb_include });
    exe.root_module.addIncludePath(.{ .cwd_relative = sys_include });
    exe.root_module.addLibraryPath(.{ .cwd_relative = sys_lib });
    for (deps) |dep| {
        exe.root_module.linkSystemLibrary(dep, .{ .search_strategy = .paths_first, .needed = false });
    }
    if (is_windows) {
        exe.root_module.linkSystemLibrary("dwrite", .{});
        exe.root_module.linkSystemLibrary("rpcrt4", .{});
        exe.root_module.linkSystemLibrary("ole32", .{});
        exe.root_module.linkSystemLibrary("usp10", .{});
        exe.root_module.linkSystemLibrary("ws2_32", .{});
        exe.root_module.linkSystemLibrary("shlwapi", .{});
        exe.root_module.linkSystemLibrary("gdi32", .{});
        exe.root_module.linkSystemLibrary("stdc++", .{ .search_strategy = .paths_first, .needed = false });
        exe.root_module.linkSystemLibrary("gcc_s", .{ .search_strategy = .paths_first, .needed = false });
        exe.root_module.linkSystemLibrary("ssp", .{ .search_strategy = .paths_first, .needed = false });
        exe.root_module.linkSystemLibrary("ucrt", .{ .search_strategy = .paths_first, .needed = false });
    }
    exe.root_module.addCSourceFile(.{ .file = b.path("src/text/backends/ft_glyph.c"), .flags = &.{ft_cflag} });
    return exe;
}
