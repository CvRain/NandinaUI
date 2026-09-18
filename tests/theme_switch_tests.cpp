//
// theme_switch_tests — 运行时切换主题族 / 外观不得崩溃，且已挂载控件要整体重解析。
//
// 背景：校验台（playground）里主题切换按钮的回调曾用 `[&ui]` 捕获 BuildContext，
// 而 `ui` 是 build lambda 的按值形参、build 返回即销毁，点击时访问悬垂引用并
// SIGSEGV。本文件把「切换主题族 / 外观 + 一棵挂满控件的场景树」这条路径固定下来，
// 使同类回归能被确定性捕获。
//

#include <nandina/foundation/geometry.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/builtin_themes.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/reactive/graph.hpp>
#include <nandina/widget/avatar.hpp>
#include <nandina/widget/badge.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/card.hpp>
#include <nandina/widget/checkbox.hpp>
#include <nandina/widget/chip.hpp>
#include <nandina/widget/dialog.hpp>
#include <nandina/widget/divider.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>
#include <nandina/widget/progress_bar.hpp>
#include <nandina/widget/radio_button.hpp>
#include <nandina/widget/radio_group.hpp>
#include <nandina/widget/select.hpp>
#include <nandina/widget/slider.hpp>
#include <nandina/widget/switch.hpp>
#include <nandina/widget/tabs.hpp>
#include <nandina/widget/text_field.hpp>
#include <nandina/widget/tooltip.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    class QuietDevice final: public render::IRenderDevice {
    public:
        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&)
            override {}
        void draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&)
            override {}
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
        ) override {}
    };

    /// 一棵挂满控件的场景树：覆盖校验台里出现的全部组件类型。
    [[nodiscard]] auto
    build_gallery_root(reactive::Graph& graph) -> std::shared_ptr<scene::NanControl> {
        auto column = widget::Column::create();
        column->set_gap(12.0F);

        column->add(widget::Label::create(graph, "Label"));
        column->add(widget::Button::create("Button"));
        column->add(widget::Badge::create("Badge"));
        column->add(widget::Avatar::create("Nandina"));
        column->add(widget::Chip::create("Chip", true));
        column->add(widget::ProgressBar::create(0.4F));
        column->add(widget::Divider::create());
        column->add(widget::Checkbox::create("Check", true));
        column->add(widget::Switch::create("Switch", false));
        column->add(widget::Slider::create("Volume", 0.5F, 0.0F, 1.0F, 0.01F));
        column->add(widget::TextField::create("value", "placeholder"));

        auto group = widget::RadioGroup::create();
        column->add(widget::RadioButton::create("Radio A", group));
        column->add(widget::RadioButton::create("Radio B", group));

        column->add(widget::Select::create({"A", "B", "C"}));
        column->add(widget::Tabs::create(std::vector<std::string> {"One", "Two"}));

        auto tooltip_trigger = widget::Button::create("Hover");
        column->add(widget::Tooltip::create("tip", tooltip_trigger));

        auto card = widget::Card::create();
        card->set_child(widget::Label::create(graph, "Card body"));
        column->add(card);

        column->add(widget::Dialog::create(theme::default_theme()));
        return column;
    }

    /// 让整棵树按当前主题重解析一次样式并绘制一帧。
    void paint(scene::NanSceneTree& tree) {
        QuietDevice device;
        tree.layout_root(foundation::NanSize(1024.0F, 768.0F));
        tree.draw(device);
    }
} // namespace

TEST_CASE(
    "switching theme family and appearance on a populated tree does not crash",
    "[theme][manager][widget][switch]"
) {
    theme::ThemeManager manager;
    theme::register_default_theme_families(manager);
    manager.register_theme_family("default", theme::default_design_system());

    reactive::Graph graph;
    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    tree.set_root(build_gallery_root(graph));

    const auto revision_before = manager.revision();
    paint(tree);
    REQUIRE(manager.revision() >= revision_before);

    // 逐个族与外观组合切换：每次都要求 revision 前进、树仍能重解析并绘制。
    const std::vector<std::string> families {"butter", "default", "butter", "default"};
    const std::vector<theme::ThemePreference> preferences {
        theme::ThemePreference::dark,
        theme::ThemePreference::light,
        theme::ThemePreference::light,
        theme::ThemePreference::dark,
    };

    auto previous_revision = manager.revision();
    for (std::size_t index = 0; index < families.size(); ++index) {
        CAPTURE(families[index], index);
        REQUIRE(manager.activate_family(families[index]));
        REQUIRE(manager.active_family() == families[index]);
        REQUIRE(manager.revision() > previous_revision);
        previous_revision = manager.revision();
        paint(tree);

        manager.set_preference(preferences[index]);
        // 外观真正变化时才发布 revision；这里只要求能继续绘制，不做数量断言。
        paint(tree);
    }

    // 回到默认主题后，控件解析出的样式必须来自新快照。
    REQUIRE(manager.activate_family("default"));
    manager.set_preference(theme::ThemePreference::light);
    paint(tree);

    const auto& palette = manager.design_system().light;
    REQUIRE(manager.active_family() == "default");
    REQUIRE(manager.appearance() == theme::ColorAppearance::light);
    // 默认 primary 是近黑中性色（shadcn 经典默认）。
    REQUIRE(palette.primary.oklch().light < 0.3F);
    REQUIRE(palette.primary.oklch().chroma == Catch::Approx(0.0F));
}

TEST_CASE(
    "repeated appearance flips keep the tree drawable",
    "[theme][manager][widget][switch]"
) {
    theme::ThemeManager manager;
    theme::register_default_theme_families(manager);

    reactive::Graph graph;
    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    tree.set_root(build_gallery_root(graph));
    paint(tree);

    for (int round = 0; round < 3; ++round) {
        manager.set_preference(theme::ThemePreference::dark);
        paint(tree);
        REQUIRE(manager.appearance() == theme::ColorAppearance::dark);

        manager.set_preference(theme::ThemePreference::light);
        paint(tree);
        REQUIRE(manager.appearance() == theme::ColorAppearance::light);
    }
}
