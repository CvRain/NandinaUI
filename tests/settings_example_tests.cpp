#include "settings_example.hpp"
#include "gallery_page.hpp"

#include "app/nan_router.hpp"
#include "app/ui_dispatcher.hpp"
#include "foundation/contrast.hpp"
#include "foundation/geometry.hpp"
#include "render/render_device.hpp"
#include "resource/backends/builtin_backend.hpp"
#include "resource/resource_manager.hpp"
#include "scene/input_event.hpp"
#include "scene/scene_tree.hpp"
#include "semantics/semantics.hpp"
#include "text/font_family.hpp"
#include "text/font_loader.hpp"
#include "text/font_pipeline.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"
#include "widget/checkbox.hpp"
#include "widget/label.hpp"
#include "widget/image.hpp"
#include "widget/radio_button.hpp"
#include "widget/slider.hpp"
#include "widget/switch.hpp"
#include "widget/text_field.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <span>
#include <string_view>
#include <vector>

using namespace nandina;
namespace settings = nandina::examples::settings;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void
        draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void
        draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        [[nodiscard]] auto supports_alpha_textures() const -> bool override {
            return true;
        }
        [[nodiscard]] auto create_alpha_texture(
            int,
            int,
            std::span<const std::uint8_t>
        ) -> render::TextureHandle override {
            return {.value = next_texture++};
        }

    private:
        std::uint64_t next_texture = 1;
    };

    template<typename Node, typename Predicate>
    [[nodiscard]] auto find_node(scene::NanNode& root, Predicate&& predicate) -> Node* {
        if (auto* node = dynamic_cast<Node*>(&root); node != nullptr && predicate(*node)) {
            return node;
        }
        for (std::size_t index = 0; index < root.child_count(); ++index) {
            if (auto* found = find_node<Node>(*root.get_child(index), predicate); found != nullptr)
            {
                return found;
            }
        }
        return nullptr;
    }

    [[nodiscard]] auto checkbox_named(scene::NanNode& root, const std::string_view label)
        -> widget::Checkbox* {
        return find_node<widget::Checkbox>(root, [label](const widget::Checkbox& checkbox) {
            return checkbox.label() == label;
        });
    }

    [[nodiscard]] auto switch_named(scene::NanNode& root, const std::string_view label)
        -> widget::Switch* {
        return find_node<widget::Switch>(root, [label](const widget::Switch& switch_control) {
            return switch_control.label() == label;
        });
    }

    [[nodiscard]] auto button_named(scene::NanNode& root, const std::string_view text)
        -> widget::Button* {
        return find_node<widget::Button>(root, [text](const widget::Button& button) {
            return button.text() == text;
        });
    }

    [[nodiscard]] auto radio_named(scene::NanNode& root, const std::string_view label)
        -> widget::RadioButton* {
        return find_node<widget::RadioButton>(root, [label](const widget::RadioButton& radio) {
            return radio.label() == label;
        });
    }

    /// 推进页面转场直至完成（淡入淡出 + 延迟 drop 的 detach）。
    void settle_transition(scene::NanSceneTree& tree) {
        for (int i = 0; i < 30; ++i) {
            tree.process(1.0F / 60.0F);
            tree.flush_tree_mutations();
            {
                auto phase = tree.enter_phase(scene::FramePhase::animation);
                tree.advance_animations(1.0F / 60.0F);
            }
        }
    }
} // namespace

