//
// key_lifecycle_tests — 键身份与键生命周期的消费者行为回归（P3.1b）。
//
// 目的：enter/space 这类单次激活不能被自动重复触发；方向键、删除与光标移动必须接受
// 自动重复；释放不改变状态；桌面编辑命令按 Ctrl 匹配且 AltGr 的 Alt 不冒充命令。
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/primitives/editable_text.hpp>
#include <nandina/widget/select.hpp>
#include <nandina/widget/slider.hpp>
#include <nandina/widget/text_field.hpp>

#include <memory>
#include <string>

namespace
{
    using namespace nandina;

    auto key(foundation::LogicalKey logical, scene::KeyEvent::Action action) -> scene::KeyEvent {
        return scene::KeyEvent(logical, action);
    }

    auto
    key(foundation::LogicalKey logical,
        scene::KeyEvent::Action action,
        scene::KeyModifiers modifiers) -> scene::KeyEvent {
        return scene::KeyEvent(logical, action, modifiers);
    }
} // namespace

TEST_CASE("editing repeats deletion and caret movement but not commands", "[input][keys][repeat]") {
    auto edit = std::make_shared<widget::primitives::EditableText>("abc");
    scene::NanSceneTree tree;
    tree.set_root(edit);
    tree.set_focus(edit.get());

    tree.dispatch_key(key(foundation::LogicalKey::backspace, scene::KeyEvent::Action::press));
    tree.dispatch_key(key(foundation::LogicalKey::backspace, scene::KeyEvent::Action::repeat));
    tree.dispatch_key(key(foundation::LogicalKey::backspace, scene::KeyEvent::Action::repeat));
    REQUIRE(edit->value().empty());

    tree.dispatch_text_input(scene::TextInputEvent("abcd"));
    tree.dispatch_key(key(foundation::LogicalKey::home, scene::KeyEvent::Action::press));
    tree.dispatch_key(key(foundation::LogicalKey::right, scene::KeyEvent::Action::press));
    tree.dispatch_key(key(foundation::LogicalKey::right, scene::KeyEvent::Action::repeat));
    tree.dispatch_text_input(scene::TextInputEvent("x"));
    REQUIRE(edit->value() == "abxcd");

    // 释放不编辑：光标停在原位。
    tree.dispatch_key(key(foundation::LogicalKey::backspace, scene::KeyEvent::Action::release));
    REQUIRE(edit->value() == "abxcd");

    // 撤销只在初次按下执行；重复的 Ctrl+Z 不得再退一步。
    scene::KeyModifiers ctrl;
    ctrl.ctrl = true;
    tree.dispatch_key(key(foundation::LogicalKey::z, scene::KeyEvent::Action::press, ctrl));
    REQUIRE(edit->value() == "abcd");
    tree.dispatch_key(key(foundation::LogicalKey::z, scene::KeyEvent::Action::repeat, ctrl));
    REQUIRE(edit->value() == "abcd");

    // AltGr 的聚合 Alt 不能让普通字符变成编辑命令。
    scene::KeyModifiers alt_gr;
    alt_gr.ctrl = true;
    alt_gr.alt = true;
    tree.dispatch_key(key(foundation::LogicalKey::a, scene::KeyEvent::Action::press, alt_gr));
    // 若被误当成 Select All，"z" 会替换整段；这里只在撤销后的光标处插入。
    tree.dispatch_text_input(scene::TextInputEvent("z"));
    REQUIRE(edit->value() == "abzcd");
}

TEST_CASE("single-shot activation ignores auto repeat", "[input][keys][repeat]") {
    auto button = std::make_shared<widget::Button>("Save");
    int clicks = 0;
    button->set_on_click([&clicks] { ++clicks; });
    scene::NanSceneTree tree;
    tree.set_root(button);
    tree.set_focus(button.get());

    tree.dispatch_key(key(foundation::LogicalKey::enter, scene::KeyEvent::Action::press));
    tree.dispatch_key(key(foundation::LogicalKey::enter, scene::KeyEvent::Action::repeat));
    tree.dispatch_key(key(foundation::LogicalKey::space, scene::KeyEvent::Action::repeat));
    tree.dispatch_key(key(foundation::LogicalKey::enter, scene::KeyEvent::Action::release));
    REQUIRE(clicks == 1);
}

TEST_CASE("slider repeats arrow steps and ignores release", "[input][keys][repeat]") {
    auto slider = std::make_shared<widget::Slider>("Scale", 1.0F, 0.5F, 2.0F, 0.1F);
    scene::NanSceneTree tree;
    tree.set_root(slider);
    tree.set_focus(slider.get());

    tree.dispatch_key(key(foundation::LogicalKey::right, scene::KeyEvent::Action::press));
    tree.dispatch_key(key(foundation::LogicalKey::right, scene::KeyEvent::Action::repeat));
    tree.dispatch_key(key(foundation::LogicalKey::right, scene::KeyEvent::Action::repeat));
    REQUIRE(slider->value() == Catch::Approx(1.3F));

    tree.dispatch_key(key(foundation::LogicalKey::right, scene::KeyEvent::Action::release));
    REQUIRE(slider->value() == Catch::Approx(1.3F));
}

TEST_CASE("select repeats navigation but activation happens once", "[input][keys][repeat]") {
    auto select = widget::Select::create({"A", "B", "C"});
    scene::NanSceneTree tree;
    tree.set_root(select);
    tree.set_focus(select.get());

    tree.dispatch_key(key(foundation::LogicalKey::space, scene::KeyEvent::Action::press));
    REQUIRE(select->is_open());

    tree.dispatch_key(key(foundation::LogicalKey::down, scene::KeyEvent::Action::press));
    tree.dispatch_key(key(foundation::LogicalKey::down, scene::KeyEvent::Action::repeat));
    REQUIRE(select->selected_index() == 2);

    tree.dispatch_key(key(foundation::LogicalKey::enter, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(select->is_open());
    // 按住 Enter 不应把刚关闭的列表重新打开。
    tree.dispatch_key(key(foundation::LogicalKey::enter, scene::KeyEvent::Action::repeat));
    REQUIRE_FALSE(select->is_open());
}

TEST_CASE("text field submission is not repeated", "[input][keys][repeat]") {
    auto field = std::make_shared<widget::TextField>("draft");
    int submissions = 0;
    field->set_on_submit([&submissions](std::string_view) { ++submissions; });
    scene::NanSceneTree tree;
    tree.set_root(field);
    tree.set_focus(field.get());

    tree.dispatch_key(key(foundation::LogicalKey::enter, scene::KeyEvent::Action::press));
    tree.dispatch_key(key(foundation::LogicalKey::enter, scene::KeyEvent::Action::repeat));
    tree.dispatch_key(key(foundation::LogicalKey::enter, scene::KeyEvent::Action::release));
    REQUIRE(submissions == 1);
}
