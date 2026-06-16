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

    // ── 第三方依赖 ─────────────────────────────────────────────────────────────
    // SDL3：窗口管理、事件、渲染目标呈现。
    // 使用 castholm/SDL zon 包，编译 SDL3 静态库。
    const sdl_dep = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
        .preferred_linkage = .static,
        .pic = true,
    });
    const sdl_lib = sdl_dep.artifact("SDL3");

    // ── 库模块 ──────────────────────────────────────────────────────────────────
    // NandinaUI 核心库模块。
    // 注意：模块本身不链接 SDL3（保持核心纯逻辑），使用 SDL3 的可执行目标额外链接。
    const mod = b.addModule("NandinaUI", .{
        .root_source_file = b.path("src/root.zig"),
        .target = target,
    });
    // mod 需要 FreeType/HarfBuzz include 路径（hb_ft.zig 通过 @cImport 引用）
    mod.addIncludePath(.{ .cwd_relative = "/usr/include/freetype2" });
    mod.addIncludePath(.{ .cwd_relative = "/usr/include/harfbuzz" });
    mod.addLibraryPath(.{ .cwd_relative = "/usr/lib" });
    mod.linkSystemLibrary("freetype", .{ .search_strategy = .paths_first, .needed = true });
    mod.linkSystemLibrary("harfbuzz", .{ .search_strategy = .paths_first, .needed = true });

    // ── SDL3 后端模块 ──────────────────────────────────────────────────────────
    // 独立的模块，封装 SDL3 窗口后端。需要 SDL3 的可执行目标导入此模块。
    const sdl_backend_mod = b.createModule(.{
        .root_source_file = b.path("src/runtime/backends/sdl3.zig"),
        .target = target,
        .imports = &.{
            .{ .name = "NandinaUI", .module = mod },
        },
    });
    sdl_backend_mod.linkLibrary(sdl_lib);
    sdl_backend_mod.addIncludePath(sdl_dep.path("include"));
    sdl_backend_mod.addIncludePath(sdl_dep.path("src"));
    // 字体后端 include 路径在 mod 上统一添加
    sdl_backend_mod.addLibraryPath(.{ .cwd_relative = "/usr/lib" });
    sdl_backend_mod.linkSystemLibrary("freetype", .{ .search_strategy = .paths_first, .needed = true });
    sdl_backend_mod.linkSystemLibrary("harfbuzz", .{ .search_strategy = .paths_first, .needed = true });

    // ── 可执行目标 ─────────────────────────────────────────────────────────────
    const exe = b.addExecutable(.{
        .name = "NandinaUI",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
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
    // 字体后端需要的系统库
    exe.root_module.addIncludePath(.{ .cwd_relative = "/usr/include/freetype2" });
    exe.root_module.addIncludePath(.{ .cwd_relative = "/usr/include/harfbuzz" });
    exe.root_module.addLibraryPath(.{ .cwd_relative = "/usr/lib" });
    exe.root_module.linkSystemLibrary("freetype", .{ .search_strategy = .paths_first, .needed = true });
    exe.root_module.linkSystemLibrary("harfbuzz", .{ .search_strategy = .paths_first, .needed = true });
    // FreeType glyph 渲染 C 包装器（Zig @cImport 无法直接访问 FT_FaceRec_ 字段）
    // 编译 FreeType glyph 渲染 C 包装器（只加一次，避免重复符号）
    exe.root_module.addCSourceFile(.{ .file = b.path("src/text/backends/ft_glyph.c"), .flags = &.{"-I/usr/include/freetype2"} });
    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    // ── Showcase 子项目 ───────────────────────────────────────────────────────
    // 独立可执行目标：组件 / 能力演示运行器。既用于展示库已落地的能力，
    // 也方便开发时实际跑一下运行效果。通过 `zig build showcase` 运行。
    const showcase_exe = b.addExecutable(.{
        .name = "showcase",
        .root_module = b.createModule(.{
            .root_source_file = b.path("showcase/main.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{
                .{ .name = "NandinaUI", .module = mod },
                .{ .name = "sdl_backend", .module = sdl_backend_mod },
            },
        }),
    });
    showcase_exe.root_module.linkLibrary(sdl_lib);
    showcase_exe.root_module.addIncludePath(sdl_dep.path("include"));
    showcase_exe.root_module.addIncludePath(sdl_dep.path("src"));
    // 字体后端
    showcase_exe.root_module.addIncludePath(.{ .cwd_relative = "/usr/include/freetype2" });
    showcase_exe.root_module.addIncludePath(.{ .cwd_relative = "/usr/include/harfbuzz" });
    showcase_exe.root_module.addLibraryPath(.{ .cwd_relative = "/usr/lib" });
    showcase_exe.root_module.linkSystemLibrary("freetype", .{ .search_strategy = .paths_first, .needed = true });
    showcase_exe.root_module.linkSystemLibrary("harfbuzz", .{ .search_strategy = .paths_first, .needed = true });
    showcase_exe.root_module.addCSourceFile(.{ .file = b.path("src/text/backends/ft_glyph.c"), .flags = &.{"-I/usr/include/freetype2"} });

    b.installArtifact(showcase_exe);

    const showcase_step = b.step("showcase", "Run the component / capability showcase");
    const showcase_run = b.addRunArtifact(showcase_exe);
    showcase_step.dependOn(&showcase_run.step);
    showcase_run.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        showcase_run.addArgs(args);
    }

    // Creates an executable that will run `test` blocks from the provided module.
    // Here `mod` needs to define a target, which is why earlier we made sure to
    // set the releative field.
    const mod_tests = b.addTest(.{
        .root_module = mod,
    });

    // A run step that will run the test executable.
    const run_mod_tests = b.addRunArtifact(mod_tests);

    // Creates an executable that will run `test` blocks from the executable's
    // root module. Note that test executables only test one module at a time,
    // hence why we have to create two separate ones.
    const exe_tests = b.addTest(.{
        .root_module = exe.root_module,
    });

    // A run step that will run the second test executable.
    const run_exe_tests = b.addRunArtifact(exe_tests);

    // A top level step for running all tests. dependOn can be called multiple
    // times and since the two run steps do not depend on one another, this will
    // make the two of them run in parallel.
    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_mod_tests.step);
    test_step.dependOn(&run_exe_tests.step);

    // Showcase 的 test 块（注册表完整性检查等）也纳入 `zig build test`。
    const showcase_tests = b.addTest(.{
        .root_module = showcase_exe.root_module,
    });
    const run_showcase_tests = b.addRunArtifact(showcase_tests);
    test_step.dependOn(&run_showcase_tests.step);

    // Just like flags, top level steps are also listed in the `--help` menu.
    //
    // The Zig build system is entirely implemented in userland, which means
    // that it cannot hook into private compiler APIs. All compilation work
    // orchestrated by the build system will result in other Zig compiler
    // subcommands being invoked with the right flags defined. You can observe
    // these invocations when one fails (or you pass a flag to increase
    // verbosity) to validate assumptions and diagnose problems.
    //
    // Lastly, the Zig build system is relatively simple and self-contained,
    // and reading its source code will allow you to master it.
}
