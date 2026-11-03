// Surface/box authoring and Card content geometry are tested without a window.
#include <nandina/reactive/scope.hpp>
#include <nandina/render/draw_context.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/surface.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    class Device final: public render::IRenderDevice {
    public:
        struct Draw {
            foundation::NanRect rect;
            float radius;
            float alpha;
        };
        bool rounded = true;
        std::vector<Draw> fills;
        std::vector<Draw> outlines;
        std::vector<foundation::NanRect> clips;
        std::vector<std::string> order;
        int clip_clears = 0;
        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect& rect) override {
            clips.push_back(rect);
        }
        void clear_clip() override {
            ++clip_clears;
        }
        auto supports_rounded_rect() const -> bool override {
            return rounded;
        }
        void
        draw_rect(const foundation::NanRect& rect, const foundation::NanColor& color) override {
            fills.push_back({rect, 0, color.alpha()});
            order.push_back("fill");
        }
        void draw_rounded_rect(
            const foundation::NanRect& rect,
            float radius,
            const foundation::NanColor& color
        ) override {
            fills.push_back({rect, radius, color.alpha()});
            order.push_back("round");
        }
        void draw_rect_outline(
            const foundation::NanRect& rect,
            float,
            const foundation::NanColor& color
        ) override {
            outlines.push_back({rect, 0, color.alpha()});
            order.push_back("outline");
        }
        void draw_rounded_rect_outline(
            const foundation::NanRect& rect,
            float radius,
            float,
            const foundation::NanColor& color
        ) override {
            outlines.push_back({rect, radius, color.alpha()});
            order.push_back("outline");
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
        ) override {
            order.push_back("text");
        }
    };

    auto box(float radius = 6.0F) -> theme::BoxStyle {
        return {
            .fill = theme::ThemeColor::token(theme::ColorToken::surface),
            .border = theme::ThemeColor::token(theme::ColorToken::border),
            .border_width = theme::ThemeScalar::literal(2),
            .radius = theme::ThemeScalar::literal(radius),
        };
    }
} // namespace

TEST_CASE("Surface shares single-child inset measurement and replacement", "[surface][layout]") {
    auto surface = widget::Surface::create();
    CHECK(surface->padding().is_zero());
    auto first = std::make_shared<scene::NanControl>(foundation::NanSize(30, 20));
    surface->set_padding(foundation::NanInsets(3, 5, 7, 11));
    surface->set_child(first);
    auto measured = surface->measure_layout(foundation::NanLayoutConstraints::loose());
    CHECK(measured == foundation::NanSize(38, 38));
    surface->layout_to(foundation::NanRect::from_xywh(0, 0, 38, 38));
    CHECK(first->position() == foundation::NanPoint(3, 7));
    CHECK(first->size() == foundation::NanSize(30, 20));
    auto next = std::make_shared<scene::NanControl>(foundation::NanSize(10, 5));
    surface->set_child(next);
    CHECK(surface->child_count() == 1);
    CHECK(first->parent() == nullptr);
    CHECK(next->parent() == surface.get());
    CHECK(
        surface->measure_layout(foundation::NanLayoutConstraints::loose())
        == foundation::NanSize(18, 23)
    );
    REQUIRE_THROWS_AS(surface->set_child(nullptr), std::runtime_error);
}

TEST_CASE(
    "Surface paints its shared rounded box before content with one viewport scale",
    "[surface][paint]"
) {
    Device device;
    reactive::Graph graph;
    auto surface = widget::Surface::create();
    surface->set_box_style(box());
    surface->set_local_opacity(0.5F);
    surface->set_child(widget::Label::create(graph, "content"));
    scene::NanSceneTree tree;
    tree.set_root(surface);
    (void)tree.layout_root(foundation::NanSize(100, 60));
    render::DrawContext context(
        device,
        foundation::NanTransform2D::from_scale(2),
        {.logical_to_screen = 2, .screen_to_physical = 3}
    );
    tree.render(context);
    REQUIRE(device.fills.size() == 1);
    CHECK(device.fills.front().rect == foundation::NanRect::from_xywh(0, 0, 200, 120));
    CHECK(device.fills.front().radius == Catch::Approx(12));
    CHECK(device.fills.front().alpha == Catch::Approx(0.5));
    REQUIRE(device.outlines.size() == 1);
    CHECK(device.outlines.front().radius == Catch::Approx(12));
    REQUIRE(device.order.size() == 3);
    CHECK(device.order == std::vector<std::string> {"round", "outline", "text"});
    CHECK(device.clips.empty());
    device.rounded = false;
    tree.render(context);
    CHECK(device.fills.back().radius == 0);
}

