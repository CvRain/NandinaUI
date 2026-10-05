//
// HoverCard tests: hover-triggered floating card over arbitrary content.
//

#include <nandina/foundation/contrast.hpp>
#include <nandina/reactive/graph.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/hover_card.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string>

using namespace nandina;

namespace
{
    inline constexpr float kOpenDelay = 0.3F;
    inline constexpr float kCloseDelay = 0.2F;

    struct CardHarness {
        reactive::Graph graph;
        theme::ThemeManager themes;
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<scene::NanControl> body =
            std::make_shared<scene::NanControl>(foundation::NanSize(480.0F, 320.0F));
        std::shared_ptr<scene::NanControl> trigger =
            std::make_shared<scene::NanControl>(foundation::NanSize(120.0F, 32.0F));
        std::shared_ptr<scene::NanControl> content =
            std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 50.0F));
        std::shared_ptr<widget::HoverCard> card;
        scene::NanSceneTree tree;
        foundation::NanSize viewport {480.0F, 320.0F};

        explicit CardHarness(bool with_content = true):
            card(widget::HoverCard::create(trigger, with_content ? content : nullptr)) {
            tree.set_theme_manager(themes);
            card->set_open_delay(kOpenDelay);
            card->set_close_delay(kCloseDelay);
            // 不注入服务：host 作为树根，resolve_overlay_host() 沿祖先链就能找到它。
            body->add_child(card);
            body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
            host->set_content(body);
            tree.set_root(host);
        }

        void layout() {
            (void)tree.layout_root(viewport);
        }

        /// 浮层里被托管的卡片表面（HoverCard 直接 present 表面，没有 DismissLayer 层）。
        [[nodiscard]] auto surface() const -> scene::NanControl* {
            auto* layer = host->layer_at(1);
            auto* root = layer != nullptr ? layer->layout_root() : nullptr;
            if (root == nullptr || root->child_count() == 0) {
                return nullptr;
            }
            return root->get_child(0)->as_control();
        }

        /// 指针进入 / 离开触发器（事件从触发控件冒泡到 HoverCard）。
        void enter_trigger() {
            scene::MouseEnterEvent event(foundation::NanPoint {});
            card->on_input(event);
        }
        void leave_trigger() {
            scene::MouseLeaveEvent event(foundation::NanPoint {});
            card->on_input(event);
        }
        /// 指针进入 / 离开卡片（表面自己观察，再回调进 HoverCard）。
        void enter_content() {
            auto* target = surface();
            REQUIRE(target != nullptr);
            scene::MouseEnterEvent event(foundation::NanPoint {});
            target->on_input(event);
        }
        void leave_content() {
            auto* target = surface();
            REQUIRE(target != nullptr);
            scene::MouseLeaveEvent event(foundation::NanPoint {});
            target->on_input(event);
        }

        /// 悬停推进若干帧。
        void advance(float seconds, float step = 0.05F) {
            for (float elapsed = 0.0F; elapsed < seconds; elapsed += step) {
                card->on_process(step);
            }
        }
    };
} // namespace

TEST_CASE("hover card opens after the open delay and closes after the close delay", "[hover-card][interaction]") {
    CardHarness harness;
    harness.layout();

    harness.enter_trigger();
    harness.advance(kOpenDelay * 0.5F);
    REQUIRE_FALSE(harness.card->is_open());

    harness.advance(kOpenDelay);
    REQUIRE(harness.card->is_open());

    // 离开触发器后不是立刻关：close_delay 就是"移进卡片"的窗口。
    harness.leave_trigger();
    harness.card->on_process(kCloseDelay * 0.5F);
    REQUIRE(harness.card->is_open());

    harness.advance(kCloseDelay);
    REQUIRE_FALSE(harness.card->is_open());
}

TEST_CASE("hover card keeps open when the pointer moves into the content", "[hover-card][interaction]") {
    CardHarness harness;
    harness.layout();
    harness.enter_trigger();
    harness.advance(kOpenDelay);
    REQUIRE(harness.card->is_open());

    // 指针离开触发器进入卡片：leave 先到，enter 随后到（同一帧内）。
    harness.leave_trigger();
    harness.enter_content();
    harness.advance(kOpenDelay + kCloseDelay);
    REQUIRE(harness.card->is_open());

    // 指针离开卡片后才开始倒计时。
    harness.leave_content();
    harness.advance(kCloseDelay);
    REQUIRE_FALSE(harness.card->is_open());
}

