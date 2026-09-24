//
// Combobox tests: filterable text input + Popover-backed option list over MenuItem.
//

#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/semantics/semantics.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/builtin_component_traits.hpp>
#include <nandina/widget/combobox.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rounded_rects = 0;
        int text_calls = 0;
        foundation::NanColor last_fill {};

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor& color) override {
            last_fill = color;
        }
        void
        draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {}
        void draw_rounded_rect(
            const foundation::NanRect&,
            float,
            const foundation::NanColor& color
        ) override {
            ++rounded_rects;
            last_fill = color;
        }
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
        ) override {
            ++text_calls;
        }
    };

    /// 根控件：记录冒泡到根的 Escape，用来验证「关闭状态下 Escape 继续冒泡」。
    class RecordingRoot final: public scene::NanControl {
    public:
        explicit RecordingRoot(const foundation::NanSize size): scene::NanControl(size) {}

        int escapes = 0;

        auto on_input(scene::InputEvent& event) -> bool override {
            if (event.type() == scene::EventType::key
                && static_cast<scene::KeyEvent&>(event).keycode() == 256)
            {
                ++escapes;
                event.accept();
                return true;
            }
            return false;
        }
    };

    [[nodiscard]] auto item(
        std::string id,
        std::string label,
        const widget::MenuItemKind kind = widget::MenuItemKind::action,
        const bool disabled = false
    ) -> widget::MenuItem {
        widget::MenuItem result;
        result.id = std::move(id);
        result.label = std::move(label);
        result.kind = kind;
        result.disabled = disabled;
        return result;
    }

    /// 覆盖 action / 结构性条目 / 禁用条目 / 大小写差异的一层条目。
    [[nodiscard]] auto sample_items() -> std::vector<widget::MenuItem> {
        return {
            item("apple", "Apple"),
            item("sep", "", widget::MenuItemKind::separator),
            item("banana", "Banana"),
            item("group", "Fruits", widget::MenuItemKind::label),
            item("cherry", "Cherry", widget::MenuItemKind::action, /*disabled=*/true),
            item("apricot", "apricot"),
        };
    }

    struct ComboboxHarness {
        reactive::Graph graph;
        theme::ThemeManager themes;
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<RecordingRoot> body =
            std::make_shared<RecordingRoot>(foundation::NanSize(340.0F, 260.0F));
        std::shared_ptr<widget::Combobox> combo;
        scene::NanSceneTree tree;
        foundation::NanSize viewport {340.0F, 260.0F};

        explicit ComboboxHarness(
            std::vector<widget::MenuItem> items = sample_items(),
            std::string value = {}
        ):
            combo(widget::Combobox::create(std::move(items), std::move(value), "Search")) {
            tree.set_theme_manager(themes);
            combo->set_position(foundation::NanPoint(10.0F, 20.0F));
            body->add_child(combo);
            // 第二个子节点阻止单子拉伸路径把 combobox 撑满内容层。
            body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
            host->set_content(body);
            tree.set_root(host);
        }

        void layout() {
            (void)tree.layout_root(viewport);
        }

        /// 把焦点放到内部输入框（不使用鼠标，避免留下指针捕获影响 hover 用例）。
        void focus_field() {
            layout();
            (void)tree.focus_first_within(*combo);
        }

        /// 浮层面板：DismissLayer → FocusScope → PopoverSurface。
        [[nodiscard]] auto panel(const std::size_t overlay_index = 0) const -> scene::NanControl* {
            auto* layer = host->layer_at(1);
            auto* overlay_root = layer != nullptr ? layer->layout_root() : nullptr;
            if (overlay_root == nullptr || overlay_root->child_count() <= overlay_index) {
                return nullptr;
            }
            auto* dismiss = overlay_root->get_child(overlay_index)->as_control();
            auto* scope = dismiss != nullptr && dismiss->child_count() > 0
                ? dismiss->get_child(0)->as_control()
                : nullptr;
            return scope != nullptr && scope->child_count() > 0 ? scope->get_child(0)->as_control()
                                                               : nullptr;
        }

        /// 选项列表容器 = Popover 的 content。
        [[nodiscard]] auto surface(const std::size_t overlay_index = 0) const
            -> scene::NanControl* {
            auto* popover_surface = panel(overlay_index);
            return popover_surface != nullptr && popover_surface->child_count() > 0
                ? popover_surface->get_child(0)->as_control()
                : nullptr;
        }

        /// 聚焦输入框并打开浮层，返回列表表面。
        auto open_and_layout() -> scene::NanControl* {
            focus_field();
            combo->open();
            layout();
            return surface();
        }
    };

    void collect_semantics(const semantics::Node& node, std::vector<const semantics::Node*>& out) {
        out.push_back(&node);
        for (const auto& child: node.children) {
            collect_semantics(child, out);
        }
    }

    [[nodiscard]] auto semantics_nodes(const semantics::Tree& tree)
        -> std::vector<const semantics::Node*> {
        std::vector<const semantics::Node*> out;
        for (const auto& root: tree.roots) {
            collect_semantics(root, out);
        }
        return out;
    }

    [[nodiscard]] auto find_semantics_role(
        const semantics::Tree& tree,
        const semantics::Role role,
        const std::string_view label = {}
    ) -> const semantics::Node* {
        for (const auto* node: semantics_nodes(tree)) {
            if (node->properties.role == role && (label.empty() || node->properties.label == label))
            {
                return node;
            }
        }
        return nullptr;
    }
} // namespace

