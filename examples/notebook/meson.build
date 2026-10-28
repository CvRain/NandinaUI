# Notebook example: one executable, one resource package, SQL migrations loaded from
# that package at startup. The data layer is complete; UI/services grow in later batches.
if not is_variable('nandina_resource_toolchain')
    error(
        'examples/notebook requires the resource toolchain: configure with -Dexamples=false or '
        + 'provide -Dexternal_nanres=/path/to/nanres'
    )
endif
if not resource_sqlite
    error(
        'examples/notebook needs the SQLite runtime backend for its app database and package: '
        + 'configure with -Dresource_sqlite=enabled or -Dexamples=false'
    )
endif

notebook_resource_depends = []
if resource_tools
    notebook_resource_depends += [nanres]
endif

notebook_resources = custom_target(
    'nandina-notebook-resources',
    input: files('resources.toml'),
    output: 'resources',
    command: [nanres_build_helper, 'build', toolchain_nanres, '@INPUT@', '@OUTPUT@'],
    depends: notebook_resource_depends,
    build_by_default: true,
    build_always_stale: true,
)

notebook_resource_keys = custom_target(
    'nandina-notebook-resource-keys',
    input: files('resources.toml'),
    output: 'resource_keys.hpp',
    command: [nanres_build_helper, 'keys', toolchain_nanres, '@INPUT@', '@OUTPUT@'],
    depends: notebook_resource_depends,
    build_by_default: true,
    build_always_stale: true,
)

notebook_root_include = include_directories('.')
subdir('src')

notebook_exe = executable(
    'nandina_examples_notebook',
    notebook_sources,
    notebook_resource_keys,
    dependencies: [core_dep],
    include_directories: [notebook_src_include, notebook_root_include],
    link_depends: notebook_resources,
    install: false,
)

if is_variable('catch2_with_main_dep')
    notebook_data_tests = executable(
        'notebook_data_tests',
        files('tests/database_tests.cpp'),
        dependencies: [data_dep, catch2_with_main_dep],
    )
    notebook_test_env = environment()
    notebook_test_env.set(
        'NOTEBOOK_RESOURCE_DIR',
        meson.current_build_dir() / 'resources',
    )
    test(
        'notebook-data',
        notebook_data_tests,
        env: notebook_test_env,
        depends: [notebook_resources, notebook_resource_keys],
        suite: 'unit',
        timeout: 60,
    )
endif
