//
// ToggleGroup coordination, roaming, theme, and authoring tests.
//

#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/key_codes.hpp>
#include <nandina/widget/layout.hpp>
#include <nandina/widget/roving_focus.hpp>
#include <nandina/widget/toggle.hpp>
#include <nandina/widget/toggle_group.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

using namespace nandina;

TEST_CASE("toggle group single mode keeps at most one member checked", "[toggle-group][selection]") {
    auto group = widget::ToggleGroup::create();
    auto bold = widget::Toggle::create("Bold", group);
    auto italic = widget::Toggle::create("Italic", group);
    auto mute = widget::Toggle::create("Mute", group);

    REQUIRE(group->mode() == widget::ToggleGroupMode::single);
    REQUIRE(group->allow_empty());
    REQUIRE(group->member_count() == 3);

    std::vector<std::vector<int>> observed;
    auto subscription = group->selection_changed().subscribe([&](std::vector<int> indices) {
        observed.push_back(std::move(indices));
    });

    bold->toggle();
    REQUIRE(bold->checked());
    REQUIRE(group->checked_indices() == std::vector<int> {0});

    italic->toggle();
    REQUIRE(italic->checked());
    REQUIRE_FALSE(bold->checked());
    REQUIRE_FALSE(mute->checked());
    REQUIRE(group->checked_indices() == std::vector<int> {1});

    // allow_empty 默认 true：再点一次已选中的成员取消它（工具栏习惯）。
    italic->toggle();
    REQUIRE_FALSE(italic->checked());
    REQUIRE(group->checked_indices().empty());

    REQUIRE(observed.size() == 3);
    REQUIRE(observed[0] == std::vector<int> {0});
    REQUIRE(observed[1] == std::vector<int> {1});
    REQUIRE(observed[2].empty());
}

TEST_CASE("toggle group can forbid an empty single selection", "[toggle-group][selection]") {
    auto group = widget::ToggleGroup::create();
    group->set_allow_empty(false);
    auto bold = widget::Toggle::create("Bold", group);
    auto italic = widget::Toggle::create("Italic", group);

    bold->toggle();
    REQUIRE(bold->checked());

    int changes = 0;
    auto subscription =
        group->selection_changed().subscribe([&](std::vector<int>) { ++changes; });

    // 取消唯一选中项被忽略：既不改状态，也不发事件。
    bold->toggle();
    REQUIRE(bold->checked());
    REQUIRE(changes == 0);

    italic->toggle();
    REQUIRE(italic->checked());
    REQUIRE_FALSE(bold->checked());
    REQUIRE(changes == 1);
}

TEST_CASE("toggle group multiple mode toggles members independently", "[toggle-group][selection]") {
    auto group = widget::ToggleGroup::create();
    group->set_mode(widget::ToggleGroupMode::multiple);
    auto bold = widget::Toggle::create("Bold", group);
    auto italic = widget::Toggle::create("Italic", group);
    auto mute = widget::Toggle::create("Mute", group);

    REQUIRE(group->mode() == widget::ToggleGroupMode::multiple);

    bold->toggle();
    italic->toggle();
    REQUIRE(bold->checked());
    REQUIRE(italic->checked());
    REQUIRE_FALSE(mute->checked());
    REQUIRE(group->checked_indices() == std::vector<int> {0, 1});

    bold->toggle();
    REQUIRE_FALSE(bold->checked());
    REQUIRE(group->checked_indices() == std::vector<int> {1});
}

TEST_CASE("toggle group select is mode aware and ignores bad indices", "[toggle-group][selection]") {
    auto group = widget::ToggleGroup::create();
    auto first = widget::Toggle::create("A", group);
    auto second = widget::Toggle::create("B", group);

    int changes = 0;
    auto subscription =
        group->selection_changed().subscribe([&](std::vector<int>) { ++changes; });

    group->select(1);
    REQUIRE(second->checked());
    REQUIRE_FALSE(first->checked());
    REQUIRE(group->checked_indices() == std::vector<int> {1});
    REQUIRE(changes == 1);

    group->select(1);
    REQUIRE(changes == 1); // 已选中：no-op

    group->select(-1);
    group->select(7);
    REQUIRE(changes == 1); // 越界：no-op

    group->set_mode(widget::ToggleGroupMode::multiple);
    group->select(0);
    REQUIRE(group->checked_indices() == std::vector<int> {0, 1});
    REQUIRE(changes == 2);
}

