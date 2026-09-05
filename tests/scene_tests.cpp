//
// Scene-layer unit tests (Catch2 v3).
// Headless: scene/ never calls raylib, so these run without a window.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "scene/control.hpp"
#include "scene/canvas_layer.hpp"
#include "scene/input_event.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"

#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nandina;

namespace
{

/// Records lifecycle + input callbacks into a shared trace for order assertions.
class ProbeNode : public scene::NanNode2D {
public:
    ProbeNode(std::vector<std::string>* trace, std::string tag, bool focusable = false)
        : trace_(trace), tag_(std::move(tag)), focusable_(focusable) {}

    [[nodiscard]] auto contains_point(foundation::NanPoint local) const -> bool override {
        // Unit box centered at origin (half-extent 10) for simple hit tests.
        return local.get_x() >= -10 && local.get_x() <= 10
            && local.get_y() >= -10 && local.get_y() <= 10;
    }

    [[nodiscard]] auto global_bounds() const -> foundation::NanRect override {
        const auto p = global_position();
        return foundation::NanRect::from_xywh(p.get_x() - 10, p.get_y() - 10, 20, 20);
    }

    [[nodiscard]] auto is_focusable() const -> bool override { return focusable_; }

    void on_enter_tree() override { NanNode2D::on_enter_tree(); log("enter"); }
    void on_ready() override { log("ready"); }
    void on_exit_tree() override { NanNode2D::on_exit_tree(); log("exit"); }

    auto on_input(scene::InputEvent& e) -> bool override {
        switch (e.type()) {
        case scene::EventType::mouse_enter: log("hover_enter"); break;
        case scene::EventType::mouse_leave: log("hover_leave"); break;
        case scene::EventType::focus_enter: log("focus_enter"); break;
        case scene::EventType::focus_leave: log("focus_leave"); break;
        case scene::EventType::mouse_wheel: {
            const auto& w = static_cast<const scene::MouseWheelEvent&>(e);
            log("wheel:" + std::to_string(static_cast<int>(w.delta().get_y())));
            break;
        }
        case scene::EventType::text_input: {
            const auto& t = static_cast<const scene::TextInputEvent&>(e);
            log("text:" + std::string(t.text()));
            break;
        }
        default: break;
        }
        return false;
    }

    std::function<void(ProbeNode&)> on_enter_hook;

protected:
    void log(const std::string& ev) {
        if (trace_) { trace_->push_back(tag_ + ":" + ev); }
    }

private:
    std::vector<std::string>* trace_;
    std::string tag_;
    bool focusable_;
};

/// ProbeNode variant that runs a hook during on_enter_tree (P0 regression).
class HookNode : public ProbeNode {
public:
    using ProbeNode::ProbeNode;
    void on_enter_tree() override {
        ProbeNode::on_enter_tree();
        if (on_enter_hook) { on_enter_hook(*this); }
    }
};

class ProcessMutationNode final: public ProbeNode {
public:
    using ProbeNode::ProbeNode;

    void on_process(float) override {
        if (mutated) {
            return;
        }
        mutated = true;
        added = std::make_shared<ProbeNode>(nullptr, "added");
        add_child(added);
    }

    bool mutated = false;
    std::shared_ptr<ProbeNode> added;
};

auto count(const std::vector<std::string>& trace, const std::string& what) -> int {
    int n = 0;
    for (const auto& s : trace) { if (s == what) { ++n; } }
    return n;
}

auto index_of(const std::vector<std::string>& trace, const std::string& what) -> long {
    for (size_t i = 0; i < trace.size(); ++i) { if (trace[i] == what) { return static_cast<long>(i); } }
    return -1;
}

} // namespace

