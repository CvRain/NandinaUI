# nandina 库

builtin_font_source = files('assets/default_font/default-ui.ttf')
builtin_font_license_source = files('assets/default_font/default-ui-license.txt')
builtin_font_data = custom_target(
    'nandina-builtin-default-font',
    input : builtin_font_source,
    output : 'builtin_default_font_data.cpp',
    command : [embed_binary, '@INPUT@', '@OUTPUT@', 'builtin_default_font_bytes'],
)
builtin_font_license_data = custom_target(
    'nandina-builtin-default-font-license',
    input : builtin_font_license_source,
    output : 'builtin_default_font_license_data.cpp',
    command : [embed_binary, '@INPUT@', '@OUTPUT@', 'builtin_default_font_license_bytes'],
)
builtin_font_size = fs.size(builtin_font_source)
builtin_font_license_size = fs.size(builtin_font_license_source)

module_fundation = files(
    'foundation/nandina_color.hpp',
    'foundation/nandina_color.cpp',
    'foundation/nandina_color_space.hpp',
    'foundation/nandina_color_space.cpp',
    'foundation/contrast.hpp',
    'foundation/geometry.hpp',
    'foundation/geometry.cpp',
    'foundation/transform2d.hpp',
    'foundation/transform2d.cpp',
    'foundation/utf8.hpp',
    'foundation/utf8.cpp',
    'foundation/json.hpp',
    'foundation/nan_logger.hpp',
    'foundation/nan_logger.cpp',
)

module_reactive = files(
    'reactive/graph.hpp',
    'reactive/graph.cpp',
    'reactive/signal.hpp',
    'reactive/computed.hpp',
    'reactive/effect.hpp',
    'reactive/event.hpp',
    'reactive/property.hpp',
    'reactive/batch.hpp',
    'reactive/scope.hpp',
    'reactive/reactive.hpp',
)

module_resource = files(
    'resource/resource.hpp',
    'resource/resource.cpp',
    'resource/resource_backend.hpp',
    'resource/resource_backend.cpp',
    'resource/resource_stream.hpp',
    'resource/resource_stream.cpp',
    'resource/file_resource_stream.hpp',
    'resource/file_resource_stream.cpp',
    'resource/resource_manager.hpp',
    'resource/resource_manager.cpp',
    'resource/resource_uri.hpp',
    'resource/resource_uri.cpp',
    'resource/resource_scanner.hpp',
    'resource/resource_scanner.cpp',
    'resource/platform_resource_locator.hpp',
    'resource/platform_resource_locator.cpp',
    'resource/build_location.hpp',
    'resource/build_location.cpp',
    'resource/backends/memory_backend.hpp',
    'resource/backends/memory_backend.cpp',
    'resource/backends/builtin_backend.hpp',
    'resource/backends/builtin_backend.cpp',
    'resource/backends/directory_backend.hpp',
    'resource/backends/directory_backend.cpp',
    'resource/backends/sqlite_backend.hpp',
    'resource/backends/sqlite_backend.cpp',
)

module_scene = files(
    'scene/canvas_layer.hpp',
    'scene/canvas_layer.cpp',
    'scene/frame_scheduler.hpp',
    'scene/clipboard.hpp',
    'scene/input_event.hpp',
    'scene/node.hpp',
    'scene/node.cpp',
    'scene/node2d.hpp',
    'scene/node2d.cpp',
    'scene/scene_tree.hpp',
    'scene/scene_tree.cpp',
    'scene/control.hpp',
    'scene/control.cpp',
)

module_semantics = files(
    'semantics/semantics.hpp',
    'semantics/semantics.cpp',
)

module_render = files(
    'render/render_device.hpp',
    'render/image_decoder.hpp',
    'render/image_decoder.cpp',
    'render/texture_cache.hpp',
    'render/texture_cache.cpp',
    'render/clip_stack.hpp',
    'render/draw_context.hpp',
    'render/backends/sdf_primitive_geometry.hpp',
    'render/backends/raylib_device.hpp',
    'render/backends/raylib_device.cpp',
)

module_theme = files(
    'theme/tokens.hpp',
    'theme/motion.hpp',
    'theme/theme.hpp',
    'theme/theme.cpp',
    'theme/appearance.hpp',
    'theme/design_system.hpp',
    'theme/design_system.cpp',
    'theme/visual_state.hpp',
    'theme/style_context.hpp',
    'theme/nan_style.hpp',
    'theme/nan_style.cpp',
    'theme/theme_manager.hpp',
    'theme/theme_manager.cpp',
    'theme/builtin_themes.hpp',
    'theme/builtin_themes.cpp',
    'theme/style_document.hpp',
    'theme/style_document.cpp',
)

module_text = files(
    'text/font_face.hpp',
    'text/freetype_font_face.cpp',
    'text/font_loader.hpp',
    'text/font_loader.cpp',
    'text/font_family.hpp',
    'text/font_family.cpp',
    'text/system_fonts.hpp',
    'text/system_fonts.cpp',
    'text/font_pipeline.hpp',
    'text/font_pipeline.cpp',
    'text/glyph_atlas.hpp',
    'text/glyph_atlas.cpp',
    'text/harfbuzz_text_backend.hpp',
    'text/harfbuzz_text_backend.cpp',
    'text/glyph_run_renderer.hpp',
    'text/glyph_run_renderer.cpp',
)

