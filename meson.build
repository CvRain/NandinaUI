project(
    'nandina_experiment',
    'cpp',
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
raylib_options.add_cmake_defines({
    'CUSTOMIZE_BUILD': 'ON',
    'SUPPORT_FILEFORMAT_JPG': 'ON',
})
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
sqlite3_dep = dependency(
    'sqlite3',
    version: '>=3.37',
    fallback: ['sqlite3', 'sqlite3_dep'],
    default_options: ['default_library=static'],
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
    subdir('example')
    if get_option('build_tests')
        subdir('tests')
    endif
endif