TEST_CASE("combobox is closed and measured by default", "[combobox]") {
    widget::Combobox empty;
    REQUIRE_FALSE(empty.is_open());
    REQUIRE(empty.item_count() == 0);
    REQUIRE(empty.filtered_ids().empty());
    REQUIRE(empty.text().empty());
    REQUIRE(empty.selected_id().empty());
    REQUIRE(empty.active_index() == -1);
    REQUIRE(empty.z_index_hint() == 0);
    REQUIRE_FALSE(empty.disabled());
    REQUIRE_FALSE(empty.allow_custom_value());

    const auto measured = empty.measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(measured.get_width() == Catch::Approx(200.0F));
    REQUIRE(measured.get_height() == Catch::Approx(36.0F));

    const auto populated = widget::Combobox::create(sample_items());
    REQUIRE(populated->item_count() == 6);
    // 结构性条目（separator / label）不进列表，禁用条目仍然列出。
    REQUIRE(
        populated->filtered_ids()
        == std::vector<std::string> {"apple", "banana", "cherry", "apricot"}
    );
}

TEST_CASE("combobox typing filters case-insensitively on label", "[combobox][filter]") {
    ComboboxHarness harness;
    REQUIRE(harness.open_and_layout() != nullptr);
    REQUIRE(harness.combo->is_open());
    REQUIRE(
        harness.combo->filtered_ids()
        == std::vector<std::string> {"apple", "banana", "cherry", "apricot"}
    );

    // 大写查询命中大小写不同的两个标签。
    harness.tree.dispatch_text_input(scene::TextInputEvent("AP"));
    REQUIRE(harness.combo->text() == "AP");
    REQUIRE(harness.combo->filtered_ids() == std::vector<std::string> {"apple", "apricot"});

    // 过滤只作用于列表视图，绝不改写存储的条目。
    REQUIRE(harness.combo->item_count() == 6);
    REQUIRE(harness.combo->items()[0].id == "apple");
    REQUIRE(harness.combo->items()[0].label == "Apple");

    // 子串匹配（不是前缀匹配）：全选后替换查询。
    harness.tree.dispatch_key(
        scene::KeyEvent(65, scene::KeyEvent::Action::press, scene::KeyModifiers {.ctrl = true})
    );
    harness.tree.dispatch_text_input(scene::TextInputEvent("nan"));
    REQUIRE(harness.combo->text() == "nan");
    REQUIRE(harness.combo->filtered_ids() == std::vector<std::string> {"banana"});

    // 空查询回到全部可聚焦条目。
    harness.combo->set_text("");
    REQUIRE(harness.combo->text().empty());
    REQUIRE(
        harness.combo->filtered_ids()
        == std::vector<std::string> {"apple", "banana", "cherry", "apricot"}
    );

    // 无匹配时列表为空，高亮为 -1。
    harness.combo->set_text("zzz");
    REQUIRE(harness.combo->filtered_ids().empty());
    REQUIRE(harness.combo->active_index() == -1);
}

TEST_CASE("combobox arrows walk the filtered list", "[combobox][keyboard]") {
    ComboboxHarness harness;
    auto* surface = harness.open_and_layout();
    REQUIRE(surface != nullptr);
    REQUIRE(harness.combo->active_index() == 0);

    // Down 跳过被排除的 separator / label：下标只属于过滤后的列表。
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press));
    REQUIRE(harness.combo->active_index() == 1);
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press));
    REQUIRE(harness.combo->active_index() == 2); // Cherry：禁用但仍可聚焦

    // 过滤后高亮重新落在首个匹配项。
    harness.combo->set_text("ap");
    REQUIRE(harness.combo->filtered_ids() == std::vector<std::string> {"apple", "apricot"});
    REQUIRE(harness.combo->active_index() == 0);
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press));
    REQUIRE(harness.combo->active_index() == 1);
    harness.tree.dispatch_key(scene::KeyEvent(265, scene::KeyEvent::Action::press));
    REQUIRE(harness.combo->active_index() == 0);

    // Home / End / PageUp / PageDown 由共享漫游设施回答。
    harness.combo->set_text("");
    harness.tree.dispatch_key(scene::KeyEvent(269, scene::KeyEvent::Action::press)); // end
    REQUIRE(harness.combo->active_index() == 3);
    harness.tree.dispatch_key(scene::KeyEvent(268, scene::KeyEvent::Action::press)); // home
    REQUIRE(harness.combo->active_index() == 0);
    harness.tree.dispatch_key(scene::KeyEvent(267, scene::KeyEvent::Action::press)); // page down
    REQUIRE(harness.combo->active_index() == 3);
    harness.tree.dispatch_key(scene::KeyEvent(266, scene::KeyEvent::Action::press)); // page up
    REQUIRE(harness.combo->active_index() == 0);
}

