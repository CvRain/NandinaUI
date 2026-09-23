//
// TextArea multi-line input, wrapping, scrolling, theme, and authoring tests.
//

#include <nandina/reactive/scope.hpp>
#include <nandina/render/draw_context.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/semantics/semantics.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/key_codes.hpp>
#include <nandina/widget/text_area.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace nandina;

namespace
{
    /// 记录文本绘制位置、插入符线段与矩形，用于断言多行落位与滚动偏移。
    class RecordingDevice final: public render::IRenderDevice {
    public:
        struct TextCall {
            std::string text;
            foundation::NanPoint position;
        };

        std::vector<TextCall> texts;
        std::vector<foundation::NanPoint> line_starts;
        std::vector<foundation::NanRect> rects;
        int line_count = 0;
        int outline_count = 0;
        int rounded_count = 0;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect& rect, const foundation::NanColor&) override {
            rects.push_back(rect);
        }
        void draw_rect_outline(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {
            ++outline_count;
        }
        void draw_rounded_rect(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {
            ++rounded_count;
        }
        void draw_rounded_rect_outline(
            const foundation::NanRect&,
            float,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_line(
            const foundation::NanPoint& start,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {
            ++line_count;
            line_starts.push_back(start);
        }
        void draw_circle(
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_text(
            std::string_view text,
            const foundation::NanPoint& position,
            float,
            const foundation::NanColor&
        ) override {
            texts.push_back({.text = std::string(text), .position = position});
        }
    };

    constexpr int key_kp_enter = 335;
} // namespace

TEST_CASE("text area default measure follows the visible row count", "[text-area][measure]") {
    widget::TextArea area;
    REQUIRE(area.value().empty());
    REQUIRE(area.rows() == 3);
    REQUIRE_FALSE(area.read_only());
    REQUIRE_FALSE(area.disabled());

    const auto style = area.resolved_style();
    const auto three_rows = area.measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(
        three_rows.get_height()
        == Catch::Approx(style.metrics.line_height * 3.0F + style.metrics.padding_y * 2.0F)
    );
    // 空内容下宽度只剩水平内边距（与 TextField 一样由约束收紧）。
    REQUIRE(three_rows.get_width() == Catch::Approx(style.metrics.padding_x * 2.0F));

    area.set_rows(5);
    REQUIRE(area.rows() == 5);
    const auto five_rows = area.measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(
        five_rows.get_height()
        == Catch::Approx(style.metrics.line_height * 5.0F + style.metrics.padding_y * 2.0F)
    );
}

TEST_CASE("text area typing inserts and notifies change and value_changed", "[text-area][input]") {
    auto area = widget::TextArea::create();
    std::vector<std::string> changes;
    area->set_on_change([&](std::string_view value) { changes.emplace_back(value); });
    int events = 0;
    auto subscription = area->value_changed().subscribe([&](std::string_view) { ++events; });

    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 120.0F)) >= 1);
    tree.set_focus(area.get());

    tree.dispatch_text_input(scene::TextInputEvent("hello"));
    REQUIRE(area->value() == "hello");
    REQUIRE(area->editable_text().caret() == 5);
    REQUIRE(changes == std::vector<std::string> {"hello"});
    REQUIRE(events == 1);

    // 程序化 set_value 不冒充用户输入，不发出 change。
    area->set_value("reset");
    REQUIRE(area->value() == "reset");
    REQUIRE(changes.size() == 1);
    REQUIRE(events == 1);
}

TEST_CASE("text area Enter inserts a newline instead of submitting", "[text-area][input][enter]") {
    auto area = widget::TextArea::create();
    std::vector<std::string> changes;
    area->set_on_change([&](std::string_view value) { changes.emplace_back(value); });

    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 160.0F)) >= 1);
    tree.set_focus(area.get());

    tree.dispatch_text_input(scene::TextInputEvent("a"));
    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));
    tree.dispatch_key(scene::KeyEvent(key_kp_enter, scene::KeyEvent::Action::press));
    tree.dispatch_text_input(scene::TextInputEvent("b"));

    // 两次 Enter 都插入换行；不存在 TextField 那种 on_submit 提交路径。
    REQUIRE(area->value() == "a\n\nb");
    REQUIRE(area->editable_text().caret() == 4);
    REQUIRE(changes.size() == 4);
    REQUIRE(area->editable_text().text_node().layout_result().lines.size() == 3);

    // Enter 的按下-抬起不产生额外字符。
    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::release));
    REQUIRE(area->value() == "a\n\nb");
}

