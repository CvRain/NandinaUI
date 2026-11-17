// data_table_3b2_tests — 批 3b②：作者 HeaderDelegate、框架语义 wrapper 与主题重解析。
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/semantics/semantics.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/internal/data_table.hpp>
#include <nandina/widget/internal/header_delegate.hpp>
#include <nandina/widget/label.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
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
    };

    struct Fixture {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme::ThemeManager themes;
        widget::BuildContext ui {graph, scope, themes};
    };

    auto column_id(const char* value) -> widget::ColumnId {
        return widget::ColumnId {.value = value};
    }

    struct HeaderSnapshot {
        std::optional<widget::SortDirection> direction;
        bool focused = false;
        std::uint64_t theme_revision = 0;
    };

    class CapturingHeaderDelegate final: public widget::IHeaderDelegate {
    public:
        int create_calls = 0;
        int update_calls = 0;
        std::vector<HeaderSnapshot> snapshots;
        std::vector<std::string> titles;
        std::shared_ptr<widget::Label> visual;
        std::shared_ptr<widget::Label> created_visual;

        auto create_header(const widget::BuildContext& ui, const widget::HeaderBinding& binding)
            -> std::shared_ptr<scene::NanControl> override {
            ++create_calls;
            auto label =
                widget::Label::create(ui.graph(), "h:" + binding.column.title, binding.theme);
            visual = label;
            created_visual = label;
            record(binding);
            return label;
        }

        void
        update_header(scene::NanControl& control, const widget::HeaderBinding& binding) override {
            ++update_calls;
            static_cast<widget::Label&>(control).set_text("h:" + binding.column.title);
            record(binding);
        }

    private:
        void record(const widget::HeaderBinding& binding) {
            snapshots.push_back(
                HeaderSnapshot {
                    .direction = binding.state.sort_direction,
                    .focused = binding.state.focused,
                    .theme_revision = binding.state.theme_revision,
                }
            );
            titles.push_back(binding.column.title);
        }
    };

    class ThrowingHeaderDelegate final: public widget::IHeaderDelegate {
    public:
        auto create_header(const widget::BuildContext&, const widget::HeaderBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            throw std::runtime_error("header create failed");
        }
        void update_header(scene::NanControl&, const widget::HeaderBinding&) override {}
    };

    class ScopedHeaderDelegate final: public widget::IHeaderDelegate {
    public:
        int create_calls = 0;
        int effect_runs = 0;

        auto create_header(const widget::BuildContext& ui, const widget::HeaderBinding& binding)
            -> std::shared_ptr<scene::NanControl> override {
            ++create_calls;
            (void)ui.effect([this] { ++effect_runs; });
            return widget::Label::create(ui.graph(), binding.column.title, binding.theme);
        }
        void
        update_header(scene::NanControl& control, const widget::HeaderBinding& binding) override {
            static_cast<widget::Label&>(control).set_text(binding.column.title);
        }
    };

    class CapturingItemDelegate final: public widget::IItemDelegate {
    public:
        int update_calls = 0;
        std::uint64_t last_theme_revision = 0;

        auto create_item(const widget::BuildContext& ui, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            return widget::Label::create(ui.graph(), "cell");
        }
        void update_item(scene::NanControl& control, const widget::ItemBinding& binding) override {
            ++update_calls;
            last_theme_revision = binding.state.theme_revision;
            static_cast<widget::Label&>(control).set_text("cell");
        }
    };

    auto make_name_column() -> widget::DataTableColumn {
        return widget::text_column<TestModel>(
            column_id("name"),
            "Name",
            [](const TestModel& model, const std::size_t row) { return model.names.at(row); },
            widget::TableColumn {.flex = 1.0F}
        );
    }

    auto make_second_column() -> widget::DataTableColumn {
        return widget::text_column<TestModel>(
            column_id("kind"),
            "Kind",
            [](const TestModel&, const std::size_t row) { return "k" + std::to_string(row); },
            widget::TableColumn {.width = 120.0F}
        );
    }

    auto
    make_table(Fixture& fixture, TestModel& model, std::vector<widget::DataTableColumn> columns)
        -> std::shared_ptr<widget::DataTable> {
        model.reset({"a", "b", "c"}, 1);
        auto table = widget::DataTable::create(fixture.ui, std::move(columns));
        table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
        return table;
    }

    auto mount(
        Fixture& fixture,
        const std::shared_ptr<widget::DataTable>& table,
        const bool attach_theme_manager = false
    ) -> std::unique_ptr<scene::NanSceneTree> {
        auto tree = std::make_unique<scene::NanSceneTree>();
        tree->set_root(table);
        if (attach_theme_manager) {
            tree->set_theme_manager(fixture.themes);
        }
        (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
        return tree;
    }

    auto header_visual(const std::shared_ptr<widget::DataTable>& table, const char* id)
        -> scene::NanControl* {
        auto* wrapper = table->header_node(column_id(id));
        REQUIRE(wrapper != nullptr);
        REQUIRE(wrapper->child_count() == 1);
        auto* visual = wrapper->get_child(0)->as_control();
        REQUIRE(visual != nullptr);
        return visual;
    }
} // namespace

