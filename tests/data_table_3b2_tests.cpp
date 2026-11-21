// data_table_3b2_tests — 批 3b②：作者 HeaderDelegate、框架语义 wrapper 与主题重解析。
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/render/draw_context.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/semantics/semantics.hpp>
#include <nandina/theme/builtin_themes.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/data_table.hpp>
#include <nandina/widget/header_delegate.hpp>
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

    auto* header_cell = table->header_node(column_id("name"));
    REQUIRE(header_cell != nullptr);
    auto* header = static_cast<scene::NanControl*>(header_cell->parent());
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

namespace
{
    class CellPaintSpy final: public render::IRenderDevice {
    public:
        std::vector<foundation::NanColor> rounded_fills;
        std::vector<foundation::NanColor> rounded_outlines;

        void begin_frame() override {}
        void end_frame() override {}
        void clear(const foundation::NanColor&) override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void
        draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void draw_rounded_rect(
            const foundation::NanRect&,
            float,
            const foundation::NanColor& color
        ) override {
            rounded_fills.push_back(color);
        }
        void draw_rounded_rect_outline(
            const foundation::NanRect&,
            float,
            float,
            const foundation::NanColor& color
        ) override {
            rounded_outlines.push_back(color);
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {
        }
        void draw_arc(
            const foundation::NanPoint&,
            float,
            float,
            float,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_texture_region(
            render::TextureHandle,
            const foundation::NanRect&,
            const foundation::NanRect&,
            const foundation::NanColor&
        ) override {}
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void destroy_texture(render::TextureHandle) override {}
    };
} // namespace

TEST_CASE(
    "TextItemDelegate draws themed selection fill and current ring",
    "[data-table][3b2][delegate][visual]"
) {
    Fixture fixture;
    theme::register_default_theme_families(fixture.themes);
    TestModel model;
    auto table = make_table(fixture, model, {make_name_column(), make_second_column()});
    auto tree = mount(fixture, table, true);

    const auto target =
        widget::CellId {.row = widget::RowId {.value = 1}, .column = column_id("name")};
    auto* cell = table->cell_node(target);
    REQUIRE(cell != nullptr);
    auto* label = static_cast<widget::Label*>(cell);
    CHECK(label->text() == "a");
    CHECK(label->color_token() == theme::ColorToken::on_surface);

    // 未选中且非当前格：不绘制选中背景与当前边框。
    {
        CellPaintSpy spy;
        render::DrawContext ctx {spy};
        cell->on_draw(ctx);
        CHECK(spy.rounded_fills.empty());
        CHECK(spy.rounded_outlines.empty());
    }

    // 选中并设为当前格：绘制 accent 背景 + accent_foreground 文本色 + ring 边框。
    REQUIRE(table->apply_selection(widget::SelectionOp::replace, target));
    CHECK(label->color_token() == theme::ColorToken::accent_foreground);
    const auto light_accent = theme::resolve_theme_color(
        fixture.themes.theme(),
        theme::ThemeColor::token(theme::ColorToken::accent)
    );
    const auto light_ring = theme::resolve_theme_color(
        fixture.themes.theme(),
        theme::ThemeColor::token(theme::ColorToken::ring)
    );
    {
        CellPaintSpy spy;
        render::DrawContext ctx {spy};
        cell->on_draw(ctx);
        REQUIRE(spy.rounded_fills.size() == 1);
        REQUIRE(spy.rounded_outlines.size() == 1);
        CHECK(spy.rounded_fills.front() == light_accent);
        CHECK(spy.rounded_outlines.front() == light_ring);
    }

    // 切暗色主题后，单元格重解析 accent 与 ring。
    const auto dark_id = theme::ThemeSchemeId::parse("nandina:default/dark");
    REQUIRE(dark_id.has_value());
    REQUIRE(fixture.themes.select(theme::FixedThemeSelection {*dark_id}).has_value());
    const auto dark_accent = theme::resolve_theme_color(
        fixture.themes.theme(),
        theme::ThemeColor::token(theme::ColorToken::accent)
    );
    const auto dark_ring = theme::resolve_theme_color(
        fixture.themes.theme(),
        theme::ThemeColor::token(theme::ColorToken::ring)
    );
    REQUIRE(dark_accent != light_accent);
    {
        CellPaintSpy spy;
        render::DrawContext ctx {spy};
        cell->on_draw(ctx);
        REQUIRE(spy.rounded_fills.size() == 1);
        REQUIRE(spy.rounded_outlines.size() == 1);
        CHECK(spy.rounded_fills.front() == dark_accent);
        CHECK(spy.rounded_outlines.front() == dark_ring);
    }

    // 仅保留 current（清空 selection）：不再画填充背景，仍保留 ring 边框与普通前景色。
    table->clear_selection();
    CHECK(label->color_token() == theme::ColorToken::on_surface);
    {
        CellPaintSpy spy;
        render::DrawContext ctx {spy};
        cell->on_draw(ctx);
        CHECK(spy.rounded_fills.empty());
        REQUIRE(spy.rounded_outlines.size() == 1);
    }
}

TEST_CASE(
    "DataTable exposes top-level selection_changed and sort_changed events",
    "[data-table][3b2][events]"
) {
    Fixture fixture;
    TestModel model;
    auto sortable = make_name_column();
    widget::set_local_sort<TestModel>(
        sortable,
        [](const TestModel& m, const std::size_t a, const std::size_t b) {
            return m.names.at(a) <=> m.names.at(b);
        }
    );
    auto table = make_table(fixture, model, {sortable, make_second_column()});
    auto tree = mount(fixture, table);

    int selection_events = 0;
    int selection_callbacks = 0;
    auto sel_sub =
        table->selection_changed().subscribe([&selection_events] { ++selection_events; });
    table->set_on_selection_changed([&selection_callbacks] { ++selection_callbacks; });

    const auto cell1 =
        widget::CellId {.row = widget::RowId {.value = 1}, .column = column_id("name")};
    REQUIRE(table->apply_selection(widget::SelectionOp::replace, cell1));
    CHECK(selection_events == 1);
    CHECK(selection_callbacks == 1);
    table->clear_selection();
    CHECK(selection_events == 2);
    CHECK(selection_callbacks == 2);

    int sort_events = 0;
    int sort_callbacks = 0;
    auto sort_sub = table->sort_changed().subscribe([&sort_events] { ++sort_events; });
    table->set_on_sort_changed([&sort_callbacks] { ++sort_callbacks; });

    const auto asc = widget::SortSpec {
        .column = column_id("name"),
        .direction = widget::SortDirection::ascending
    };
    REQUIRE(table->set_sort(asc));
    CHECK(sort_events == 1);
    CHECK(sort_callbacks == 1);
    // 幂等排序不重复发事件。
    CHECK_FALSE(table->set_sort(asc));
    CHECK(sort_events == 1);

    // set_columns 移除已排序列时清理排序并通知 sort_changed。
    table->set_columns({make_second_column()});
    CHECK_FALSE(table->sort_spec().has_value());
    CHECK(sort_events == 2);
    CHECK(sort_callbacks == 2);

    // External sort follows the same notification contract, including explicit clearing.
    auto external_column = make_name_column();
    external_column.sort = widget::SortStrategy::external;
    auto external = make_table(fixture, model, {external_column});
    int external_events = 0;
    auto external_sub =
        external->sort_changed().subscribe([&external_events] { ++external_events; });
    REQUIRE(external->set_sort(asc));
    CHECK(external_events == 1);
    CHECK_FALSE(external->set_sort(asc));
    CHECK(external_events == 1);
    REQUIRE(external->set_sort(std::nullopt));
    CHECK(external_events == 2);

    // 回调释放 owner 时不产生 use-after-free。
    auto owned = make_table(fixture, model, {sortable});
    int trailing = 0;
    owned->set_on_sort_changed([&owned] { owned.reset(); });
    auto extra = owned->sort_changed().subscribe([&trailing] { ++trailing; });
    CHECK(owned->set_sort(asc));
    CHECK(owned == nullptr);
    CHECK(trailing == 1);

    // set_columns clearing a sort also survives owner release.
    auto columns_sorted = make_table(fixture, model, {sortable, make_second_column()});
    REQUIRE(columns_sorted->set_sort(asc));
    int columns_sort_events = 0;
    columns_sorted->set_on_sort_changed([&columns_sorted] { columns_sorted.reset(); });
    auto columns_sort_extra =
        columns_sorted->sort_changed().subscribe([&columns_sort_events] { ++columns_sort_events; });
    columns_sorted->set_columns({make_second_column()});
    CHECK(columns_sorted == nullptr);
    CHECK(columns_sort_events == 1);

    // selection_changed follows the same release-owner contract as sort_changed.
    auto selection_owned = make_table(fixture, model, {make_name_column()});
    int selection_trailing = 0;
    selection_owned->set_on_selection_changed([&selection_owned] { selection_owned.reset(); });
    auto selection_extra = selection_owned->selection_changed().subscribe([&selection_trailing] {
        ++selection_trailing;
    });
    CHECK(selection_owned->apply_selection(widget::SelectionOp::replace, cell1));
    CHECK(selection_owned == nullptr);
    CHECK(selection_trailing == 1);

    // A column replacement can remove selected cells through retain while notifying.
    auto columns_owned = make_table(fixture, model, {make_name_column(), make_second_column()});
    REQUIRE(columns_owned->apply_selection(widget::SelectionOp::replace, cell1));
    int columns_trailing = 0;
    columns_owned->set_on_selection_changed([&columns_owned] { columns_owned.reset(); });
    auto columns_extra =
        columns_owned->selection_changed().subscribe([&columns_trailing] { ++columns_trailing; });
    columns_owned->set_columns({make_second_column()});
    CHECK(columns_owned == nullptr);
    CHECK(columns_trailing == 1);

    // A model reset can likewise prune the selected identity during rebuild.
    auto model_owned = make_table(fixture, model, {make_name_column()});
    REQUIRE(model_owned->apply_selection(widget::SelectionOp::replace, cell1));
    int model_trailing = 0;
    model_owned->set_on_selection_changed([&model_owned] { model_owned.reset(); });
    auto model_extra =
        model_owned->selection_changed().subscribe([&model_trailing] { ++model_trailing; });
    model.reset({"replacement"}, 100);
    CHECK(model_owned == nullptr);
    CHECK(model_trailing == 1);
}
