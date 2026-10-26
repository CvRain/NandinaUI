# P2.4 example application. One executable, several routes, one resource package.
#
# The resource directory is a runtime input, so the custom target runs before the
# link and link_depends makes it ready even for a targeted `meson compile ... nandina_lab`.
# The generated key header is a real compile input: it is listed in sources so the
# compiler waits for resources.toml to be scanned.

lab_sources = files(
    'main.cpp',
    'lab_store.cpp',
    'lab_window.cpp',
    'pages/account_pages.cpp',
    'pages/notes_page.cpp',
    'pages/preview_page.cpp',
    'pages/pomodoro_page.cpp',
)

if not is_variable('nandina_resource_toolchain')
    error(
        'examples/lab requires the resource toolchain: configure with -Dexamples=false or '
        + 'provide -Dexternal_nanres=/path/to/nanres'
    )
endif

lab_resource_depends = []
if resource_tools
    lab_resource_depends += [nanres]
endif

lab_resources = custom_target(
    'nandina-lab-resources',
    input: files('resources/resources.toml'),
    output: 'resources',
    command: [nanres_build_helper, 'build', toolchain_nanres, '@INPUT@', '@OUTPUT@'],
    depends: lab_resource_depends,
    build_by_default: true,
    build_always_stale: true,
)

lab_resource_keys = custom_target(
    'nandina-lab-resource-keys',
    input: files('resources/resources.toml'),
    output: 'resource_keys.hpp',
    command: [nanres_build_helper, 'keys', toolchain_nanres, '@INPUT@', '@OUTPUT@'],
    depends: lab_resource_depends,
    build_by_default: true,
    build_always_stale: true,
)

lab_exe = executable(
    'nandina_lab',
    lab_sources,
    lab_resource_keys,
    dependencies : nandina_dep,
    include_directories : include_directories('.'),
    link_depends : lab_resources,
    install : false,
)

# P2.4c: build and run checks are separate. The run check stages the executable
# into a portable tree, moves it, and verifies the package still mounts. It skips
# when no display is available; the build check always runs.
lab_build_dir = meson.current_build_dir()
lab_example_test = files('../../tests/example_lab_test.py')
test(
    'example-lab-build',
    python,
    args: [
        lab_example_test,
        'build',
        lab_exe,
        lab_build_dir / 'resources',
        lab_build_dir / 'resource_keys.hpp',
    ],
    depends: [lab_exe, lab_resources, lab_resource_keys],
    suite: 'integration',
    timeout: 60,
)
test(
    'example-lab-run',
    python,
    args: [
        lab_example_test,
        'run',
        lab_exe,
        lab_build_dir / 'resources',
        nandina_portable_stage_helper[1],
    ],
    depends: [lab_exe, lab_resources],
    suite: 'integration',
    timeout: 60,
)
test(
    'example-lab-incremental',
    python,
    args: [
        lab_example_test,
        'incremental',
        toolchain_nanres,
        files('../../tools/nanres_build_helper.py'),
        meson.current_source_dir() / 'resources',
    ],
    depends: lab_resource_depends,
    suite: 'integration',
    timeout: 60,
)