TEST_CASE("toggle group arrow keys roam focus and wrap", "[toggle-group][keyboard]") {
    auto group = widget::ToggleGroup::create();
    auto bold = widget::Toggle::create("Bold", group);
    auto italic = widget::Toggle::create("Italic", group);
    auto mute = widget::Toggle::create("Mute", group);
    auto row = widget::Row::create();
    row->add(bold).add(italic).add(mute);

    scene::NanSceneTree tree;
    tree.set_root(row);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 48.0F)) >= 1);
    tree.set_focus(italic.get());
    REQUIRE(tree.focused_node() == italic.get());

    // 默认纵向：下键移到下一个成员。漫游只移动焦点，不改选中值。
    tree.dispatch_key(scene::KeyEvent(widget::keys::down, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == mute.get());
    REQUIRE(group->checked_indices().empty());

    // 末尾环绕到首个成员。
    tree.dispatch_key(scene::KeyEvent(widget::keys::down, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == bold.get());

    // 反方向同样环绕。
    tree.dispatch_key(scene::KeyEvent(widget::keys::up, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == mute.get());

    // 切到横向：左右键漫游，上下键不再是导航键（共享设施按 orientation 过滤）。
    group->set_orientation(widget::RovingOrientation::horizontal);
    tree.dispatch_key(scene::KeyEvent(widget::keys::right, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == bold.get());

    // 横向组里上键不参与漫游：焦点不动，也没有产生 post-layout 动作。
    tree.dispatch_key(scene::KeyEvent(widget::keys::up, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == bold.get());

    // Home / End 也由共享设施回答。
    tree.dispatch_key(scene::KeyEvent(widget::keys::end, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == mute.get());
    tree.dispatch_key(scene::KeyEvent(widget::keys::home, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == bold.get());
}

TEST_CASE("toggle group roaming skips disabled members", "[toggle-group][keyboard]") {
    auto group = widget::ToggleGroup::create();
    auto first = widget::Toggle::create("A", group);
    auto middle = widget::Toggle::create("B", group);
    auto last = widget::Toggle::create("C", group);
    middle->set_disabled(true);

    auto row = widget::Row::create();
    row->add(first).add(middle).add(last);

    scene::NanSceneTree tree;
    tree.set_root(row);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 48.0F)) >= 1);
    tree.set_focus(first.get());
    REQUIRE(tree.focused_node() == first.get());

    tree.dispatch_key(scene::KeyEvent(widget::keys::down, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == last.get());
}

TEST_CASE("toggle group members still activate with space and enter", "[toggle-group][keyboard]") {
    auto group = widget::ToggleGroup::create();
    auto bold = widget::Toggle::create("Bold", group);
    auto italic = widget::Toggle::create("Italic", group);
    auto row = widget::Row::create();
    row->add(bold).add(italic);

    scene::NanSceneTree tree;
    tree.set_root(row);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 48.0F)) >= 1);
    tree.set_focus(italic.get());

    // 成员把按键先交给组，但激活键必须仍然落到成员自己身上。
    tree.dispatch_key(scene::KeyEvent(widget::keys::space, scene::KeyEvent::Action::press));
    REQUIRE(italic->checked());
    REQUIRE(group->checked_indices() == std::vector<int> {1});

    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(italic->checked());
    REQUIRE(group->checked_indices().empty());
}

TEST_CASE("toggle group move_focus follows the configured orientation", "[toggle-group][keyboard]") {
    auto group = widget::ToggleGroup::create();
    auto first = widget::Toggle::create("A", group);
    auto second = widget::Toggle::create("B", group);
    auto third = widget::Toggle::create("C", group);
    auto row = widget::Row::create();
    row->add(first).add(second).add(third);

    scene::NanSceneTree tree;
    tree.set_root(row);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 48.0F)) >= 1);
    tree.set_focus(second.get());

    REQUIRE(group->move_focus(second.get(), 1));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == third.get());

    REQUIRE(group->move_focus(third.get(), 1)); // 末尾环绕
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == first.get());

    REQUIRE_FALSE(group->move_focus(first.get(), 0));
    REQUIRE_FALSE(group->move_focus(nullptr, 1));

    // 未注册的控件不是合法起点。
    auto outsider = widget::Toggle::create("Outsider");
    REQUIRE_FALSE(group->move_focus(outsider.get(), 1));

    // 横向组：方向语义入口按 orientation 合成左右键，仍然有效。
    group->set_orientation(widget::RovingOrientation::horizontal);
    REQUIRE(group->move_focus(first.get(), -1));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == third.get());
}

TEST_CASE("toggle group typeahead moves focus by label prefix", "[toggle-group][keyboard]") {
    auto group = widget::ToggleGroup::create();
    auto bold = widget::Toggle::create("Bold", group);
    auto italic = widget::Toggle::create("Italic", group);
    auto mute = widget::Toggle::create("Mute", group);
    auto row = widget::Row::create();
    row->add(bold).add(italic).add(mute);

    scene::NanSceneTree tree;
    tree.set_root(row);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 48.0F)) >= 1);
    tree.set_focus(bold.get());

    // typeahead 同样由 RovingFocus 提供：命中后焦点跟随，不改变选中值。
    tree.dispatch_text_input(scene::TextInputEvent("m"));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == mute.get());
    REQUIRE(group->checked_indices().empty());

    tree.dispatch_text_input(scene::TextInputEvent("i"));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == italic.get());
}

