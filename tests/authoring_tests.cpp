#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "reactive/graph.hpp"
#include "reactive/signal.hpp"
#include "scene/canvas_layer.hpp"
#include "scene/input_event.hpp"
#include "scene/scene_tree.hpp"
#include "semantics/semantics.hpp"
#include "widget/authoring.hpp"
#include "widget/build_context.hpp"
#include "widget/button.hpp"
#include "widget/controls.hpp"
#include "widget/declarative.hpp"
#include "widget/grid.hpp"
#include "widget/label.hpp"
#include "widget/layout.hpp"
#include "widget/text_field.hpp"

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nandina;

namespace
{
    struct Item {
        int key = 0;
        std::string value;

        auto operator==(const Item&) const -> bool = default;
    };

    class ItemControl final: public scene::NanControl {
    public:
        explicit ItemControl(const int key): key(key) {}

        int key;
        std::string value;
    };

    class ScopedComponent final: public scene::NanControl {
    public:
        ScopedComponent(widget::BuildContext ui, const reactive::Event<int>& event, int& observed) {
            ui.connect(event, [&observed](const int value) { observed += value; });
        }
    };

    class ScopedItemControl final: public scene::NanControl {
    public:
        ScopedItemControl(
            widget::BuildContext ui,
            const reactive::Event<int>& selected,
            int key,
            int& selections
        ):
            key(key) {
            ui.connect(selected, [this, &selections](const int selected_key) {
                if (selected_key == this->key) {
                    ++selections;
                }
            });
        }

        int key;
        std::string value;
    };
} // namespace