TEST_CASE("combobox arrow opens the list when closed", "[combobox][keyboard]") {
    ComboboxHarness harness;
    harness.focus_field();
    REQUIRE_FALSE(harness.combo->is_open());

    // 关闭时 Down：打开浮层并高亮**第一个**匹配项。这一次按键只负责落位，
    // 不再叠加一次移动（否则会落到第 2 项，与平台惯例不符）。
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press));
    REQUIRE(harness.combo->is_open());
    REQUIRE(harness.combo->active_index() == 0);
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 1);

    // 打开状态下 Down 继续移动高亮。
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press));
    REQUIRE(harness.combo->active_index() == 1);

    // 关闭时 Up：打开并高亮**最后一个**匹配项。
    harness.combo->close();
    REQUIRE_FALSE(harness.combo->is_open());
    harness.tree.dispatch_key(scene::KeyEvent(265, scene::KeyEvent::Action::press));
    REQUIRE(harness.combo->is_open());
    REQUIRE(harness.combo->active_index() == 3);

    // Alt+Down / Alt+Up 是显式开合，不移动高亮。
    harness.combo->close();
    REQUIRE_FALSE(harness.combo->is_open());
    harness.tree.dispatch_key(
        scene::KeyEvent(264, scene::KeyEvent::Action::press, scene::KeyModifiers {.alt = true})
    );
    REQUIRE(harness.combo->is_open());
    REQUIRE(harness.combo->active_index() == 0);
    harness.tree.dispatch_key(
        scene::KeyEvent(265, scene::KeyEvent::Action::press, scene::KeyModifiers {.alt = true})
    );
    REQUIRE_FALSE(harness.combo->is_open());
}

TEST_CASE("combobox enter selects the highlighted item", "[combobox][keyboard][callback]") {
    ComboboxHarness harness;
    REQUIRE(harness.open_and_layout() != nullptr);

    std::vector<std::pair<std::string, std::string>> changes;
    harness.combo->set_on_change(
        [&changes](const std::string_view text, const std::string_view id) {
            changes.emplace_back(std::string(text), std::string(id));
        }
    );
    std::vector<std::string> selected;
    auto subscription = harness.combo->item_selected().subscribe(
        [&selected](const std::string& id) { selected.push_back(id); }
    );

    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press)); // down → 1
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    REQUIRE(harness.combo->text() == "Banana");
    REQUIRE(harness.combo->selected_id() == "banana");
    REQUIRE(changes == std::vector<std::pair<std::string, std::string>> {{"Banana", "banana"}});
    REQUIRE(selected == std::vector<std::string> {"banana"});
    REQUIRE_FALSE(harness.combo->is_open());
    REQUIRE(harness.host->overlay_count() == 0);
}

TEST_CASE("combobox enter never activates a disabled item", "[combobox][keyboard]") {
    ComboboxHarness harness;
    REQUIRE(harness.open_and_layout() != nullptr);
    int selected = 0;
    auto subscription = harness.combo->item_selected().subscribe([&selected](const std::string&) {
        ++selected;
    });

    harness.combo->set_text("cherry");
    REQUIRE(harness.combo->filtered_ids() == std::vector<std::string> {"cherry"});
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    REQUIRE(selected == 0);
    REQUIRE(harness.combo->selected_id().empty());
    REQUIRE(harness.combo->is_open());
}

TEST_CASE("combobox escape closes and bubbles when closed", "[combobox][keyboard]") {
    ComboboxHarness harness;
    REQUIRE(harness.open_and_layout() != nullptr);
    int closes = 0;
    harness.combo->set_on_close([&closes] { ++closes; });

    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press)); // escape
    REQUIRE_FALSE(harness.combo->is_open());
    REQUIRE(closes == 1);
    REQUIRE(harness.host->overlay_count() == 0);
    REQUIRE(harness.body->escapes == 0);

    // 关闭状态下 Escape 继续冒泡到祖先。
    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    REQUIRE(harness.body->escapes == 1);
}

TEST_CASE("combobox printable text falls through to the field", "[combobox][input]") {
    ComboboxHarness harness;
    harness.focus_field();
    harness.tree.dispatch_text_input(scene::TextInputEvent("App"));
    REQUIRE(harness.combo->text() == "App");
    // 空格字符由 TextInputEvent 送达，不被键码拦截吞掉。
    harness.tree.dispatch_key(scene::KeyEvent(32, scene::KeyEvent::Action::press));
    harness.tree.dispatch_text_input(scene::TextInputEvent(" "));
    REQUIRE(harness.combo->text() == "App ");
    REQUIRE_FALSE(harness.combo->is_open());
}

