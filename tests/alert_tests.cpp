//
// Theme / Alert tests.
//

#include <nandina/render/render_device.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/alert.hpp>
#include <nandina/widget/badge.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/key_codes.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string_view>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rounded_rects = 0;
        int rounded_outlines = 0;
        int text_draws = 0;
        int lines = 0;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void draw_rect_outline(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_rounded_rect(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {
            ++rounded_rects;
        }
        void draw_rounded_rect_outline(
            const foundation::NanRect&,
            float,
            float,
            const foundation::NanColor&
        ) override {
            ++rounded_outlines;
        }
        bool supports_rounded_rect() const override {
            return true;
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {
            ++lines;
        }
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {}
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {
            ++text_draws;
        }
    };

    [[nodiscard]] auto light_of(const foundation::NanColor& color) -> float {
        return color.oklch().light;
    }
} // namespace

TEST_CASE("alert resolves each tone to its semantic colour pair", "[alert][theme]") {
    const auto design = theme::default_design_system();

    struct Expectation {
        theme::AlertTone tone;
        const foundation::NanColor* tone_color;
    };
    const Expectation expectations[] = {
        {theme::AlertTone::info, &design.light.info},
        {theme::AlertTone::success, &design.light.success},
        {theme::AlertTone::warning, &design.light.warning},
        {theme::AlertTone::error, &design.light.error},
    };

    for (const auto& expected: expectations) {
        const auto style = theme::resolve_alert(
            design,
            theme::ColorAppearance::light,
            expected.tone,
            theme::AlertVisualState::normal
        );
        const float tone_light = light_of(*expected.tone_color);
        // 容器填充是该 tone 色的低透明度版本，边框 / 图标用实色。
        REQUIRE(light_of(style.container.fill) == Catch::Approx(tone_light));
        REQUIRE(style.container.fill.alpha() == Catch::Approx(0.12F));
        REQUIRE(light_of(style.container.border) == Catch::Approx(tone_light));
        REQUIRE(style.container.border.alpha() == Catch::Approx(1.0F));
        REQUIRE(light_of(style.icon) == Catch::Approx(tone_light));
        // 文字统一用正文 / 弱化正文，保证在浅色低透明填充上仍可读。
        REQUIRE(light_of(style.title.color) == Catch::Approx(light_of(design.light.foreground)));
        REQUIRE(
            light_of(style.description.color)
            == Catch::Approx(light_of(design.light.muted_foreground))
        );
        REQUIRE(style.title.font_size == Catch::Approx(design.tokens.typography.label_lg));
        REQUIRE(style.description.font_size == Catch::Approx(design.tokens.typography.label_sm));
        REQUIRE(style.container.radius == Catch::Approx(design.tokens.radius.md));
        REQUIRE(style.container.border_width == Catch::Approx(design.tokens.border.thin));
        REQUIRE(style.metrics.gap == Catch::Approx(design.tokens.spacing.sm));
        REQUIRE(style.metrics.padding_x == Catch::Approx(design.tokens.spacing.lg));
        REQUIRE(style.metrics.padding_y == Catch::Approx(design.tokens.spacing.md));
        REQUIRE(style.metrics.box_size == Catch::Approx(20.0F));
        REQUIRE(style.metrics.preferred_width == Catch::Approx(320.0F));
    }

    // 组件级入口：resolved_style() 走同一条 tone 选择路径。
    for (const auto& expected: expectations) {
        auto alert = widget::Alert::create();
        alert->set_title("tone");
        alert->set_tone(expected.tone);
        const auto widget_style = alert->resolved_style();
        REQUIRE(
            light_of(widget_style.container.border)
            == Catch::Approx(light_of(*expected.tone_color))
        );
        REQUIRE(light_of(widget_style.icon) == Catch::Approx(light_of(*expected.tone_color)));
    }
}

TEST_CASE("alert with no title description or icon measures to zero", "[alert][layout]") {
    auto alert = widget::Alert::create();
    REQUIRE(alert->title().empty());
    REQUIRE(alert->description().empty());
    REQUIRE(alert->icon() == nullptr);
    REQUIRE(alert->action() == nullptr);
    REQUIRE(alert->tone() == theme::AlertTone::info);
    REQUIRE_FALSE(alert->dismissible());

    const auto loose = alert->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(loose.get_width() == Catch::Approx(0.0F));
    REQUIRE(loose.get_height() == Catch::Approx(0.0F));

    // 只有描述没有标题 / 图标时仍然占位（Alert 的 is_empty 以三者全空为界）。
    alert->set_description("Saved just now");
    const auto description_only = alert->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(description_only.get_width() > 0.0F);
    REQUIRE(description_only.get_height() > 0.0F);
}

TEST_CASE("alert lays title and description out as a text column", "[alert][layout]") {
    auto alert = widget::Alert::create();
    alert->set_title("Saved");
    REQUIRE(alert->title() == "Saved");
    const auto title_only = alert->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(title_only.get_width() == Catch::Approx(320.0F));
    REQUIRE(title_only.get_height() > 0.0F);

    alert->set_description("Your changes are safe");
    REQUIRE(alert->description() == "Your changes are safe");
    const auto stacked = alert->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(stacked.get_height() > title_only.get_height());

    // 有界约束下铺满上限宽度，高度仍由内容决定。
    const auto bounded = alert->measure_layout(scene::LayoutConstraints {
        .max_width = 120.0F,
        .max_height = 200.0F,
    });
    REQUIRE(bounded.get_width() == Catch::Approx(120.0F));
}

TEST_CASE("alert includes icon and action slots in measurement and layout", "[alert][layout]") {
    auto alert = widget::Alert::create();
    alert->set_title("Saved");
    const float text_only = alert->measure_layout(scene::LayoutConstraints::loose()).get_width();

    auto icon = widget::Badge::create("i");
    alert->set_icon(icon);
    REQUIRE(alert->icon() == icon.get());

    auto action = widget::Button::create("Undo");
    alert->set_action(action);
    REQUIRE(alert->action() == action.get());

    scene::NanSceneTree tree;
    tree.set_root(alert);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 120.0F)) >= 1);
    REQUIRE(icon->width() > 0.0F);
    REQUIRE(action->width() > 0.0F);
    // 图标在文本左侧，操作在文本右侧，各列垂直居中。
    REQUIRE(icon->position().get_x() < action->position().get_x());
    REQUIRE(text_only == Catch::Approx(320.0F));
}

