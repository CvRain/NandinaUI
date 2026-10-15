# 「真实页面走 SDL 设备」探测的构建。
#
# 与 CMake 那份（../CMakeLists.txt）的区别：这里**链接 libnandina**、并复用 showcase 的
# 页面源码，所以它测的是真实页面（真实主题、HarfBuzz 整形、GlyphAtlasTexture 图集）在
# SDL3 设备上的表现，而不是手写图元。CMake 那份刻意只依赖头文件 + 三个 foundation TU，
# 用来回答「设备契约能不能独立实现」；两份并存的理由见 README。

sdl3_dep = dependency('sdl3', version: '>=3.4.0', required: true)

# 消费者回归要覆盖每个 showcase 页面，所以三个页面源码都编进来。
showcase_page_sources = files(
    '../../../showcase/pages/anchors_page.cpp',
    '../../../showcase/pages/home_page.cpp',
    '../../../showcase/pages/label_page.cpp',
    '../../../showcase/components/page_chrome.hpp',
    '../../../showcase/components/component_info_table.cpp',
)

sdl_page_probe = executable(
    'nandina_sdl_page_probe',
    files('page_probe.cpp', 'sdl_render_device.cpp') + showcase_page_sources,
    include_directories : include_directories('../../..'),
    dependencies : [nandina_dep, sdl3_dep],
    install : false,
)