TEST_CASE("toggle group typeahead buffer expires on the focused member tick", "[toggle-group][keyboard]") {
    auto group = widget::ToggleGroup::create();
    auto bold = widget::Toggle::create("Bold", group);
    auto italic = widget::Toggle::create("Italic", group);
    auto row = widget::Row::create();
    row->add(bold).add(italic);

    scene::NanSceneTree tree;
    tree.set_root(row);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 48.0F)) >= 1);
    tree.set_focus(bold.get());

    tree.dispatch_text_input(scene::TextInputEvent("i"));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(tree.focused_node() == italic.get());
    REQUIRE(group->typeahead_buffer() == "i");

    // 组没有自己的 dt 来源：持焦点的成员在 on_process 里代为推进（焦点排他 = 每帧一次）。
    for (int frame = 0; frame < 10; ++frame) {
        tree.process(0.1F);
    }
    REQUIRE(group->typeahead_buffer().empty());
}

TEST_CASE("toggle group registration keeps indices consistent", "[toggle-group][lifecycle]") {
    auto group = widget::ToggleGroup::create();
    auto first = widget::Toggle::create("A", group);
    auto second = widget::Toggle::create("B", group);
    auto third = widget::Toggle::create("C", group);

    group->select(1);
    REQUIRE(group->checked_indices() == std::vector<int> {1});
    REQUIRE(group->index_of(second.get()) == 1);

    // 注销未选中成员：索引重排，选中项跟着前移。
    group->unregister_toggle(first.get());
    REQUIRE(group->member_count() == 2);
    REQUIRE(group->index_of(second.get()) == 0);
    REQUIRE(group->checked_indices() == std::vector<int> {0});

    // 注销选中成员：集合里不再有它，且不应留下悬垂/错位状态。
    group->unregister_toggle(second.get());
    REQUIRE(group->member_count() == 1);
    REQUIRE(group->index_of(second.get()) == -1);
    REQUIRE(group->checked_indices().empty());
    // 成员自身的 checked 状态不被组改写（只是不再属于任何组索引）。
    REQUIRE(second->checked());

    // 重复注册是 no-op。
    group->register_toggle(third.get());
    group->register_toggle(third.get());
    REQUIRE(group->member_count() == 1);

    // 重新注册后仍可程序化选中。
    group->select(0);
    REQUIRE(group->checked_indices() == std::vector<int> {0});
    REQUIRE(third->checked());
}

TEST_CASE("toggle group unregisters on member destruction", "[toggle-group][lifecycle]") {
    auto group = widget::ToggleGroup::create();
    {
        auto temporary = widget::Toggle::create("Temporary", group);
        REQUIRE(group->member_count() == 1);
    }
    REQUIRE(group->member_count() == 0);
    REQUIRE(group->checked_indices().empty());
}

TEST_CASE("toggle group override survives a theme switch", "[toggle-group][theme]") {
    theme::ThemeManager themes;
    auto group = widget::ToggleGroup::create();
    auto toggle = widget::Toggle::create("Bold", group);

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(toggle);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);

    REQUIRE(
        group->resolved_style().metrics.gap
        == Catch::Approx(themes.design_system().tokens.spacing.xs)
    );

    group->set_override(theme::ToggleGroupRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::error),
    });
    REQUIRE(
        group->resolved_style().container.fill.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );

    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(group->resolved_style().container.fill.oklch().light == Catch::Approx(0.60F));
}

TEST_CASE("toggle group is creatable through its component traits", "[toggle-group][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    // ToggleGroup 不是场景节点（同 RadioGroup）：BuildContext::make<T>() 的约束是
    // scene::NanNode，因此这里走 ComponentTraits 直接构造。
    auto group = widget::ComponentTraits<widget::ToggleGroup>::make(ui);
    auto bold = ui.make<widget::Toggle>("Bold", group).build();
    auto italic = ui.make<widget::Toggle>("Italic", group).build();

    REQUIRE(group->member_count() == 2);
    REQUIRE(bold->group() == group);
    REQUIRE(italic->group() == group);

    bold->toggle();
    REQUIRE(group->checked_indices() == std::vector<int> {0});
    italic->toggle();
    REQUIRE(group->checked_indices() == std::vector<int> {1});
    REQUIRE_FALSE(bold->checked());
}