TEST_CASE("combobox pointer selects and hovers rows", "[combobox][pointer]") {
    ComboboxHarness harness;
    auto* surface = harness.open_and_layout();
    REQUIRE(surface != nullptr);
    REQUIRE(surface->child_count() == 4);

    // 悬停移动高亮。
    auto* third = surface->get_child(2)->as_control();
    harness.tree.dispatch_mouse_move(
        scene::MouseMoveEvent(third->global_bounds().get_center(), foundation::NanPoint(1.0F, 0.0F))
    );
    REQUIRE(harness.combo->active_index() == 2);

    // 点击选中同一路径（与 Enter 一致）。
    auto* second = surface->get_child(1)->as_control();
    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            second->global_bounds().get_center()
        )
    );
    REQUIRE(harness.combo->text() == "Banana");
    REQUIRE(harness.combo->selected_id() == "banana");
    REQUIRE_FALSE(harness.combo->is_open());
    REQUIRE(harness.host->overlay_count() == 0);
}

TEST_CASE("combobox clicking outside closes the popup", "[combobox][pointer]") {
    ComboboxHarness harness;
    REQUIRE(harness.open_and_layout() != nullptr);
    REQUIRE(harness.host->overlay_count() == 1);

    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(harness.viewport.get_width() - 5.0F, harness.viewport.get_height() - 5.0F)
        )
    );
    REQUIRE_FALSE(harness.combo->is_open());
    REQUIRE(harness.host->overlay_count() == 0);
}

TEST_CASE("combobox selected id is programmatic and id based", "[combobox][value]") {
    ComboboxHarness harness;

    REQUIRE(harness.combo->set_selected_id("apple"));
    REQUIRE(harness.combo->text() == "Apple");
    REQUIRE(harness.combo->selected_id() == "apple");

    // 未知 id 与不可激活（disabled）条目都拒绝。
    REQUIRE_FALSE(harness.combo->set_selected_id("nope"));
    REQUIRE_FALSE(harness.combo->set_selected_id("cherry"));
    REQUIRE(harness.combo->text() == "Apple");

    harness.combo->clear_selection();
    REQUIRE(harness.combo->text().empty());
    REQUIRE(harness.combo->selected_id().empty());
}

TEST_CASE("combobox keeps text and selection consistent while typing", "[combobox][value]") {
    ComboboxHarness harness;
    harness.combo->set_selected_id("apple");
    REQUIRE(harness.combo->selected_id() == "apple");

    harness.focus_field();
    // 全选后输入自定义文本：不再是任何条目的标签 -> 选中被清空。
    harness.tree.dispatch_key(
        scene::KeyEvent(65, scene::KeyEvent::Action::press, scene::KeyModifiers {.ctrl = true})
    );
    harness.tree.dispatch_text_input(scene::TextInputEvent("apricot"));
    REQUIRE(harness.combo->text() == "apricot");
    REQUIRE(harness.combo->selected_id() == "apricot"); // 恰好等于某个 label -> 重新选中

    harness.tree.dispatch_text_input(scene::TextInputEvent("!"));
    REQUIRE(harness.combo->text() == "apricot!");
    REQUIRE(harness.combo->selected_id().empty());
}

TEST_CASE("combobox commits custom text only when allowed", "[combobox][value][callback]") {
    SECTION("allow_custom_value lets unmatched text through") {
        ComboboxHarness harness;
        harness.combo->set_allow_custom_value(true);
        REQUIRE(harness.combo->allow_custom_value());

        std::vector<std::pair<std::string, std::string>> changes;
        harness.combo->set_on_change(
            [&changes](const std::string_view text, const std::string_view id) {
                changes.emplace_back(std::string(text), std::string(id));
            }
        );
        REQUIRE(harness.open_and_layout() != nullptr);
        harness.combo->set_text("Zebra"); // 程序化设置是静默的
        REQUIRE(changes.empty());

        harness.combo->close();
        REQUIRE(changes == std::vector<std::pair<std::string, std::string>> {{"Zebra", ""}});
        // 未匹配文本被保留（不强制匹配）。
        REQUIRE(harness.combo->text() == "Zebra");
        REQUIRE(harness.combo->selected_id().empty());
    }

    SECTION("without allow_custom_value close falls back to the committed label") {
        ComboboxHarness harness;
        REQUIRE(harness.combo->set_selected_id("banana"));
        harness.combo->open();
        REQUIRE(harness.combo->is_open());
        harness.combo->set_text("Zebra");
        harness.combo->close();
        REQUIRE(harness.combo->text() == "Banana");
        REQUIRE(harness.combo->selected_id() == "banana");
    }

    SECTION("enter while closed commits typed text only when allowed") {
        ComboboxHarness first;
        first.focus_field();
        first.combo->set_text("Alpha");
        first.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press));
        REQUIRE_FALSE(first.combo->is_open());

        std::vector<std::string> committed;
        first.combo->set_on_change(
            [&committed](const std::string_view text, const std::string_view) {
                committed.emplace_back(text);
            }
        );
        first.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press));
        REQUIRE(committed.empty()); // 默认不允许自由文本

        ComboboxHarness second;
        second.focus_field();
        second.combo->set_allow_custom_value(true);
        std::vector<std::string> custom;
        second.combo->set_on_change(
            [&custom](const std::string_view text, const std::string_view id) {
                custom.emplace_back(text);
                REQUIRE(id.empty());
            }
        );
        second.tree.dispatch_text_input(scene::TextInputEvent("Alpha"));
        second.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press));
        REQUIRE(custom == std::vector<std::string> {"Alpha"});
    }
}