TEST_CASE(
    "Surface box/presentation updates are paint-only and theme tokens stay live",
    "[surface][theme]"
) {
    theme::ThemeManager themes;
    Device device;
    auto surface = widget::Surface::create();
    surface->set_box_style(box());
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(surface);
    (void)tree.layout_root(foundation::NanSize(100, 60));
    surface->clear_dirty(scene::DirtyFlags::paint);
    surface->set_box_style(box(10));
    CHECK(surface->is_dirty(scene::DirtyFlags::paint));
    CHECK_FALSE(surface->layout_dirty());
    CHECK(tree.layout_root(foundation::NanSize(100, 60)) == 0);
    CHECK(surface->resolved_style().radius == 10);
    themes.set_preference(theme::ThemePreference::dark);
    CHECK(surface->resolved_style().fill.approx_equals(
        theme::resolve_theme_color(
            themes.theme(),
            theme::ThemeColor::token(theme::ColorToken::surface)
        )
    ));
    widget::property::write(*surface, widget::visual::container.radius, 14.0F);
    themes.set_preference(theme::ThemePreference::light);
    CHECK(surface->resolved_style().radius == 14);
    CHECK_FALSE(surface->layout_dirty());
    CHECK(surface->padding().is_zero());
}

TEST_CASE(
    "Surface clipping and hit geometry retain the rectangular scene contract",
    "[surface][clip]"
) {
    Device device;
    auto surface = widget::Surface::create();
    surface->set_box_style(box(20));
    auto child = std::make_shared<scene::NanControl>(foundation::NanSize(30, 30));
    child->set_background(theme::default_theme().palette.primary);
    surface->set_child(child);
    surface->set_overflow(scene::ControlOverflow::clip);
    scene::NanSceneTree tree;
    tree.set_root(surface);
    (void)tree.layout_root(foundation::NanSize(100, 60));
    child->set_position(foundation::NanPoint(90, 0));
    tree.draw(device);
    REQUIRE(device.clips.size() == 1);
    CHECK(device.clips.front() == foundation::NanRect::from_xywh(0, 0, 100, 60));
    CHECK(device.clip_clears == 1);
    CHECK(tree.hit_test(foundation::NanPoint(110, 10)) == nullptr);
    CHECK(surface->contains_point(foundation::NanPoint(0, 0))); // radius does not change hit shape
}

TEST_CASE("Card uses four-sided content insets for measure arrange and reset", "[surface][card]") {
    auto card = widget::Card::create();
    auto child = std::make_shared<scene::NanControl>(foundation::NanSize(20, 10));
    card->set_child(child);
    const auto defaults = card->resolved_style().metrics;
    CHECK(
        card->content_insets()
        == foundation::NanInsets::symmetric(defaults.padding_x, defaults.padding_y)
    );
    card->set_content_insets(foundation::NanInsets(3, 5, 7, 11));
    const auto measured = card->measure_layout(foundation::NanLayoutConstraints::loose());
    CHECK(measured.get_width() == Catch::Approx(28));
    CHECK(measured.get_height() == Catch::Approx(std::max(28.0F, defaults.min_height)));
    card->layout_to(foundation::NanRect::from_xywh(0, 0, 100, 80));
    CHECK(child->position() == foundation::NanPoint(3, 7));
    CHECK(child->size() == foundation::NanSize(20, 10));
    card->set_content_insets(foundation::NanInsets::all(0));
    (void)card->measure_layout(
        foundation::NanLayoutConstraints::tight(foundation::NanSize(100, 80))
    );
    card->layout_to(foundation::NanRect::from_xywh(0, 0, 100, 80));
    CHECK(child->position() == foundation::NanPoint::zero());
    CHECK(child->size() == foundation::NanSize(100, 80));
    card->set_override(
        {.metrics_padding_x = theme::ThemeScalar::literal(13),
         .metrics_padding_y = theme::ThemeScalar::literal(17)}
    );
    CHECK(card->content_insets().is_zero());
    card->clear_content_insets();
    CHECK(card->content_insets() == foundation::NanInsets::symmetric(13, 17));
    card->set_content_insets(foundation::NanInsets::all(200));
    (void)card->measure_layout(
        foundation::NanLayoutConstraints::tight(foundation::NanSize(100, 80))
    );
    card->layout_to(foundation::NanRect::from_xywh(0, 0, 100, 80));
    CHECK(child->size() == foundation::NanSize(0, 0));
}

TEST_CASE(
    "author content insets and Surface box metrics reject invalid input",
    "[surface][validation]"
) {
    auto card = widget::Card::create();
    auto surface = widget::Surface::create();
    const auto original = card->content_insets();
    for (const auto value:
         {-1.0F,
          std::numeric_limits<float>::infinity(),
          std::numeric_limits<float>::quiet_NaN(),
          std::numeric_limits<float>::max()})
    {
        REQUIRE_THROWS_AS(
            card->set_content_insets(foundation::NanInsets::all(value)),
            std::invalid_argument
        );
        CHECK(card->content_insets() == original);
        REQUIRE_THROWS_AS(
            surface->set_padding(foundation::NanInsets::all(value)),
            std::invalid_argument
        );
        CHECK(surface->padding().is_zero());
    }
    REQUIRE_THROWS_AS(surface->set_box_style(box(-1)), std::invalid_argument);
    CHECK(surface->resolved_style().radius == 0);
}