TEST_CASE(
    "DataTable default header visual renders the title and sort state",
    "[data-table][3b2][header]"
) {
    Fixture fixture;
    TestModel model;
    auto name = make_name_column();
    widget::set_local_sort<TestModel>(
        name,
        [](const TestModel& lhs, const std::size_t left, const std::size_t right) {
            return lhs.names.at(left) <=> lhs.names.at(right);
        }
    );
    auto table = make_table(fixture, model, {name, make_second_column()});
    auto tree = mount(fixture, table);

    auto* visual = header_visual(table, "name");
    REQUIRE(visual->child_count() == 1);
    auto* label = static_cast<widget::Label*>(visual->get_child(0)->as_control());
    REQUIRE(label != nullptr);
    CHECK(label->text() == "Name");

    REQUIRE(table->set_sort(
        widget::SortSpec {
            .column = column_id("name"),
            .direction = widget::SortDirection::ascending
        }
    ));
    (void)tree->update_semantics();
    const auto* root = tree->semantics_tree().find(table->semantics_id());
    REQUIRE(root != nullptr);
    CHECK(root->properties.role == semantics::Role::table);
    bool saw_column_header = false;
    bool saw_ascending = false;
    for (const auto& child: root->children) {
        if (child.properties.role != semantics::Role::row) {
            continue;
        }
        for (const auto& column: child.children) {
            saw_column_header =
                saw_column_header || column.properties.role == semantics::Role::column_header;
            saw_ascending = saw_ascending || column.properties.value == "ascending";
        }
    }
    CHECK(saw_column_header);
    CHECK(saw_ascending);
}

TEST_CASE(
    "DataTable author header delegate owns visuals but not semantics",
    "[data-table][3b2][header][delegate]"
) {
    Fixture fixture;
    TestModel model;
    auto delegate = std::make_shared<CapturingHeaderDelegate>();
    auto column = make_name_column();
    widget::set_local_sort<TestModel>(
        column,
        [](const TestModel& lhs, const std::size_t left, const std::size_t right) {
            return lhs.names.at(left) <=> lhs.names.at(right);
        }
    );
    column.header_delegate = delegate;
    auto table = make_table(fixture, model, {column, make_second_column()});
    auto tree = mount(fixture, table);

    CHECK(delegate->create_calls == 1);
    CHECK(delegate->update_calls >= 1);
    auto* visual = header_visual(table, "name");
    CHECK(visual == delegate->visual.get());
    CHECK(static_cast<widget::Label*>(visual)->text() == "h:Name");

    // 框架 wrapper 仍拥有 column_header 语义与排序事实。
    (void)tree->update_semantics();
    const auto* root = tree->semantics_tree().find(table->semantics_id());
    REQUIRE(root != nullptr);
    bool found_wrapper = false;
    for (const auto& child: root->children) {
        if (child.properties.role != semantics::Role::row) {
            continue;
        }
        for (const auto& header: child.children) {
            found_wrapper =
                found_wrapper || header.properties.role == semantics::Role::column_header;
        }
    }
    CHECK(found_wrapper);

    const auto updates_before = delegate->update_calls;
    REQUIRE(table->set_sort(
        widget::SortSpec {
            .column = column_id("name"),
            .direction = widget::SortDirection::descending
        }
    ));
    CHECK(delegate->update_calls > updates_before);
    REQUIRE_FALSE(delegate->snapshots.empty());
    CHECK(delegate->snapshots.back().direction == widget::SortDirection::descending);
}

TEST_CASE(
    "DataTable replaces the header visual when the delegate identity changes",
    "[data-table][3b2][header][delegate]"
) {
    Fixture fixture;
    TestModel model;
    auto first = std::make_shared<CapturingHeaderDelegate>();
    auto column = make_name_column();
    column.header_delegate = first;
    auto second_column = make_second_column();
    auto table = make_table(fixture, model, {column, second_column});
    auto tree = mount(fixture, table);
    (void)tree;

    auto* old_visual = header_visual(table, "name");
    REQUIRE(old_visual == first->visual.get());

    auto replacement = std::make_shared<CapturingHeaderDelegate>();
    column.header_delegate = replacement;
    table->set_columns({column, second_column});

    CHECK(replacement->create_calls == 1);
    CHECK(first->create_calls == 1);
    auto* new_visual = header_visual(table, "name");
    CHECK(new_visual == replacement->visual.get());
    CHECK(new_visual != old_visual);
}