TEST_CASE("lifecycle callbacks fire in the correct order", "[scene][lifecycle]") {
    std::vector<std::string> trace;

    auto root = std::make_shared<ProbeNode>(&trace, "root");
    auto child = std::make_shared<ProbeNode>(&trace, "child");
    auto grandchild = std::make_shared<ProbeNode>(&trace, "gc");
    child->add_child(grandchild);
    root->add_child(child);

    scene::NanSceneTree tree;
    tree.set_root(root);

    SECTION("enter_tree is top-down") {
        REQUIRE(index_of(trace, "root:enter") < index_of(trace, "child:enter"));
        REQUIRE(index_of(trace, "child:enter") < index_of(trace, "gc:enter"));
    }
    SECTION("ready is bottom-up") {
        REQUIRE(index_of(trace, "gc:ready") < index_of(trace, "child:ready"));
        REQUIRE(index_of(trace, "child:ready") < index_of(trace, "root:ready"));
    }
    SECTION("each callback fires exactly once") {
        REQUIRE(count(trace, "root:ready") == 1);
        REQUIRE(count(trace, "gc:enter") == 1);
    }
}

TEST_CASE("adding a child during on_enter_tree does not double-fire (P0)", "[scene][lifecycle][regression]") {
    std::vector<std::string> trace;

    auto root = std::make_shared<HookNode>(&trace, "root");
    root->on_enter_hook = [&trace](ProbeNode& self) {
        self.add_child(std::make_shared<ProbeNode>(&trace, "late"));
    };

    scene::NanSceneTree tree;
    tree.set_root(root);

    REQUIRE(count(trace, "late:enter") == 1);
    REQUIRE(count(trace, "late:ready") == 1);
}

TEST_CASE("hover tracking sends one enter and one leave", "[scene][hover]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto a = std::make_shared<ProbeNode>(nullptr, "a");
    std::vector<std::string> trace;
    auto b = std::make_shared<ProbeNode>(&trace, "b");
    a->set_position(foundation::NanPoint(50, 50));
    b->set_position(foundation::NanPoint(200, 200));
    root->add_child(a);
    root->add_child(b);

    scene::NanSceneTree tree;
    tree.set_root(root);

    tree.dispatch_mouse_move(scene::MouseMoveEvent(foundation::NanPoint(200, 200), foundation::NanPoint(0, 0)));
    REQUIRE(tree.hovered_node() == b.get());
    REQUIRE(count(trace, "b:hover_enter") == 1);

    tree.dispatch_mouse_move(scene::MouseMoveEvent(foundation::NanPoint(500, 500), foundation::NanPoint(0, 0)));
    REQUIRE(tree.hovered_node() == nullptr);
    REQUIRE(count(trace, "b:hover_leave") == 1);
}

TEST_CASE("control overflow constrains descendant hit testing", "[scene][hit-test][clip]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto clip = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 40.0F));
    auto child = std::make_shared<ProbeNode>(nullptr, "child");
    clip->set_position(foundation::NanPoint(100.0F, 100.0F));
    child->set_position(foundation::NanPoint(35.0F, 20.0F));
    clip->add_child(child);
    root->add_child(clip);

    scene::NanSceneTree tree;
    tree.set_root(root);

    const auto inside = foundation::NanPoint(130.0F, 120.0F);
    const auto protruding = foundation::NanPoint(145.0F, 120.0F);
    REQUIRE(tree.hit_test(inside) == child.get());
    REQUIRE(tree.hit_test(protruding) == child.get());

    clip->set_overflow(scene::ControlOverflow::clip);
    REQUIRE(tree.hit_test(inside) == child.get());
    REQUIRE(tree.hit_test(protruding) == nullptr);
}

TEST_CASE("nested control clips prune complete descendant branches", "[scene][hit-test][clip]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto outer = std::make_shared<scene::NanControl>(foundation::NanSize(50.0F, 50.0F));
    auto intermediary = std::make_shared<scene::NanNode2D>();
    auto inner = std::make_shared<scene::NanControl>(foundation::NanSize(50.0F, 50.0F));
    auto child = std::make_shared<ProbeNode>(nullptr, "child");
    outer->set_position(foundation::NanPoint(100.0F, 100.0F));
    outer->set_overflow(scene::ControlOverflow::clip);
    intermediary->set_position(foundation::NanPoint(30.0F, 0.0F));
    inner->set_overflow(scene::ControlOverflow::clip);
    child->set_position(foundation::NanPoint(30.0F, 20.0F));
    inner->add_child(child);
    intermediary->add_child(inner);
    outer->add_child(intermediary);
    root->add_child(outer);

    scene::NanSceneTree tree;
    tree.set_root(root);

    REQUIRE(tree.hit_test(foundation::NanPoint(150.0F, 120.0F)) == child.get());
    REQUIRE(tree.hit_test(foundation::NanPoint(160.0F, 120.0F)) == nullptr);
}