TEST_CASE(
    "Surface and content geometry are reachable through constrained authoring",
    "[surface][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager themes;
    widget::BuildContext ui(graph, scope, themes);
    auto surface = ui.surface()
                       .box_style(box())
                       .padding(foundation::NanInsets::all(5))
                       .set(widget::visual::container.radius, 12.0F)
                       .child(ui.make<widget::Label>("surface"))
                       .build();
    CHECK(surface->resolved_style().radius == 12);
    CHECK(surface->padding() == foundation::NanInsets::all(5));
    CHECK(surface->child_count() == 1);
    static_assert(
        widget::property::Writable<widget::Surface, decltype(widget::visual::container.radius)>
    );
    const auto bare = widget::authoring::surface().build();
    CHECK(bare->padding().is_zero());
}

TEST_CASE(
    "Surface scalar tokens re-resolve while presentation overrides survive",
    "[surface][theme]"
) {
    theme::ThemeManager themes;
    auto surface = widget::Surface::create();
    auto style = box();
    style.radius = theme::ThemeScalar::token(theme::ScalarToken::radius_md);
    surface->set_box_style(style);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(surface);
    (void)tree.layout_root(foundation::NanSize(100, 60));
    auto system = themes.design_system();
    system.tokens.radius.md = 19;
    themes.apply(std::make_shared<const theme::DesignSystem>(system));
    CHECK(surface->resolved_style().radius == 19);
    CHECK_FALSE(surface->is_focusable());
    CHECK(surface->resolved_semantics_properties().role == semantics::Role::none);
    widget::property::write(*surface, widget::visual::container.radius, 7.0F);
    system.tokens.radius.md = 25;
    themes.apply(std::make_shared<const theme::DesignSystem>(system));
    CHECK(surface->resolved_style().radius == 7);
    CHECK_FALSE(surface->layout_dirty());
}

TEST_CASE(
    "Card inset reset follows the latest theme and wrapping stays converged",
    "[surface][card][resize]"
) {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto label = widget::Label::create(
        graph,
        "Content follows a finite width and grows naturally when a card becomes narrow. 中文 日本語."
    );
    label->set_overflow(text::TextOverflow::wrap);
    label->set_max_lines(30);
    auto card = widget::Card::create();
    card->set_child(label);
    card->set_content_insets(foundation::NanInsets(2, 4, 6, 8));
    auto scroll = widget::ScrollView::create();
    scroll->set_child(card);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(scroll);
    float wide = 0;
    for (const float width: {400.0F, 160.0F, 400.0F}) {
        (void)tree.layout_root(foundation::NanSize(width, 80));
        REQUIRE(tree.layout_root(foundation::NanSize(width, 80)) == 0);
        CHECK(label->position() == foundation::NanPoint(2, 6));
        CHECK(label->height() == Catch::Approx(label->measured_text_height()));
        CHECK(scroll->content_size().get_height() == Catch::Approx(card->height()));
        if (wide == 0)
            wide = label->height();
        else if (width == 160)
            CHECK(label->height() > wide);
        else
            CHECK(label->height() == Catch::Approx(wide));
    }
    auto system = themes.design_system();
    system.components.card.base.metrics.padding_x = theme::ThemeScalar::literal(23);
    system.components.card.base.metrics.padding_y = theme::ThemeScalar::literal(29);
    themes.apply(std::make_shared<const theme::DesignSystem>(system));
    CHECK(card->content_insets() == foundation::NanInsets(2, 4, 6, 8));
    card->clear_content_insets();
    CHECK(card->content_insets() == foundation::NanInsets::symmetric(23, 29));
    (void)tree.layout_root(foundation::NanSize(400, 80));
    CHECK(label->position() == foundation::NanPoint(23, 29));
    CHECK(tree.layout_root(foundation::NanSize(400, 80)) == 0);
}

namespace
{
    template<typename T>
    concept SurfaceBuilder = requires(T& builder, theme::BoxStyle style) {
        builder.box_style(style);
        builder.padding(foundation::NanInsets::all(0));
    };
    template<typename T>
    concept CoverBuilder =
        requires(T& builder) { builder.cover_anchor(scene::percent(10), scene::percent(90)); };
    static_assert(SurfaceBuilder<widget::authoring::NodeBuilder<widget::Surface>>);
    static_assert(!SurfaceBuilder<widget::authoring::NodeBuilder<widget::Label>>);
    static_assert(!SurfaceBuilder<widget::authoring::NodeBuilder<widget::Column>>);
    static_assert(CoverBuilder<widget::authoring::NodeBuilder<widget::Image>>);
    static_assert(!CoverBuilder<widget::authoring::NodeBuilder<widget::Card>>);
} // namespace