TEST_CASE("hover card without hoverable content behaves like a tooltip", "[hover-card][interaction]") {
    CardHarness harness;
    harness.card->set_hoverable_content(false);
    harness.layout();
    harness.enter_trigger();
    harness.advance(kOpenDelay);
    REQUIRE(harness.card->is_open());

    // 明确关掉"可移入"后，指针留在卡片里不再算作停留依据。
    harness.leave_trigger();
    harness.enter_content();
    harness.advance(kCloseDelay);
    REQUIRE_FALSE(harness.card->is_open());
}

TEST_CASE("hover card does not open without hovering", "[hover-card][interaction]") {
    // 边界：没有任何悬停时，时间再长也不该自己弹出来。
    CardHarness harness;
    harness.layout();
    harness.advance(2.0F);
    REQUIRE_FALSE(harness.card->is_open());

    // 零延迟：进入即展开。
    harness.card->set_open_delay(0.0F);
    harness.enter_trigger();
    harness.card->on_process(0.0F);
    REQUIRE(harness.card->is_open());

    // 负延迟与非法值被拒绝。
    REQUIRE_THROWS_AS(harness.card->set_open_delay(-1.0F), std::invalid_argument);
    REQUIRE_THROWS_AS(harness.card->set_close_delay(-1.0F), std::invalid_argument);
}

TEST_CASE("hover card tolerates the gap between trigger and card", "[hover-card][interaction]") {
    // 触发器与卡片之间有 gap：指针穿过去时鼠标既不在触发器上也不在卡片上。
    // close_delay 必须足够兜住这一段，否则卡片会在移入途中闪一下。
    CardHarness harness;
    harness.card->set_open_delay(0.0F);
    harness.card->set_close_delay(0.25F);
    harness.layout();
    harness.enter_trigger();
    harness.card->on_process(0.0F);
    REQUIRE(harness.card->is_open());

    harness.leave_trigger();
    // 跨越空隙（这一小段时间里两个区域都没命中）。
    harness.advance(0.1F);
    REQUIRE(harness.card->is_open());
    harness.enter_content();
    harness.advance(1.0F);
    REQUIRE(harness.card->is_open());
}

TEST_CASE("hover card mounts a portal while open and releases it on exit", "[hover-card][overlay]") {
    CardHarness harness;
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);

    harness.enter_trigger();
    harness.advance(kOpenDelay);
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 1);
    REQUIRE(harness.surface() != nullptr);

    harness.card->close();
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);

    // 重新打开复用同一个表面（内容节点不被销毁重建）。
    harness.enter_trigger();
    harness.advance(kOpenDelay);
    harness.layout();
    auto* first = harness.surface();
    REQUIRE(first != nullptr);
    harness.card->close();
    harness.enter_trigger();
    harness.advance(kOpenDelay);
    harness.layout();
    REQUIRE(harness.surface() == first);

    // 离开场景树必须收掉浮层。
    harness.body->remove_and_delete(*harness.card);
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);
    REQUIRE_FALSE(harness.card->is_open());
}

TEST_CASE("hover card sizes itself from the content plus padding", "[hover-card][layout]") {
    CardHarness harness;
    harness.layout();
    harness.card->open();
    harness.layout();

    const auto style = harness.card->resolved_style();
    auto* surface = harness.surface();
    REQUIRE(surface != nullptr);
    const auto size = surface->size();

    REQUIRE(
        size.get_height()
        == Catch::Approx(50.0F + style.metrics.padding_y * 2.0F)
    );
    // 内容比 min_width 窄，所以宽度走 min_width 下限。
    REQUIRE(
        size.get_width()
        == Catch::Approx(
            std::max(100.0F + style.metrics.padding_x * 2.0F, style.metrics.min_width)
        )
    );

    // 触发控件铺满 HoverCard 的占位矩形。
    const auto trigger_size = harness.trigger->size();
    REQUIRE(trigger_size.get_height() == Catch::Approx(harness.card->size().get_height()));
}

TEST_CASE("hover card shows nothing without an overlay host", "[hover-card][overlay]") {
    // detached 上下文：不显示卡片（与 Tooltip 同一取舍），但状态机本身仍然一致。
    auto card = widget::HoverCard::create(
        std::make_shared<scene::NanControl>(foundation::NanSize(80.0F, 24.0F)),
        std::make_shared<scene::NanControl>(foundation::NanSize(60.0F, 40.0F))
    );
    card->set_open_delay(0.0F);
    scene::MouseEnterEvent enter(foundation::NanPoint {});
    card->on_input(enter);
    card->on_process(0.0F);
    REQUIRE(card->is_open());
}