TEST_CASE("transformed control clips use rendering world bounds", "[scene][hit-test][clip][transform]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto clip = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 20.0F));
    auto child = std::make_shared<ProbeNode>(nullptr, "child");
    clip->set_position(foundation::NanPoint(100.0F, 100.0F));
    clip->set_rotation(0.5F);
    clip->set_scale(1.5F, 0.75F);
    clip->set_overflow(scene::ControlOverflow::clip);
    child->set_position(foundation::NanPoint(20.0F, 10.0F));
    clip->add_child(child);
    root->add_child(clip);

    scene::NanSceneTree tree;
    tree.set_root(root);

    const auto child_center = child->to_global(foundation::NanPoint::zero());
    REQUIRE(clip->global_bounds().contains_point(child_center));
    REQUIRE(tree.hit_test(child_center) == child.get());

    const auto outside = foundation::NanPoint(
        clip->global_bounds().get_right() + 1.0F,
        clip->global_bounds().get_top()
    );
    child->set_position(clip->to_local(outside));
    REQUIRE(tree.hit_test(outside) == nullptr);
}

TEST_CASE("clipped descendants lose hover and cannot receive click focus", "[scene][input][clip]") {
    std::vector<std::string> trace;
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto clip = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 40.0F));
    auto child = std::make_shared<ProbeNode>(&trace, "child", true);
    clip->set_position(foundation::NanPoint(100.0F, 100.0F));
    clip->set_overflow(scene::ControlOverflow::clip);
    child->set_position(foundation::NanPoint(35.0F, 20.0F));
    clip->add_child(child);
    root->add_child(clip);

    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.dispatch_mouse_move(scene::MouseMoveEvent(
        foundation::NanPoint(130.0F, 120.0F), foundation::NanPoint::zero()
    ));
    REQUIRE(tree.hovered_node() == child.get());

    tree.dispatch_mouse_move(scene::MouseMoveEvent(
        foundation::NanPoint(145.0F, 120.0F), foundation::NanPoint(15.0F, 0.0F)
    ));
    REQUIRE(tree.hovered_node() == nullptr);
    REQUIRE(count(trace, "child:hover_leave") == 1);

    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(145.0F, 120.0F)
    ));
    REQUIRE(tree.focused_node() == nullptr);
}

TEST_CASE("focus traversal skips non-focusable and wraps", "[scene][focus]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto f1 = std::make_shared<ProbeNode>(nullptr, "f1", true);
    auto f2 = std::make_shared<ProbeNode>(nullptr, "f2", true);
    auto plain = std::make_shared<ProbeNode>(nullptr, "plain", false);
    root->add_child(f1);
    root->add_child(plain);
    root->add_child(f2);

    scene::NanSceneTree tree;
    tree.set_root(root);

    tree.focus_next();
    REQUIRE(tree.focused_node() == f1.get());
    tree.focus_next();
    REQUIRE(tree.focused_node() == f2.get());  // skips non-focusable plain
    tree.focus_next();
    REQUIRE(tree.focused_node() == f1.get());  // wraps
    tree.focus_previous();
    REQUIRE(tree.focused_node() == f2.get());  // wraps back
}

TEST_CASE("queue_delete dedups descendants and never dangles", "[scene][delete]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    std::weak_ptr<scene::NanNode2D> parentw;
    std::weak_ptr<scene::NanNode2D> childw;
    {
        auto parent = std::make_shared<ProbeNode>(nullptr, "parent");
        auto child = std::make_shared<ProbeNode>(nullptr, "child");
        parentw = parent;
        childw = child;
        parent->add_child(child);
        root->add_child(parent);  // root becomes the sole strong owner
    }

    scene::NanSceneTree tree;
    tree.set_root(root);

    SECTION("ancestor request absorbs descendant request") {
        tree.queue_delete(*parentw.lock());
        tree.queue_delete(*childw.lock());  // deduped under parent

        REQUIRE_FALSE(childw.expired());
        tree.process(0.016F);  // flush
        REQUIRE(childw.expired());
        REQUIRE(parentw.expired());
        REQUIRE(root->child_count() == 0);
    }

    SECTION("node destroyed by another path before flush is skipped safely") {
        auto lone = std::make_shared<ProbeNode>(nullptr, "lone");
        root->add_child(lone);
        tree.queue_delete(*lone);
        root->remove_and_delete(*lone);  // gone before flush
        REQUIRE_NOTHROW(tree.process(0.016F));  // expired weak_ptr must not crash
        REQUIRE(root->child_count() == 1);  // only 'parent' subtree remains
    }
}

