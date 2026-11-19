// list_view_tests — 批 5 新虚拟化 ListView（IRowModel + IItemDelegate + SelectionModel<RowId>）。
//
// 覆盖：窗口物化上限、滚动换窗与行重建、data_changed 复用、delegate 变化重建、
// 指针 replace/toggle/extend、程序化选择与未知身份、reset 保留身份、键盘单/多选
// 行为、Enter 激活、首布局前 scroll_to_row 挂起、行高校验、模型令牌校验与主题 revision。
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/list_view.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace nandina;

namespace
{
    class TestModel final: public widget::TypedRowModelBase<TestModel> {
    public:
        std::vector<std::string> names;
        std::vector<widget::RowId> ids;

        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return names.size();
        }
        [[nodiscard]] auto row_id(const std::size_t row) const -> widget::RowId override {
            return ids.at(row);
        }

        void reset(std::vector<std::string> next_names, const std::uint64_t first_id) {
            names = std::move(next_names);
            ids.clear();
            ids.reserve(names.size());
            for (std::size_t index = 0; index < names.size(); ++index) {
                ids.push_back(widget::RowId {.value = first_id + index});
            }
            notify_reset();
        }
        void change(const widget::RowRange range) {
            notify_data_changed(range);
        }
    };

    class OtherModel final: public widget::TypedRowModelBase<OtherModel> {
    public:
        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return 0;
        }
        [[nodiscard]] auto row_id(std::size_t) const -> widget::RowId override {
            return {};
        }
    };

    struct StateRecord {
        bool selected = false;
        bool current = false;
        std::uint64_t theme_revision = 0;
    };

    class TestDelegate final: public widget::TypedItemDelegate<TestModel> {
    public:
        std::size_t created = 0;
        std::size_t updated = 0;
        std::unordered_map<std::uint64_t, StateRecord> states;

    protected:
        auto create_typed(
            const widget::BuildContext& ui,
            const TestModel& model,
            const widget::ItemBinding& binding
        ) -> std::shared_ptr<scene::NanControl> override {
            ++created;
            record(binding);
            auto label =
                widget::Label::create(ui.graph(), model.names.at(binding.model_row), ui.theme());
            label->set_typography_role(theme::TypographyRole::label);
            return label;
        }
        void update_typed(
            scene::NanControl& control,
            const TestModel& model,
            const widget::ItemBinding& binding
        ) override {
            ++updated;
            record(binding);
            static_cast<widget::Label&>(control).set_text(model.names.at(binding.model_row));
        }

    private:
        void record(const widget::ItemBinding& binding) {
            states[binding.row.value] = StateRecord {
                .selected = binding.state.selected,
                .current = binding.state.current,
                .theme_revision = binding.state.theme_revision,
            };
        }
    };

    class OtherDelegate final: public widget::TypedItemDelegate<OtherModel> {
    protected:
        auto create_typed(
            const widget::BuildContext& ui,
            const OtherModel&,
            const widget::ItemBinding&
        ) -> std::shared_ptr<scene::NanControl> override {
            return widget::Label::create(ui.graph(), "", ui.theme());
        }
        void update_typed(scene::NanControl&, const OtherModel&, const widget::ItemBinding&)
            override {}
    };

    struct Fixture {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme::ThemeManager themes;
        widget::BuildContext ui {graph, scope, themes};
    };

    struct Harness {
        std::shared_ptr<widget::ListView> list;
        std::shared_ptr<TestDelegate> delegate;
    };

    auto make_names(const std::size_t count) -> std::vector<std::string> {
        std::vector<std::string> names;
        names.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            names.push_back("n" + std::to_string(index));
        }
        return names;
    }

    auto make_harness(
        Fixture& fixture,
        TestModel& model,
        const std::size_t rows = 1000,
        const widget::SelectionMode mode = widget::SelectionMode::multiple,
        const float row_height = 32.0F
    ) -> Harness {
        model.reset(make_names(rows), 1);
        auto list = widget::ListView::create(fixture.ui, mode, row_height);
        auto delegate = std::make_shared<TestDelegate>();
        list->set_delegate(delegate);
        list->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
        return Harness {.list = std::move(list), .delegate = std::move(delegate)};
    }

    auto mount(const std::shared_ptr<widget::ListView>& list)
        -> std::unique_ptr<scene::NanSceneTree> {
        auto tree = std::make_unique<scene::NanSceneTree>();
        tree->set_root(list);
        (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
        return tree;
    }

    auto row_id(const std::uint64_t value) -> widget::RowId {
        return widget::RowId {.value = value};
    }

    void click_node(
        scene::NanSceneTree& tree,
        scene::NanControl& node,
        const scene::KeyModifiers mods = {}
    ) {
        const auto bounds = node.global_bounds();
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent {
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                foundation::NanPoint(
                    (bounds.get_left() + bounds.get_right()) * 0.5F,
                    (bounds.get_top() + bounds.get_bottom()) * 0.5F
                ),
                mods,
            }
        );
    }

    void press_key(
        scene::NanSceneTree& tree,
        const foundation::LogicalKey key,
        const scene::KeyModifiers mods = {}
    ) {
        tree.dispatch_key(scene::KeyEvent(key, scene::KeyEvent::Action::press, mods));
    }

    // 400×300 视口、32 行高 → 可见窗口 10 行。
    constexpr std::size_t kVisibleRows = 10;
    constexpr float kRowHeight = 32.0F;
} // namespace