TEST_CASE("disabled combobox never opens and ignores input", "[combobox][disabled]") {
    ComboboxHarness harness;
    harness.combo->set_disabled(true);
    REQUIRE(harness.combo->disabled());

    harness.layout();
    // 内部输入框不可聚焦 -> 整个控件没有焦点落点。
    REQUIRE_FALSE(harness.tree.focus_first_within(*harness.combo));
    harness.combo->open();
    REQUIRE_FALSE(harness.combo->is_open());
    REQUIRE(harness.host->overlay_count() == 0);
    REQUIRE(harness.combo->z_index_hint() == 0);

    harness.tree.dispatch_text_input(scene::TextInputEvent("x"));
    REQUIRE(harness.combo->text().empty());
    // 程序化入口不受 disabled 影响（禁用的是交互，不是 API）。
    REQUIRE(harness.combo->set_selected_id("apple"));
    REQUIRE(harness.combo->text() == "Apple");
}

TEST_CASE("combobox set_items while open re-filters in place", "[combobox][mutation]") {
    ComboboxHarness harness;
    auto* surface = harness.open_and_layout();
    REQUIRE(surface != nullptr);
    const auto before = harness.combo->filtered_ids(); // 取副本，绝不跨 set_items 持引用
    REQUIRE(before.size() == 4);

    harness.combo->set_items({
        item("x", "Xenon"),
        item("sep2", "", widget::MenuItemKind::separator),
        item("y", "Yttrium", widget::MenuItemKind::action, /*disabled=*/true),
    });
    harness.layout();

    REQUIRE(harness.combo->item_count() == 3);
    REQUIRE(harness.combo->filtered_ids() == std::vector<std::string> {"x", "y"});
    REQUIRE(harness.combo->is_open());
    REQUIRE(harness.combo->active_index() == 0);

    surface = harness.surface();
    REQUIRE(surface != nullptr);
    REQUIRE(surface->child_count() == 2);

    // 重建后键盘仍然可用，禁用条目不可激活。
    int selected = 0;
    auto subscription = harness.combo->item_selected().subscribe([&selected](const std::string&) {
        ++selected;
    });
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press)); // down → 1
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    REQUIRE(selected == 0);
    REQUIRE(harness.combo->is_open());
}

TEST_CASE("combobox filters and selects inside an input phase", "[combobox][mutation]") {
    // 真实窗口把输入派发包在 FramePhase::input 里：过滤会重建浮层里的条目节点，
    // 这些增删必须走场景树的延迟变更，不能依赖“测试里恰好是 idle 相位”。
    ComboboxHarness harness;
    REQUIRE(harness.open_and_layout() != nullptr);
    std::vector<std::string> selected;
    auto subscription = harness.combo->item_selected().subscribe([&selected](const std::string& id) {
        selected.push_back(id);
    });

    {
        auto phase = harness.tree.enter_phase(scene::FramePhase::input);
        harness.tree.dispatch_text_input(scene::TextInputEvent("ban"));
        harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    }
    harness.tree.flush_tree_mutations();
    harness.layout();

    REQUIRE(harness.combo->text() == "Banana");
    REQUIRE(harness.combo->selected_id() == "banana");
    REQUIRE(selected == std::vector<std::string> {"banana"});
    REQUIRE_FALSE(harness.combo->is_open());
    REQUIRE(harness.host->overlay_count() == 0);
}

TEST_CASE("combobox handles empty and oversized data", "[combobox][boundary]") {
    ComboboxHarness harness({item("a", "A")});
    REQUIRE(harness.open_and_layout() != nullptr);

    // 无匹配：列表为空、高亮 -1，Enter 不选中也不关闭。
    harness.combo->set_text("no-such-label");
    REQUIRE(harness.combo->filtered_ids().empty());
    REQUIRE(harness.combo->active_index() == -1);
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    REQUIRE(harness.combo->selected_id().empty());
    REQUIRE(harness.combo->is_open());

    // 空条目集：过滤为空、漫游无成员、不崩。
    harness.combo->set_items({});
    harness.layout();
    REQUIRE(harness.combo->item_count() == 0);
    REQUIRE(harness.combo->filtered_ids().empty());
    REQUIRE(harness.combo->active_index() == -1);
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press)); // down
    REQUIRE(harness.combo->active_index() == -1);
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    REQUIRE(harness.combo->is_open());

    // 超长标签与超长查询也不改变行为。
    const std::string long_label(4096, 'x');
    harness.combo->set_items({item("long", long_label)});
    harness.combo->set_text("XXXX");
    REQUIRE(harness.combo->filtered_ids() == std::vector<std::string> {"long"});
    REQUIRE(harness.combo->set_selected_id("long"));
    REQUIRE(harness.combo->text() == long_label);
    REQUIRE_FALSE(harness.combo->set_selected_id(""));
}