TEST_CASE(
    "settings dashboard exercises stateful controls through semantics",
    "[example][settings]"
) {
    reactive::Graph graph;
    theme::ThemeManager themes;
    settings::SettingsStore store {graph};
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<settings::SettingsStore>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher
    };
    (void)router.push<settings::ShellPage>();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(960.0F, 640.0F)) >= 1);

    auto* diagnostics = checkbox_named(*router.host(), "Send anonymous diagnostics");
    auto* reduced_motion = checkbox_named(*router.host(), "Reduce interface motion");
    auto* notifications = switch_named(*router.host(), "Desktop notifications");
    auto* input = find_node<widget::TextField>(*router.host(), [](const auto&) { return true; });
    auto* save = button_named(*router.host(), "Save preferences");
    auto* reset = button_named(*router.host(), "Reset");
    auto* scale = find_node<widget::Slider>(*router.host(), [](const auto&) { return true; });
    REQUIRE(diagnostics != nullptr);
    REQUIRE(reduced_motion != nullptr);
    REQUIRE(notifications != nullptr);
    REQUIRE(input != nullptr);
    REQUIRE(save != nullptr);
    REQUIRE(reset != nullptr);
    REQUIRE(scale != nullptr);
    REQUIRE(tree.focused_node() == input);
    REQUIRE(notifications->checked());
    REQUIRE_FALSE(diagnostics->checked());
    REQUIRE_FALSE(reduced_motion->checked());
    REQUIRE_FALSE(themes.reduced_motion());
    REQUIRE(scale->value() == 1.0F);

    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        diagnostics->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(diagnostics->checked());
    REQUIRE(tree.perform_semantics_action(
        reduced_motion->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(reduced_motion->checked());
    REQUIRE(themes.motion_preference() == theme::MotionPreference::reduced);
    REQUIRE(themes.reduced_motion());
    REQUIRE(tree.perform_semantics_action(
        scale->semantics_id(),
        {.action = semantics::Action::set_value, .value = "1.25"}
    ));
    REQUIRE(scale->value() == 1.25F);
    REQUIRE(
        find_node<widget::Label>(
            *router.host(),
            [](const widget::Label& label) { return label.text().contains("diagnostics help"); }
        )
        != nullptr
    );

    tree.dispatch_key(scene::KeyEvent(65, scene::KeyEvent::Action::press, {.ctrl = true}));
    tree.dispatch_text_input(scene::TextInputEvent("Review profile"));
    REQUIRE(tree.update_semantics());
    REQUIRE(
        tree.perform_semantics_action(save->semantics_id(), {.action = semantics::Action::activate})
    );
    REQUIRE(
        find_node<widget::Label>(
            *router.host(),
            [](const widget::Label& label) {
                return label.text() == "Preferences saved for Review profile";
            }
        )
        != nullptr
    );

    // Reset 现在先弹出确认对话框，再点 Confirm reset 真正重置。
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        reset->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    auto* confirm = button_named(*router.host(), "Confirm reset");
    REQUIRE(confirm != nullptr);
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        confirm->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(input->value() == "Nandina developer");
    REQUIRE(notifications->checked());
    REQUIRE_FALSE(diagnostics->checked());
    REQUIRE_FALSE(reduced_motion->checked());
    REQUIRE(themes.motion_preference() == theme::MotionPreference::system);
    REQUIRE_FALSE(themes.reduced_motion());
    REQUIRE(scale->value() == 1.0F);
}

TEST_CASE("settings dashboard survives appearance switching and redraw", "[example][settings]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    settings::SettingsStore store {graph};
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<settings::SettingsStore>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher
    };
    (void)router.push<settings::ShellPage>();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(960.0F, 640.0F)) >= 1);

    RecordingDevice dev;
    themes.set_preference(theme::ThemePreference::dark);
    REQUIRE(tree.layout_root(foundation::NanSize(960.0F, 640.0F)) >= 1);
    tree.draw(dev);

    themes.set_preference(theme::ThemePreference::light);
    REQUIRE(tree.layout_root(foundation::NanSize(960.0F, 640.0F)) >= 1);
    tree.draw(dev);
}

TEST_CASE(
    "settings General page visibly exercises percentage sizing",
    "[example][settings][layout]"
) {
    reactive::Graph graph;
    theme::ThemeManager themes;
    settings::SettingsStore store {graph};
    app::NanRouter router {graph, themes, &store, app::nan_type_key<settings::SettingsStore>()};
    (void)router.push<settings::GeneralPage>();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());

    REQUIRE(tree.layout_root(foundation::NanSize(720.0F, 520.0F)) >= 1);
    auto* save = button_named(*router.host(), "Save preferences");
    REQUIRE(save != nullptr);
    auto* actions = save->parent() != nullptr ? save->parent()->as_control() : nullptr;
    REQUIRE(actions != nullptr);
    REQUIRE(save->width() == Catch::Approx(actions->width() * 0.5F));
    const float wide_width = save->width();

    REQUIRE(tree.layout_root(foundation::NanSize(520.0F, 520.0F)) >= 1);
    REQUIRE(save->width() == Catch::Approx(actions->width() * 0.5F));
    REQUIRE(save->width() < wide_width);
}

