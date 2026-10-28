data_sources = files(
    'data_error.hpp',
    'data_error.cpp',
    'statement.hpp',
    'statement.cpp',
    'database.hpp',
    'database.cpp',
    'migration.hpp',
    'migration.cpp',
    'mappers/row_mapper.hpp',
    'mappers/row_mapper.cpp',
    'repositories/repository_util.hpp',
    'repositories/repository_util.cpp',
    'repositories/user_repository.hpp',
    'repositories/user_repository.cpp',
    'repositories/note_group_repository.hpp',
    'repositories/note_group_repository.cpp',
    'repositories/note_repository.hpp',
    'repositories/note_repository.cpp',
    'repositories/todo_repository.hpp',
    'repositories/todo_repository.cpp',
)

lib_data = static_library(
    'notebook_data',
    data_sources,
    notebook_resource_keys,
    dependencies: [nandina_dep, sqlite3_dep],
    include_directories: [notebook_src_include, notebook_root_include],
    install: false,
)

data_dep = declare_dependency(
    link_with: lib_data,
    dependencies: [nandina_dep, sqlite3_dep],
    include_directories: notebook_src_include,
)