TEST_CASE("tree mutation during process is committed after traversal", "[scene][scheduler]") {
    auto root = std::make_shared<ProcessMutationNode>(nullptr, "root");
    scene::NanSceneTree tree;
    tree.set_root(root);

    {
        auto phase = tree.enter_phase(scene::FramePhase::process);
        tree.process(0.016F);
        REQUIRE(root->child_count() == 0);
        REQUIRE_FALSE(root->added->is_inside_tree());
    }

    tree.flush_tree_mutations();
    REQUIRE(root->child_count() == 1);
    REQUIRE(root->added->is_inside_tree());
}

TEST_CASE("ownership-returning removal is rejected during traversal", "[scene][scheduler]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto child = std::make_shared<ProbeNode>(nullptr, "child");
    root->add_child(child);
    scene::NanSceneTree tree;
    tree.set_root(root);

    {
        auto phase = tree.enter_phase(scene::FramePhase::paint);
        REQUIRE_THROWS_AS(root->remove_child(*child), std::logic_error);
        REQUIRE_NOTHROW(root->remove_and_delete(*child));
        REQUIRE(root->child_count() == 1);
    }

    tree.flush_tree_mutations();
    REQUIRE(root->child_count() == 0);
}

TEST_CASE("moving a child preserves lifecycle and focus", "[scene][scheduler][identity]") {
    std::vector<std::string> trace;
    auto root = std::make_shared<ProbeNode>(&trace, "root");
    auto first = std::make_shared<ProbeNode>(&trace, "first", true);
    auto second = std::make_shared<ProbeNode>(&trace, "second", true);
    auto third = std::make_shared<ProbeNode>(&trace, "third", true);
    root->add_child(first);
    root->add_child(third);
    root->insert_child(1, second);
    REQUIRE(root->get_child(0) == first.get());
    REQUIRE(root->get_child(1) == second.get());
    REQUIRE(root->get_child(2) == third.get());
    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.set_focus(second.get());
    trace.clear();

    REQUIRE(root->move_child(*third, 0));
    REQUIRE(root->get_child(0) == third.get());
    REQUIRE(root->get_child(1) == first.get());
    REQUIRE(root->get_child(2) == second.get());
    REQUIRE(tree.focused_node() == second.get());
    REQUIRE(trace.empty());

    REQUIRE_FALSE(root->move_child(*root, 0));
}

TEST_CASE("replacing the scene root discards old frame queues", "[scene][scheduler]") {
    auto old_root = std::make_shared<ProbeNode>(nullptr, "old");
    auto queued_child = std::make_shared<ProbeNode>(nullptr, "queued");
    auto new_root = std::make_shared<ProbeNode>(nullptr, "new");
    scene::NanSceneTree tree;
    tree.set_root(old_root);

    bool post_layout_ran = false;
    {
        auto phase = tree.enter_phase(scene::FramePhase::process);
        old_root->add_child(queued_child);
        tree.post_layout([&post_layout_ran] { post_layout_ran = true; });
    }

    tree.set_root(new_root);
    tree.flush_tree_mutations();
    (void)tree.flush_post_layout_actions();

    REQUIRE(old_root->child_count() == 0);
    REQUIRE_FALSE(queued_child->is_inside_tree());
    REQUIRE_FALSE(post_layout_ran);
    REQUIRE(tree.root() == new_root.get());
}