TEST_CASE("settings appearance radios switch the theme preference", "[example][settings]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    settings::SettingsStore store {graph};
    app::NanRouter router {graph, themes, &store, app::nan_type_key<settings::SettingsStore>()};
    (void)router.push<settings::AppearancePage>();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(720.0F, 520.0F)) >= 1);

    auto* system = radio_named(*router.host(), "System");
    auto* dark = radio_named(*router.host(), "Dark");
    auto* light = radio_named(*router.host(), "Light");
    REQUIRE(system != nullptr);
    REQUIRE(dark != nullptr);
    REQUIRE(light != nullptr);
    // 初始选中 System（build() 中 select 了首项）。
    REQUIRE(system->checked());
    REQUIRE_FALSE(dark->checked());

    // 通过语义激活真实触发单选回调（build() 返回后回调仍持有主题管理器引用）。
    REQUIRE(tree.update_semantics());
    REQUIRE(
        tree.perform_semantics_action(dark->semantics_id(), {.action = semantics::Action::activate})
    );
    REQUIRE(themes.preference() == theme::ThemePreference::dark);
    REQUIRE_FALSE(system->checked());

    REQUIRE(tree.perform_semantics_action(
        light->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(themes.preference() == theme::ThemePreference::light);
    REQUIRE_FALSE(dark->checked());
}

TEST_CASE(
    "settings brand theme keeps paired contrast in both appearances",
    "[example][settings][contrast]"
) {
    reactive::Graph graph;
    theme::ThemeManager themes;
    settings::SettingsStore store {graph};
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<settings::SettingsStore>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher
    };
    (void)router.push<settings::ShellPage>();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(960.0F, 640.0F)) >= 1);

    const auto& design = themes.design_system();
    const auto require_aa_text = [](const theme::NanColorScheme& scheme) {
        REQUIRE(
            foundation::nan_contrast_ratio(scheme.primary, scheme.on_primary)
            >= foundation::nan_contrast_aa_text
        );
        REQUIRE(
            foundation::nan_contrast_ratio(scheme.surface, scheme.on_surface)
            >= foundation::nan_contrast_aa_text
        );
        REQUIRE(
            foundation::nan_contrast_ratio(scheme.background, scheme.on_background)
            >= foundation::nan_contrast_aa_text
        );
    };
    require_aa_text(design.light);
    require_aa_text(design.dark);

    // 默认族 butter 暗色品牌提升到更亮档位（dark_brand = shade_300）。
    REQUIRE(design.dark.primary.oklch().light > design.light.primary.oklch().light);
}

TEST_CASE(
    "settings sidebar navigates sections and pushes a parameterized detail page",
    "[example][settings][router]"
) {
    reactive::Graph graph;
    theme::ThemeManager themes;
    settings::SettingsStore store {graph};
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<settings::SettingsStore>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher
    };
    (void)router.push<settings::ShellPage>();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(960.0F, 640.0F)) >= 1);

    // 初始内容为 General 页。
    REQUIRE(button_named(*router.host(), "Save preferences") != nullptr);

    // 侧边栏切到 Appearance：General 被替换，出现 Appearance 单选组。
    auto* appearance_nav = button_named(*router.host(), "Appearance");
    REQUIRE(appearance_nav != nullptr);
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        appearance_nav->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(dispatcher.drain() == 1);
    settle_transition(tree);
    REQUIRE(radio_named(*router.host(), "Dark") != nullptr);
    REQUIRE(button_named(*router.host(), "Save preferences") == nullptr);

    // 侧边栏切到 About：出现 detail 入口按钮。
    auto* about_nav = button_named(*router.host(), "About");
    REQUIRE(about_nav != nullptr);
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        about_nav->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(dispatcher.drain() == 1);
    settle_transition(tree);
    auto* detail_button = button_named(*router.host(), "Open component detail");
    REQUIRE(detail_button != nullptr);

    // push 带参数的 DetailPage：内容区显示参数化标题。
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        detail_button->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(dispatcher.drain() == 1);
    settle_transition(tree);
    auto* back = button_named(*router.host(), "Back");
    REQUIRE(back != nullptr);
    REQUIRE(
        find_node<widget::Label>(
            *router.host(),
            [](const widget::Label& label) { return label.text() == "Select"; }
        )
        != nullptr
    );

    // Back 弹出 DetailPage，回到 About。
    REQUIRE(tree.update_semantics());
    REQUIRE(
        tree.perform_semantics_action(back->semantics_id(), {.action = semantics::Action::activate})
    );
    REQUIRE(dispatcher.drain() == 1);
    settle_transition(tree);
    REQUIRE(button_named(*router.host(), "Open component detail") != nullptr);
    REQUIRE(
        find_node<widget::Label>(
            *router.host(),
            [](const widget::Label& label) { return label.text() == "Select"; }
        )
        == nullptr
    );
}

