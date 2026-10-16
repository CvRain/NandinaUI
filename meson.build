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
nandina_source_root = meson.current_source_dir()

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

# 配置 raylib 的CMake选项
raylib_options = cmake.subproject_options()
raylib_defines = {
    'CUSTOMIZE_BUILD': 'ON',
    'SUPPORT_FILEFORMAT_JPG': 'ON',
}
if host_machine.system() == 'linux'
    linux_window_system = get_option('linux_window_system')
    if linux_window_system == 'auto'
        session_type = run_command('printenv', 'XDG_SESSION_TYPE', check: false).stdout().strip()
        wayland_display = run_command('printenv', 'WAYLAND_DISPLAY', check: false).stdout().strip()
        linux_window_system = session_type == 'wayland' or wayland_display != '' ? 'wayland' : 'x11'
    endif
    raylib_defines += linux_window_system == 'wayland' ? {
        'GLFW_BUILD_WAYLAND': 'ON',
        'GLFW_BUILD_X11': 'OFF',
    } : {
        'GLFW_BUILD_WAYLAND': 'OFF',
        'GLFW_BUILD_X11': 'ON',
    }
    message('Linux window system: ' + linux_window_system)
endif
raylib_options.add_cmake_defines(raylib_defines)
raylib_options.set_install(false)

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
raylib_subproj = cmake.subproject('raylib', options: raylib_options)
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
raylib_dep = raylib_subproj.dependency('raylib')
utf8proc_dep = utf8proc_subproj.dependency('utf8proc')
# Meson 的 CMake 适配器将命名空间 target `tomlplusplus::tomlplusplus`
# 规范化为底层实际 target 名。
tomlplusplus_dep = tomlplusplus_subproj.dependency('tomlplusplus_tomlplusplus')
freetype_dep = freetype_subproj.get_variable('freetype_dep')
harfbuzz_dep = harfbuzz_subproj.get_variable('libharfbuzz_dep')
fribidi_dep = fribidi_subproj.get_variable('libfribidi_dep')
nlohmann_json_dep = nlohmann_json_subproj.get_variable('nlohmann_json_dep')
# SDL3 是 P3/P4 的目标平台层：窗口/输入/帧驱动 + SDL 2D 渲染设备。用 feature 选项控制，
# 找不到时（auto）静默退化为纯 raylib 构建；enabled 时找不到会让配置失败。
sdl_backend_option = get_option('sdl_backend')
sdl3_dep = disabler()
if not sdl_backend_option.disabled()
    sdl3_dep = dependency('sdl3', version: '>=3.4.0', required: sdl_backend_option)
endif

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
# D3：项目级工作流 CLI（脚手架/体检），只依赖 C++ 标准库，不链接框架运行时。
# 放在 tools/ 子目录，构建产物为 buildDir/tools/nandina。
subdir('tools')
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
    if fs.exists('playground/meson.build')
        subdir('playground')
    endif

    # 组件展示子项目：只作为主项目时构建。放在 subproject 保护之外会让下游消费者
    # 连带编译这个演示程序（见 showcase/meson.build），所以必须留在本块内。
    if get_option('showcase') and fs.exists('showcase/meson.build')
        subdir('showcase')
    endif

    # 跨平台探测里「真实页面走 SDL 设备」的那一个：需要 libnandina 与 showcase 的页面源码，
    # 所以它属于本构建（而不是 CMake 的独立探测）。默认关闭：它需要 SDL3 与一个真窗口。
    if get_option('sdl_page_probe') and fs.exists('probe/cross_platform/desktop/meson.build')
        subdir('probe/cross_platform/desktop')
    endif
endif