TEST_CASE("canvas layer input honors order transform and blocking", "[scene][canvas-layer][input]") {
    auto stack = scene::LayerStack::create();
    auto world = scene::CanvasLayer::create(scene::CanvasSpace::world, 0);
    auto hud = scene::CanvasLayer::create(scene::CanvasSpace::screen, 10);
    auto world_target = std::make_shared<ProbeNode>(nullptr, "world");
    auto hud_target = std::make_shared<ProbeNode>(nullptr, "hud");
    foundation::NanTransform2D camera;
    camera.set_position(foundation::NanPoint(100.0F, 50.0F));
    world->set_canvas_transform(camera);
    world_target->set_position(foundation::NanPoint(10.0F, 10.0F));
    hud_target->set_position(foundation::NanPoint(110.0F, 60.0F));
    world->add_child(world_target);
    hud->add_child(hud_target);
    stack->add_layer(world);
    stack->add_layer(hud);
    scene::NanSceneTree tree;
    tree.set_root(stack);

    REQUIRE(tree.hit_test(foundation::NanPoint(110.0F, 60.0F)) == hud_target.get());
    hud_target->set_position(foundation::NanPoint(300.0F, 300.0F));
    REQUIRE(tree.hit_test(foundation::NanPoint(110.0F, 60.0F)) == world_target.get());

    hud->set_input_mode(scene::LayerInputMode::block_below);
    REQUIRE(tree.hit_test(foundation::NanPoint(110.0F, 60.0F)) == nullptr);
    hud->set_input_mode(scene::LayerInputMode::disabled);
    REQUIRE(tree.hit_test(foundation::NanPoint(110.0F, 60.0F)) == world_target.get());
}

TEST_CASE("nested canvas transforms preserve global positioning", "[scene][canvas-layer][transform]") {
    auto stack = scene::LayerStack::create();
    auto layer = scene::CanvasLayer::create(scene::CanvasSpace::world);
    foundation::NanTransform2D canvas;
    canvas.set_position(foundation::NanPoint(80.0F, 40.0F));
    canvas.set_rotation(0.35F);
    canvas.set_scale_xy(1.5F, 1.5F);
    layer->set_canvas_transform(canvas);
    auto parent = std::make_shared<scene::NanNode2D>();
    parent->set_position(foundation::NanPoint(12.0F, 9.0F));
    parent->set_rotation(-0.2F);
    auto child = std::make_shared<ProbeNode>(nullptr, "child");
    parent->add_child(child);
    layer->add_child(parent);
    stack->add_layer(layer);
    scene::NanSceneTree tree;
    tree.set_root(stack);

    const foundation::NanPoint target(175.0F, 95.0F);
    child->set_global_position(target);
    REQUIRE(child->global_position().get_x() == Catch::Approx(target.get_x()).margin(0.001F));
    REQUIRE(child->global_position().get_y() == Catch::Approx(target.get_y()).margin(0.001F));
    REQUIRE(child->to_global(foundation::NanPoint::zero()).get_x()
            == Catch::Approx(target.get_x()).margin(0.001F));
}

TEST_CASE("disabled canvas invalidates focus capture and traversal", "[scene][canvas-layer][input]") {
    auto stack = scene::LayerStack::create();
    auto back = scene::CanvasLayer::create(scene::CanvasSpace::screen, 0);
    auto front = scene::CanvasLayer::create(scene::CanvasSpace::screen, 10);
    auto back_target = std::make_shared<ProbeNode>(nullptr, "back", true);
    auto front_target = std::make_shared<ProbeNode>(nullptr, "front", true);
    back->add_child(back_target);
    front->add_child(front_target);
    stack->add_layer(front);
    stack->add_layer(back);
    scene::NanSceneTree tree;
    tree.set_root(stack);

    tree.focus_next();
    REQUIRE(tree.focused_node() == back_target.get());
    tree.focus_next();
    REQUIRE(tree.focused_node() == front_target.get());
    tree.set_pointer_capture(front_target.get());
    REQUIRE(tree.pointer_capture() == front_target.get());

    front->set_input_mode(scene::LayerInputMode::disabled);
    tree.dispatch_key(scene::KeyEvent(65, scene::KeyEvent::Action::press));
    REQUIRE(tree.focused_node() == nullptr);
    tree.dispatch_mouse_move(scene::MouseMoveEvent(
        foundation::NanPoint::zero(), foundation::NanPoint::zero()
    ));
    REQUIRE(tree.pointer_capture() == nullptr);
    tree.focus_next();
    REQUIRE(tree.focused_node() == back_target.get());
}

