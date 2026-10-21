project(
    'NandinaUI',
    'cpp',
    # `c` 只服务于 SQLite 的 vendored amalgamation（third_party/sqlite3/sqlite3.c）：
    # 它是 C 源码，必须由 C 编译器编译。任何提供 C++ 编译器的工具链都同时提供 C 编译器。
    'c',
    version: '0.1.0-alpha.1',
    default_options: ['default_library=static', 'cpp_std=c++26'],
)

fs = import('fs')
python = import('python').find_installation('python3')

embed_binary = executable(
    'nandina_embed_binary',
    'tools/embed_binary.cpp',
    native: true,
)

# 使用 cmake subproject 集成第三方库
cmake = import('cmake')

# 配置 spdlog 的 CMake选项
spdlog_options = cmake.subproject_options()
spdlog_options.add_cmake_defines({})
spdlog_options.set_install(false)

# 配置 utf8proc 的 CMake 选项
utf8proc_options = cmake.subproject_options()
utf8proc_options.add_cmake_defines({
    'BUILD_SHARED_LIBS': 'OFF',
    'UTF8PROC_INSTALL': 'OFF',
    'UTF8PROC_ENABLE_TESTING': 'OFF',
})
utf8proc_options.set_install(false)

# toml++ 是 header-only CMake 项目。固定为 Git submodule 后，Meson 和未来的
# 顶层 CMake 构建都消费同一份官方源码，不依赖系统安装或 WrapDB 下载。
tomlplusplus_options = cmake.subproject_options()
tomlplusplus_options.add_cmake_defines({
    'TOMLPLUSPLUS_BUILD_MODULES': 'OFF',
    'tomlplusplus_INSTALL': 'OFF',
})
tomlplusplus_options.set_install(false)

# 配置 subproject
spdlog_subproj = cmake.subproject('spdlog', options: spdlog_options)
utf8proc_subproj = cmake.subproject('utf8proc', options: utf8proc_options)
tomlplusplus_subproj = cmake.subproject('tomlplusplus', options: tomlplusplus_options)
freetype_subproj = subproject(
    'freetype',
    default_options: ['tests=disabled', 'harfbuzz=disabled'],
)
harfbuzz_subproj = subproject(
    'harfbuzz',
    default_options: [
        'tests=disabled',
        'docs=disabled',
        # 基准目录（perf/）不随 SDK 交付；不开这个选项，meson 会 subdir('perf') 并因缺目录失败。
        'benchmark=disabled',
        'utilities=disabled',
        'introspection=disabled',
        'glib=disabled',
        'gobject=disabled',
        'cairo=disabled',
        'chafa=disabled',
        'png=disabled',
        'icu=disabled',
        'graphite2=disabled',
        'raster=disabled',
        'vector=disabled',
        'gpu=disabled',
        'subset=disabled',
        'freetype=enabled',
    ],
)
fribidi_subproj = subproject(
    'fribidi',
    default_options: [
        'default_library=static',
        'deprecated=false',
        'docs=false',
        'bin=false',
        'tests=false',
    ],
)
# nlohmann/json 是 header-only meson 项目（Git submodule，见 .gitmodules）。
nlohmann_json_subproj = subproject('nlohmann_json')