TEST_CASE("text area renders newline-separated values on multiple lines", "[text-area][layout]") {
    auto area = std::make_shared<widget::TextArea>("line1\nline2\nline3");
    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(220.0F, 200.0F)) >= 1);

    const auto& layout = area->editable_text().text_node().layout_result();
    REQUIRE(layout.lines.size() == 3);
    REQUIRE(layout.lines[0].visible_text == "line1");
    REQUIRE(layout.lines[1].visible_text == "line2");
    REQUIRE(layout.lines[2].visible_text == "line3");

    RecordingDevice device;
    tree.draw(device);
    REQUIRE(device.texts.size() == 3);
    REQUIRE(device.texts[0].text == "line1");
    REQUIRE(device.texts[1].text == "line2");
    REQUIRE(device.texts[2].text == "line3");
    // 行依次向下排布，不重叠。
    REQUIRE(device.texts[1].position.get_y() > device.texts[0].position.get_y());
    REQUIRE(device.texts[2].position.get_y() > device.texts[1].position.get_y());
}

TEST_CASE("text area wraps long lines to the available width", "[text-area][layout][wrap]") {
    auto area = std::make_shared<widget::TextArea>("");
    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 200.0F)) >= 1);

    area->set_value(std::string(60, 'a'));
    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 200.0F)) >= 1);

    const auto& layout = area->editable_text().text_node().layout_result();
    REQUIRE(layout.lines.size() > 1);
    // 软换行不引入换行符，也不丢字符。
    std::string reassembled;
    for (const auto& line: layout.lines) {
        reassembled += line.visible_text;
    }
    REQUIRE(reassembled == std::string(60, 'a'));
}

TEST_CASE("text area pointer click places the caret on the clicked line", "[text-area][input][pointer]") {
    auto area = std::make_shared<widget::TextArea>("first\nsecond\nthird");
    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(220.0F, 220.0F)) >= 1);

    const auto& layout = area->editable_text().text_node().layout_result();
    REQUIRE(layout.lines.size() == 3);
    const auto style = area->resolved_style();

    // 用布局算出第二行中间某个停靠点的局部坐标，而不是猜一个像素偏移。
    float line_top = 0.0F;
    for (std::size_t index = 0; index < 1; ++index) {
        line_top += layout.lines[index].size.get_height();
    }
    // 取**副本**：layout 是对 TextArea 内部 TextLayoutResult 的引用，下面的
    // dispatch_mouse_button 会触发重新布局并 move-assign 掉 lines，持有元素引用
    // 会在点击后悬垂（ASan: heap-use-after-free）。
    const auto line = layout.lines[1];
    REQUIRE(line.caret_stops.size() >= 4);
    const auto stop = line.caret_stops[3];
    const foundation::NanPoint click(
        style.metrics.padding_x + stop.x,
        style.metrics.padding_y + line_top + line.size.get_height() * 0.5F
    );

    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        click
    ));
    REQUIRE(area->editable_text().caret() == stop.source_offset);
    REQUIRE(stop.source_offset == line.text_offset + 3);

    // 第三行点击落在更大的 source 偏移上。
    float third_top = line_top + line.size.get_height();
    const auto third = layout.lines[2];
    const auto third_stop = third.caret_stops[2];
    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(
            style.metrics.padding_x + third_stop.x,
            style.metrics.padding_y + third_top + third.size.get_height() * 0.5F
        )
    ));
    REQUIRE(area->editable_text().caret() == third.text_offset + 2);

    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        click
    ));
    REQUIRE(tree.pointer_capture() == nullptr);
}