TEST_CASE("alert rejects null and already-mounted slot content", "[alert][boundary]") {
    auto alert = widget::Alert::create();
    REQUIRE_THROWS_AS(alert->set_icon(nullptr), std::invalid_argument);
    REQUIRE_THROWS_AS(alert->set_action(nullptr), std::invalid_argument);

    scene::NanSceneTree other;
    auto mounted = widget::Badge::create("mounted");
    other.set_root(mounted);
    REQUIRE_THROWS_AS(alert->set_icon(mounted), std::logic_error);
}

TEST_CASE("alert paints nothing while inactive and a container once titled", "[alert][paint]") {
    auto alert = widget::Alert::create();
    scene::NanSceneTree tree;
    tree.set_root(alert);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 120.0F)) >= 1);

    RecordingDevice inactive;
    tree.draw(inactive);
    REQUIRE(inactive.rounded_rects == 0);
    REQUIRE(inactive.text_draws == 0);

    alert->set_title("Saved");
    RecordingDevice titled;
    tree.draw(titled);
    REQUIRE(titled.rounded_rects >= 1);
    REQUIRE(titled.text_draws >= 1);
}

TEST_CASE("alert set_tone switches the resolved tone colours", "[alert][theme]") {
    auto alert = widget::Alert::create();
    alert->set_title("Careful");
    const auto info_fill = alert->resolved_style().container.fill;
    REQUIRE(alert->tone() == theme::AlertTone::info);

    alert->set_tone(theme::AlertTone::error);
    REQUIRE(alert->tone() == theme::AlertTone::error);
    const auto error_fill = alert->resolved_style().container.fill;
    REQUIRE(error_fill.oklch().light != Catch::Approx(info_fill.oklch().light));
    REQUIRE(
        error_fill.oklch().light
        == Catch::Approx(theme::default_design_system().light.error.oklch().light)
    );
}

TEST_CASE("alert override patches fields and survives a system apply", "[alert][override]") {
    theme::ThemeManager themes;
    auto alert = widget::Alert::create();
    alert->set_title("Saved");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(alert);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 160.0F)) >= 1);

    alert->set_override(theme::AlertRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::error),
        .icon_color = theme::ThemeColor::token(theme::ColorToken::primary),
        .metrics_padding_x = theme::ThemeScalar::literal(28.0F),
    });
    const auto overridden = alert->resolved_style();
    REQUIRE(
        overridden.container.fill.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );
    REQUIRE(
        overridden.icon.oklch().light
        == Catch::Approx(themes.design_system().light.primary.oklch().light)
    );
    REQUIRE(overridden.metrics.padding_x == Catch::Approx(28.0F));

    // 系统 apply 后 override 不冻结，仍跟随新快照重解析。
    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(alert->resolved_style().container.fill.oklch().light == Catch::Approx(0.60F));
}

