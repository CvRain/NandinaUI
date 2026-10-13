page_files = [
    'pages/home_page.hpp',
    'pages/home_page.cpp',
    'pages/anchors_page.hpp',
    'pages/anchors_page.cpp',
    'pages/label_page.hpp',
    'pages/label_page.cpp'
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


playground_exe = executable(
    'nandina_showcase',
    sources,
    dependencies : nandina_dep,
    install : false,
)