# 获取依赖（静态库）
spdlog_dep = spdlog_subproj.dependency('spdlog')
utf8proc_dep = utf8proc_subproj.dependency('utf8proc')
# Meson 的 CMake 适配器将命名空间 target `tomlplusplus::tomlplusplus`
# 规范化为底层实际 target 名。
tomlplusplus_dep = tomlplusplus_subproj.dependency('tomlplusplus_tomlplusplus')
freetype_dep = freetype_subproj.get_variable('freetype_dep')
harfbuzz_dep = harfbuzz_subproj.get_variable('libharfbuzz_dep')
fribidi_dep = fribidi_subproj.get_variable('libfribidi_dep')
nlohmann_json_dep = nlohmann_json_subproj.get_variable('nlohmann_json_dep')
# SDL3 是 P3/P4 的平台层：窗口/输入/帧驱动 + SDL 2D 渲染设备。
#
# 来源解析按 docs/references/dependency_policy.md §2.4 的实测结论写成**显式三态**：
#   - system   ：只查系统（`method: 'pkg-config'` 显式过滤；否则 wrap 的存在会遮蔽同名依赖，
#                实测会让 dependency() 返回 not found）；
#   - vendored ：只用 subprojects/SDL3（wrap-git + method=cmake，target 名实测为 SDL3-static）；
#   - auto     ：先系统，缺失/版本不足再回落 vendored。
# 不使用 wrap 的 `[provide]` 变量名机制：CMake-only 上游在系统缺失时解析不到（实测），
# 显式分支更小也更可测。默认 vendored：构建来源确定，不依赖机器上装了什么。
#
# 失败策略：`sdl_backend=enabled` 且没有任何来源解析成功 → error（不许静默）；
# `auto` 且都没解析到 → 无内置窗口后端；`disabled` → 完全不查找、不下载。
sdl_backend_option = get_option('sdl_backend')
sdl3_source_option = get_option('sdl3_source')
sdl3_linkage_option = get_option('sdl3_linkage')
sdl3_min_version = '3.4.0'
# pin 必须是不可变的完整 commit（tag 只写在 wrap 的注释里供人阅读）。
sdl3_pin = '829a65d769d935c4852f8159e964312c0957260a'
sdl3_pin_version = '3.4.18'
sdl3_dep = disabler()
sdl3_origin = 'disabled'
sdl3_method = 'none'
sdl3_linkage = 'none'
sdl3_version = 'n/a'
# 报告字段默认值：system/disabled 下没有"项目固定的源码"，因此 pin/revision 为 null，
# 不能让 system 看起来像 vendored（docs/references/dependency_policy.md §5）。
sdl3_pin_json = 'null'
sdl3_pin_version_json = 'null'
sdl3_revision_json = 'null'
sdl3_path = ''
sdl3_resolved_by = 'none'

if not sdl_backend_option.disabled()
    if sdl3_source_option != 'vendored'
        # `method: 'pkg-config'` 只查系统副本：wrap 存在时依赖名会被遮蔽（实测），
        # 显式限定 method 才能让 system 状态真的只看系统。
        sdl3_dep = dependency(
            'sdl3',
            version: '>=' + sdl3_min_version,
            method: 'pkg-config',
            required: false,
        )
    endif
    if sdl3_dep.found()
        sdl3_origin = 'system'
        sdl3_method = 'pkg-config'
        sdl3_resolved_by = 'pkg-config'
        # 系统副本的静态/动态由发行版决定，不假装知道；pin/revision 保持 null。
        sdl3_linkage = 'unknown'
        sdl3_version = sdl3_dep.version()
        if sdl3_linkage_option != 'static'
            warning('sdl3_linkage only controls vendored SDL3; system linkage is selected by pkg-config and reported as unknown')
        endif
    elif sdl3_source_option != 'system'
        sdl3_cmake_options = cmake.subproject_options()
        sdl3_cmake_options.add_cmake_defines({
            'SDL_TESTS': 'OFF',
            'SDL_EXAMPLES': 'OFF',
            'SDL_INSTALL': 'OFF',
            'SDL_SHARED': sdl3_linkage_option == 'shared' ? 'ON' : 'OFF',
            'SDL_STATIC': sdl3_linkage_option == 'static' ? 'ON' : 'OFF',
            'SDL_TEST_LIBRARY': 'OFF',
        })
        if host_machine.system() == 'linux' and get_option('linux_window_system') != 'auto'
            sdl3_cmake_options.add_cmake_defines({
                'SDL_WAYLAND': get_option('linux_window_system') == 'wayland' ? 'ON' : 'OFF',
                'SDL_X11': get_option('linux_window_system') == 'x11' ? 'ON' : 'OFF',
            })
        endif
        sdl3_subproject = cmake.subproject('SDL3', options: sdl3_cmake_options)
        sdl3_dep = sdl3_subproject.dependency('SDL3-' + sdl3_linkage_option)
        sdl3_origin = 'vendored'
        sdl3_method = 'cmake'
        sdl3_resolved_by = 'cmake.subproject'
        sdl3_path = 'subprojects/SDL3'
        sdl3_pin_json = '"' + sdl3_pin + '"'
        sdl3_pin_version_json = '"' + sdl3_pin_version + '"'
        sdl3_revision_json = '"' + sdl3_pin + '"'
        sdl3_linkage = sdl3_linkage_option
        sdl3_version = sdl3_pin_version
    endif
    if not sdl3_dep.found()
        if sdl_backend_option.enabled()
            error(
                'sdl_backend=enabled 但 sdl3_source='
                + sdl3_source_option
                + ' 未解析到 SDL3 >= '
                + sdl3_min_version
                + '（system 需 pkg-config，vendored 需 subprojects/SDL3；'
                + '可用 meson subprojects download 预热）',
            )
        endif
        sdl3_origin = 'unavailable'
        warning(
            'SDL3 未解析到（sdl3_source='
            + sdl3_source_option
            + '），没有内置窗口后端；打开窗口需启用 SDL3 或注入 IPlatformWindow',
        )
    endif
