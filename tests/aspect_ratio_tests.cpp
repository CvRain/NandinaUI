#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/aspect_ratio.hpp>
#include <nandina/widget/authoring.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/builtin_component_traits.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/label.hpp>

#include <limits>
#include <memory>
#include <stdexcept>

namespace
{
    using namespace nandina;

    class FixedBox final: public scene::NanControl {
    public:
        explicit FixedBox(const foundation::NanSize natural): natural_(natural) {}

    protected:
        [[nodiscard]] auto on_measure(const foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override {
            return constraints.constrain(natural_);
        }

    private:
        foundation::NanSize natural_;
    };
} // namespace

TEST_CASE(
    "AspectRatio resolves measure across bounded and unbounded constraints",
    "[widget][aspect_ratio]"
) {
    auto child = std::make_shared<FixedBox>(foundation::NanSize(100.0F, 80.0F));
    auto box = widget::AspectRatio::create(16.0F / 9.0F);
    box->set_child(child);

    SECTION("width-bounded and height-unbounded derives height from width") {
        const foundation::NanLayoutConstraints width_only {
            .min_width = 0.0F,
            .max_width = 320.0F,
            .min_height = 0.0F,
            .max_height = std::numeric_limits<float>::infinity(),
        };
        const auto measured = box->measure_layout(width_only);
        CHECK(measured.get_width() == Catch::Approx(320.0F));
        CHECK(measured.get_height() == Catch::Approx(180.0F));
        CHECK(child->measured_size().get_width() == Catch::Approx(320.0F));
        CHECK(child->measured_size().get_height() == Catch::Approx(180.0F));
    }

    SECTION("height-bounded and width-unbounded derives width from height") {
        const foundation::NanLayoutConstraints height_only {
            .min_width = 0.0F,
            .max_width = std::numeric_limits<float>::infinity(),
            .min_height = 0.0F,
            .max_height = 180.0F,
        };
        const auto measured = box->measure_layout(height_only);
        CHECK(measured.get_width() == Catch::Approx(320.0F));
        CHECK(measured.get_height() == Catch::Approx(180.0F));
    }

    SECTION("both axes bounded fits largest inscribed box matching ratio") {
        const foundation::NanLayoutConstraints square {
            .min_width = 0.0F,
            .max_width = 320.0F,
            .min_height = 0.0F,
            .max_height = 320.0F,
        };
        const auto wide = box->measure_layout(square);
        CHECK(wide.get_width() == Catch::Approx(320.0F));
        CHECK(wide.get_height() == Catch::Approx(180.0F));

        box->set_ratio(0.5F);
        const auto tall = box->measure_layout(square);
        CHECK(tall.get_width() == Catch::Approx(160.0F));
        CHECK(tall.get_height() == Catch::Approx(320.0F));
    }

    SECTION("fully unbounded loose constraints enclose child natural size at target ratio") {
        box->set_ratio(2.0F);
        const auto measured = box->measure_layout(foundation::NanLayoutConstraints::loose());
        // Child natural size is 100x80; enclosing 2:1 box has width = max(100, 80 * 2) = 160, height = 80.
        CHECK(measured.get_width() == Catch::Approx(160.0F));
        CHECK(measured.get_height() == Catch::Approx(80.0F));
    }
}

TEST_CASE(
    "AspectRatio inscribes and aligns child when parent assigns a mismatched slot",
    "[widget][aspect_ratio]"
) {
    auto child = std::make_shared<scene::NanControl>();
    auto box = widget::AspectRatio::create(2.0F, widget::LayoutAlignment::center);
    box->set_child(child);

    // Parent forces a 200x200 square slot (e.g. inside Expanded or Stack).
    (void)box->measure_layout(
        foundation::NanLayoutConstraints::tight(foundation::NanSize(200.0F, 200.0F))
    );
    box->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 200.0F, 200.0F));

    CHECK(child->width() == Catch::Approx(200.0F));
    CHECK(child->height() == Catch::Approx(100.0F));
    CHECK(child->position().get_x() == Catch::Approx(0.0F));
    CHECK(child->position().get_y() == Catch::Approx(50.0F));
    CHECK(box->content_rect().get_y() == Catch::Approx(50.0F));

    box->set_alignment(widget::LayoutAlignment::start);
    CHECK(child->position().get_x() == Catch::Approx(0.0F));
    CHECK(child->position().get_y() == Catch::Approx(0.0F));

    box->set_alignment(widget::LayoutAlignment::end);
    CHECK(child->position().get_x() == Catch::Approx(0.0F));
    CHECK(child->position().get_y() == Catch::Approx(100.0F));

    // Switch to portrait 1:2 ratio inside the same 200x200 outer slot.
    box->set_ratio(0.5F);
    box->set_alignment(widget::LayoutAlignment::center);
    CHECK(child->width() == Catch::Approx(100.0F));
    CHECK(child->height() == Catch::Approx(200.0F));
    CHECK(child->position().get_x() == Catch::Approx(50.0F));
    CHECK(child->position().get_y() == Catch::Approx(0.0F));
}