TEST_CASE("combobox works without an overlay host", "[combobox][fallback]") {
    auto combo = widget::Combobox::create(sample_items());
    std::vector<std::string> selected;
    auto subscription = combo->item_selected().subscribe([&selected](const std::string& id) {
        selected.push_back(id);
    });

    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(320.0F, 240.0F));
    root->add_child(combo);
    root->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    scene::NanSceneTree tree;
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 240.0F)) >= 1);
    REQUIRE(tree.focus_first_within(*combo));

    combo->open();
    root->mark_layout_dirty();
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 240.0F)) >= 1);
    REQUIRE(combo->is_open());
    REQUIRE(combo->z_index_hint() == 1); // 树内回退靠 z 序压过后续兄弟
    REQUIRE(combo->is_visible_in_tree());

    // 过滤与选择在回退模式下同样工作。
    tree.dispatch_text_input(scene::TextInputEvent("ban"));
    REQUIRE(combo->filtered_ids() == std::vector<std::string> {"banana"});
    REQUIRE(combo->active_index() == 0);
    tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    REQUIRE(combo->text() == "Banana");
    REQUIRE(selected == std::vector<std::string> {"banana"});
    REQUIRE_FALSE(combo->is_open());
}

TEST_CASE("combobox exposes combobox and option semantics", "[combobox][semantics]") {
    ComboboxHarness harness;
    harness.focus_field();
    (void)harness.tree.update_semantics();

    const auto* node = harness.tree.semantics_tree().find(harness.combo->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::combobox);
    REQUIRE(node->properties.label == "Search");
    REQUIRE(node->properties.value.empty());
    REQUIRE(node->properties.state.checked.has_value());
    REQUIRE_FALSE(*node->properties.state.checked); // collapsed
    REQUIRE(semantics::supports(node->properties.actions, semantics::Action::activate));

    harness.combo->open();
    harness.layout();
    (void)harness.tree.update_semantics();

    node = harness.tree.semantics_tree().find(harness.combo->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.state.checked.has_value());
    REQUIRE(*node->properties.state.checked); // expanded

    const auto& tree = harness.tree.semantics_tree();
    const auto* list = find_semantics_role(tree, semantics::Role::list, "options");
    REQUIRE(list != nullptr);

    const auto* action = find_semantics_role(tree, semantics::Role::list_item, "Banana");
    REQUIRE(action != nullptr);
    REQUIRE_FALSE(action->properties.state.disabled);
    REQUIRE(semantics::supports(action->properties.actions, semantics::Action::activate));

    const auto* disabled = find_semantics_role(tree, semantics::Role::list_item, "Cherry");
    REQUIRE(disabled != nullptr);
    REQUIRE(disabled->properties.state.disabled);
    REQUIRE_FALSE(semantics::supports(disabled->properties.actions, semantics::Action::activate));

    // 结构性条目根本不进列表。
    REQUIRE(find_semantics_role(tree, semantics::Role::list_item, "Fruits") == nullptr);

    // 输入值同步到语义节点。
    harness.combo->set_text("Banana");
    (void)harness.tree.update_semantics();
    node = harness.tree.semantics_tree().find(harness.combo->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.value == "Banana");
}

TEST_CASE("combobox recipe resolves semantic roles", "[combobox][theme]") {
    const auto design = theme::default_design_system();
    const auto normal =
        theme::resolve_combobox(design, theme::ColorAppearance::light, theme::ComboboxVisualState::normal);
    const auto dark =
        theme::resolve_combobox(design, theme::ColorAppearance::dark, theme::ComboboxVisualState::normal);
    const auto focused = theme::resolve_combobox(
        design, theme::ColorAppearance::light, theme::ComboboxVisualState::focused
    );
    const auto disabled = theme::resolve_combobox(
        design, theme::ColorAppearance::light, theme::ComboboxVisualState::disabled
    );

    REQUIRE(
        normal.input.fill.oklch().light == Catch::Approx(design.light.background.oklch().light)
    );
    REQUIRE(normal.input.border.oklch().light == Catch::Approx(design.light.input.oklch().light));
    REQUIRE(
        normal.option.color.oklch().light == Catch::Approx(design.light.foreground.oklch().light)
    );
    REQUIRE(
        normal.placeholder.color.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );
    REQUIRE(normal.hover_fill.oklch().light == Catch::Approx(design.light.muted.oklch().light));
    REQUIRE(normal.focus_fill.oklch().light == Catch::Approx(design.light.accent.oklch().light));
    // 默认值引用语义标量：改 token 会跟着变。
    REQUIRE(normal.metrics.height == Catch::Approx(36.0F));
    REQUIRE(normal.metrics.padding_x == Catch::Approx(design.tokens.spacing.md));
    REQUIRE(normal.metrics.list_padding_x == Catch::Approx(design.tokens.spacing.xs));
    REQUIRE(normal.metrics.preferred_width == Catch::Approx(200.0F));
    // 亮暗文本不同：没有硬编码颜色。
    REQUIRE_FALSE(normal.option.color == dark.option.color);
    REQUIRE_FALSE(normal.input.fill == dark.input.fill);

    // 状态规则：focused 开启焦点环，disabled 弱化外壳与文本。
    REQUIRE(normal.focus.width == Catch::Approx(0.0F));
    REQUIRE(focused.focus.width > 0.0F);
    const float alpha = design.tokens.opacity.disabled;
    REQUIRE(
        disabled.input.fill.alpha() == Catch::Approx(normal.input.fill.alpha() * alpha)
    );
    REQUIRE(
        disabled.value.color.alpha() == Catch::Approx(normal.value.color.alpha() * alpha)
    );
}