endif

message('[deps] sdl3 -> ' + sdl3_origin + ' ' + sdl3_version)
# 机器可读的解析报告：CI 与 bug 报告共用（docs/references/dependency_policy.md §5）。
# 用模板而不是 content:（`configure_file` 在 Meson 1.12 里没有 `content` 关键字，
# `output_format: 'json'` 也只接受扁平 dict）。
configure_file(
    input: 'meson/deps.json.in',
    output: 'deps.json',
    configuration: {
        # `enabled` 表示"SDL3 实际被采用"，而不是"后端开关没关"：
        # auto + 未解析时后端开关未关，但没有内置窗口后端。
        'SDL3_ENABLED': sdl3_dep.found().to_string(),
        'SDL3_SOURCE': sdl3_origin,
        'SDL3_VERSION': sdl3_version,
        'SDL3_MIN': sdl3_min_version,
        'SDL3_PIN_JSON': sdl3_pin_json,
        'SDL3_PIN_VERSION_JSON': sdl3_pin_version_json,
        'SDL3_REVISION_JSON': sdl3_revision_json,
        'SDL3_PATH': sdl3_path,
        'SDL3_RESOLVED_BY': sdl3_resolved_by,
        'SDL3_METHOD': sdl3_method,
        'SDL3_LINKAGE': sdl3_linkage,
    },
)

physics2d_option = get_option('physics2d')
box2d_dep = disabler()
if not physics2d_option.disabled()
    box2d_options = cmake.subproject_options()
    box2d_options.add_cmake_defines({
        'BUILD_SHARED_LIBS': 'OFF',
        'BOX2D_SAMPLES': 'OFF',
        'BOX2D_BENCHMARKS': 'OFF',
        'BOX2D_DOCS': 'OFF',
        'BOX2D_UNIT_TESTS': 'OFF',
    })
    box2d_options.set_install(false)
    box2d_subproj = cmake.subproject(
        'box2d',
        options: box2d_options,
        required: physics2d_option,
    )
    if box2d_subproj.found()
        box2d_dep = box2d_subproj.dependency('box2d')
    endif
