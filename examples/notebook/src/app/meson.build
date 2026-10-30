app_sources = files(
    'notebook_store.hpp',
    'notebook_store.cpp',
    'main_window.hpp',
    'main_window.cpp'
)

lib_app = static_library(
    'notebook_app',
    app_sources,
    dependencies: [core_dep],
    include_directories: [notebook_src_include],
    install: false,
)

app_dep = declare_dependency(
    link_with: lib_app,
    dependencies: [core_dep],
    include_directories: notebook_src_include,
)