module_widget = files(
    'widget/visual_property.hpp',
    'widget/authoring.hpp',
    'widget/build_context.hpp',
    'widget/reactive_control.hpp',
    'widget/bindable_rect.hpp',
    'widget/declarative.hpp',
    'widget/list_view.hpp',
    'widget/primitives/box_painter.hpp',
    'widget/primitives/box_presentation.hpp',
    'widget/primitives/shadow_painter.hpp',
    'widget/primitives/focus_ring_painter.hpp',
    'widget/primitives/ripple_painter.hpp',
    'widget/primitives/pressable.hpp',
    'widget/primitives/pressable.cpp',
    'widget/primitives/text_layout.hpp',
    'widget/primitives/text_layout.cpp',
    'widget/primitives/text_layout_backend.hpp',
    'widget/primitives/text_layout_backend.cpp',
    'widget/primitives/text.hpp',
    'widget/primitives/text.cpp',
    'widget/primitives/text_presentation.hpp',
    'widget/primitives/editable_text.hpp',
    'widget/primitives/editable_text.cpp',
    'widget/component_traits.hpp',
    'widget/builtin_component_traits.hpp',
    'widget/controls.hpp',
    'widget/label.hpp',
    'widget/label.cpp',
    'widget/layout.hpp',
    'widget/layout.cpp',
    'widget/grid.hpp',
    'widget/grid.cpp',
    'widget/scroll_view.hpp',
    'widget/scroll_view.cpp',
    'widget/button.hpp',
    'widget/button.cpp',
    'widget/checkbox.hpp',
    'widget/checkbox.cpp',
    'widget/slider.hpp',
    'widget/slider.cpp',
    'widget/text_field.hpp',
    'widget/text_field.cpp',
    'widget/switch.hpp',
    'widget/switch.cpp',
    'widget/badge.hpp',
    'widget/badge.cpp',
    'widget/card.hpp',
    'widget/card.cpp',
    'widget/progress_bar.hpp',
    'widget/progress_bar.cpp',
    'widget/radio_group.hpp',
    'widget/radio_group.cpp',
    'widget/radio_button.hpp',
    'widget/radio_button.cpp',
    'widget/tabs.hpp',
    'widget/tabs.cpp',
    'widget/tooltip.hpp',
    'widget/tooltip.cpp',
    'widget/select.hpp',
    'widget/select.cpp',
    'widget/divider.hpp',
    'widget/divider.cpp',
    'widget/avatar.hpp',
    'widget/avatar.cpp',
    'widget/chip.hpp',
    'widget/chip.cpp',
    'widget/dialog.hpp',
    'widget/dialog.cpp',
    'widget/image.hpp',
    'widget/image.cpp',
)

module_animation = files(
    'animation/animation_host.hpp',
    'animation/animation_host.cpp',
    'animation/animated_property.hpp',
    'animation/behavior.hpp',
    'animation/easing.hpp',
    'animation/group.hpp',
    'animation/group.cpp',
    'animation/keyframes.hpp',
    'animation/motion.hpp',
    'animation/property_endpoint.hpp',
    'animation/spring.hpp',
    'animation/tween.hpp',
)

module_app = files(
    'app/window_metrics.hpp',
    'app/window_metrics.cpp',
    'app/viewport_scaling.hpp',
    'app/viewport_scaling.cpp',
    'app/ui_dispatcher.hpp',
    'app/ui_dispatcher.cpp',
    'app/async_scope.hpp',
    'app/async_scope.cpp',
    'app/application_config.hpp',
    'app/application_config.cpp',
    'app/window_config.hpp',
    'app/nan_store.hpp',
    'app/nan_page.hpp',
    'app/root_view.hpp',
    'app/nan_router.hpp',
    'app/nan_router.cpp',
    'app/nan_application.hpp',
    'app/nan_application.cpp',
    'app/nan_window.hpp',
    'app/nan_window.cpp',
)
if host_machine.system() == 'linux'
    module_app += files(
        'app/detail/linux_clipboard.hpp',
        'app/detail/linux_clipboard.cpp',
    )
endif

module_physics2d = []
if box2d_dep.found()
    module_physics2d = files(
        'physics2d/physics_world2d.hpp',
        'physics2d/physics_world2d.cpp',
    )
endif

# 创建静态库
nandina_dependencies = [spdlog_dep, raylib_dep, freetype_dep, harfbuzz_dep, fribidi_dep, utf8proc_dep, sqlite3_dep, tomlplusplus_dep, nlohmann_json_dep]
nandina_cpp_args = [
    '-DNANDINA_BUILTIN_DEFAULT_FONT_SIZE=' + builtin_font_size.to_string(),
    '-DNANDINA_BUILTIN_DEFAULT_FONT_LICENSE_SIZE=' + builtin_font_license_size.to_string(),
]
if box2d_dep.found()
    nandina_dependencies += box2d_dep
    nandina_cpp_args += '-DNANDINA_HAS_PHYSICS2D=1'
endif

nandina_lib = static_library(
    'nandina',
    [module_fundation, module_reactive, module_resource, module_scene, module_semantics, module_render, module_theme, module_text, module_widget, module_app, module_animation, module_physics2d, builtin_font_data, builtin_font_license_data],
    cpp_args : nandina_cpp_args,
    dependencies : nandina_dependencies,
)

# 创建依赖对象，供 example 使用
nandina_dep = declare_dependency(
    link_with : nandina_lib,
    include_directories : include_directories('.'),
    dependencies : nandina_dependencies,
)