TEST_CASE("authoring builders preserve concrete widget identity", "[authoring][widget]") {
    reactive::Graph graph;
    std::shared_ptr<widget::Label> title;
    std::shared_ptr<widget::Button> action;

    auto root = widget::authoring::make<widget::Column>()
                    .configure([](widget::Column& column) {
                        column.set_gap(12.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
                    })
                    .children(
                        widget::authoring::make<widget::Label>(graph, "Tasks").expose(title),
                        widget::authoring::make<widget::Button>("Add")
                            .configure([](widget::Button& button) {
                                button.set_treatment(theme::ButtonTreatment::outlined);
                            })
                            .expose(action)
                    )
                    .build();

    static_assert(std::same_as<decltype(root), std::shared_ptr<widget::Column>>);
    REQUIRE(root->child_count() == 2);
    REQUIRE(root->get_child(0) == title.get());
    REQUIRE(root->get_child(1) == action.get());
    REQUIRE(title->text() == "Tasks");
    REQUIRE(action->treatment() == theme::ButtonTreatment::outlined);
    REQUIRE(root->gap() == Catch::Approx(12.0F));
}

TEST_CASE(
    "imperative and authored trees resolve equivalent layout and style",
    "[authoring][layout][style]"
) {
    auto imperative_button = widget::Button::create("Continue");
    imperative_button->set_tone(theme::ButtonTone::secondary);
    imperative_button->set_treatment(theme::ButtonTreatment::outlined);
    auto imperative = widget::Column::create();
    imperative->set_gap(7.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
    imperative->add(imperative_button);

    std::shared_ptr<widget::Button> authored_button;
    auto authored =
        widget::authoring::make<widget::Column>()
            .configure([](widget::Column& value) {
                value.set_gap(7.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
            })
            .children(
                widget::authoring::make<widget::Button>("Continue")
                    .configure([](widget::Button& value) {
                        value.set_tone(theme::ButtonTone::secondary);
                        value.set_treatment(theme::ButtonTreatment::outlined);
                    })
                    .expose(authored_button)
            )
            .build();

    scene::NanSceneTree imperative_tree;
    imperative_tree.set_root(imperative);
    scene::NanSceneTree authored_tree;
    authored_tree.set_root(authored);
    REQUIRE(imperative_tree.layout_root(foundation::NanSize(240.0F, 100.0F)) == 1);
    REQUIRE(authored_tree.layout_root(foundation::NanSize(240.0F, 100.0F)) == 1);

    REQUIRE(authored_button->global_bounds() == imperative_button->global_bounds());
    REQUIRE(
        authored_button->resolved_style().container.border_width
        == Catch::Approx(imperative_button->resolved_style().container.border_width)
    );
    REQUIRE(authored_button->tone() == imperative_button->tone());
    REQUIRE(authored_button->treatment() == imperative_button->treatment());
}

TEST_CASE("authoring child and expose use the original concrete nodes", "[authoring][widget]") {
    std::shared_ptr<widget::Button> button;
    auto padding = widget::authoring::make<widget::Padding>(foundation::NanInsets::all(8.0F))
                       .child(widget::authoring::make<widget::Button>("Save").expose(button))
                       .build();

    REQUIRE(padding->child_count() == 1);
    REQUIRE(padding->get_child(0) == button.get());
    REQUIRE(button->parent() == padding.get());
}

TEST_CASE("authored bindings keep the concrete widget lifecycle", "[authoring][reactive]") {
    reactive::Graph graph;
    reactive::Signal<std::string> text {graph, "first"};
    auto label = widget::authoring::make<widget::Label>(graph)
                     .configure([&](widget::Label& value) { value.bind_text(text); })
                     .build();

    scene::NanSceneTree tree;
    tree.set_root(label);
    REQUIRE(label->text() == "first");
    text.set("second");
    REQUIRE(label->text() == "second");

    tree.set_root({});
    text.set("after detach");
    REQUIRE(label->text() == "second");
}

TEST_CASE("authored buttons retain input and semantics behavior", "[authoring][semantics]") {
    int activations = 0;
    auto button =
        widget::authoring::make<widget::Button>("Run")
            .configure([&](widget::Button& value) { value.set_on_click([&] { ++activations; }); })
            .build();
    scene::NanSceneTree tree;
    tree.set_root(button);
    REQUIRE(tree.layout_root(foundation::NanSize(160.0F, 60.0F)) == 1);
    REQUIRE(tree.update_semantics());

    REQUIRE(tree.perform_semantics_action(
        button->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(activations == 1);
}

TEST_CASE("authoring wraps keyed regions without changing reuse", "[authoring][declarative]") {
    reactive::Graph graph;
    reactive::Signal<std::vector<Item>> items {
        graph,
        {{.key = 1, .value = "one"}, {.key = 2, .value = "two"}},
    };
    using Region = widget::ForEach<Item, int, ItemControl>;
    auto region = widget::authoring::from(
                      Region::create(
                          graph,
                          [](const Item& item) { return item.key; },
                          [](reactive::ReactiveScope&, const Item& item) {
                              return std::make_shared<ItemControl>(item.key);
                          },
                          [](ItemControl& control, const Item& item) { control.value = item.value; }
                      )
    ).configure([&](Region& value) {
         value.bind(items);
     }).build();
    scene::NanSceneTree tree;
    tree.set_root(region);

    auto* first = region->node_for(1);
    items.set({{.key = 2, .value = "updated"}, {.key = 1, .value = "retained"}});
    REQUIRE(region->node_for(1) == first);
    REQUIRE(first->value == "retained");
    REQUIRE(region->get_child(1) == first);
}

TEST_CASE("ListView binds a structural model without inheritance", "[widget][list][model]") {
    reactive::Graph graph;
    reactive::Signal<std::vector<Item>> items {
        graph,
        {{.key = 1, .value = "one"}, {.key = 2, .value = "two"}},
    };
    using View = widget::ListView<Item, int, ItemControl>;
    auto view = View::create(
        graph,
        [](const Item& item) { return item.key; },
        [](reactive::ReactiveScope&, const Item& item) {
            return std::make_shared<ItemControl>(item.key);
        },
        [](ItemControl& control, const Item& item) { control.value = item.value; }
    );
    view->set_model(items);

    scene::NanSceneTree tree;
    tree.set_root(view);
    REQUIRE(view->item_count() == 2);
    auto* retained = view->node_for(1);

    items.set({{.key = 2, .value = "updated"}, {.key = 1, .value = "retained"}});
    REQUIRE(view->node_for(1) == retained);
    REQUIRE(retained->value == "retained");
    REQUIRE(view->get_child(1) == retained);
}

TEST_CASE("BuildContext authors scoped conditional and keyed regions", "[authoring][region]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    reactive::Signal<bool> visible {graph, false};
    reactive::Event<int> selected;
    int branch_events = 0;

    auto conditional = ui.when(visible, [&](widget::BuildContext branch) {
        return branch.make<ScopedComponent>(selected, branch_events);
    }).build();
    scene::NanSceneTree conditional_tree;
    conditional_tree.set_root(conditional);
    REQUIRE(conditional->active_node() == nullptr);

    visible.set(true);
    REQUIRE(conditional->active_node() != nullptr);
    selected.emit(4);
    REQUIRE(branch_events == 4);
    visible.set(false);
    selected.emit(4);
    REQUIRE(branch_events == 4);

    reactive::Signal<bool> choose_label {graph, false};
    auto choice = ui.when(
        choose_label,
        [](widget::BuildContext branch) {
            return branch.make<widget::Label>("true branch");
        },
        [](widget::BuildContext branch) {
            return branch.make<widget::Button>("false branch");
        }
    ).build();
    scene::NanSceneTree choice_tree;
    choice_tree.set_root(choice);
    REQUIRE(dynamic_cast<widget::Button*>(choice->active_node()) != nullptr);
    choose_label.set(true);
    REQUIRE(dynamic_cast<widget::Label*>(choice->active_node()) != nullptr);

    reactive::Signal<std::vector<Item>> items {
        graph,
        {{.key = 1, .value = "one"}, {.key = 2, .value = "two"}},
    };
    int selections = 0;
    auto rows = ui.for_each(
        items,
        &Item::key,
        [&](widget::BuildContext item, const Item& value) {
            return item.make<ScopedItemControl>(selected, value.key, selections);
        },
        [](ScopedItemControl& row, const Item& value) { row.value = value.value; }
    ).build();
    scene::NanSceneTree rows_tree;
    rows_tree.set_root(rows);
    auto* retained = rows->node_for(1);
    REQUIRE(retained != nullptr);
    REQUIRE(retained->value == "one");

    items.set({{.key = 2, .value = "updated"}, {.key = 1, .value = "retained"}});
    REQUIRE(rows->node_for(1) == retained);
    REQUIRE(retained->value == "retained");
    REQUIRE(rows->get_child(1) == retained);

    selected.emit(1);
    REQUIRE(selections == 1);
    items.set({{.key = 2, .value = "remaining"}});
    selected.emit(1);
    REQUIRE(selections == 1);

    auto simple_rows = ui.for_each(
        items,
        &Item::key,
        [](widget::BuildContext, const Item& value) {
            return widget::authoring::make<ItemControl>(value.key);
        }
    ).build();
    scene::NanSceneTree simple_tree;
    simple_tree.set_root(simple_rows);
    REQUIRE(simple_rows->item_count() == 1);
    REQUIRE(simple_rows->node_for(2) != nullptr);
}

TEST_CASE("authoring configures existing canvas factories", "[authoring][canvas]") {
    auto layer = widget::authoring::from(scene::CanvasLayer::create(scene::CanvasSpace::world, 2))
                     .configure([](scene::CanvasLayer& value) {
                         value.set_input_mode(scene::LayerInputMode::block_below);
                     })
                     .build();
    auto stack = widget::authoring::from(scene::LayerStack::create())
                     .configure([&](scene::LayerStack& value) { value.add_layer(layer); })
                     .build();

    static_assert(std::same_as<decltype(layer), std::shared_ptr<scene::CanvasLayer>>);
    REQUIRE(stack->layer_at(0) == layer.get());
    REQUIRE(layer->order() == 2);
    REQUIRE(layer->input_mode() == scene::LayerInputMode::block_below);
}

TEST_CASE("authoring rejects null factory results", "[authoring]") {
    REQUIRE_THROWS_AS(
        widget::authoring::from(std::shared_ptr<widget::Button> {}),
        std::invalid_argument
    );
}

TEST_CASE("free function factories produce same types as make<T>", "[authoring][factory]") {
    using namespace widget::authoring;
    reactive::Graph graph;

    // Layout factories
    {
        auto r = row().build();
        static_assert(std::same_as<decltype(r), std::shared_ptr<widget::Row>>);
        auto c = column().build();
        static_assert(std::same_as<decltype(c), std::shared_ptr<widget::Column>>);
        auto p = padding(foundation::NanInsets::all(8.0F)).build();
        static_assert(std::same_as<decltype(p), std::shared_ptr<widget::Padding>>);
        auto ct = center().build();
        static_assert(std::same_as<decltype(ct), std::shared_ptr<widget::Center>>);
        auto ex = expanded(2).build();
        static_assert(std::same_as<decltype(ex), std::shared_ptr<widget::Expanded>>);
        REQUIRE(ex->layout_flex_factor() == 2.0F);
        auto fi = flex_item(scene::LayoutFlexPolicy {.grow = 1.0F}).build();
        static_assert(std::same_as<decltype(fi), std::shared_ptr<widget::FlexItem>>);
        auto sv = scroll_view(widget::ScrollAxis::horizontal).build();
        static_assert(std::same_as<decltype(sv), std::shared_ptr<widget::ScrollView>>);
        auto fl = flex(widget::LayoutAxis::vertical).build();
        static_assert(std::same_as<decltype(fl), std::shared_ptr<widget::Flex>>);
    }

    // Control factories
    {
        auto lb = make<widget::Label>(graph, "Hello").build();
        static_assert(std::same_as<decltype(lb), std::shared_ptr<widget::Label>>);
        REQUIRE(lb->text() == "Hello");

        auto bt = make<widget::Button>("Click").build();
        static_assert(std::same_as<decltype(bt), std::shared_ptr<widget::Button>>);
        REQUIRE(bt->text() == "Click");

        auto tf = make<widget::TextField>("val", "placeholder").build();
        static_assert(std::same_as<decltype(tf), std::shared_ptr<widget::TextField>>);
        REQUIRE(tf->value() == "val");
    }
}

TEST_CASE("authoring sizing uses the shared control constraints", "[authoring][layout][sizing]") {
    using namespace widget::authoring;

    auto control = make<scene::NanControl>(foundation::NanSize(40.0F, 20.0F))
                       .width(percent(50.0F))
                       .min_width(80.0F)
                       .max_width(180.0F)
                       .aspect_ratio(2.0F)
                       .build();

    const auto measured = control->measure_layout(scene::LayoutConstraints {
        .max_width = 400.0F,
        .max_height = 200.0F,
    });
    REQUIRE(measured.get_width() == Catch::Approx(180.0F));
    REQUIRE(measured.get_height() == Catch::Approx(90.0F));
}

TEST_CASE("authoring exposes percentage font size and limits", "[authoring][layout][sizing]") {
    using namespace widget::authoring;

    auto button = make<widget::Button>("DSL")
                      .width(percent(50.0F))
                      .height(percent(50.0F))
                      .max_width(percent(40.0F))
                      .font_size(percent(45.0F))
                      .build();
    const auto size = button->measure_layout(scene::LayoutConstraints {
        .max_width = 400.0F,
        .max_height = 200.0F,
    });
    REQUIRE(size.get_width() == Catch::Approx(160.0F));
    REQUIRE(size.get_height() == Catch::Approx(100.0F));
    REQUIRE(button->text_node().font_size() == Catch::Approx(45.0F));

    auto fixed = make<widget::Button>("Fixed").font_size(22.0F).build();
    (void)fixed->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(fixed->text_node().font_size() == Catch::Approx(22.0F));
}

TEST_CASE("BuildContext carries page services into context factories", "[authoring][context]") {
    reactive::Graph graph;
    reactive::ReactiveScope page_scope {graph};
    reactive::ReactiveScope item_scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, page_scope, themes};

    REQUIRE(&ui.graph() == &graph);
    REQUIRE(&ui.scope() == &page_scope);
    REQUIRE(&ui.theme_manager() == &themes);

    auto item_ui = ui.with_scope(item_scope);
    REQUIRE(&item_ui.graph() == &graph);
    REQUIRE(&item_ui.scope() == &item_scope);
    REQUIRE(&item_ui.theme_manager() == &themes);

    auto current = theme::default_theme();
    current.palette.primary = theme::nan_color(0.75F, 0.12F, 140.0F);
    themes.set_theme(current);
    auto action = ui.make<widget::Button>("Run").build();
    auto title = item_ui.make<widget::Label>("Scoped").build();

    REQUIRE(action->text() == "Run");
    REQUIRE(title->text() == "Scoped");
    REQUIRE(action->theme_ref().palette.primary.oklch().light == Catch::Approx(0.75F));
}

TEST_CASE("BuildContext owns custom component subscriptions", "[authoring][context][lifecycle]") {
    reactive::Graph graph;
    reactive::ReactiveScope page_scope {graph};
    reactive::Event<int> event;
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, page_scope, themes};
    int observed = 0;

    auto component = ui.make<ScopedComponent>(event, observed).build();
    REQUIRE_FALSE(component->weak_from_this().expired());
    REQUIRE(event.subscriber_count() == 1);
    event.emit(2);
    REQUIRE(observed == 2);

    component.reset();
    REQUIRE(event.subscriber_count() == 0);
    event.emit(3);
    REQUIRE(observed == 2);
}

TEST_CASE("BuildContext make uses typed built-in component traits", "[authoring][traits]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    reactive::Signal<std::string> text {graph, "Ready"};

    auto label = ui.make<widget::Label>(text).build();
    auto button = ui.make<widget::Button>(text).build();
    REQUIRE(label->text() == "Ready");
    REQUIRE(button->text() == "Ready");

    text.set("Running");
    REQUIRE(label->text() == "Running");
    REQUIRE(button->text() == "Running");
}

TEST_CASE("BuildContext callbacks expire with their owning scope", "[authoring][lifecycle]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    int calls = 0;

    auto first = ui.make<widget::Button>("First").on_click([&calls] { ++calls; }).build();
    scene::KeyEvent activate_first {257, scene::KeyEvent::Action::press};
    REQUIRE(first->on_input(activate_first));
    REQUIRE(calls == 1);

    scope.clear();
    scene::KeyEvent activate_expired {257, scene::KeyEvent::Action::press};
    REQUIRE(first->on_input(activate_expired));
    REQUIRE(calls == 1);

    auto second = ui.make<widget::Button>("Second").on_click([&calls] { ++calls; }).build();
    scene::KeyEvent activate_second {257, scene::KeyEvent::Action::press};
    REQUIRE(second->on_input(activate_second));
    REQUIRE(calls == 2);
}

TEST_CASE("boolean component traits preserve two-way signal bindings", "[authoring][traits]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    reactive::Signal<bool> checked {graph, false};

    auto checkbox = ui.make<widget::Checkbox>(checked, "Checkbox").build();
    auto switch_control = ui.make<widget::Switch>(checked, "Switch").build();
    checked.set(true);
    REQUIRE(checkbox->checked());
    REQUIRE(switch_control->checked());

    checkbox->toggle();
    REQUIRE_FALSE(checked.peek());
    REQUIRE_FALSE(switch_control->checked());
}

TEST_CASE(
    "BuildContext binds tracked values through ordinary widget setters",
    "[authoring][binding]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    reactive::Signal<std::string> text {graph, "Ready"};

    auto label = ui.make<widget::Label>(text).build();
    auto button = ui.make<widget::Button>(text).build();
    REQUIRE(label->text() == "Ready");
    REQUIRE(button->text() == "Ready");

    text.set("Running");
    REQUIRE(label->text() == "Running");
    REQUIRE(button->text() == "Running");

    label.reset();
    text.set("Done");
    REQUIRE(button->text() == "Done");
}

TEST_CASE(
    "typed visual properties compose shared parts with component capabilities",
    "[authoring][binding][visual-property]"
) {
    using LabelColor = decltype(widget::visual::label.color);
    using LabelFontSize = decltype(widget::visual::label.font_size);
    using ContainerRadius = decltype(widget::visual::container.radius);

    static_assert(widget::property::Writable<widget::Label, LabelColor>);
    static_assert(widget::property::Writable<widget::Label, LabelFontSize>);
    static_assert(widget::property::Writable<widget::Button, LabelColor>);
    static_assert(widget::property::Writable<widget::Button, ContainerRadius>);
    static_assert(!widget::property::Writable<widget::Label, ContainerRadius>);

    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    const auto initial = foundation::NanColor::from_oklch(0.62F, 0.12F, 255.0F);
    const auto changed = foundation::NanColor::from_oklch(0.72F, 0.18F, 35.0F);
    reactive::Signal<foundation::NanColor> color {graph, initial};
    reactive::Signal<float> radius {graph, 6.0F};

    auto label = ui.make<widget::Label>("Motion label")
                     .set(widget::visual::label.font_size, 22.0F)
                     .build();
    auto button = ui.make<widget::Button>("Motion button").build();
    ui.bind(label, widget::visual::label.color, color);
    ui.bind(button, widget::visual::label.color, color);
    ui.bind(button, widget::visual::container.radius, radius);

    REQUIRE(label->font_size() == Catch::Approx(22.0F));
    REQUIRE(label->color().approx_equals(initial));
    REQUIRE(button->resolved_style().label.color.approx_equals(initial));
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(6.0F));

    color.set(changed);
    radius.set(24.0F);

    REQUIRE(label->color().approx_equals(changed));
    REQUIRE(button->resolved_style().label.color.approx_equals(changed));
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(24.0F));
}

TEST_CASE(
    "BuildContext text fields synchronize writable string signals",
    "[authoring][binding]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    reactive::Signal<std::string> draft {graph, "Task"};
    auto field = ui.make<widget::TextField>(draft, "Add a task").build();

    REQUIRE(field->value() == "Task");
    draft.set("Updated");
    REQUIRE(field->value() == "Updated");

    scene::NanSceneTree tree;
    tree.set_root(field);
    tree.set_focus(field.get());
    field->editable_text().set_caret(field->value().size());
    tree.dispatch_text_input(scene::TextInputEvent("!"));
    REQUIRE(draft.peek() == "Updated!");
}

TEST_CASE("free function DSL trees match imperative construction", "[authoring][factory][layout]") {
    using namespace widget::authoring;
    reactive::Graph graph;

    // Imperative
    auto imp_button = widget::Button::create("Save");
    imp_button->set_tone(theme::ButtonTone::primary);
    auto imp = widget::Column::create();
    imp->set_gap(4.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
    imp->add(imp_button);

    // DSL with free functions
    std::shared_ptr<widget::Button> dsl_button;
    auto dsl = column()
                   .configure([](widget::Column& c) {
                       c.set_gap(4.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
                   })
                   .children(make<widget::Button>("Save")
                                 .configure([](widget::Button& b) {
                                     b.set_tone(theme::ButtonTone::primary);
                                 })
                                 .expose(dsl_button))
                   .build();

    scene::NanSceneTree imp_tree;
    imp_tree.set_root(imp);
    scene::NanSceneTree dsl_tree;
    dsl_tree.set_root(dsl);
    REQUIRE(imp_tree.layout_root(foundation::NanSize(200.0F, 80.0F)) == 1);
    REQUIRE(dsl_tree.layout_root(foundation::NanSize(200.0F, 80.0F)) == 1);

    REQUIRE(dsl_button->tone() == imp_button->tone());
    REQUIRE(dsl_button->global_bounds() == imp_button->global_bounds());
    REQUIRE(dsl->child_count() == imp->child_count());
    REQUIRE(dsl->gap() == Catch::Approx(imp->gap()));
}

TEST_CASE("NodeBuilder forwards common widget modifiers", "[authoring][modifier]") {
    reactive::Graph graph;
    int clicks = 0;
    auto button = widget::authoring::make<widget::Button>("Save")
                      .tone(theme::ButtonTone::secondary)
                      .treatment(theme::ButtonTreatment::outlined)
                      .on_click([&] { ++clicks; })
                      .build();
    auto label = widget::authoring::make<widget::Label>(graph, "Heading")
                     .font_size(22.0F)
                     .color_token(theme::ColorToken::on_surface_variant)
                     .build();
    auto field = widget::authoring::make<widget::TextField>("", "Task").autofocus().build();
    auto column = widget::authoring::column()
                      .gap(9.0F)
                      .cross_alignment(widget::LayoutAlignment::stretch)
                      .children(label, field, button)
                      .build();

    REQUIRE(column->gap() == Catch::Approx(9.0F));
    REQUIRE(column->cross_alignment() == widget::LayoutAlignment::stretch);
    REQUIRE(label->font_size() == Catch::Approx(22.0F));
    REQUIRE(label->color_token() == theme::ColorToken::on_surface_variant);
    REQUIRE(button->tone() == theme::ButtonTone::secondary);
    REQUIRE(button->treatment() == theme::ButtonTreatment::outlined);

    scene::NanSceneTree tree;
    tree.set_root(column);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);
    REQUIRE(tree.focused_node() == field.get());
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        button->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(clicks == 1);
}

TEST_CASE("free function factories compose nested layout trees", "[authoring][factory][layout]") {
    using namespace widget::authoring;
    reactive::Graph graph;

    std::shared_ptr<widget::Label> title_label;
    std::shared_ptr<widget::Button> action_btn;
    auto root =
        padding(foundation::NanInsets::all(12.0F))
            .child(
                column()
                    .configure([](widget::Column& c) {
                        c.set_gap(8.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
                    })
                    .children(
                        make<widget::Label>(graph, "Settings").expose(title_label),
                        row()
                            .configure([](widget::Row& r) {
                                r.set_gap(6.0F).set_cross_alignment(
                                    widget::LayoutAlignment::center
                                );
                            })
                            .children(
                                expanded().child(make<widget::Label>(graph, "Status")),
                                make<widget::Button>("Apply").expose(action_btn)
                            )
                    )
            )
            .build();

    REQUIRE(root->child_count() == 1);
    REQUIRE(root->get_child(0) != nullptr);
    auto* col = dynamic_cast<widget::Column*>(root->get_child(0));
    REQUIRE(col != nullptr);
    REQUIRE(col->child_count() == 2);
    REQUIRE(col->get_child(0) == title_label.get());
    REQUIRE(title_label->text() == "Settings");

    auto* row = dynamic_cast<widget::Row*>(col->get_child(1));
    REQUIRE(row != nullptr);
    REQUIRE(row->child_count() == 2);
    REQUIRE(row->get_child(1) == action_btn.get());
    REQUIRE(action_btn->text() == "Apply");
}

TEST_CASE("free function factories support expose and configure", "[authoring][factory]") {
    using namespace widget::authoring;
    reactive::Graph graph;

    std::shared_ptr<widget::Label> exposed;
    auto root = column()
                    .children(
                        make<widget::Label>(graph, "First"),
                        make<widget::Label>(graph, "Second")
                            .configure([](widget::Label& l) { l.set_font_size(18.0F); })
                            .expose(exposed)
                    )
                    .build();

    REQUIRE(root->child_count() == 2);
    REQUIRE(exposed != nullptr);
    REQUIRE(exposed->text() == "Second");
    REQUIRE(exposed->font_size() == Catch::Approx(18.0F));
    REQUIRE(root->get_child(1) == exposed.get());
}

TEST_CASE("grid factory produces correct concrete type", "[authoring][factory][grid]") {
    using namespace widget::authoring;
    auto g = grid(3).build();
    static_assert(std::same_as<decltype(g), std::shared_ptr<widget::Grid>>);
    REQUIRE(g->columns() == 3);
    REQUIRE(g->column_gap() == Catch::Approx(0.0F));
    REQUIRE(g->row_gap() == Catch::Approx(0.0F));
}

TEST_CASE("grid layout arranges children in rows and columns", "[authoring][factory][grid]") {
    using namespace widget::authoring;
    reactive::Graph graph;

    std::shared_ptr<widget::Label> a;
    std::shared_ptr<widget::Label> b;
    std::shared_ptr<widget::Label> c;
    std::shared_ptr<widget::Label> d;
    auto g = grid(2)
                 .configure([](widget::Grid& gr) { gr.set_column_gap(8.0F).set_row_gap(4.0F); })
                 .children(
                     make<widget::Label>(graph, "A").expose(a),
                     make<widget::Label>(graph, "B").expose(b),
                     make<widget::Label>(graph, "C").expose(c),
                     make<widget::Label>(graph, "D").expose(d)
                 )
                 .build();

    REQUIRE(g->columns() == 2);
    REQUIRE(g->column_gap() == Catch::Approx(8.0F));
    REQUIRE(g->row_gap() == Catch::Approx(4.0F));
    REQUIRE(g->child_count() == 4);

    scene::NanSceneTree tree;
    tree.set_root(g);
    REQUIRE(tree.layout_root(foundation::NanSize(400.0F, 200.0F)) == 1);

    // A (row 0, col 0) should be above C (row 1, col 0)
    REQUIRE(a->global_bounds().get_y() < c->global_bounds().get_y());
    // B (row 0, col 1) should be to the right of A
    REQUIRE(b->global_bounds().get_x() == Catch::Approx(a->global_bounds().get_right() + 8.0F));
    // C and D should be in row 1 (below A and B)
    REQUIRE(c->global_bounds().get_y() > b->global_bounds().get_bottom());
}

TEST_CASE("grid with single column behaves like a column", "[authoring][factory][grid]") {
    using namespace widget::authoring;
    reactive::Graph graph;

    auto g = grid(1)
                 .configure([](widget::Grid& gr) {
                     gr.set_row_gap(5.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
                 })
                 .children(
                     make<widget::Label>(graph, "One"),
                     make<widget::Label>(graph, "Two"),
                     make<widget::Label>(graph, "Three")
                 )
                 .build();

    scene::NanSceneTree tree;
    tree.set_root(g);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 300.0F)) == 1);

    REQUIRE(g->child_count() == 3);
    auto* first = g->get_child(0)->as_control();
    auto* last = g->get_child(2)->as_control();
    REQUIRE(first != nullptr);
    REQUIRE(last != nullptr);
    // Stacked vertically
    REQUIRE(first->global_bounds().get_y() < last->global_bounds().get_y());
    // All same x position (left-aligned)
    REQUIRE(first->global_bounds().get_x() == Catch::Approx(last->global_bounds().get_x()));
}

TEST_CASE("grid with 0 children produces empty measured size", "[authoring][factory][grid]") {
    using namespace widget::authoring;
    auto g = grid(3).configure(
                        [](widget::Grid& gr) { gr.set_column_gap(10.0F).set_row_gap(6.0F); }
    ).build();

    scene::NanSceneTree tree;
    tree.set_root(g);
    // Layout succeeds with zero children (no crash).
    REQUIRE(tree.layout_root(foundation::NanSize(400.0F, 200.0F)) == 1);
    REQUIRE(g->child_count() == 0);
}

TEST_CASE("grid with cross_alignment lays out children correctly", "[authoring][factory][grid]") {
    using namespace widget::authoring;
    reactive::Graph graph;

    std::shared_ptr<widget::Label> left_label;
    std::shared_ptr<widget::Label> right_label;
    auto g = grid(2)
                 .configure([](widget::Grid& gr) {
                     gr.set_row_gap(4.0F).set_cross_alignment(widget::LayoutAlignment::center);
                 })
                 .children(
                     make<widget::Label>(graph, "Short").expose(left_label),
                     make<widget::Label>(graph, "Longer").expose(right_label)
                 )
                 .build();

    scene::NanSceneTree tree;
    tree.set_root(g);
    REQUIRE(tree.layout_root(foundation::NanSize(400.0F, 200.0F)) == 1);

    // Both children exist and are in the same row.
    REQUIRE(left_label != nullptr);
    REQUIRE(right_label != nullptr);
    REQUIRE(
        left_label->global_bounds().get_y() == Catch::Approx(right_label->global_bounds().get_y())
    );
}
