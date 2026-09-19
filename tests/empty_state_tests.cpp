//
// Theme / EmptyState tests.
//

#include <nandina/render/render_device.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/badge.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/empty_state.hpp>

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
        int text_draws = 0;

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
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
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
} // namespace

TEST_CASE("empty state resolves transparent container and typography tokens", "[empty-state][theme]") {
    const auto design = theme::default_design_system();
    const auto style = theme::resolve_empty_state(
        design,
        theme::ColorAppearance::light,
        theme::EmptyStateVisualState::normal
    );

    REQUIRE(style.container.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(style.container.border.alpha() == Catch::Approx(0.0F));
    REQUIRE(style.container.radius == Catch::Approx(design.tokens.radius.md));
    REQUIRE(style.title.color.oklch().light == Catch::Approx(design.light.foreground.oklch().light));
    REQUIRE(style.title.font_size == Catch::Approx(design.tokens.typography.label_lg));
    REQUIRE(
        style.description.color.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );
    REQUIRE(style.description.font_size == Catch::Approx(design.tokens.typography.label_sm));
    REQUIRE(style.metrics.gap == Catch::Approx(design.tokens.spacing.md));
    REQUIRE(style.metrics.padding_x == Catch::Approx(design.tokens.spacing.lg));
    REQUIRE(style.metrics.padding_y == Catch::Approx(design.tokens.spacing.lg));
    REQUIRE(style.metrics.preferred_width == Catch::Approx(240.0F));
}

TEST_CASE("empty state with no title and no icon measures to zero", "[empty-state][layout]") {
    auto empty = widget::EmptyState::create();
    REQUIRE(empty->title().empty());
    REQUIRE(empty->description().empty());
    REQUIRE(empty->icon() == nullptr);
    REQUIRE(empty->action() == nullptr);

    const auto loose = empty->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(loose.get_width() == Catch::Approx(0.0F));
    REQUIRE(loose.get_height() == Catch::Approx(0.0F));

    // 描述单独存在但标题为空、无图标时仍不占位（按契约以标题/图标判定激活）。
    empty->set_description("Nothing matched the filter");
    const auto description_only = empty->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(description_only.get_width() == Catch::Approx(0.0F));
    REQUIRE(description_only.get_height() == Catch::Approx(0.0F));
}

TEST_CASE("empty state measures title and description in a vertical stack", "[empty-state][layout]") {
    auto empty = widget::EmptyState::create();
    empty->set_title("No results");
    REQUIRE(empty->title() == "No results");

    const auto title_only = empty->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(title_only.get_width() == Catch::Approx(240.0F));
    REQUIRE(title_only.get_height() > 32.0F);

    empty->set_description("Try a different search term");
    REQUIRE(empty->description() == "Try a different search term");
    const auto with_description = empty->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(with_description.get_height() > title_only.get_height());

    // 有界约束下铺满上限宽度，高度仍由内容决定。
    const auto bounded = empty->measure_layout(scene::LayoutConstraints {
        .max_width = 120.0F,
        .max_height = 200.0F,
    });
    REQUIRE(bounded.get_width() == Catch::Approx(120.0F));
}

TEST_CASE("empty state includes icon and action slots in measurement", "[empty-state][layout]") {
    auto empty = widget::EmptyState::create();
    empty->set_title("No results");
    const float title_only = empty->measure_layout(scene::LayoutConstraints::loose()).get_height();

    auto icon = widget::Badge::create("icon");
    empty->set_icon(icon);
    REQUIRE(empty->icon() == icon.get());
    const float with_icon = empty->measure_layout(scene::LayoutConstraints::loose()).get_height();
    REQUIRE(with_icon > title_only);

    auto action = widget::Button::create("Retry");
    empty->set_action(action);
    REQUIRE(empty->action() == action.get());
    const float with_action = empty->measure_layout(scene::LayoutConstraints::loose()).get_height();
    REQUIRE(with_action > with_icon);

    scene::NanSceneTree tree;
    tree.set_root(empty);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 160.0F)) >= 1);
    REQUIRE(icon->width() > 0.0F);
    REQUIRE(action->width() > 0.0F);
    // 图标在标题上方，操作在标题下方。
    REQUIRE(icon->position().get_y() < action->position().get_y());
}

TEST_CASE("empty state rejects null and already-mounted slot content", "[empty-state][boundary]") {
    auto empty = widget::EmptyState::create();
    REQUIRE_THROWS_AS(empty->set_icon(nullptr), std::invalid_argument);
    REQUIRE_THROWS_AS(empty->set_action(nullptr), std::invalid_argument);

    scene::NanSceneTree other;
    auto mounted = widget::Badge::create("mounted");
    other.set_root(mounted);
    REQUIRE_THROWS_AS(empty->set_icon(mounted), std::logic_error);
}

TEST_CASE("empty state paints nothing while inactive and text once titled", "[empty-state][paint]") {
    auto empty = widget::EmptyState::create();
    scene::NanSceneTree tree;
    tree.set_root(empty);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    RecordingDevice inactive;
    tree.draw(inactive);
    REQUIRE(inactive.text_draws == 0);
    REQUIRE(inactive.rounded_rects == 0);

    empty->set_title("No results");
    RecordingDevice titled;
    tree.draw(titled);
    REQUIRE(titled.text_draws >= 1);
}

TEST_CASE("empty state override patches fields and survives a system apply", "[empty-state][override]") {
    theme::ThemeManager themes;
    auto empty = widget::EmptyState::create();
    empty->set_title("No results");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(empty);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);

    empty->set_override(theme::EmptyStateRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::error),
        .description_color = theme::ThemeColor::token(theme::ColorToken::primary),
        .metrics_gap = theme::ThemeScalar::literal(20.0F),
    });
    const auto overridden = empty->resolved_style();
    REQUIRE(
        overridden.container.fill.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );
    REQUIRE(
        overridden.description.color.oklch().light
        == Catch::Approx(themes.design_system().light.primary.oklch().light)
    );
    REQUIRE(overridden.metrics.gap == Catch::Approx(20.0F));

    // 系统 apply 后 override 不冻结，仍跟随新快照重解析。
    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(empty->resolved_style().container.fill.oklch().light == Catch::Approx(0.60F));
}

TEST_CASE("empty state re-resolves after an appearance switch", "[empty-state][theme]") {
    theme::ThemeManager themes;
    auto empty = widget::EmptyState::create();
    empty->set_title("No results");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(empty);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);

    const float light = empty->resolved_style().title.color.oklch().light;
    themes.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(themes.appearance() == theme::ColorAppearance::dark);
    REQUIRE(empty->resolved_style().title.color.oklch().light != Catch::Approx(light));
    REQUIRE(
        empty->resolved_style().title.color.oklch().light
        == Catch::Approx(themes.design_system().dark.foreground.oklch().light)
    );
}

TEST_CASE("empty state exposes the title as a generic semantics label", "[empty-state][semantics]") {
    auto empty = widget::EmptyState::create();
    empty->set_title("No results");
    empty->set_description("Try a different filter");
    scene::NanSceneTree tree;
    tree.set_root(empty);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(empty->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::generic);
    REQUIRE(node->properties.label == "No results");
    REQUIRE(node->properties.hint == "Try a different filter");
}