TEST_CASE("ListView materializes only the visible window", "[list-view][virtualization]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000);
    auto tree = mount(harness.list);

    CHECK(harness.list->row_count() == 1000);
    CHECK(harness.list->item_count() == kVisibleRows);
    CHECK(harness.delegate->created == kVisibleRows);
    CHECK(harness.list->row_node(row_id(1)) != nullptr);
    CHECK(harness.list->row_node(row_id(500)) == nullptr);
}

TEST_CASE("ListView scroll moves the window and recreates rows", "[list-view][scroll]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000);
    auto tree = mount(harness.list);

    harness.list->set_scroll_offset(kRowHeight * 100.0F);
    CHECK(harness.list->item_count() == kVisibleRows);
    CHECK(harness.list->row_node(row_id(101)) != nullptr);
    CHECK(harness.list->row_node(row_id(110)) != nullptr);
    CHECK(harness.list->row_node(row_id(1)) == nullptr);
    CHECK(harness.delegate->created == 2 * kVisibleRows);

    harness.list->set_scroll_offset(0.0F);
    CHECK(harness.list->row_node(row_id(1)) != nullptr);
    CHECK(harness.delegate->created == 3 * kVisibleRows);

    harness.list->set_scroll_offset(-1000.0F);
    CHECK(harness.list->scroll_offset() == 0.0F);
    harness.list->set_scroll_offset(1'000'000.0F);
    CHECK(harness.list->scroll_offset() == Catch::Approx(1000.0F * kRowHeight - 300.0F));
}

TEST_CASE("ListView updates reused rows on data_changed", "[list-view][delegate][reuse]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000);
    auto tree = mount(harness.list);

    auto* node = harness.list->row_node(row_id(1));
    REQUIRE(node != nullptr);
    model.names[0] = "updated";
    model.change({.first = 0, .last_exclusive = 1});

    CHECK(harness.list->row_node(row_id(1)) == node);
    CHECK(static_cast<widget::Label*>(node)->text() == "updated");
    CHECK(harness.delegate->created == kVisibleRows);
    CHECK(harness.delegate->updated >= 1);
}

TEST_CASE("ListView recreates rows when the delegate changes", "[list-view][delegate][reuse]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000);
    auto tree = mount(harness.list);

    // 旧节点可能被分配器复用同一地址，不能以地址变化证明重建；改为验证新 delegate
    // 创建了整窗行，且后续 update 只命中新 delegate。
    const auto old_created = harness.delegate->created;
    const auto old_updated = harness.delegate->updated;
    auto next = std::make_shared<TestDelegate>();
    harness.list->set_delegate(next);

    CHECK(next->created == kVisibleRows);
    CHECK(harness.delegate->created == old_created);

    model.names[0] = "updated";
    model.change({.first = 0, .last_exclusive = 1});
    CHECK(next->updated >= 1);
    CHECK(harness.delegate->updated == old_updated);
}

TEST_CASE("ListView pointer selection replaces, toggles and extends", "[list-view][selection]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000);
    auto tree = mount(harness.list);

    click_node(*tree, *harness.list->row_node(row_id(2)));
    CHECK(harness.list->selection().selected() == std::vector<widget::RowId> {row_id(2)});
    CHECK(tree->focused_node() == harness.list.get());
    CHECK(harness.delegate->states.at(2).selected);
    CHECK(harness.delegate->states.at(2).current);

    click_node(*tree, *harness.list->row_node(row_id(3)), {.ctrl = true});
    CHECK(harness.list->selection().count() == 2);

    click_node(*tree, *harness.list->row_node(row_id(4)));
    CHECK(harness.list->selection().selected() == std::vector<widget::RowId> {row_id(4)});

    click_node(*tree, *harness.list->row_node(row_id(6)), {.shift = true});
    CHECK(harness.list->selection().count() == 3);
    CHECK(harness.list->selection().contains(row_id(5)));
}

TEST_CASE("ListView programmatic selection validates identity and scrolls", "[list-view][selection]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000);
    auto tree = mount(harness.list);

    CHECK(harness.list->select(row_id(500)));
    CHECK(harness.list->selection().contains(row_id(500)));
    CHECK(harness.list->row_node(row_id(500)) != nullptr);

    // 合法但已选中：仍返回 true（“身份有效”语义），选择集合不变。
    CHECK(harness.list->select(row_id(500)));
    CHECK(harness.list->selection().count() == 1);

    CHECK_FALSE(harness.list->select(row_id(9999)));
    harness.list->clear_selection();
    CHECK(harness.list->selection().count() == 0);
}

