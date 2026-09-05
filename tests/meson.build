# 单元测试 (Catch2 v3, 无窗口)

foundation_tests = executable(
    'foundation_tests',
    files('foundation_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('foundation', foundation_tests)

contrast_tests = executable(
    'contrast_tests',
    files('contrast_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('contrast', contrast_tests)

font_test_path = meson.project_source_root() / 'subprojects' / 'harfbuzz' / 'test' / 'api' / 'fonts' / 'Inconsolata-Regular.ab.ttf'
rtl_font_test_path = meson.project_source_root() / 'subprojects' / 'harfbuzz' / 'test' / 'shape' / 'data' / 'text-rendering-tests' / 'fonts' / 'TestShapeAran.ttf'
arabic_mark_font_test_path = meson.project_source_root() / 'subprojects' / 'harfbuzz' / 'test' / 'shape' / 'data' / 'in-house' / 'fonts' / '65984dfce552a785f564422aadf4715fa07795ad.ttf'
bold_font_test_path = meson.project_source_root() / 'subprojects' / 'harfbuzz' / 'test' / 'api' / 'fonts' / 'NotoSans-Bold.ttf'
italic_font_test_path = meson.project_source_root() / 'subprojects' / 'harfbuzz' / 'test' / 'api' / 'fonts' / 'notosansitalic.ttf'
font_tests = executable(
    'font_tests',
    files('font_tests.cpp'),
    cpp_args : [
        '-DNANDINA_TEST_FONT_PATH="' + font_test_path + '"',
        '-DNANDINA_RTL_TEST_FONT_PATH="' + rtl_font_test_path + '"',
        '-DNANDINA_ARABIC_MARK_TEST_FONT_PATH="' + arabic_mark_font_test_path + '"',
        '-DNANDINA_BOLD_TEST_FONT_PATH="' + bold_font_test_path + '"',
        '-DNANDINA_ITALIC_TEST_FONT_PATH="' + italic_font_test_path + '"',
    ],
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('font', font_tests)

scene_tests = executable(
    'scene_tests',
    files('scene_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('scene', scene_tests)

if box2d_dep.found()
    physics2d_tests = executable(
        'physics2d_tests',
        files('physics2d_tests.cpp'),
        dependencies : [nandina_dep, catch2_with_main_dep],
    )
    test('physics2d', physics2d_tests)
endif

render_tests = executable(
    'render_tests',
    files('render_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('render', render_tests)

image_tests = executable(
    'image_tests',
    files('image_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('image', image_tests)

reactive_tests = executable(
    'reactive_tests',
    files('reactive_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('reactive', reactive_tests)

widget_tests = executable(
    'widget_tests',
    files('widget_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('widget', widget_tests)

authoring_tests = executable(
    'authoring_tests',
    files('authoring_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('authoring', authoring_tests)

build_context_header_tests = executable(
    'build_context_header_tests',
    files('build_context_header_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('build-context-header', build_context_header_tests)

settings_example_tests = executable(
    'settings_example_tests',
    files('settings_example_tests.cpp'),
    dependencies : [settings_example_dep, catch2_with_main_dep],
)
test('settings-example', settings_example_tests)

test(
    'example-source-budget',
    python,
    args: [files('example_source_budget_test.py'), meson.project_source_root()],
)

router_tests = executable(
    'router_tests',
    files('router_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('router', router_tests)

async_tests = executable(
    'async_tests',
    files('async_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('async', async_tests)

viewport_scaling_tests = executable(
    'viewport_scaling_tests',
    files('viewport_scaling_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('viewport-scaling', viewport_scaling_tests)

window_metrics_tests = executable(
    'window_metrics_tests',
    files('window_metrics_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('window-metrics', window_metrics_tests)

if host_machine.system() == 'linux'
    linux_clipboard_tests = executable(
        'linux_clipboard_tests',
        files('linux_clipboard_tests.cpp'),
        dependencies : [nandina_dep, catch2_with_main_dep],
    )
    test('linux-clipboard', linux_clipboard_tests)
endif

button_tests = executable(
    'button_tests',
    files('button_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('button', button_tests)

theme_design_system_tests = executable(
    'theme_design_system_tests',
    files('theme_design_system_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('theme-design-system', theme_design_system_tests)

checkbox_tests = executable(
    'checkbox_tests',
    files('checkbox_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('checkbox', checkbox_tests)

slider_tests = executable(
    'slider_tests',
    files('slider_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('slider', slider_tests)

switch_tests = executable(
    'switch_tests',
    files('switch_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('switch', switch_tests)

badge_tests = executable(
    'badge_tests',
    files('badge_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('badge', badge_tests)

card_tests = executable(
    'card_tests',
    files('card_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('card', card_tests)

progress_bar_tests = executable(
    'progress_bar_tests',
    files('progress_bar_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('progress-bar', progress_bar_tests)

radio_button_tests = executable(
    'radio_button_tests',
    files('radio_button_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('radio-button', radio_button_tests)

theme_families_tests = executable(
    'theme_families_tests',
    files('theme_families_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('theme-families', theme_families_tests)

color_tests = executable(
    'color_tests',
    files('color_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('color', color_tests)

tabs_tests = executable(
    'tabs_tests',
    files('tabs_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('tabs', tabs_tests)

tooltip_tests = executable(
    'tooltip_tests',
    files('tooltip_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('tooltip', tooltip_tests)

select_tests = executable(
    'select_tests',
    files('select_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('select', select_tests)

basic_widgets_tests = executable(
    'basic_widgets_tests',
    files('basic_widgets_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('basic-widgets', basic_widgets_tests)

dialog_tests = executable(
    'dialog_tests',
    files('dialog_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('dialog', dialog_tests)

animation_tests = executable(
    'animation_tests',
    files('animation_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('animation', animation_tests)

system_fonts_tests = executable(
    'system_fonts_tests',
    files('system_fonts_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('system-fonts', system_fonts_tests)

resource_tests = executable(
    'resource_tests',
    files('resource_tests.cpp'),
    dependencies : [nandina_dep, sqlite3_dep, catch2_with_main_dep],
)
test('resource', resource_tests)

nanres_cli_test = find_program('nanres_cli_test.py')
nanres_package_probe = executable(
    'nanres_package_probe',
    files('nanres_package_probe.cpp'),
    dependencies: [nandina_dep, sqlite3_dep],
)
test(
    'nanres-cli',
    nanres_cli_test,
    args: [nanres, nanres_package_probe],
)
nandina_cli_test = find_program('nandina_cli_test.py')
test(
    'nandina-cli',
    nandina_cli_test,
    args: [nandina, meson.project_source_root()],
    timeout: 600,
)
portable_stage_test = find_program('portable_stage_test.py')
test(
    'portable-stage',
    portable_stage_test,
    args: [
        meson.project_source_root() / 'tools' / 'portable_stage.py',
        nanres,
        nanres_package_probe,
    ],
)
nanres_build_workflow_test = find_program('nanres_build_workflow_test.py')
test(
    'nanres-build-workflow',
    nanres_build_workflow_test,
    args: [
        meson.project_source_root() / 'tools' / 'nanres_build_helper.py',
        nanres,
    ],
)
nandina_subproject_fixture_test = find_program('nandina_subproject_fixture_test.py')
test(
    'nandina-subproject-fixture',
    nandina_subproject_fixture_test,
    args: [
        meson.project_source_root(),
        meson.project_source_root() / 'tools' / 'sdk_bundle.py',
    ],
    timeout: 600,
)
release_metadata_test = find_program('release_metadata_test.py')
test(
    'release-metadata',
    release_metadata_test,
    args: [meson.project_source_root()],
)
font_resource_tests = executable(
    'font_resource_tests',
    files('font_resource_tests.cpp'),
    cpp_args : ['-DNANDINA_TEST_FONT_PATH="' + font_test_path + '"'],
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('font-resource', font_resource_tests)

application_resource_tests = executable(
    'application_resource_tests',
    files('application_resource_tests.cpp'),
    dependencies: [nandina_dep, catch2_with_main_dep],
)
test('application-resource', application_resource_tests)
