page_files = [
    'pages/home_page.hpp',
    'pages/home_page.cpp',
    'pages/anchors_page.hpp',
    'pages/anchors_page.cpp',
    'pages/label_page.hpp',
    'pages/label_page.cpp',
    'pages/image_page.hpp',
    'pages/image_page.cpp'
]

components = [
    'components/sidebar.hpp',
    'components/sidebar.cpp',
    'components/page_chrome.hpp',
    'components/component_info_table.hpp',
    'components/component_info_table.cpp',
]

sources = files(
    'application.cpp',
    'component_catalog.hpp',
    'main_window.hpp',
    'main_window.cpp',
)

foreach file: page_files
    sources += file
endforeach

foreach file: components
    sources += file
endforeach

showcase_resource_depends = []
if not is_variable('nandina_resource_toolchain')
    error(
        'showcase requires the resource toolchain: configure with -Dshowcase=false or provide '
        + '-Dexternal_nanres=/path/to/nanres'
    )
endif
if resource_tools
    showcase_resource_depends += [nanres]
endif

showcase_resources = custom_target(
    'nandina-showcase-resources',
    input: files('resources/resources.toml'),
    output: 'resources',
    command: [nanres_build_helper, 'build', toolchain_nanres, '@INPUT@', '@OUTPUT@'],
    depends: showcase_resource_depends,
    build_by_default: true,
    build_always_stale: true,
)

playground_exe = executable(
    'nandina_showcase',
    sources,
    dependencies : nandina_dep,
    # 资源目录是运行期输入：自定义目标先构建，目录内容变化时重链接（pack 无变化时不动）。
    link_depends : showcase_resources,
    install : false,
)
