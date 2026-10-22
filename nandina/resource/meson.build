# D5: runtime protocols, backends and build algorithms have separate link targets.
resource_include = include_directories('../..')
resource_core = static_library(
    'resource_core',
    files('resource.cpp', 'resource_backend.cpp', 'resource_stream.cpp',
          'file_resource_stream.cpp', 'resource_manager.cpp', 'resource_uri.cpp'),
)
resource_core_dep = declare_dependency(link_with: resource_core, include_directories: resource_include)

# Builtin assets are bytes; this target has no font engine dependency.
builtin_font_source = files('../assets/default_font/default-ui.ttf')
builtin_font_license_source = files('../assets/default_font/default-ui-license.txt')
builtin_font_data = custom_target(
    'nandina-builtin-default-font', input: builtin_font_source,
    output: 'builtin_default_font_data.cpp',
    command: [embed_binary, '@INPUT@', '@OUTPUT@', 'builtin_default_font_bytes'],
)
builtin_font_license_data = custom_target(
    'nandina-builtin-default-font-license', input: builtin_font_license_source,
    output: 'builtin_default_font_license_data.cpp',
    command: [embed_binary, '@INPUT@', '@OUTPUT@', 'builtin_default_font_license_bytes'],
)
resource_runtime = static_library(
    'resource_runtime',
    files('backends/memory_backend.cpp', 'backends/directory_backend.cpp',
          'backends/builtin_backend.cpp', 'platform_resource_locator.cpp'),
    builtin_font_data, builtin_font_license_data,
    cpp_args: [
        '-DNANDINA_BUILTIN_DEFAULT_FONT_SIZE=' + fs.size(builtin_font_source).to_string(),
        '-DNANDINA_BUILTIN_DEFAULT_FONT_LICENSE_SIZE=' + fs.size(builtin_font_license_source).to_string(),
    ],
    dependencies: resource_core_dep,
)
resource_runtime_dep = declare_dependency(link_with: resource_runtime, dependencies: resource_core_dep)
if resource_sqlite
    resource_sqlite_lib = static_library(
        'resource_sqlite', 'backends/sqlite_backend.cpp', dependencies: [resource_core_dep, sqlite3_dep],
    )
    resource_runtime_dep = declare_dependency(
        link_with: resource_sqlite_lib,
        dependencies: [resource_runtime_dep, sqlite3_dep],
        compile_args: ['-DNANDINA_HAS_RESOURCE_SQLITE=1'],
    )
endif

if resource_tools
    resource_build = static_library(
        'resource_build',
        files('resource_scanner.cpp', '../../tools/resource_manifest.cpp', '../../tools/resource_packager.cpp'),
        dependencies: [resource_core_dep, tomlplusplus_dep, openssl_dep, sqlite3_dep],
    )
    resource_build_dep = declare_dependency(
        link_with: resource_build,
        dependencies: [resource_core_dep, tomlplusplus_dep, openssl_dep, sqlite3_dep],
    )
endif