TEST_CASE(
    "DataTable keeps the old header visual when create fails",
    "[data-table][3b2][header][delegate]"
) {
    Fixture fixture;
    TestModel model;
    auto delegate = std::make_shared<CapturingHeaderDelegate>();
    auto column = make_name_column();
    column.header_delegate = delegate;
    auto second_column = make_second_column();
    auto table = make_table(fixture, model, {column, second_column});
    (void)mount(fixture, table);

    auto* old_visual = header_visual(table, "name");
    REQUIRE(old_visual == delegate->visual.get());

    auto throwing = std::make_shared<ThrowingHeaderDelegate>();
    column.header_delegate = throwing;
    CHECK_THROWS_AS(table->set_columns({column, second_column}), std::runtime_error);

    auto* visual_after = header_visual(table, "name");
    CHECK(visual_after == old_visual);
    CHECK(static_cast<widget::Label*>(visual_after)->text() == "h:Name");
}

TEST_CASE(
    "DataTable re-resolves header and cell delegates on theme revision",
    "[data-table][3b2][header][theme]"
) {
    Fixture fixture;
    TestModel model;
    auto header_delegate = std::make_shared<CapturingHeaderDelegate>();
    auto item_delegate = std::make_shared<CapturingItemDelegate>();
    auto column = make_name_column();
    column.header_delegate = header_delegate;
    auto custom = widget::custom_column(
        column_id("kind"),
        "Kind",
        widget::TableColumn {.width = 120.0F},
        item_delegate
    );
    auto table = make_table(fixture, model, {column, custom});
    auto tree = mount(fixture, table, true);
    (void)tree;

    const auto initial_revision = fixture.themes.revision();
    REQUIRE(header_delegate->create_calls == 1);
    REQUIRE_FALSE(header_delegate->snapshots.empty());
    CHECK(header_delegate->snapshots.back().theme_revision == initial_revision);

    fixture.themes.set_text_scaling(1.25F);
    const auto new_revision = fixture.themes.revision();
    REQUIRE(new_revision > initial_revision);

    // 表头只 update、不重建；新 theme/revision 通过 HeaderBinding 下发。
    CHECK(header_delegate->create_calls == 1);
    REQUIRE_FALSE(header_delegate->snapshots.empty());
    CHECK(header_delegate->snapshots.back().theme_revision == new_revision);
    // 可见单元格 delegate 收到新的 ItemState.theme_revision。
    CHECK(item_delegate->last_theme_revision == new_revision);
}

TEST_CASE(
    "DataTable rebuilds header delegate scopes after tree re-entry",
    "[data-table][3b2][header][delegate][binding]"
) {
    Fixture fixture;
    TestModel model;
    auto delegate = std::make_shared<ScopedHeaderDelegate>();
    auto column = make_name_column();
    column.header_delegate = delegate;
    auto table = make_table(fixture, model, {column});
    auto tree = mount(fixture, table);

    REQUIRE(delegate->create_calls == 1);
    const int runs_before_exit = delegate->effect_runs;
    tree->set_root(nullptr);
    tree->set_root(table);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));

    CHECK(delegate->create_calls == 2);
    CHECK(delegate->effect_runs > runs_before_exit);
}

TEST_CASE(
    "DataTable header delegate observes keyboard focus changes",
    "[data-table][3b2][header][focus]"
) {
    Fixture fixture;
    TestModel model;
    auto name_delegate = std::make_shared<CapturingHeaderDelegate>();
    auto kind_delegate = std::make_shared<CapturingHeaderDelegate>();
    auto column = make_name_column();
    column.header_delegate = name_delegate;
    auto second = make_second_column();
    second.header_delegate = kind_delegate;
    auto table = make_table(fixture, model, {column, second});
    auto tree = mount(fixture, table);

    auto* header = table->header_host();
    REQUIRE(header != nullptr);
    tree->set_focus(header);
    tree->dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::right, scene::KeyEvent::Action::press)
    );

    // 表头拥有焦点且焦点列移到第二列：第二列 focused=true，第一列 false。
    REQUIRE_FALSE(name_delegate->snapshots.empty());
    REQUIRE_FALSE(kind_delegate->snapshots.empty());
    CHECK(name_delegate->snapshots.back().focused == false);
    CHECK(kind_delegate->snapshots.back().focused == true);
}
