# 单元测试 (Catch2 v3, 无窗口)
#
# 每个测试必须声明 suite，CI 依据 suite 选择要跑的子集，不再维护手写名单：
#   unit        —— C++ 单测；ASan/UBSan 任务跑 `--suite unit`
#   integration —— Python CLI / 构建工作流测试（慢），只在常规任务与对应矩阵中跑
# 新增测试时必须带上 suite，否则会静默脱离 sanitizer 覆盖。

parameter_state_cache_tests = executable(
    'parameter_state_cache_tests',
    files('parameter_state_cache_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('parameter-state-cache', parameter_state_cache_tests, suite: 'unit')

foundation_tests = executable(
    'foundation_tests',
    files('foundation_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('foundation', foundation_tests, suite: 'unit')

layout_constraints_tests = executable(
    'layout_constraints_tests',
    files('layout_constraints_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('layout-constraints', layout_constraints_tests, suite: 'unit')

anchors_boundary_tests = executable(
    'anchors_boundary_tests',
    files('anchors_boundary_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('anchors-boundary', anchors_boundary_tests, suite: 'unit')

anchors_tests = executable(
    'anchors_tests',
    files('anchors_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('anchors', anchors_tests, suite: 'unit')

anchors_authoring_tests = executable(
    'anchors_authoring_tests',
    files('anchors_authoring_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('anchors-authoring', anchors_authoring_tests, suite: 'unit')

anchors_showcase_tests = executable(
    'anchors_showcase_tests',
    files('anchors_showcase_tests.cpp', '../showcase/pages/anchors_page.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('anchors-showcase', anchors_showcase_tests, suite: 'unit')

text_layout_header_tests = executable(
    'text_layout_header_tests',
    files('text_layout_header_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('text-protocol-header', text_layout_header_tests, suite: 'unit')

font_request_header_tests = executable(
    'font_request_header_tests',
    files('font_request_header_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('font-request-header', font_request_header_tests, suite: 'unit')

motion_spec_header_tests = executable(
    'motion_spec_header_tests',
    files('motion_spec_header_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('motion-spec-header', motion_spec_header_tests, suite: 'unit')

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

# SDL 视觉回归：用**软件渲染器**做确定性像素断言（不需要窗口/显示器/GPU），
# 期望值是解析计算出来的合成色，而不是某个后端某次画出来的图。
# 没有 SDL3 后端时不注册：没有 SDL 设备可测，注册一个必然跳过的目标只会制造噪声。
if sdl3_dep.found()
    sdl_visual_tests = executable(
        'sdl_visual_tests',
        files('sdl_visual_tests.cpp'),
        dependencies : [nandina_dep, catch2_with_main_dep],
    )
    test('sdl-visual', sdl_visual_tests, suite: 'unit')
    sdf_state_probe = executable(
        'sdf_state_probe',
        files('../probe/cross_platform/desktop/sdf_state_probe.cpp'),
        dependencies: [sdl3_dep],
    )
    # 生产 SDF 路径的设备级像素测试：GPU renderer 走 SDF、软件 renderer 走 CPU 几何，
    # 同一几何与解析预期对照。无显示或拿不到 GPU renderer 时以 77 跳过。
    sdf_device_tests = executable(
        'sdf_device_test',
        files('sdf_device_test.cpp'),
        dependencies : [nandina_dep, sdl3_dep],
    )
    test(
        'sdf-device',
        sdf_device_tests,
        suite: 'integration',
        is_parallel: false,
        timeout: 120,
    )
    test(
        'sdf-state-probe',
        find_program('python3'),
        args: [files('sdf_state_probe_test.py'), sdf_state_probe],
        suite: 'integration',
        is_parallel: false,
        timeout: 180,
    )
endif

# 着色器产物一致性是纯文件检查（不需要 SDL、不需要编译器）：放在 SDL 条件之外，
# 关闭 SDL 的构建同样会执行它。覆盖探针与生产两条 shader。
test(
    'shader-artifacts',
    find_program('python3'),
    args: [files('../tools/shader_artifacts.py')],
    suite: 'integration',
)
test(
    'shader-artifacts-negative',
    find_program('python3'),
    args: [files('shader_artifacts_test.py')],
    suite: 'integration',
)

if box2d_dep.found()
    physics2d_tests = executable(
        'physics2d_tests',
        files('physics2d_tests.cpp'),
        dependencies : [nandina_dep, catch2_with_main_dep],
    )
    test('physics2d', physics2d_tests, suite: 'unit')
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

# 默认 CPU 解码器（stb）：格式、crop/resize/tint、错误输入与后台线程。
image_decoder_tests = executable(
    'image_decoder_tests',
    files('image_decoder_tests.cpp'),
    cpp_args : [
        '-DNANDINA_TEST_IMAGE_DIR="' + meson.current_source_dir() / 'fixtures' / 'images' + '"',
    ],
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('image-decoder', image_decoder_tests, suite: 'unit')

reactive_tests = executable(
    'reactive_tests',
    files('reactive_tests.cpp', 'effect_initialization_tests.cpp'),
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

group_authoring_tests = executable(
    'group_authoring_tests',
    files('group_authoring_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('group-authoring', group_authoring_tests, suite: 'unit')

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

# 布局收敛：一次 layout_root 之后树必须不再脏；arrange 不得越过子项自己的尺寸规格。
layout_convergence_tests = executable(
    'layout_convergence_tests',
    files('layout_convergence_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('layout-convergence', layout_convergence_tests, suite: 'unit')

node_reparent_tests = executable(
    'node_reparent_tests',
    files('node_reparent_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('node-reparent', node_reparent_tests, suite: 'unit')

# 真实 SDL3 窗口测试；wrapper 将 Catch2 的全跳过码 4 映射为 Meson 的 77。
window_tests_option = get_option('window_tests')
window_display_available = host_machine.system() == 'windows' or \
    run_command('printenv', 'DISPLAY', check: false).stdout().strip() != '' or \
    run_command('printenv', 'WAYLAND_DISPLAY', check: false).stdout().strip() != ''
if window_tests_option.enabled() and not sdl3_dep.found()
    error('window_tests=enabled requires sdl_backend=enabled')
endif
window_tests_enabled = sdl3_dep.found() and (window_tests_option.enabled() or \
    (window_tests_option.auto() and window_display_available))
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
test(
    'window-lifecycle',
    python,
    args: [files('../tools/run_window_test.py'), window_lifecycle_tests],
    suite: 'unit',
)
test(
    'window-test-runner',
    python,
    args: [files('window_test_runner_test.py'), files('../tools/run_window_test.py')],
    suite: 'integration',
)
test(
    'windows-runtime-dlls',
    python,
    args: [files('copy_runtime_dlls_test.py'), files('../tools/copy_runtime_dlls.py')],
    suite: 'integration',
)

window_backend_tests = executable(
    'window_backend_tests',
    files('window_backend_tests.cpp'),
    cpp_args: sdl3_dep.found() ? ['-DNANDINA_TEST_HAS_SDL=1'] : [],
    dependencies: [nandina_dep, catch2_with_main_dep],
)
test('window-backend', window_backend_tests, suite: 'unit')

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

context_menu_tests = executable(
    'context_menu_tests',
    files('context_menu_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('context-menu', context_menu_tests, suite: 'unit')

combobox_tests = executable(
    'combobox_tests',
    files('combobox_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('combobox', combobox_tests, suite: 'unit')

command_palette_tests = executable(
    'command_palette_tests',
    files('command_palette_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('command-palette', command_palette_tests, suite: 'unit')

hover_card_tests = executable(
    'hover_card_tests',
    files('hover_card_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('hover-card', hover_card_tests, suite: 'unit')

alert_dialog_tests = executable(
    'alert_dialog_tests',
    files('alert_dialog_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('alert-dialog', alert_dialog_tests, suite: 'unit')

animation_tests = executable(
    'animation_tests',
    files('animation_tests.cpp'),
    dependencies : [nandina_dep, catch2_with_main_dep],
)
test('animation', animation_tests, suite: 'unit')

# 系统字体发现测试依赖 Linux 的字体目录布局（HOME/XDG 与 /usr/share/fonts），
# 用 setenv 隔离 HOME；Windows 上 setenv 不存在、字体目录也不同，未验收前只跑 Linux。
if host_machine.system() == 'linux'
    system_fonts_tests = executable(
        'system_fonts_tests',
        files('system_fonts_tests.cpp'),
        dependencies : [nandina_dep, catch2_with_main_dep],
    )
    test('system-fonts', system_fonts_tests, suite: 'unit')
endif

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
# 页面 golden 清单完整性（候选层）：文件存在、哈希/尺寸匹配、状态合法。
# 像素比较是手动工具（tools/compare_backend_golden.py），approved 条目才判失败。
test(
    'golden-manifest',
    find_program('python3'),
    args: [files('golden_manifest_test.py')],
    suite: 'integration',
)

# 头文件表面门禁（docs/references/sdk_surface.md §4）：分类完整性、包含方向、独立可包含。
# 独立可包含检查会编译每个 public/experimental 头（182 个，约 16 s），可选功能关闭时按配置跳过。
header_surface_gate = find_program('../tools/check_headers.py')
test(
    'header-surface',
    header_surface_gate,
    args: [
        '--source', meson.project_source_root(),
        '--build-dir', meson.project_build_root(),
    ],
    timeout: 300,
    suite: 'integration',
)

# SDK 打包测试需要 vendored SDL3 源码树（subprojects/SDL3 与 wrap 检出）：
# 无后端（sdl_backend=disabled）或系统 SDL 构建没有这份源码，跳过并给出理由，
# 而不是让 `sdk_bundle` 在缺 wrap 检出时以异常中断整套集成测试。
if sdl3_origin == 'vendored'
    nandina_subproject_fixture_test = find_program('nandina_subproject_fixture_test.py')
    test(
        'nandina-subproject-fixture',
        nandina_subproject_fixture_test,
        args: [
            meson.project_source_root(),
            meson.project_source_root() / 'tools' / 'sdk_bundle.py',
        ],
        # 打包现在把依赖源码（SDL3，约 53 MB）一起收进 SDK，且 fixture 会打包两次；
        # 超时按实测留余量，不靠"重跑一次就过"。
        timeout: 900,
        suite: 'integration',
    )
    # 静态 SDL3 检查无 SDL3 DSO 依赖；共享模式检查便携布局（ldd/readelf）。
    if host_machine.system() == 'linux' and get_option('showcase')
        portable_bundle_test = find_program('portable_bundle_test.py')
        test(
            'portable-bundle',
            portable_bundle_test,
            args: [meson.project_build_root()],
            timeout: 120,
            suite: 'integration',
        )
    endif
    # SDK 下游 vendored SDL 消费：解压到全新目录、断网配置/编译/链接/运行。
    # 与资源 fixture 分开，因为后者用 sdl_backend=disabled 控制时长，不覆盖 SDL 交付路径；
    # 这里跑一次完整 SDK 打包 + 带 SDL3 的下游构建（实测见 docs/references/dependency_policy.md §2.5）。
    sdk_vendored_consumer_test = find_program('sdk_vendored_consumer_test.py')
    test(
        'sdk-vendored-consumer',
        sdk_vendored_consumer_test,
        args: [
            meson.project_source_root(),
            meson.project_source_root() / 'tools' / 'sdk_bundle.py',
        ],
        timeout: 1500,
        is_parallel: false,
        suite: 'integration',
    )
else
    message('SDK archive fixtures not registered: SDL3 source is not vendored')
endif

test(
    'ci-test-suites',
    python,
    args: [
        files('ci_test_suites_test.py'),
        meson.project_source_root() / 'tools' / 'check_test_suites.py',
    ],
    suite: 'integration',
)
test(
    'format-gate',
    python,
    args: [
        files('format_gate_test.py'),
        meson.project_source_root() / 'tools' / 'check_format.py',
    ],
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