TEST_CASE("LayerStack rejects non-layer children", "[scene][canvas-layer]") {
    auto stack = scene::LayerStack::create();
    REQUIRE_THROWS_AS(stack->add_child(std::make_shared<scene::NanNode2D>()), std::runtime_error);
    REQUIRE(stack->child_count() == 0);
    REQUIRE(stack->layer_count() == 0);
}

TEST_CASE("derived LayerStack preserves layer admission contract", "[scene][canvas-layer]") {
    class CustomStack final: public scene::LayerStack {};
    auto stack = std::make_shared<CustomStack>();
    auto layer = scene::CanvasLayer::create();
    REQUIRE_NOTHROW(stack->add_layer(layer));
    REQUIRE_THROWS_AS(stack->add_child(std::make_shared<scene::NanNode2D>()), std::runtime_error);
    REQUIRE(stack->layer_count() == 1);
}

TEST_CASE("removed and re-added node readies again", "[scene][lifecycle]") {
    std::vector<std::string> trace;
    auto root = std::make_shared<ProbeNode>(&trace, "root");
    auto n = std::make_shared<ProbeNode>(&trace, "n");
    root->add_child(n);

    scene::NanSceneTree tree;
    tree.set_root(root);
    REQUIRE(count(trace, "n:ready") == 1);

    auto detached = root->remove_child(*n);
    REQUIRE(count(trace, "n:exit") == 1);
    root->add_child(detached);
    REQUIRE(count(trace, "n:ready") == 2);
}

TEST_CASE("mouse wheel dispatches to hovered node and bubbles", "[scene][wheel]") {
    std::vector<std::string> parent_trace;
    std::vector<std::string> child_trace;
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto parent = std::make_shared<ProbeNode>(&parent_trace, "parent");
    auto child = std::make_shared<ProbeNode>(&child_trace, "child");
    parent->set_position(foundation::NanPoint(100, 100));
    // child sits at same world position as parent (local origin), so hit-test
    // lands on child (deepest) and the wheel event bubbles up to parent.
    parent->add_child(child);
    root->add_child(parent);

    scene::NanSceneTree tree;
    tree.set_root(root);

    SECTION("falls back to hit-test when nothing hovered") {
        tree.dispatch_mouse_wheel(scene::MouseWheelEvent(
            foundation::NanPoint(100, 100), foundation::NanPoint(0, 3)));
        REQUIRE(count(child_trace, "child:wheel:3") == 1);
        REQUIRE(count(parent_trace, "parent:wheel:3") == 1);  // bubbled to parent
    }

    SECTION("uses current hover target") {
        tree.dispatch_mouse_move(scene::MouseMoveEvent(
            foundation::NanPoint(100, 100), foundation::NanPoint(0, 0)));
        REQUIRE(tree.hovered_node() == child.get());
        tree.dispatch_mouse_wheel(scene::MouseWheelEvent(
            foundation::NanPoint(100, 100), foundation::NanPoint(0, -2)));
        REQUIRE(count(child_trace, "child:wheel:-2") == 1);
    }
}

TEST_CASE("text input dispatches to the focused node", "[scene][text]") {
    std::vector<std::string> trace;
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto field = std::make_shared<ProbeNode>(&trace, "field", true);
    root->add_child(field);

    scene::NanSceneTree tree;
    tree.set_root(root);

    SECTION("no focus: text input is dropped") {
        tree.dispatch_text_input(scene::TextInputEvent("hi"));
        REQUIRE(count(trace, "field:text:hi") == 0);
    }

    SECTION("with focus: text reaches the focused node") {
        tree.set_focus(field.get());
        tree.dispatch_text_input(scene::TextInputEvent("hi"));
        REQUIRE(count(trace, "field:text:hi") == 1);
    }
}