TEST_CASE(
    "settings About navigation falls back when optional font families are unavailable",
    "[example][settings][font][router]"
) {
    reactive::Graph graph;
    theme::ThemeManager themes;
    settings::SettingsStore store {graph};
    app::UiDispatcher dispatcher;
    resource::ResourceManager resources;
    (void)resources.mount(resource::BuiltinBackend::create(), -1000);
    text::FontLoader font_loader(resources);
    text::FontFamilyRegistry font_families;
    REQUIRE(text::register_builtin_default_font_family(font_families).has_value());
    RecordingDevice device;
    text::FontPipelineCache font_cache(device, font_loader, font_families);
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<settings::SettingsStore>(),
        &resources,
        &font_loader,
        &font_families,
        &dispatcher
    };
    (void)router.push<settings::ShellPage>();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_font_context(font_cache);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(960.0F, 640.0F)) >= 1);

    auto* about_nav = button_named(*router.host(), "About");
    REQUIRE(about_nav != nullptr);
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        about_nav->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    std::size_t drained = 0;
    REQUIRE_NOTHROW(drained = dispatcher.drain());
    REQUIRE(drained == 1);
    REQUIRE_NOTHROW(settle_transition(tree));

    auto* custom_font = find_node<widget::Label>(
        *router.host(),
        [](const widget::Label& label) {
            return label.text() == "Custom font: the quick brown fox";
        }
    );
    REQUIRE(custom_font != nullptr);
    REQUIRE(custom_font->font().family == resource::ResourceKey("demo"));
    REQUIRE(custom_font->text_pipeline().backend != nullptr);
    REQUIRE(custom_font->text_pipeline().renderer != nullptr);
}

TEST_CASE("settings gallery exposes all packaged image samples", "[example][settings][gallery]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    settings::SettingsStore store {graph};
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<settings::SettingsStore>()
    };
    (void)router.push<settings::GalleryPage>();

    std::vector<std::string> sources;
    const auto collect = [&sources](const auto& self, scene::NanNode& node) -> void {
        if (auto* image = dynamic_cast<widget::Image*>(&node); image != nullptr) {
            sources.emplace_back(image->source());
        }
        for (std::size_t index = 0; index < node.child_count(); ++index) {
            self(self, *node.get_child(index));
        }
    };
    collect(collect, *router.host());

    REQUIRE(sources == std::vector<std::string> {
        "res://random_wallpaper.png",
        "res://random_wallpaper.jpg",
        "res://random_wallpaper-1.png",
        "res://random_wallpaper-1.jpg",
    });
}

TEST_CASE(
    "settings nested router inherits application resource services",
    "[example][settings][gallery][resources]"
) {
    reactive::Graph graph;
    theme::ThemeManager themes;
    settings::SettingsStore store {graph};
    app::UiDispatcher dispatcher;
    resource::ResourceManager resources;
    (void)resources.mount(resource::BuiltinBackend::create(), -1000);
    text::FontLoader font_loader(resources);
    text::FontFamilyRegistry font_families;
    REQUIRE(text::register_builtin_default_font_family(font_families).has_value());
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<settings::SettingsStore>(),
        &resources,
        &font_loader,
        &font_families,
        &dispatcher
    };
    (void)router.push<settings::ShellPage>();

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(960.0F, 640.0F)) >= 1);

    auto* gallery_nav = button_named(*router.host(), "Gallery");
    REQUIRE(gallery_nav != nullptr);
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        gallery_nav->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(dispatcher.drain() == 1);
    settle_transition(tree);

    std::vector<widget::Image*> images;
    const auto collect = [&images](const auto& self, scene::NanNode& node) -> void {
        if (auto* image = dynamic_cast<widget::Image*>(&node); image != nullptr) {
            images.push_back(image);
        }
        for (std::size_t index = 0; index < node.child_count(); ++index) {
            self(self, *node.get_child(index));
        }
    };
    collect(collect, *router.host());
    REQUIRE(images.size() == 4);
    for (const auto* image : images) {
        REQUIRE(image->resource_manager() == &resources);
    }
}