TEST_CASE("text area arrow keys move the caret across visual lines", "[text-area][input][caret]") {
    auto area = std::make_shared<widget::TextArea>("abc\ndef");
    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(220.0F, 200.0F)) >= 1);
    tree.set_focus(area.get());

    // "abc" 占 [0, 3)，'\n' 在 3，"def" 占 [4, 7)。
    area->editable_text().set_caret(3);
    tree.dispatch_key(scene::KeyEvent(widget::keys::right, scene::KeyEvent::Action::press));
    REQUIRE(area->editable_text().caret() == 4);
    tree.dispatch_key(scene::KeyEvent(widget::keys::left, scene::KeyEvent::Action::press));
    REQUIRE(area->editable_text().caret() == 3);

    // 第二行内左右移动；Home / End 只作用于当前视觉行。
    area->editable_text().set_caret(5);
    tree.dispatch_key(scene::KeyEvent(widget::keys::home, scene::KeyEvent::Action::press));
    REQUIRE(area->editable_text().caret() == 4);
    tree.dispatch_key(scene::KeyEvent(widget::keys::end, scene::KeyEvent::Action::press));
    REQUIRE(area->editable_text().caret() == 7);

    // 上下移动保留视觉列，并夹到目标行长度内。
    area->editable_text().set_caret(5);
    tree.dispatch_key(scene::KeyEvent(widget::keys::up, scene::KeyEvent::Action::press));
    REQUIRE(area->editable_text().caret() == 1);
    tree.dispatch_key(scene::KeyEvent(widget::keys::down, scene::KeyEvent::Action::press));
    REQUIRE(area->editable_text().caret() == 5);

    // 第一行再向上、最后一行再向下都是空操作。
    area->editable_text().set_caret(1);
    tree.dispatch_key(scene::KeyEvent(widget::keys::up, scene::KeyEvent::Action::press));
    REQUIRE(area->editable_text().caret() == 1);
    area->editable_text().set_caret(5);
    tree.dispatch_key(scene::KeyEvent(widget::keys::down, scene::KeyEvent::Action::press));
    REQUIRE(area->editable_text().caret() == 5);
}

TEST_CASE("text area scrolls vertically to keep the caret visible", "[text-area][scroll]") {
    auto area = std::make_shared<widget::TextArea>("");
    area->set_rows(2);
    const auto measured = area->measure_layout(scene::LayoutConstraints::loose());

    scene::NanSceneTree tree;
    tree.set_root(area);
    const auto viewport = foundation::NanSize(160.0F, measured.get_height());
    REQUIRE(tree.layout_root(viewport) >= 1);
    tree.set_focus(area.get());

    area->set_value("l1\nl2\nl3\nl4\nl5\nl6");
    REQUIRE(tree.layout_root(viewport) >= 1);
    REQUIRE(area->scroll_offset().get_y() == Catch::Approx(0.0F));

    area->editable_text().set_caret(area->value().size());
    RecordingDevice device;
    tree.draw(device);

    const float scrolled = area->scroll_offset().get_y();
    REQUIRE(scrolled > 0.0F);
    // 插入符完全落在视口内。
    const auto geometry = area->editable_text().caret_geometry();
    const auto style = area->resolved_style();
    const float caret_top = geometry.top - scrolled;
    REQUIRE(caret_top >= 0.0F);
    REQUIRE(
        caret_top + geometry.height
        <= Catch::Approx(area->height() - style.metrics.padding_y * 2.0F)
    );
    // 内容整体上移，首行被滚出内容框顶部。
    REQUIRE_FALSE(device.texts.empty());
    REQUIRE(device.texts.front().position.get_y() < style.metrics.padding_y);

    // 插入符回到开头时滚回顶部。
    area->editable_text().set_caret(0);
    tree.draw(device);
    REQUIRE(area->scroll_offset().get_y() == Catch::Approx(0.0F));
}