TEST_CASE("combobox theme switch and instance override change resolved style", "[combobox][theme]") {
    ComboboxHarness harness;
    harness.layout();
    auto* combo = harness.combo.get();
    const auto& design = harness.themes.design_system();
    // 先取成具体值：themes.apply() 之后旧的 DesignSystem 可能被释放，不能再持有它的引用。
    const float error_light = design.light.error.oklch().light;
    const auto initial = combo->resolved_style();

    combo->set_override(
        theme::ComboboxRecipeRule {
            .input_fill = theme::ThemeColor::token(theme::ColorToken::error),
            .metrics_item_height = theme::ThemeScalar::literal(21.0F),
        }
    );
    const auto overridden = combo->resolved_style();
    REQUIRE(
        overridden.input.fill.oklch().light == Catch::Approx(design.light.error.oklch().light)
    );
    REQUIRE(overridden.metrics.item_height == Catch::Approx(21.0F));
    // 未覆盖的字段保持默认。
    REQUIRE(overridden.metrics.padding_x == Catch::Approx(initial.metrics.padding_x));
    REQUIRE(overridden.option.color.approx_equals(initial.option.color));

    // 主题切换：DesignSystem 的规则层同样作用到解析结果（守住 apply_rule 四步同步）。
    auto next = theme::default_design_system();
    next.components.combobox.rules.push_back(
        theme::ComboboxRecipeRule {
            .hover_fill = theme::ThemeColor::token(theme::ColorToken::warning),
            .metrics_list_padding_y = theme::ThemeScalar::literal(19.0F),
        }
    );
    harness.themes.apply(std::make_shared<const theme::DesignSystem>(std::move(next)));
    const auto themed = combo->resolved_style();
    REQUIRE(
        themed.hover_fill.oklch().light
        == Catch::Approx(harness.themes.design_system().light.warning.oklch().light)
    );
    REQUIRE(themed.metrics.list_padding_y == Catch::Approx(19.0F));
    // 实例覆盖在系统切换后保留并压过规则层。
    REQUIRE(themed.metrics.item_height == Catch::Approx(21.0F));
    REQUIRE(themed.input.fill.oklch().light == Catch::Approx(error_light));

    // 显式 NanTheme 覆盖（不再跟随系统切换）。先清掉实例覆盖，否则它会继续压过主题。
    combo->set_override(theme::ComboboxRecipeRule {});
    auto explicit_theme = theme::default_theme();
    explicit_theme.tokens.spacing.xs = 7.0F;
    combo->set_theme(explicit_theme);
    REQUIRE(combo->theme_ref().tokens.spacing.xs == Catch::Approx(7.0F));
    REQUIRE(combo->resolved_style().metrics.list_padding_x == Catch::Approx(7.0F));
}