TEST_CASE("alert re-resolves after an appearance switch", "[alert][theme]") {
    theme::ThemeManager themes;
    auto alert = widget::Alert::create();
    alert->set_title("Saved");
    alert->set_tone(theme::AlertTone::info);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(alert);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 160.0F)) >= 1);

    const float light = alert->resolved_style().container.border.oklch().light;
    themes.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(themes.appearance() == theme::ColorAppearance::dark);
    REQUIRE(alert->resolved_style().container.border.oklch().light != Catch::Approx(light));
    REQUIRE(
        alert->resolved_style().container.border.oklch().light
        == Catch::Approx(themes.design_system().dark.info.oklch().light)
    );
}

TEST_CASE("alert dismiss affordance is focusable only when dismissible", "[alert][dismiss]") {
    auto alert = widget::Alert::create();
    alert->set_title("Saved");
    scene::NanSceneTree tree;
    tree.set_root(alert);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 120.0F)) >= 1);

    // 不可关闭时 Alert 内没有任何可聚焦节点。
    tree.focus_next();
    REQUIRE(tree.focused_node() == nullptr);

    int dismissed = 0;
    alert->set_on_dismiss([&] { ++dismissed; });
    alert->set_dismissible(true);
    REQUIRE(alert->dismissible());
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 120.0F)) >= 1);

    tree.focus_next();
    auto* dismiss = tree.focused_node();
    REQUIRE(dismiss != nullptr);
    REQUIRE(dismiss != alert.get());
    REQUIRE(dismiss->is_focusable());

    // 键盘路径：Enter 激活同一条 on_dismiss 回调。
    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));
    REQUIRE(dismissed == 1);

    // 指针路径：移动 + 按下 + 抬起落在按钮世界包围盒内。
    const auto bounds = dismiss->global_bounds();
    const auto point = foundation::NanPoint(
        (bounds.get_left() + bounds.get_right()) * 0.5F,
        (bounds.get_top() + bounds.get_bottom()) * 0.5F
    );
    tree.dispatch_mouse_move(scene::MouseMoveEvent {point, foundation::NanPoint::zero()});
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        point
    });
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        point
    });
    REQUIRE(dismissed == 2);

    // 关掉 dismissible 后按钮从场景树移除，焦点回到空。
    alert->set_dismissible(false);
    REQUIRE_FALSE(alert->dismissible());
    tree.focus_next();
    REQUIRE(tree.focused_node() == nullptr);
}

TEST_CASE("alert dismiss affordance draws a cross and hides while empty", "[alert][dismiss][paint]") {
    auto alert = widget::Alert::create();
    alert->set_title("Saved");
    alert->set_dismissible(true);
    scene::NanSceneTree tree;
    tree.set_root(alert);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 120.0F)) >= 1);

    RecordingDevice device;
    tree.draw(device);
    REQUIRE(device.lines >= 2); // 关闭按钮的 × 是两条对角线

    // 标题清空后整体不绘制，关闭按钮也随之隐藏。
    alert->set_title("");
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 120.0F)) >= 1);
    RecordingDevice empty;
    tree.draw(empty);
    REQUIRE(empty.lines == 0);
    REQUIRE(empty.rounded_rects == 0);
}

TEST_CASE("alert exposes the title as a generic semantics label with a hint", "[alert][semantics]") {
    auto alert = widget::Alert::create();
    alert->set_title("Saved");
    alert->set_description("Your changes are safe");
    scene::NanSceneTree tree;
    tree.set_root(alert);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 160.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(alert->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::generic);
    REQUIRE(node->properties.label == "Saved");
    REQUIRE(node->properties.hint == "Your changes are safe");
}

TEST_CASE("alert without a title reports no semantics", "[alert][semantics]") {
    auto alert = widget::Alert::create();
    alert->set_description("Only a description");
    scene::NanSceneTree tree;
    tree.set_root(alert);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 160.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    // semantics_properties() 返回 {}（role none）时节点不会被暴露到语义树。
    const auto* node = tree.semantics_tree().find(alert->semantics_id());
    REQUIRE(node == nullptr);
}

TEST_CASE("alert is buildable through BuildContext", "[alert][authoring]") {
    // ComponentTraits 是模板，不实例化就不会被编译；这里真实走一遍 ui.make<>。
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    auto alert = ui.make<widget::Alert>(theme::AlertTone::warning, "磁盘空间不足", "请清理缓存")
                     .build();
    REQUIRE(alert != nullptr);
    REQUIRE(alert->tone() == theme::AlertTone::warning);
    REQUIRE(alert->title() == "磁盘空间不足");
    REQUIRE(alert->description() == "请清理缓存");

    // 无描述路径。
    auto bare = ui.make<widget::Alert>(theme::AlertTone::success, "已保存").build();
    REQUIRE(bare != nullptr);
    REQUIRE(bare->description().empty());
}
