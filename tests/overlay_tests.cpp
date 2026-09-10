#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/internal/overlay_host.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>

using namespace nandina;

namespace
{
    class HitControl final: public scene::NanControl {
    public:
        explicit HitControl(foundation::NanSize size): scene::NanControl(size) {}

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override {
            return constraints.constrain(size());
        }
    };
}

TEST_CASE("overlay host lays out content and portal surface to the viewport", "[overlay][layout]") {
    auto host = widget::internal::OverlayHost::create();
    auto content = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    host->set_content(content);
    scene::NanSceneTree tree;
    tree.set_root(host);

    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 180.0F)) == 1);
    REQUIRE(content->size() == foundation::NanSize(320.0F, 180.0F));
    REQUIRE(host->layer_count() == 2);
    REQUIRE(host->layer_at(1)->layout_root()->size() == foundation::NanSize(320.0F, 180.0F));
}

TEST_CASE("presented content is hit above clipped application content", "[overlay][portal][input]") {
    auto host = widget::internal::OverlayHost::create();
    auto content = std::make_shared<HitControl>(foundation::NanSize(100.0F, 100.0F));
    content->set_overflow(scene::ControlOverflow::clip);
    host->set_content(content);
    auto overlay = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    overlay->set_position(foundation::NanPoint(140.0F, 20.0F));
    auto handle = host->present(overlay);
    scene::NanSceneTree tree;
    tree.set_root(host);
    (void)tree.layout_root(foundation::NanSize(240.0F, 120.0F));

    REQUIRE(tree.hit_test(foundation::NanPoint(150.0F, 25.0F)) == overlay.get());
    REQUIRE(handle.mounted());
    REQUIRE(host->overlay_count() == 1);

    handle.close();
    REQUIRE_FALSE(handle.mounted());
    REQUIRE(host->overlay_count() == 0);
    REQUIRE_FALSE(overlay->is_inside_tree());
    REQUIRE(tree.hit_test(foundation::NanPoint(150.0F, 25.0F)) == content.get());
}

TEST_CASE("overlay handles own presentation lifetime and preserve order", "[overlay][lifetime]") {
    auto host = widget::internal::OverlayHost::create();
    host->set_content(std::make_shared<HitControl>(foundation::NanSize(200.0F, 100.0F)));
    auto lower = std::make_shared<HitControl>(foundation::NanSize(80.0F, 50.0F));
    auto upper = std::make_shared<HitControl>(foundation::NanSize(80.0F, 50.0F));
    auto lower_handle = host->present(lower, {.order = 1});
    auto upper_handle = host->present(upper, {.order = 2});
    scene::NanSceneTree tree;
    tree.set_root(host);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    REQUIRE(tree.hit_test(foundation::NanPoint(10.0F, 10.0F)) == upper.get());
    REQUIRE(host->overlay_count() == 2);

    auto moved = std::move(upper_handle);
    REQUIRE_FALSE(upper_handle.mounted());
    REQUIRE(moved.mounted());
    moved.close();
    REQUIRE(tree.hit_test(foundation::NanPoint(10.0F, 10.0F)) == lower.get());
}

TEST_CASE("overlay host rejects attached portal content", "[overlay][contract]") {
    auto host = widget::internal::OverlayHost::create();
    auto parent = std::make_shared<scene::NanControl>();
    auto attached = std::make_shared<HitControl>(foundation::NanSize(20.0F, 20.0F));
    parent->add_child(attached);

    REQUIRE_THROWS_AS(host->present(attached), std::logic_error);
}