TEST_CASE("combobox recipe rule covers every field", "[combobox][theme][override]") {
    ComboboxHarness harness;
    harness.layout();
    const auto& ds = harness.themes.design_system();

    harness.combo->set_override(
        theme::ComboboxRecipeRule {
            .input_fill = theme::ThemeColor::token(theme::ColorToken::error),
            .input_border = theme::ThemeColor::token(theme::ColorToken::warning),
            .input_border_width = theme::ThemeScalar::literal(2.5F),
            .input_radius = theme::ThemeScalar::literal(9.0F),
            .value_color = theme::ThemeColor::token(theme::ColorToken::success),
            .value_font_size = theme::ThemeScalar::literal(11.0F),
            .placeholder_color = theme::ThemeColor::token(theme::ColorToken::info),
            .placeholder_font_size = theme::ThemeScalar::literal(12.0F),
            .selection_color = theme::ThemeColor::token(theme::ColorToken::tertiary),
            .focus_ring_color = theme::ThemeColor::token(theme::ColorToken::primary),
            .focus_ring_width = theme::ThemeScalar::literal(3.0F),
            .option_color = theme::ThemeColor::token(theme::ColorToken::destructive),
            .option_font_size = theme::ThemeScalar::literal(13.0F),
            .disabled_label = theme::ThemeColor::token(theme::ColorToken::ring),
            .hover_fill = theme::ThemeColor::token(theme::ColorToken::primary),
            .focus_fill = theme::ThemeColor::token(theme::ColorToken::tertiary),
            .metrics_height = theme::ThemeScalar::literal(41.0F),
            .metrics_padding_x = theme::ThemeScalar::literal(12.0F),
            .metrics_preferred_width = theme::ThemeScalar::literal(222.0F),
            .metrics_gap = theme::ThemeScalar::literal(6.0F),
            .metrics_item_height = theme::ThemeScalar::literal(23.0F),
            .metrics_list_padding_x = theme::ThemeScalar::literal(14.0F),
            .metrics_list_padding_y = theme::ThemeScalar::literal(15.0F),
            .metrics_item_radius = theme::ThemeScalar::literal(16.0F),
            .metrics_min_width = theme::ThemeScalar::literal(244.0F),
        }
    );

    const auto style = harness.combo->resolved_style();
    REQUIRE(style.input.fill.oklch().light == Catch::Approx(ds.light.error.oklch().light));
    REQUIRE(style.input.border.oklch().light == Catch::Approx(ds.light.warning.oklch().light));
    REQUIRE(style.input.border_width == Catch::Approx(2.5F));
    REQUIRE(style.input.radius == Catch::Approx(9.0F));
    REQUIRE(style.value.color.oklch().light == Catch::Approx(ds.light.success.oklch().light));
    REQUIRE(style.value.font_size == Catch::Approx(11.0F));
    REQUIRE(
        style.placeholder.color.oklch().light == Catch::Approx(ds.light.info.oklch().light)
    );
    REQUIRE(style.placeholder.font_size == Catch::Approx(12.0F));
    REQUIRE(style.selection.oklch().light == Catch::Approx(ds.light.tertiary.oklch().light));
    REQUIRE(style.focus.color.oklch().light == Catch::Approx(ds.light.primary.oklch().light));
    REQUIRE(style.focus.width == Catch::Approx(3.0F));
    REQUIRE(style.option.color.oklch().light == Catch::Approx(ds.light.destructive.oklch().light));
    REQUIRE(style.option.font_size == Catch::Approx(13.0F));
    REQUIRE(style.disabled_label.oklch().light == Catch::Approx(ds.light.ring.oklch().light));
    REQUIRE(style.hover_fill.oklch().light == Catch::Approx(ds.light.primary.oklch().light));
    REQUIRE(style.focus_fill.oklch().light == Catch::Approx(ds.light.tertiary.oklch().light));
    REQUIRE(style.metrics.height == Catch::Approx(41.0F));
    REQUIRE(style.metrics.padding_x == Catch::Approx(12.0F));
    REQUIRE(style.metrics.preferred_width == Catch::Approx(222.0F));
    REQUIRE(style.metrics.gap == Catch::Approx(6.0F));
    REQUIRE(style.metrics.item_height == Catch::Approx(23.0F));
    REQUIRE(style.metrics.list_padding_x == Catch::Approx(14.0F));
    REQUIRE(style.metrics.list_padding_y == Catch::Approx(15.0F));
    REQUIRE(style.metrics.item_radius == Catch::Approx(16.0F));
    REQUIRE(style.metrics.min_width == Catch::Approx(244.0F));
}

TEST_CASE("combobox shell is painted by the composed text field", "[combobox][paint][theme]") {
    ComboboxHarness harness;
    harness.layout();

    RecordingDevice closed;
    harness.tree.draw(closed);
    REQUIRE(closed.rounded_rects >= 1);
    const auto plain_fill = closed.last_fill;

    // 实例覆盖必须落到实际绘制上：ComboboxRecipe 的外壳转成 TextField 的实例覆盖。
    harness.combo->set_override(
        theme::ComboboxRecipeRule {
            .input_fill = theme::ThemeColor::token(theme::ColorToken::error),
            .input_radius = theme::ThemeScalar::literal(0.0F),
        }
    );
    harness.layout();
    RecordingDevice overridden;
    harness.tree.draw(overridden);
    REQUIRE_FALSE(overridden.last_fill.approx_equals(plain_fill));
    REQUIRE(
        overridden.last_fill.oklch().light
        == Catch::Approx(harness.themes.design_system().light.error.oklch().light)
    );
}

TEST_CASE("destroying an open combobox releases its overlay", "[combobox][lifetime]") {
    ComboboxHarness harness;
    REQUIRE(harness.open_and_layout() != nullptr);
    REQUIRE(harness.host->overlay_count() == 1);

    auto combo = harness.combo;
    (void)harness.body->remove_child(*combo);
    combo.reset();
    REQUIRE(harness.host->overlay_count() == 0);
}

TEST_CASE("combobox authoring via ComponentTraits", "[combobox][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    host->set_content(std::make_shared<scene::NanControl>(foundation::NanSize(240.0F, 160.0F)));
    scene::NanSceneTree host_tree;
    host_tree.set_root(host);
    REQUIRE(host_tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);

    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};

    // 默认构造路径（测试门槛里的 ui.make<Combobox>()）。
    auto empty = ui.make<widget::Combobox>().build();
    REQUIRE(empty->item_count() == 0);
    REQUIRE_FALSE(empty->is_open());

    auto combo = ui.make<widget::Combobox>(
        sample_items(), std::string {"Apple"}, std::string {"Pick a fruit"}
    ).build();
    REQUIRE(combo->item_count() == 6);
    REQUIRE(combo->text() == "Apple");
    REQUIRE(combo->selected_id() == "apple");
    REQUIRE(combo->placeholder() == "Pick a fruit");

    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(240.0F, 160.0F));
    root->add_child(combo);
    root->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);
    REQUIRE(tree.focus_first_within(*combo));

    // 服务来自 BuildContext 注入：挂在普通控件根下也能 portal。
    combo->open();
    REQUIRE(host->overlay_count() == 1);
    combo->close();
    REQUIRE(host->overlay_count() == 0);
}