TEST_CASE("text area wheel scrolls within the overflow range", "[text-area][scroll][wheel]") {
    auto area = std::make_shared<widget::TextArea>("");
    area->set_rows(2);
    const auto measured = area->measure_layout(scene::LayoutConstraints::loose());

    scene::NanSceneTree tree;
    tree.set_root(area);
    const auto viewport = foundation::NanSize(160.0F, measured.get_height());
    REQUIRE(tree.layout_root(viewport) >= 1);

    area->set_value("l1\nl2\nl3\nl4\nl5\nl6");
    REQUIRE(tree.layout_root(viewport) >= 1);
    const float maximum = area->editable_text().text_node().layout_result().size.get_height()
        - (area->height() - area->resolved_style().metrics.padding_y * 2.0F);
    REQUIRE(maximum > 0.0F);

    const foundation::NanPoint inside(20.0F, 10.0F);
    tree.dispatch_mouse_wheel(scene::MouseWheelEvent(inside, foundation::NanPoint(0.0F, -1.0F)));
    REQUIRE(area->scroll_offset().get_y() > 0.0F);

    // 多滚几格夹到内容末端。
    for (int index = 0; index < 8; ++index) {
        tree.dispatch_mouse_wheel(
            scene::MouseWheelEvent(inside, foundation::NanPoint(0.0F, -1.0F))
        );
    }
    REQUIRE(area->scroll_offset().get_y() == Catch::Approx(maximum));

    tree.dispatch_mouse_wheel(scene::MouseWheelEvent(inside, foundation::NanPoint(0.0F, 1.0F)));
    REQUIRE(area->scroll_offset().get_y() < maximum);
}

TEST_CASE("text area disabled drops focus and ignores input", "[text-area][state][disabled]") {
    auto area = std::make_shared<widget::TextArea>("value");
    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 120.0F)) >= 1);
    tree.set_focus(area.get());
    REQUIRE(tree.focused_node() == area.get());

    area->set_disabled(true);
    REQUIRE(area->disabled());
    REQUIRE_FALSE(area->is_focusable());
    REQUIRE(tree.focused_node() == nullptr);
    REQUIRE(
        theme::has_text_area_state(area->visual_state(), theme::TextAreaVisualState::disabled)
    );

    tree.dispatch_text_input(scene::TextInputEvent("x"));
    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));
    REQUIRE(area->value() == "value");

    area->set_disabled(false);
    REQUIRE(area->is_focusable());
}

TEST_CASE("text area read-only keeps the value but still edits nothing", "[text-area][state][read-only]") {
    auto area = std::make_shared<widget::TextArea>("locked");
    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 120.0F)) >= 1);
    tree.set_focus(area.get());

    const auto editable_fill = area->resolved_style().container.fill;
    area->set_read_only(true);
    REQUIRE(area->read_only());
    // read_only 选择器把容器底切到 muted。
    REQUIRE_FALSE(area->resolved_style().container.fill.approx_equals(editable_fill));

    tree.dispatch_text_input(scene::TextInputEvent("x"));
    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));
    REQUIRE(area->value() == "locked");

    area->set_read_only(false);
    tree.dispatch_text_input(scene::TextInputEvent("!"));
    REQUIRE(area->value() == "locked!");
}

TEST_CASE("text area shows the placeholder only while empty", "[text-area][placeholder]") {
    auto area = std::make_shared<widget::TextArea>("");
    area->set_placeholder("Notes");
    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 120.0F)) >= 1);

    RecordingDevice empty_device;
    tree.draw(empty_device);
    REQUIRE(empty_device.texts.size() == 1);
    REQUIRE(empty_device.texts.front().text == "Notes");

    area->set_value("typed");
    RecordingDevice filled_device;
    tree.draw(filled_device);
    REQUIRE(filled_device.texts.size() == 1);
    REQUIRE(filled_device.texts.front().text == "typed");
}

TEST_CASE("text area recipe override survives a theme switch", "[text-area][theme][override]") {
    theme::ThemeManager themes;
    auto area = widget::TextArea::create();
    area->set_override(theme::TextAreaRecipeRule {
        .container_radius = theme::ThemeScalar::literal(24.0F),
        .metrics_rows = theme::ThemeScalar::literal(6.0F),
        .metrics_padding_y = theme::ThemeScalar::literal(14.0F),
    });

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 200.0F)) >= 1);

    REQUIRE(area->resolved_style().container.radius == Catch::Approx(24.0F));
    REQUIRE(area->resolved_style().metrics.padding_y == Catch::Approx(14.0F));
    // 未显式 set_rows 时配方行数生效。
    REQUIRE(area->rows() == 6);

    const float light_fill = area->resolved_style().container.fill.oklch().light;
    themes.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(themes.appearance() == theme::ColorAppearance::dark);

    // override 不冻结：跟随新快照重解析，同时保留被覆盖的字段。
    REQUIRE(area->resolved_style().container.radius == Catch::Approx(24.0F));
    REQUIRE(area->resolved_style().metrics.padding_y == Catch::Approx(14.0F));
    REQUIRE(area->resolved_style().container.fill.oklch().light != Catch::Approx(light_fill));
    REQUIRE(
        area->resolved_style().container.fill.oklch().light
        == Catch::Approx(themes.design_system().dark.background.oklch().light)
    );

    // 显式 set_rows 后不再被配方默认值覆盖。
    area->set_rows(4);
    themes.set_system_appearance(theme::ColorAppearance::light);
    REQUIRE(area->rows() == 4);
}

