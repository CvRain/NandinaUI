model_sources = files(
    'models/note.hpp',
    'models/note_group.hpp',
    'models/todo.hpp',
    'models/user.hpp'
)

service_sources = files(
    'services/auth_service.hpp',
    'services/auth_service.cpp',
    'services/user_service.hpp',
    'services/user_service.cpp'
)

lib_core = static_library(
    'notebook_core',
    [model_sources, service_sources],
    notebook_resource_keys,
    dependencies: [nandina_dep, data_dep],
    include_directories: [notebook_src_include, notebook_root_include],
    install: false,
)

core_dep = declare_dependency(
    link_with: lib_core,
    dependencies: [nandina_dep, data_dep],
    include_directories: notebook_src_include,
)