endif
# SQLite 以官方 amalgamation 形式随仓库分发（third_party/sqlite3），不再走系统库或
# wrap 兜底：wrap 只服务 Meson、vcpkg 只服务 CMake、submodule 要求消费者 clone 镜像
# 仓，三条路都会让两套构建系统拿到来源不同的同一个依赖。版本、来源哈希与升级步骤见
# third_party/sqlite3/README.md。
sqlite3_c_args = [
    # 与运行时 SQLITE_OPEN_FULLMUTEX 的用法保持一致。
    '-DSQLITE_THREADSAFE=1',
    # 运行时打开的是可能随应用分发到用户磁盘的库文件，而框架从不加载扩展。
    '-DSQLITE_OMIT_LOAD_EXTENSION',
]
stb_include = include_directories('third_party/stb')
stb_dep = declare_dependency(include_directories: stb_include)
sqlite3_include = include_directories('third_party/sqlite3')
sqlite3_dep = declare_dependency(
    link_with: static_library(
        'sqlite3',
        'third_party/sqlite3/sqlite3.c',
        c_args: sqlite3_c_args,
        include_directories: sqlite3_include,
        install: false,
    ),
    include_directories: sqlite3_include,
)
openssl_dep = dependency('openssl', version: '>=3.0')

# Catch2 是原生 meson 项目，直接作为 subproject 引入，用于单元测试。
# catch2_with_main 自带 Catch2 默认 main，测试文件无需自己写 main。
catch2_subproj = subproject('Catch2', default_options: ['install=false', 'tests=false'])
catch2_with_main_dep = catch2_subproj.get_variable('catch2_with_main_dep')

# 引入子项目
subdir('nandina')
nanres = executable(
    'nanres',
    ['tools/nanres_main.cpp', 'tools/resource_manifest.cpp', 'tools/resource_packager.cpp'],
    dependencies: [nandina_dep, tomlplusplus_dep, openssl_dep],
)
nanres_build_helper = [python, files('tools/nanres_build_helper.py')]
nandina_portable_stage_helper = [python, files('tools/portable_stage.py')]
nandina_resource_toolchain = {
    'nanres': nanres,
    'build_helper': nanres_build_helper,
    'stage_helper': nandina_portable_stage_helper,
    'template': files('meson/nandina_resources.build'),
}
if not meson.is_subproject()
    if get_option('build_tests')
        subdir('tests')
    endif

    # 本地 playground 不随仓库发布；目录存在时才加入开发构建。
    if get_option('playground') and fs.exists('playground/meson.build')
        subdir('playground')
    endif

    # 组件展示子项目：只作为主项目时构建。放在 subproject 保护之外会让下游消费者
    # 连带编译这个演示程序（见 showcase/meson.build），所以必须留在本块内。
    if get_option('showcase') and fs.exists('showcase/meson.build')
        if not sdl3_dep.found()
            error('showcase requires SDL3; use -Dshowcase=false for a build without a window backend')
        endif
        subdir('showcase')
    endif

    # 跨平台探测里「真实页面走 SDL 设备」的那一个：需要 libnandina 与 showcase 的页面源码，
    # 所以它属于本构建（而不是 CMake 的独立探测）。默认关闭：它需要 SDL3 与一个真窗口。
    if get_option('sdl_page_probe') and fs.exists('probe/cross_platform/desktop/meson.build')
        subdir('probe/cross_platform/desktop')
    endif
endif

# Windows 的 DLL 搜索顺序不包含构建目录：把运行时 DLL 复制到每个 .exe 旁边，
# 这样终端、CLion、双击都能直接运行（否则 0xC0000135 STATUS_DLL_NOT_FOUND）。
# 只在主项目构建时挂上；CMake 子项目自己的产物不动。
if host_machine.system() == 'windows' and not meson.is_subproject()
    _windows_runtime_deps = []
    foreach _target_name : ['playground_exe', 'sdf_device_tests', 'nandina']
        if is_variable(_target_name)
            _windows_runtime_deps += [get_variable(_target_name)]
        endif
    endforeach
    custom_target(
        'windows-runtime-dlls',
        output: 'windows-runtime-dlls.stamp',
        build_by_default: true,
        build_always_stale: true,
        depends: _windows_runtime_deps,
        command: [
            python,
            files('tools/copy_runtime_dlls.py'),
            '--build-dir', meson.current_build_dir(),
        ],
    )
endif