TEST_CASE("text area read_only rule only matches read-only instances", "[text-area][theme]") {
    const auto design = theme::default_design_system();
    const auto editable = theme::resolve_text_area(
        design,
        theme::ColorAppearance::light,
        theme::TextAreaVisualState::normal,
        false
    );
    const auto read_only = theme::resolve_text_area(
        design,
        theme::ColorAppearance::light,
        theme::TextAreaVisualState::normal,
        true
    );
    REQUIRE(read_only.container.fill.approx_equals(design.light.muted));
    REQUIRE(editable.container.fill.approx_equals(design.light.background));
    // read-only 仍可获得焦点，焦点环由 focused 规则单独开启。
    const auto focused = theme::resolve_text_area(
        design,
        theme::ColorAppearance::light,
        theme::TextAreaVisualState::focused,
        true
    );
    REQUIRE(focused.focus.width > 0.0F);
}

TEST_CASE("text area exposes text field semantics and set_value action", "[text-area][semantics]") {
    auto area = std::make_shared<widget::TextArea>("hello\nworld");
    area->set_placeholder("Notes");
    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 160.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(area->semantics_id());
    REQUIRE(node != nullptr);
    // Role 枚举没有多行文本成员；多行输入沿用 text_field 的可编辑文本语义。
    REQUIRE(node->properties.role == semantics::Role::text_field);
    REQUIRE(node->properties.label == "Notes");
    REQUIRE(node->properties.value == "hello\nworld");
    REQUIRE(node->properties.state.focusable);
    REQUIRE_FALSE(node->properties.state.read_only);
    REQUIRE(semantics::supports(node->properties.actions, semantics::Action::focus));
    REQUIRE(semantics::supports(node->properties.actions, semantics::Action::set_value));

    REQUIRE(tree.perform_semantics_action(
        area->semantics_id(),
        {.action = semantics::Action::set_value, .value = "replaced"}
    ));
    REQUIRE(area->value() == "replaced");

    area->set_read_only(true);
    REQUIRE(tree.update_semantics());
    const auto* read_only_node = tree.semantics_tree().find(area->semantics_id());
    REQUIRE(read_only_node != nullptr);
    REQUIRE(read_only_node->properties.state.read_only);
    REQUIRE_FALSE(semantics::supports(read_only_node->properties.actions, semantics::Action::set_value));
    REQUIRE_FALSE(tree.perform_semantics_action(
        area->semantics_id(),
        {.action = semantics::Action::set_value, .value = "nope"}
    ));
    REQUIRE(area->value() == "replaced");
}

TEST_CASE("text area builds through BuildContext and binds a string signal", "[text-area][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    reactive::Signal<std::string> draft {graph, "Task"};
    auto area = ui.make<widget::TextArea>(draft, "Add a task").build();
    REQUIRE(area->value() == "Task");
    REQUIRE(area->placeholder() == "Add a task");

    scene::NanSceneTree tree;
    tree.set_root(area);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 140.0F)) >= 1);

    draft.set("Updated");
    REQUIRE(area->value() == "Updated");

    tree.set_focus(area.get());
    area->editable_text().set_caret(area->value().size());
    tree.dispatch_text_input(scene::TextInputEvent("!"));
    REQUIRE(draft.peek() == "Updated!");

    auto plain = ui.make<widget::TextArea>("literal", "Hint").build();
    REQUIRE(plain->value() == "literal");
    REQUIRE(plain->placeholder() == "Hint");
}
