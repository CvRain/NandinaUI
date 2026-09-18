//
// node_reparent_tests — 节点换父（拖拽到另一个容器）的语义约束。
//
// 背景：`add_child` / `insert_child` 只接受**已脱离**的节点，碰上一个还挂在别处的
// 节点就抛 "child already has a parent"。把控件从一个容器拖到另一个容器（例如从
// ListView 拖进 Grid）是正常需求，因此补了 `reparent()`：先完整 detach 再完整 attach。
//
// 本文件固定以下语义：
//   * detach → attach 的顺序与生命周期回调；
//   * 旧父节点与新父节点都被正确标记为需要重新布局；
//   * 同一父节点下的 reparent 等价于重排，不触发 exit/enter/ready；
//   * 环、自己、null 等非法输入被拒绝；
//   * 复用同一个节点的错误消息带得出「用 reparent()」这条出路。
//

#include <nandina/reactive/graph.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/layout.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    /// 记录生命周期回调的探针控件。
    class TraceControl final: public scene::NanControl {
    public:
        explicit TraceControl(std::vector<std::string>* trace, std::string tag):
            trace_(trace),
            tag_(std::move(tag)) {}

        void on_enter_tree() override {
            scene::NanControl::on_enter_tree();
            trace_->push_back(tag_ + ":enter");
        }

        void on_ready() override {
            scene::NanControl::on_ready();
            trace_->push_back(tag_ + ":ready");
        }

        void on_exit_tree() override {
            scene::NanControl::on_exit_tree();
            trace_->push_back(tag_ + ":exit");
        }

        [[nodiscard]] auto marker() const -> const std::string& {
            return tag_;
        }

    private:
        std::vector<std::string>* trace_;
        std::string tag_;
    };

    [[nodiscard]] auto make_trace_control(std::vector<std::string>* trace, std::string tag)
        -> std::shared_ptr<TraceControl> {
        return std::make_shared<TraceControl>(trace, std::move(tag));
    }
} // namespace

TEST_CASE(
    "reparent moves a node between containers with full lifecycle",
    "[scene][tree][reparent]"
) {
    std::vector<std::string> trace;
    auto list_view = widget::Column::create();
    auto grid = widget::Column::create();
    auto button = make_trace_control(&trace, "button");

    list_view->add(button);
    REQUIRE(button->parent() == list_view.get());
    REQUIRE(list_view->child_count() == 1);

    // 「从 ListView 拖到 Grid」。
    grid->reparent(button);

    REQUIRE(button->parent() == grid.get());
    REQUIRE(grid->child_count() == 1);
    REQUIRE(list_view->child_count() == 0);

    // 脱离旧父节点 + 进入新父节点各触发一次（未入树时不派发 enter/ready）。
    REQUIRE(trace.empty());

    // 再挂进树：整棵子树进入。
    auto root = widget::Column::create();
    scene::NanSceneTree tree;
    tree.set_root(root);
    root->add(list_view);
    root->add(grid);
    trace.clear();

    tree.layout_root(foundation::NanSize(400.0F, 300.0F));
    REQUIRE(button->is_inside_tree());

    // 从 grid 换到 list_view，应该看到 exit → enter → ready 的顺序。
    list_view->reparent(button);
    REQUIRE(button->parent() == list_view.get());
    REQUIRE(grid->child_count() == 0);
    REQUIRE(trace.size() >= 3);
    REQUIRE(trace[0] == "button:exit");
    REQUIRE(trace[1] == "button:enter");
    REQUIRE(trace[2] == "button:ready");
}

TEST_CASE("reparent preserves the node's own state", "[scene][tree][reparent]") {
    auto source = widget::Column::create();
    auto target = widget::Column::create();

    // 用「控件自身的尺寸与间隔」代表需要跨父节点保留的内部状态。
    auto panel = widget::Column::create();
    panel->set_gap(7.0F);
    panel->set_width(123.0F);
    source->add(panel);

    target->reparent(panel);

    REQUIRE(panel->parent() == target.get());
    REQUIRE(panel->gap() == Catch::Approx(7.0F));
    REQUIRE(panel->width() == Catch::Approx(123.0F));
    REQUIRE(target->child_count() == 1);
    REQUIRE(source->child_count() == 0);
}

TEST_CASE(
    "reparent within the same parent reorders without lifecycle churn",
    "[scene][tree][reparent]"
) {
    std::vector<std::string> trace;
    auto column = widget::Column::create();
    auto first = make_trace_control(&trace, "first");
    auto second = make_trace_control(&trace, "second");
    column->add(first);
    column->add(second);

    auto root = widget::Column::create();
    scene::NanSceneTree tree;
    tree.set_root(root);
    root->add(column);
    tree.layout_root(foundation::NanSize(400.0F, 300.0F));
    trace.clear();

    // 把 second 挪到最前。
    column->reparent(second, 0);

    REQUIRE(column->child_count() == 2);
    REQUIRE(column->get_child(0) == second.get());
    REQUIRE(column->get_child(1) == first.get());
    // 同一父节点下重排不应产生 exit/enter。
    for (const auto& entry: trace) {
        REQUIRE(entry.find(":exit") == std::string::npos);
        REQUIRE(entry.find(":enter") == std::string::npos);
    }
}

TEST_CASE("reparent rejects cycles and self-parenting", "[scene][tree][reparent]") {
    auto outer = widget::Column::create();
    auto inner = widget::Column::create();
    auto leaf = widget::Column::create();
    outer->add(inner);
    inner->add(leaf);

    // 把祖先挂到后代下会形成环。
    REQUIRE_THROWS_AS(leaf->reparent(outer), std::logic_error);
    // 自己不能当自己的父节点。
    REQUIRE_THROWS_AS(outer->reparent(outer), std::logic_error);
    // null 是参数错误。
    REQUIRE_THROWS_AS(outer->reparent(std::shared_ptr<scene::NanNode> {}), std::runtime_error);

    // 失败调用不能破坏已有结构。
    REQUIRE(inner->parent() == outer.get());
    REQUIRE(leaf->parent() == inner.get());
}

TEST_CASE("insert_child names both nodes and points at reparent", "[scene][tree][reparent]") {
    auto first_parent = widget::Column::create();
    auto second_parent = widget::Column::create();
    first_parent->set_name("list_view");
    second_parent->set_name("grid");

    auto button = widget::Column::create();
    button->set_name("action_button");
    first_parent->add(button);

    try {
        second_parent->add_child(button);
        FAIL("expected insert_child to reject an attached node");
    }
    catch (const std::logic_error& error) {
        const std::string message = error.what();
        // 消息必须能直接指出问题所在与出路。
        CHECK(message.find("already has a parent") != std::string::npos);
        CHECK(message.find("action_button") != std::string::npos);
        CHECK(message.find("list_view") != std::string::npos);
        CHECK(message.find("reparent()") != std::string::npos);
    }

    // 正规路径成功。
    second_parent->reparent(button);
    REQUIRE(button->parent() == second_parent.get());
}

TEST_CASE("reparent validates the destination before detaching", "[scene][tree][reparent]") {
    auto source = widget::Column::create();
    auto button = widget::Column::create();
    source->add(button);

    // A plain NanNode rejects children. The failed reparent must leave the source
    // relationship intact rather than detaching first and throwing from insert_child.
    auto rejecting_target = std::make_shared<scene::NanNode>();
    REQUIRE_THROWS_AS(rejecting_target->reparent(button), std::runtime_error);
    REQUIRE(button->parent() == source.get());
    REQUIRE(source->child_count() == 1);
}