TEST_CASE("modifiers propagate through button and key events", "[scene][modifiers]") {
    scene::KeyModifiers mods;
    mods.ctrl = true;
    mods.shift = true;

    scene::KeyEvent key(65, scene::KeyEvent::Action::press, mods);
    REQUIRE(key.modifiers().ctrl);
    REQUIRE(key.modifiers().shift);
    REQUIRE_FALSE(key.modifiers().alt);
    REQUIRE(key.modifiers().any());

    scene::MouseButtonEvent btn(scene::MouseButtonEvent::Button::left,
                                scene::MouseButtonEvent::Action::press,
                                foundation::NanPoint(0, 0), mods);
    REQUIRE(btn.modifiers().ctrl);
    REQUIRE(btn.type() == scene::EventType::mouse_button);
}

TEST_CASE("StyleContext resolves four-state inherited values", "[scene][style]") {
    const auto inherited = theme::ResolvedStyleContext {
        .font_size = 24.0F,
        .locale = "zh-CN",
        .direction = theme::TextDirection::left_to_right,
    };

    theme::StyleContext local;
    local.font_size = theme::StyleValue<float>::initial();
    local.locale = theme::StyleValue<std::string>::inherit();
    local.direction = theme::StyleValue<theme::TextDirection>::explicit_value(
        theme::TextDirection::right_to_left
    );

    const auto resolved = theme::resolve_style_context(local, &inherited);
    REQUIRE(resolved.font_size == Catch::Approx(16.0F));
    REQUIRE(resolved.locale == "zh-CN");
    REQUIRE(resolved.direction == theme::TextDirection::right_to_left);
    REQUIRE(theme::resolve_style_value(local.font_size, 12.0F, &inherited.font_size, false)
            == Catch::Approx(12.0F));
    REQUIRE(
        theme::resolve_style_value(
            theme::StyleValue<int> {},
            3,
            static_cast<const int*>(nullptr),
            false
        )
        == 3
    );
    REQUIRE_THROWS_AS(local.font_size.value(), std::logic_error);
}

TEST_CASE("local opacity defaults to opaque, clamps, and rejects non-finite", "[scene][opacity]") {
    auto node = std::make_shared<scene::NanNode2D>();
    REQUIRE(node->local_opacity() == Catch::Approx(1.0F));

    node->set_local_opacity(0.25F);
    REQUIRE(node->local_opacity() == Catch::Approx(0.25F));

    node->set_local_opacity(2.0F);
    REQUIRE(node->local_opacity() == Catch::Approx(1.0F));

    node->set_local_opacity(-0.5F);
    REQUIRE(node->local_opacity() == Catch::Approx(0.0F));

    // Opacity does not flip visibility: a fully transparent node stays visible.
    REQUIRE(node->visible());

    REQUIRE_THROWS_AS(
        node->set_local_opacity(std::numeric_limits<float>::quiet_NaN()),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        node->set_local_opacity(std::numeric_limits<float>::infinity()),
        std::invalid_argument
    );
}

TEST_CASE("local opacity change marks only paint dirty", "[scene][opacity]") {
    auto control = std::make_shared<scene::NanControl>(foundation::NanSize(10.0F, 10.0F));
    control->clear_dirty(
        scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
    );

    control->set_local_opacity(0.5F);

    REQUIRE(control->is_dirty(scene::DirtyFlags::paint));
    REQUIRE_FALSE(control->is_dirty(scene::DirtyFlags::layout));
    REQUIRE_FALSE(control->is_dirty(scene::DirtyFlags::semantics));
}

