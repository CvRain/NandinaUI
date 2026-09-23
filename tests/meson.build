# 单元测试 (Catch2 v3, 无窗口)
#
# 每个测试必须声明 suite，CI 依据 suite 选择要跑的子集，不再维护手写名单：
#   unit        —— C++ 单测；ASan/UBSan 任务跑 `--suite unit`
#   integration —— Python CLI / 构建工作流测试（慢），只在常规任务与对应矩阵中跑
# 新增测试时必须带上 suite，否则会静默脱离 sanitizer 覆盖。

foundation_tests = executable(
    'foundation_tests',
    files('foundation_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('foundation', foundation_tests, suite: 'unit')

contrast_tests = executable(
    'contrast_tests',
    files('contrast_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('contrast', contrast_tests, suite: 'unit')

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
test('font', font_tests, suite: 'unit')

scene_tests = executable(
    'scene_tests',
    files('scene_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('scene', scene_tests, suite: 'unit')

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
test('render', render_tests, suite: 'unit')

image_tests = executable(
    'image_tests',
    files('image_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('image', image_tests, suite: 'unit')

reactive_tests = executable(
    'reactive_tests',
    files('reactive_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('reactive', reactive_tests, suite: 'unit')

widget_tests = executable(
    'widget_tests',
    files('widget_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('widget', widget_tests, suite: 'unit')

authoring_tests = executable(
    'authoring_tests',
    files('authoring_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('authoring', authoring_tests, suite: 'unit')

build_context_header_tests = executable(
    'build_context_header_tests',
    files('build_context_header_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('build-context-header', build_context_header_tests, suite: 'unit')

router_tests = executable(
    'router_tests',
    files('router_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('router', router_tests, suite: 'unit')

async_tests = executable(
    'async_tests',
    files('async_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('async', async_tests, suite: 'unit')

viewport_scaling_tests = executable(
    'viewport_scaling_tests',
    files('viewport_scaling_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('viewport-scaling', viewport_scaling_tests, suite: 'unit')

window_metrics_tests = executable(
    'window_metrics_tests',
    files('window_metrics_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('window-metrics', window_metrics_tests, suite: 'unit')

if host_machine.system() == 'linux'
    linux_clipboard_tests = executable(
        'linux_clipboard_tests',
        files('linux_clipboard_tests.cpp'),
        dependencies : [nandina_dep, catch2_with_main_dep],
    )
    test('linux-clipboard', linux_clipboard_tests, suite: 'unit')
endif

button_tests = executable(
    'button_tests',
    files('button_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('button', button_tests, suite: 'unit')

interaction_area_tests = executable(
    'interaction_area_tests',
    files('interaction_area_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('interaction-area', interaction_area_tests, suite: 'unit')

overlay_tests = executable(
    'overlay_tests',
    files('overlay_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('overlay', overlay_tests, suite: 'unit')

anchored_positioner_tests = executable(
    'anchored_positioner_tests',
    files('anchored_positioner_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('anchored-positioner', anchored_positioner_tests, suite: 'unit')

dismiss_layer_tests = executable(
    'dismiss_layer_tests',
    files('dismiss_layer_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('dismiss-layer', dismiss_layer_tests, suite: 'unit')

focus_scope_tests = executable(
    'focus_scope_tests',
    files('focus_scope_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('focus-scope', focus_scope_tests, suite: 'unit')

theme_design_system_tests = executable(
    'theme_design_system_tests',
    files('theme_design_system_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('theme-design-system', theme_design_system_tests, suite: 'unit')

theme_palette_contrast_tests = executable(
    'theme_palette_contrast_tests',
    files('theme_palette_contrast_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('theme-palette-contrast', theme_palette_contrast_tests, suite: 'unit')

theme_switch_tests = executable(
    'theme_switch_tests',
    files('theme_switch_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('theme-switch', theme_switch_tests, suite: 'unit')

layout_overflow_tests = executable(
    'layout_overflow_tests',
    files('layout_overflow_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('layout-overflow', layout_overflow_tests, suite: 'unit')

node_reparent_tests = executable(
    'node_reparent_tests',
    files('node_reparent_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('node-reparent', node_reparent_tests, suite: 'unit')

# 这个测试会真的开一个窗口，进而初始化 raylib/GL。无显示环境时保持测试目标存在，
# 但让测试以成功状态报告 skipped，避免 Catch2 的 SKIP 返回码被 Meson 当作失败。
window_tests_option = get_option('window_tests')
window_display_available = run_command('printenv', 'DISPLAY', check: false).stdout().strip() != '' or \
    run_command('printenv', 'WAYLAND_DISPLAY', check: false).stdout().strip() != ''
window_tests_enabled = window_tests_option.enabled() or \
    (window_tests_option.auto() and window_display_available)
window_tests_optional = window_tests_option.auto() and window_display_available
window_test_cpp_args = []
if not window_tests_enabled
    window_test_cpp_args += ['-DNANDINA_SKIP_WINDOW_TESTS']
elif window_tests_optional
    window_test_cpp_args += ['-DNANDINA_OPTIONAL_WINDOW_TESTS']
endif
window_lifecycle_tests = executable(
    'window_lifecycle_tests',
    files('window_lifecycle_tests.cpp'),
    cpp_args : window_test_cpp_args,
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('window-lifecycle', window_lifecycle_tests, suite: 'unit')

drag_controller_tests = executable(
    'drag_controller_tests',
    files('drag_controller_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('drag-controller', drag_controller_tests, suite: 'unit')

pointer_area_layout_tests = executable(
    'pointer_area_layout_tests',
    files('pointer_area_layout_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('pointer-area-layout', pointer_area_layout_tests, suite: 'unit')

drag_gesture_tests = executable(
    'drag_gesture_tests',
    files('drag_gesture_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('drag-gesture', drag_gesture_tests, suite: 'unit')

drag_roundtrip_tests = executable(
    'drag_roundtrip_tests',
    files('drag_roundtrip_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('drag-roundtrip', drag_roundtrip_tests, suite: 'unit')

roving_focus_tests = executable(
    'roving_focus_tests',
    files('roving_focus_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('roving-focus', roving_focus_tests, suite: 'unit')

menu_item_tests = executable(
    'menu_item_tests',
    files('menu_item_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('menu-item', menu_item_tests, suite: 'unit')

checkbox_tests = executable(
    'checkbox_tests',
    files('checkbox_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('checkbox', checkbox_tests, suite: 'unit')

text_area_tests = executable(
    'text_area_tests',
    files('text_area_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('text-area', text_area_tests, suite: 'unit')

slider_tests = executable(
    'slider_tests',
    files('slider_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('slider', slider_tests, suite: 'unit')

switch_tests = executable(
    'switch_tests',
    files('switch_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('switch', switch_tests, suite: 'unit')

badge_tests = executable(
    'badge_tests',
    files('badge_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('badge', badge_tests, suite: 'unit')

card_tests = executable(
    'card_tests',
    files('card_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('card', card_tests, suite: 'unit')

progress_bar_tests = executable(
    'progress_bar_tests',
    files('progress_bar_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('progress-bar', progress_bar_tests, suite: 'unit')

skeleton_tests = executable(
    'skeleton_tests',
    files('skeleton_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('skeleton', skeleton_tests, suite: 'unit')

empty_state_tests = executable(
    'empty_state_tests',
    files('empty_state_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('empty-state', empty_state_tests, suite: 'unit')

alert_tests = executable(
    'alert_tests',
    files('alert_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('alert', alert_tests, suite: 'unit')

button_group_tests = executable(
    'button_group_tests',
    files('button_group_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('button-group', button_group_tests, suite: 'unit')

breadcrumb_tests = executable(
    'breadcrumb_tests',
    files('breadcrumb_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('breadcrumb', breadcrumb_tests, suite: 'unit')

pagination_tests = executable(
    'pagination_tests',
    files('pagination_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('pagination', pagination_tests, suite: 'unit')

spinner_tests = executable(
    'spinner_tests',
    files('spinner_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('spinner', spinner_tests, suite: 'unit')

arc_painter_tests = executable(
    'arc_painter_tests',
    files('arc_painter_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('arc-painter', arc_painter_tests, suite: 'unit')

radio_button_tests = executable(
    'radio_button_tests',
    files('radio_button_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('radio-button', radio_button_tests, suite: 'unit')

toggle_tests = executable(
    'toggle_tests',
    files('toggle_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('toggle', toggle_tests, suite: 'unit')

toggle_group_tests = executable(
    'toggle_group_tests',
    files('toggle_group_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('toggle-group', toggle_group_tests, suite: 'unit')

theme_families_tests = executable(
    'theme_families_tests',
    files('theme_families_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('theme-families', theme_families_tests, suite: 'unit')

color_tests = executable(
    'color_tests',
    files('color_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('color', color_tests, suite: 'unit')

tabs_tests = executable(
    'tabs_tests',
    files('tabs_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('tabs', tabs_tests, suite: 'unit')

tooltip_tests = executable(
    'tooltip_tests',
    files('tooltip_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('tooltip', tooltip_tests, suite: 'unit')

select_tests = executable(
    'select_tests',
    files('select_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('select', select_tests, suite: 'unit')

basic_widgets_tests = executable(
    'basic_widgets_tests',
    files('basic_widgets_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('basic-widgets', basic_widgets_tests, suite: 'unit')

dialog_tests = executable(
    'dialog_tests',
    files('dialog_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('dialog', dialog_tests, suite: 'unit')

popover_tests = executable(
    'popover_tests',
    files('popover_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('popover', popover_tests, suite: 'unit')

dropdown_menu_tests = executable(
    'dropdown_menu_tests',
    files('dropdown_menu_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('dropdown-menu', dropdown_menu_tests, suite: 'unit')

animation_tests = executable(
    'animation_tests',
    files('animation_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('animation', animation_tests, suite: 'unit')

system_fonts_tests = executable(
    'system_fonts_tests',
    files('system_fonts_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('system-fonts', system_fonts_tests, suite: 'unit')

resource_tests = executable(
    'resource_tests',
    files('resource_tests.cpp'),
    dependencies : [nandina_dep, sqlite3_dep, catch2_with_main_dep],
)
test('resource', resource_tests, suite: 'unit')

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
    suite: 'integration',
)
nandina_cli_test = find_program('nandina_cli_test.py')
test(
    'nandina-cli',
    nandina_cli_test,
    args: [nandina, meson.project_source_root()],
    timeout: 600,
    suite: 'integration',
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
    suite: 'integration',
)
nanres_build_workflow_test = find_program('nanres_build_workflow_test.py')
test(
    'nanres-build-workflow',
    nanres_build_workflow_test,
    args: [
        meson.project_source_root() / 'tools' / 'nanres_build_helper.py',
        nanres,
    ],
    suite: 'integration',
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
    suite: 'integration',
)
release_metadata_test = find_program('release_metadata_test.py')
test(
    'release-metadata',
    release_metadata_test,
    args: [meson.project_source_root()],
    suite: 'integration',
)
font_resource_tests = executable(
    'font_resource_tests',
    files('font_resource_tests.cpp'),
    cpp_args : ['-DNANDINA_TEST_FONT_PATH="' + font_test_path + '"'],
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('font-resource', font_resource_tests, suite: 'unit')

application_resource_tests = executable(
    'application_resource_tests',
    files('application_resource_tests.cpp'),
    dependencies: [nandina_dep, catch2_with_main_dep],
)
test('application-resource', application_resource_tests, suite: 'unit')