TEST_CASE("ListView reset retains surviving identities", "[list-view][selection][model]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 10);
    auto tree = mount(harness.list);

    int changes = 0;
    harness.list->set_on_selection_changed([&changes] { ++changes; });
    harness.list->select(row_id(2));
    harness.list->select(row_id(3));
    CHECK(harness.list->selection().count() == 2);

    changes = 0;
    model.reset({"a"}, 3);
    CHECK(harness.list->selection().selected() == std::vector<widget::RowId> {row_id(3)});
    CHECK(changes == 1);
}

TEST_CASE("ListView keyboard moves current and single selection follows", "[list-view][keyboard]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000, widget::SelectionMode::single);
    auto tree = mount(harness.list);

    click_node(*tree, *harness.list->row_node(row_id(2)));
    REQUIRE(harness.list->selection().selected() == std::vector<widget::RowId> {row_id(2)});

    press_key(*tree, foundation::LogicalKey::down);
    CHECK(harness.list->selection().selected() == std::vector<widget::RowId> {row_id(3)});
    CHECK(harness.delegate->states.at(3).current);

    press_key(*tree, foundation::LogicalKey::home);
    CHECK(harness.list->selection().selected() == std::vector<widget::RowId> {row_id(1)});

    press_key(*tree, foundation::LogicalKey::end);
    CHECK(harness.list->selection().selected() == std::vector<widget::RowId> {row_id(1000)});
    CHECK(harness.list->row_node(row_id(1000)) != nullptr);
}

TEST_CASE("ListView keyboard keeps multiple selection while moving current", "[list-view][keyboard]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000);
    auto tree = mount(harness.list);

    click_node(*tree, *harness.list->row_node(row_id(2)));
    press_key(*tree, foundation::LogicalKey::down);
    CHECK(harness.list->selection().selected() == std::vector<widget::RowId> {row_id(2)});
    CHECK(harness.delegate->states.at(3).current);

    press_key(*tree, foundation::LogicalKey::space);
    CHECK(harness.list->selection().contains(row_id(3)));

    press_key(*tree, foundation::LogicalKey::page_down);
    CHECK(harness.delegate->states.at(13).current);
}

TEST_CASE("ListView Enter activates the current row", "[list-view][activation]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000);
    auto tree = mount(harness.list);

    std::optional<widget::RowId> activated;
    harness.list->set_on_activated([&activated](const widget::RowId row) { activated = row; });

    click_node(*tree, *harness.list->row_node(row_id(2)));
    press_key(*tree, foundation::LogicalKey::enter);
    CHECK(activated == std::optional<widget::RowId>(row_id(2)));
}

TEST_CASE("ListView scroll_to_row before layout lands after layout", "[list-view][scroll]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 1000);

    harness.list->scroll_to_row(500);
    CHECK(harness.list->scroll_offset() == 0.0F);

    auto tree = mount(harness.list);
    CHECK(harness.list->scroll_offset() > 0.0F);
    CHECK(harness.list->row_node(row_id(501)) != nullptr);
}

TEST_CASE("ListView item state carries selection and theme revision", "[list-view][theme]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 10);
    auto tree = mount(harness.list);

    click_node(*tree, *harness.list->row_node(row_id(2)));
    const auto& state = harness.delegate->states.at(2);
    CHECK(state.selected);
    CHECK(state.current);
    CHECK(state.theme_revision == fixture.themes.revision());
}

TEST_CASE("ListView builds detached and materializes after mounting", "[list-view][lifecycle]") {
    Fixture fixture;
    TestModel model;
    auto harness = make_harness(fixture, model, 100);

    CHECK(harness.list->item_count() == 0);
    auto tree = mount(harness.list);
    CHECK(harness.list->item_count() == kVisibleRows);
}

TEST_CASE("ListView rejects invalid row heights", "[list-view][validation]") {
    Fixture fixture;

    CHECK_THROWS_AS(
        widget::ListView::create(fixture.ui, widget::SelectionMode::single, 0.0F),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        widget::ListView::create(fixture.ui, widget::SelectionMode::single, -1.0F),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        widget::ListView::create(
            fixture.ui,
            widget::SelectionMode::single,
            std::numeric_limits<float>::quiet_NaN()
        ),
        std::invalid_argument
    );

    TestModel model;
    auto harness = make_harness(fixture, model, 10);
    CHECK_THROWS_AS(harness.list->set_row_height(0.0F), std::invalid_argument);
}

TEST_CASE("ListView rejects delegate model token mismatches", "[list-view][validation]") {
    Fixture fixture;
    TestModel model;
    model.reset(make_names(4), 1);
    auto model_ptr = std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {});

    auto with_other_delegate = widget::ListView::create(fixture.ui);
    with_other_delegate->set_delegate(std::make_shared<OtherDelegate>());
    CHECK_THROWS_AS(with_other_delegate->set_model(model_ptr), std::invalid_argument);
    CHECK(with_other_delegate->model() == nullptr);

    auto with_other_model = widget::ListView::create(fixture.ui);
    with_other_model->set_model(model_ptr);
    CHECK_THROWS_AS(
        with_other_model->set_delegate(std::make_shared<OtherDelegate>()),
        std::invalid_argument
    );
    CHECK(with_other_model->delegate() == nullptr);
}