TEST_CASE(
    "AspectRatio validates ratio, alignment, child ownership and focus cleanup",
    "[widget][aspect_ratio]"
) {
    CHECK_THROWS_AS(widget::AspectRatio::create(0.0F), std::invalid_argument);
    CHECK_THROWS_AS(widget::AspectRatio::create(-1.5F), std::invalid_argument);
    CHECK_THROWS_AS(
        widget::AspectRatio::create(std::numeric_limits<float>::infinity()),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        widget::AspectRatio::create(std::numeric_limits<float>::quiet_NaN()),
        std::invalid_argument
    );

    auto box = widget::AspectRatio::create(1.5F);
    CHECK_THROWS_AS(box->set_ratio(0.0F), std::invalid_argument);
    CHECK_THROWS_AS(
        box->set_alignment(static_cast<widget::LayoutAlignment>(99)),
        std::invalid_argument
    );
    CHECK_THROWS_AS(box->set_alignment(widget::LayoutAlignment::stretch), std::invalid_argument);
    CHECK_THROWS_AS(
        box->set_alignment(widget::LayoutAlignment::space_between),
        std::invalid_argument
    );
    CHECK_THROWS_AS(box->set_child(nullptr), std::invalid_argument);
    CHECK_THROWS_AS(box->set_child(box), std::invalid_argument);

    auto other_parent = widget::AspectRatio::create(1.0F);
    auto shared_child = std::make_shared<scene::NanControl>();
    other_parent->set_child(shared_child);
    CHECK_THROWS_AS(box->set_child(shared_child), std::invalid_argument);

    // Focus cleanup when replacing or clearing child in an active scene tree.
    scene::NanSceneTree tree;
    auto btn1 = widget::Button::create("One");
    auto btn2 = widget::Button::create("Two");
    box->set_child(btn1);
    tree.set_root(box);
    (void)tree.layout_root(foundation::NanSize(300.0F, 200.0F));

    tree.set_focus(btn1.get());
    REQUIRE(tree.focused_node() == btn1.get());
    tree.set_pointer_capture(btn1.get());
    REQUIRE(tree.pointer_capture() == btn1.get());

    box->set_child(btn2);
    CHECK(tree.focused_node() == nullptr);
    CHECK(tree.pointer_capture() == nullptr);
    CHECK(box->child() == btn2.get());

    tree.set_focus(btn2.get());
    REQUIRE(tree.focused_node() == btn2.get());
    box->clear_child();
    CHECK(tree.focused_node() == nullptr);
    CHECK(box->child() == nullptr);
}

TEST_CASE(
    "AspectRatio supports BuildContext DSL shorthands and ComponentTraits",
    "[widget][aspect_ratio]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager themes;
    widget::BuildContext ui(graph, scope, themes);

    auto dsl_box = ui.aspect_ratio(16.0F / 9.0F, ui.make<widget::Label>("16:9 preview")).build();
    REQUIRE(dsl_box != nullptr);
    CHECK(dsl_box->ratio() == Catch::Approx(16.0F / 9.0F));
    REQUIRE(dsl_box->child() != nullptr);

    auto trait_box =
        ui.make<widget::AspectRatio>(4.0F / 3.0F, ui.make<widget::Label>("4:3").build())
            .alignment(widget::LayoutAlignment::end)
            .build();
    REQUIRE(trait_box != nullptr);
    CHECK(trait_box->ratio() == Catch::Approx(4.0F / 3.0F));
    CHECK(trait_box->alignment() == widget::LayoutAlignment::end);
    REQUIRE(trait_box->child() != nullptr);

    scene::NanSceneTree tree;
    tree.set_root(dsl_box);
    (void)tree.layout_root(foundation::NanSize(320.0F, 320.0F));
    CHECK(dsl_box->child()->width() == Catch::Approx(320.0F));
    CHECK(dsl_box->child()->height() == Catch::Approx(180.0F));
}