TEST_CASE("StyleContext changes propagate through the scene hierarchy", "[scene][style]") {
    auto root = std::make_shared<scene::NanNode2D>();
    auto inherited_child = std::make_shared<scene::NanNode2D>();
    auto initial_child = std::make_shared<scene::NanNode2D>();
    auto grandchild = std::make_shared<scene::NanNode2D>();

    theme::StyleContext initial_override;
    initial_override.font_size = theme::StyleValue<float>::initial();
    initial_child->set_style_context(initial_override);
    initial_child->add_child(grandchild);
    root->add_child(inherited_child);
    root->add_child(initial_child);

    theme::StyleContext root_style;
    root_style.font_size = theme::StyleValue<float>::explicit_value(24.0F);
    root_style.locale = theme::StyleValue<std::string>::explicit_value("zh-CN");
    root->set_style_context(root_style);

    REQUIRE(inherited_child->resolved_style_context().font_size == Catch::Approx(24.0F));
    REQUIRE(inherited_child->resolved_style_context().locale == "zh-CN");
    REQUIRE(initial_child->resolved_style_context().font_size == Catch::Approx(16.0F));
    REQUIRE(grandchild->resolved_style_context().font_size == Catch::Approx(16.0F));
    REQUIRE(grandchild->resolved_style_context().locale == "zh-CN");

    root_style.font_size = theme::StyleValue<float>::explicit_value(30.0F);
    root_style.locale = theme::StyleValue<std::string>::explicit_value("en-GB");
    root->set_style_context(root_style);

    REQUIRE(inherited_child->resolved_style_context().font_size == Catch::Approx(30.0F));
    REQUIRE(inherited_child->resolved_style_context().locale == "en-GB");
    REQUIRE(initial_child->resolved_style_context().font_size == Catch::Approx(16.0F));
    REQUIRE(grandchild->resolved_style_context().locale == "en-GB");

    auto detached = root->remove_child(*inherited_child);
    REQUIRE(detached->resolved_style_context().font_size == Catch::Approx(16.0F));
    REQUIRE(detached->resolved_style_context().locale == "und");

    root->add_child(detached);
    REQUIRE(detached->resolved_style_context().font_size == Catch::Approx(30.0F));
    REQUIRE(detached->resolved_style_context().locale == "en-GB");
}

TEST_CASE("semantics snapshots merge and hide composed descendants", "[scene][semantics]") {
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(240.0F, 120.0F));
    root->set_semantics_composition(semantics::Composition::merge_descendants);
    root->set_semantics_override(
        semantics::Properties {
            .role = semantics::Role::generic,
            .label = "Todo panel",
        }
    );

    auto visible = std::make_shared<scene::NanControl>(foundation::NanSize(80.0F, 24.0F));
    visible->set_semantics_override(
        semantics::Properties {
            .role = semantics::Role::static_text,
            .label = "Visible child",
        }
    );
    auto hidden = std::make_shared<scene::NanControl>(foundation::NanSize(80.0F, 24.0F));
    hidden->set_semantics_composition(semantics::Composition::hidden);
    hidden->set_semantics_override(
        semantics::Properties {
            .role = semantics::Role::static_text,
            .label = "Hidden child",
        }
    );
    root->add_child(visible);
    root->add_child(hidden);

    scene::NanSceneTree tree;
    tree.set_root(root);
    REQUIRE(tree.semantics_dirty());
    REQUIRE(tree.update_semantics());
    REQUIRE_FALSE(tree.semantics_dirty());
    REQUIRE(tree.semantics_tree().roots.size() == 1);

    const auto& semantic_root = tree.semantics_tree().roots.front();
    REQUIRE(semantic_root.id == root->semantics_id());
    REQUIRE(semantic_root.properties.role == semantics::Role::generic);
    REQUIRE(semantic_root.properties.label == "Todo panel, Visible child");
    REQUIRE(semantic_root.children.empty());
    REQUIRE(tree.semantics_tree().find(visible->semantics_id()) == nullptr);
    REQUIRE_FALSE(tree.update_semantics());

    const auto revision = tree.semantics_tree().revision;
    visible->set_semantics_override(
        semantics::Properties {
            .role = semantics::Role::static_text,
            .label = "Updated child",
        }
    );
    REQUIRE(tree.semantics_dirty());
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.semantics_tree().revision == revision + 1);
    REQUIRE(tree.semantics_tree().roots.front().properties.label
            == "Todo panel, Updated child");
}

TEST_CASE("semantics bounds follow the root viewport transform", "[scene][semantics][viewport]") {
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 10.0F));
    root->set_semantics_override(
        semantics::Properties {.role = semantics::Role::generic, .label = "Viewport root"}
    );

    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.set_semantics_transform(foundation::NanTransform2D {
        foundation::NanPoint(10.0F, 20.0F),
        0.0F,
        foundation::NanPoint(2.0F, 2.0F),
    });
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.semantics_tree().roots.front().bounds
            == foundation::NanRect::from_xywh(10.0F, 20.0F, 40.0F, 20.0F));
}