TEST_CASE("hover card validates its slots", "[hover-card][boundary]") {
    auto card = widget::HoverCard::create();
    REQUIRE_THROWS_AS(card->set_trigger(nullptr), std::invalid_argument);
    REQUIRE_THROWS_AS(card->set_content(nullptr), std::invalid_argument);
    REQUIRE(card->trigger() == nullptr);
    REQUIRE(card->content() == nullptr);
    // 没有触发器时测量为 0，而不是崩溃。
    REQUIRE(card->measure_layout(foundation::NanLayoutConstraints::loose()).get_width() == Catch::Approx(0.0F));
}

TEST_CASE("hover card recipe resolves semantic roles and honors rules", "[hover-card][theme]") {
    const auto design = theme::default_design_system();
    const auto light = theme::resolve_hover_card(design, theme::ColorAppearance::light);
    const auto dark = theme::resolve_hover_card(design, theme::ColorAppearance::dark);

    REQUIRE(light.panel.fill.alpha() > 0.0F);
    REQUIRE(light.panel.border_width == Catch::Approx(design.tokens.border.thin));
    REQUIRE(light.metrics.padding_x == Catch::Approx(design.tokens.spacing.md));
    REQUIRE(light.metrics.gap == Catch::Approx(design.tokens.spacing.sm));
    // 亮暗不同：默认值只用语义角色。
    REQUIRE_FALSE(light.panel.fill == dark.panel.fill);

    // 规则是增量覆盖，未指定的字段保持原值；这一条同时守住 apply_rule 重载的存在。
    auto themed = design;
    themed.components.hover_card.rules.push_back(
        theme::HoverCardRecipeRule {
            .metrics_max_width = theme::ThemeScalar::literal(420.0F),
        }
    );
    const auto overridden =
        theme::resolve_hover_card(themed, theme::ColorAppearance::light);
    REQUIRE(overridden.metrics.max_width == Catch::Approx(420.0F));
    REQUIRE(overridden.metrics.min_width == Catch::Approx(light.metrics.min_width));
    REQUIRE(overridden.panel.radius == Catch::Approx(light.panel.radius));
}

TEST_CASE("hover card instance override and theme switch change the resolved style", "[hover-card][theme]") {
    CardHarness harness;
    harness.layout();
    const auto initial = harness.card->resolved_style();

    harness.card->set_override(
        theme::HoverCardRecipeRule {
            .panel_fill = theme::ThemeColor::token(theme::ColorToken::card),
            .metrics_padding_y = theme::ThemeScalar::literal(20.0F),
        }
    );
    const auto overridden = harness.card->resolved_style();
    REQUIRE(overridden.metrics.padding_y == Catch::Approx(20.0F));
    REQUIRE_FALSE(overridden.panel.fill == initial.panel.fill);

    harness.card->on_theme_changed(harness.themes);
    REQUIRE(harness.card->resolved_style().metrics.padding_y == Catch::Approx(20.0F));
}

TEST_CASE("hover card card surface is exposed to semantics but stays out of focus", "[hover-card][semantics]") {
    CardHarness harness;
    harness.layout();
    harness.card->open();
    harness.layout();

    auto* surface = harness.surface();
    REQUIRE(surface != nullptr);
    const auto props = surface->semantics_properties();
    REQUIRE(props.role == semantics::Role::generic);
    REQUIRE_FALSE(props.label.empty());
    // 卡片不抢焦点：展开前后焦点状态不变（内容里的控件仍然自己可聚焦）。
    REQUIRE_FALSE(surface->is_focusable());
    REQUIRE(harness.tree.focused_node() == nullptr);
    harness.card->close();
    harness.layout();
    REQUIRE(harness.tree.focused_node() == nullptr);
}

TEST_CASE("hover card is reachable through BuildContext", "[hover-card][traits]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};

    auto card = ui.make<widget::HoverCard>(
                    std::make_shared<scene::NanControl>(foundation::NanSize(64.0F, 24.0F)),
                    std::make_shared<scene::NanControl>(foundation::NanSize(90.0F, 30.0F))
    )
                    .build();
    REQUIRE(card != nullptr);
    REQUIRE(card->trigger() != nullptr);
    REQUIRE(card->content() != nullptr);
}
