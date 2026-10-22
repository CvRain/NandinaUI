resource_test_dependencies = [resource_runtime_dep, catch2_with_main_dep]
resource_test_args = []
if resource_tools
    test('resource-equivalence', python,
        args: [files('../resource_equivalence_test.py'), nanres],
        suite: 'integration')
    resource_test_dependencies += resource_build_dep
    resource_test_args += '-DNANDINA_TEST_RESOURCE_TOOLS=1'
endif
resource_tests = executable(
    'resource_tests',
    files('../resource_tests.cpp'),
    dependencies : resource_test_dependencies,
    cpp_args: resource_test_args,
)
test('resource', resource_tests, suite: 'unit')

if resource_tools and resource_sqlite
    nanres_cli_test = find_program('../nanres_cli_test.py')
    nanres_package_probe = executable(
        'nanres_package_probe',
        files('../nanres_package_probe.cpp'),
        dependencies: [resource_runtime_dep],
    )
    test(
        'nanres-cli',
        nanres_cli_test,
        args: [nanres, nanres_package_probe],
        suite: 'integration',
    )
    portable_stage_test = find_program('../portable_stage_test.py')
    test(
        'portable-stage',
        portable_stage_test,
        args: [
            meson.project_source_root() / 'tools' / 'portable_stage.py',
            nanres,
            nanres_package_probe,
        ],
        suite: 'integration',
    )
    nanres_build_workflow_test = find_program('../nanres_build_workflow_test.py')
    test(
        'nanres-build-workflow',
        nanres_build_workflow_test,
        args: [
            meson.project_source_root() / 'tools' / 'nanres_build_helper.py',
            nanres,
        ],
        suite: 'integration',
    )
endif

if resource_tools
    test('tool-dependencies', python,
        args: [files('../../tools/check_tool_dependencies.py'), meson.project_build_root()],
        depends: nanres, suite: 'integration', timeout: 120)
    test('tool-dependencies-fixture', python,
        args: [files('../tool_dependencies_test.py'), files('../../tools/check_tool_dependencies.py')],
        suite: 'integration', timeout: 120)
endif
