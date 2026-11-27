#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/app/platform_window.hpp>
#include <nandina/scene/control.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/primitives/editable_text.hpp>

#include <SDL3/SDL.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <memory>
#include <optional>
#include <vector>

namespace
{
    using namespace nandina;

    class InputTarget final: public scene::NanNode2D {
    public:
        [[nodiscard]] auto contains_point(foundation::NanPoint point) const -> bool override {
            return point.get_x() >= 0 && point.get_x() < 100 && point.get_y() >= 0
                && point.get_y() < 100;
        }
        auto on_input(scene::InputEvent& event) -> bool override {
            if (event.type() == scene::EventType::mouse_button) {
                buttons.push_back(static_cast<const scene::MouseButtonEvent&>(event).screen_pos());
                return true;
            }
            if (event.type() == scene::EventType::mouse_wheel) {
                wheels.push_back(static_cast<const scene::MouseWheelEvent&>(event).screen_pos());
                return true;
            }
            return false;
        }
        std::vector<foundation::NanPoint> buttons;
        std::vector<foundation::NanPoint> wheels;
    };

    void push(SDL_Event event) {
        REQUIRE(SDL_PushEvent(&event));
    }

    /// Focusable target that records the order in which it receives input.
    class OrderedTarget final: public scene::NanNode2D {
    public:
        explicit OrderedTarget(std::vector<std::string>* trace): trace_(trace) {}

        [[nodiscard]] auto contains_point(foundation::NanPoint point) const -> bool override {
            return point.get_x() >= 0 && point.get_x() < 100 && point.get_y() >= 0
                && point.get_y() < 100;
        }
        [[nodiscard]] auto is_focusable() const -> bool override {
            return true;
        }
        auto on_input(scene::InputEvent& event) -> bool override {
            switch (event.type()) {
                case scene::EventType::mouse_button:
                    trace_->push_back(
                        static_cast<scene::MouseButtonEvent&>(event).is_pressed() ? "press"
                                                                                  : "release"
                    );
                    return true;
                case scene::EventType::text_input:
                    trace_->push_back(
                        "text:" + std::string(static_cast<scene::TextInputEvent&>(event).text())
                    );
                    return true;
                case scene::EventType::focus_enter:
                    trace_->push_back("focus");
                    return false;
                default:
                    return false;
            }
        }

    private:
        std::vector<std::string>* trace_;
    };

    /// 只记录调用的设备替身：窗口测试只关心帧序与布局尺寸，不做像素断言。
    class NoopDevice: public render::IRenderDevice {
    public:
        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void
        draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void
        draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        [[nodiscard]] auto supports_alpha_textures() const -> bool override {
            return true;
        }
        [[nodiscard]] auto create_alpha_texture(int, int, std::span<const std::uint8_t>)
            -> render::TextureHandle override {
            return render::TextureHandle {++next_texture_};
        }

    private:
        std::uint64_t next_texture_ = 0;
    };

    /// 可在 `poll_input()` 内改变窗口尺寸、并可顺带投递一次指针按下的平台替身：
    /// 模拟「OS 把 resize 事件交给本次 poll」的时序，锁定 tick 的尺寸重采样。
    class ResizeOnPollPlatform final: public app::IPlatformWindow {
    public:
        foundation::NanSize pending_size {640.0F, 360.0F};
        bool apply_resize_on_next_poll = false;
        std::optional<foundation::NanPoint> pending_press_screen_pos;

        auto create(const app::WindowConfig& config) -> bool override {
            size_ = foundation::NanSize(
                static_cast<float>(config.width),
                static_cast<float>(config.height)
            );
            ready_ = true;
            return true;
        }
        void destroy() override {
            ready_ = false;
        }
        [[nodiscard]] auto is_ready() const -> bool override {
            return ready_;
        }
        [[nodiscard]] auto should_close() const -> bool override {
            return !ready_;
        }
        [[nodiscard]] auto screen_size() const -> foundation::NanSize override {
            return size_;
        }
        [[nodiscard]] auto drawable_size() const -> foundation::NanSize override {
            return size_;
        }
        [[nodiscard]] auto frame_time() const -> float override {
            return 1.0F / 60.0F;
        }
        [[nodiscard]] auto display_scale() const -> float override {
            return 1.0F;
        }
        void poll_input(app::FrameInput& out) override {
            ++seq_;
            if (apply_resize_on_next_poll) {
                size_ = pending_size;
                apply_resize_on_next_poll = false;
            }
            if (pending_press_screen_pos.has_value()) {
                out.events.emplace_back(
                    scene::MouseButtonEvent {
                        scene::MouseButtonEvent::Button::left,
                        scene::MouseButtonEvent::Action::press,
                        *pending_press_screen_pos,
                        {},
                    }
                );
                pending_press_screen_pos.reset();
            }
            out.has_pointer = true;
            out.pointer = foundation::NanPoint::zero();
        }
        void prepare_frame(const foundation::NanColor& clear_color) override {
            prepare_order = ++seq_;
            prepared_drawable_size = size_;
            prepared_clear_color = clear_color;
            ++prepare_calls;
        }
        void present() override {}
        void set_clipboard_text(std::string_view) override {}
        [[nodiscard]] auto clipboard_text() const -> std::string override {
            return {};
        }
        [[nodiscard]] auto make_render_device() -> std::unique_ptr<render::IRenderDevice> override {
            struct OrderTrackingDevice final: public NoopDevice {
                int* seq = nullptr;
                int* begin_order = nullptr;
                int* clear_order = nullptr;
                explicit OrderTrackingDevice(int& s, int& b, int& c):
                    seq(&s),
                    begin_order(&b),
                    clear_order(&c) {}
                void begin_frame() override {
                    *begin_order = ++(*seq);
                }
                void clear(const foundation::NanColor&) override {
                    *clear_order = ++(*seq);
                }
            };
            return std::make_unique<OrderTrackingDevice>(seq_, begin_frame_order, clear_order);
        }
        [[nodiscard]] auto name() const -> std::string_view override {
            return "resize-on-poll-mock";
        }

        int prepare_calls = 0;
        int prepare_order = 0;
        int begin_frame_order = 0;
        int clear_order = 0;
        foundation::NanSize prepared_drawable_size {};
        foundation::NanColor prepared_clear_color {};

    private:
        int seq_ = 0;
        foundation::NanSize size_ {400.0F, 240.0F};
        bool ready_ = false;
    };

    /// 内容根：记录派发到它身上的指针按下位置（逻辑坐标）。
    class RecordingContent final: public scene::NanControl {
    public:
        [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
            return true;
        }
        auto on_input(scene::InputEvent& event) -> bool override {
            if (event.type() == scene::EventType::mouse_button) {
                presses.push_back(static_cast<scene::MouseButtonEvent&>(event).screen_pos());
                return true;
            }
            return false;
        }

        std::vector<foundation::NanPoint> presses;
    };
} // namespace

TEST_CASE(
    "window hit tests each pointer event before the final pointer position",
    "[app][input][snapshot]"
) {
    using namespace nandina;
    const bool scaled = GENERATE(false, true);
    app::NanApplication application;
    app::WindowConfig config;
    config.width = 600;
    config.height = 400;
    config.vsync = false;
    config.target_fps = 0;
    config.backend = app::WindowBackend::sdl3;
    if (scaled) {
        config.viewport = app::ViewportScalePolicy {
            .design_size = foundation::NanSize(200, 200),
        };
    }
    app::NanWindow window(application, config);
    window.open();
    auto root = std::make_shared<scene::NanControl>();
    auto a = std::make_shared<InputTarget>();
    auto b = std::make_shared<InputTarget>();
    b->set_position(foundation::NanPoint(100, 0));
    root->add_child(a);
    root->add_child(b);
    window.set_content(root);
    window.tick();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    // Contain mapping: 2x scale and a 100px left letterbox margin.
    const float x = scaled ? 140.0F : 20.0F;
    const float y = scaled ? 60.0F : 30.0F;
    SDL_Event button {};
    button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    button.button.button = SDL_BUTTON_LEFT;
    button.button.x = x;
    button.button.y = y;
    push(button);
    // Release capture before testing the independent wheel hit target.
    button.type = SDL_EVENT_MOUSE_BUTTON_UP;
    push(button);
    SDL_Event wheel {};
    wheel.type = SDL_EVENT_MOUSE_WHEEL;
    wheel.wheel.mouse_x = x;
    wheel.wheel.mouse_y = y;
    wheel.wheel.y = 1;
    push(wheel);
    SDL_Event motion {};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.x = scaled ? 340.0F : 120.0F;
    motion.motion.y = y;
    motion.motion.xrel = motion.motion.x - x;
    push(motion);
    window.tick();

    REQUIRE(a->buttons.size() == 2);
    REQUIRE(a->wheels.size() == 1);
    REQUIRE(b->buttons.empty());
    REQUIRE(b->wheels.empty());
    REQUIRE(a->buttons.front().get_x() == Catch::Approx(20));
    REQUIRE(a->buttons.front().get_y() == Catch::Approx(30));
    REQUIRE(a->wheels.front().get_x() == Catch::Approx(20));
    REQUIRE(a->wheels.front().get_y() == Catch::Approx(30));
    window.close();
}

TEST_CASE("window forwards typed key lifecycle to the focused editor", "[app][input][keys]") {
    using namespace nandina;
    app::NanApplication application;
    app::WindowConfig config;
    config.width = 320;
    config.height = 200;
    config.vsync = false;
    config.target_fps = 0;
    config.backend = app::WindowBackend::sdl3;
    app::NanWindow window(application, config);
    window.open();
    auto edit = std::make_shared<widget::primitives::EditableText>("ab");
    window.set_content(edit);
    window.scene_tree().set_focus(edit.get());
    window.tick();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    // dummy 视频驱动不保证键盘焦点，直接取本进程唯一的窗口 ID。
    int window_count = 0;
    SDL_Window** windows = SDL_GetWindows(&window_count);
    REQUIRE(windows != nullptr);
    REQUIRE(window_count == 1);
    const auto window_id = SDL_GetWindowID(windows[0]);
    SDL_free(windows);
    SDL_Event down {};
    down.type = SDL_EVENT_KEY_DOWN;
    down.key.windowID = window_id;
    down.key.scancode = SDL_SCANCODE_BACKSPACE;
    down.key.key = SDLK_BACKSPACE;
    down.key.down = true;
    push(down);
    SDL_Event repeat = down;
    repeat.key.repeat = true;
    push(repeat);
    SDL_Event text {};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.windowID = window_id;
    text.text.text = "c";
    push(text);
    SDL_Event up = down;
    up.type = SDL_EVENT_KEY_UP;
    up.key.down = false;
    push(up);
    window.tick();

    REQUIRE(edit->value() == "c");
    // 先释放测试持有的控件：窗口关闭后 device 已销毁，而 FontPipeline 析构会回访
    // device（已知关闭顺序缺陷，见 design_tokens.md）；应用级内容在 close() 内释放。
    edit.reset();
    window.close();
}

TEST_CASE("window routes IME composition through the text session", "[app][input][text-session]") {
    using namespace nandina;
    app::NanApplication application;
    app::WindowConfig config;
    config.width = 320;
    config.height = 200;
    config.vsync = false;
    config.target_fps = 0;
    config.backend = app::WindowBackend::sdl3;
    app::NanWindow window(application, config);
    window.open();
    auto edit = std::make_shared<widget::primitives::EditableText>("");
    window.set_content(edit);
    window.tick();
    window.scene_tree().set_focus(edit.get());
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    int window_count = 0;
    SDL_Window** windows = SDL_GetWindows(&window_count);
    REQUIRE(windows != nullptr);
    REQUIRE(window_count == 1);
    const auto window_id = SDL_GetWindowID(windows[0]);
    SDL_free(windows);

    SDL_Event gained {};
    gained.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    gained.window.windowID = window_id;
    push(gained);
    window.tick();
    REQUIRE(window.scene_tree().text_input_session_active());

    std::string preedit = "にほ";
    SDL_Event editing {};
    editing.type = SDL_EVENT_TEXT_EDITING;
    editing.edit.windowID = window_id;
    editing.edit.text = preedit.c_str();
    editing.edit.start = 1;
    editing.edit.length = 1;
    push(editing);

    SDL_Event committed {};
    committed.type = SDL_EVENT_TEXT_INPUT;
    committed.text.windowID = window_id;
    committed.text.text = "日本";
    push(committed);
    window.tick();

    REQUIRE(edit->value() == "日本"); // committed exactly once through the session
    REQUIRE_FALSE(edit->composition().has_value());
    edit.reset();
    window.close();
}

TEST_CASE("window focus events gate the scene focus hint", "[app][input][focus]") {
    using namespace nandina;
    app::NanApplication application;
    app::WindowConfig config;
    config.width = 320;
    config.height = 200;
    config.vsync = false;
    config.target_fps = 0;
    config.backend = app::WindowBackend::sdl3;
    app::NanWindow window(application, config);
    window.open();
    auto button = std::make_shared<widget::Button>("Focus");
    window.set_content(button);
    window.tick();
    window.scene_tree().set_focus(button.get());
    window.tick();
    // Programmatic focus before any input must be visible.
    REQUIRE(window.scene_tree().should_show_focus_hint());

    int window_count = 0;
    SDL_Window** windows = SDL_GetWindows(&window_count);
    REQUIRE(windows != nullptr);
    REQUIRE(window_count == 1);
    const auto window_id = SDL_GetWindowID(windows[0]);
    SDL_free(windows);

    SDL_Event lost {};
    lost.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    lost.window.windowID = window_id;
    push(lost);
    window.tick();
    REQUIRE_FALSE(window.scene_tree().window_active());
    REQUIRE_FALSE(window.scene_tree().should_show_focus_hint());
    REQUIRE(window.scene_tree().focused_node() == button.get());

    SDL_Event gained {};
    gained.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    gained.window.windowID = window_id;
    push(gained);
    window.tick();
    REQUIRE(window.scene_tree().window_active());
    REQUIRE(window.scene_tree().should_show_focus_hint());
    button.reset();
    window.close();
}

TEST_CASE(
    "window dispatches same-frame pointer and text events in capture order",
    "[app][input][order]"
) {
    using namespace nandina;
    app::NanApplication application;
    app::WindowConfig config;
    config.width = 400;
    config.height = 240;
    config.vsync = false;
    config.target_fps = 0;
    config.backend = app::WindowBackend::sdl3;
    app::NanWindow window(application, config);
    window.open();
    std::vector<std::string> first_trace;
    std::vector<std::string> second_trace;
    auto root = std::make_shared<scene::NanControl>();
    auto first = std::make_shared<OrderedTarget>(&first_trace);
    auto second = std::make_shared<OrderedTarget>(&second_trace);
    second->set_position(foundation::NanPoint(100.0F, 0.0F));
    root->add_child(first);
    root->add_child(second);
    window.set_content(root);
    window.tick();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    int window_count = 0;
    SDL_Window** windows = SDL_GetWindows(&window_count);
    REQUIRE(windows != nullptr);
    REQUIRE(window_count == 1);
    const auto window_id = SDL_GetWindowID(windows[0]);
    SDL_free(windows);

    const auto click = [window_id](const float x) {
        for (const auto type: {SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_BUTTON_UP}) {
            SDL_Event event {};
            event.type = type;
            event.button.windowID = window_id;
            event.button.button = SDL_BUTTON_LEFT;
            event.button.down = type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            event.button.x = x;
            event.button.y = 30.0F;
            push(event);
        }
    };
    click(20.0F);
    SDL_Event text {};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.windowID = window_id;
    text.text.text = "x";
    push(text);
    click(120.0F);
    window.tick();

    // 文本必须到达点击 A 时的焦点；不能被排到点击 B 之后。
    REQUIRE(first_trace == std::vector<std::string> {"focus", "press", "release", "text:x"});
    REQUIRE(second_trace == std::vector<std::string> {"focus", "press", "release"});
    REQUIRE(window.scene_tree().focused_node() == second.get());
    first.reset();
    second.reset();
    window.close();
}

TEST_CASE(
    "window tick publishes the viewport caret to SDL before drawing",
    "[app][input][text-session][area]"
) {
    using namespace nandina;
    const bool scaled = GENERATE(false, true);
    app::NanApplication application;
    app::WindowConfig config;
    config.width = 600;
    config.height = 400;
    config.vsync = false;
    config.target_fps = 0;
    config.backend = app::WindowBackend::sdl3;
    if (scaled) {
        config.viewport = app::ViewportScalePolicy {.design_size = foundation::NanSize(200, 200)};
    }
    app::NanWindow window(application, config);
    window.open();
    auto root = std::make_shared<scene::NanControl>();
    auto edit = std::make_shared<widget::primitives::EditableText>("ab");
    edit->set_presentation_translate(foundation::NanPoint(20, 30));
    edit->set_caret(1);
    root->add_child(edit);
    window.set_content(root);
    window.tick();
    window.scene_tree().set_focus(edit.get());
    int count = 0;
    SDL_Window** windows = SDL_GetWindows(&count);
    REQUIRE(windows != nullptr);
    REQUIRE(count == 1);
    auto* native = windows[0];
    SDL_free(windows);
    SDL_Event gained {};
    gained.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    gained.window.windowID = SDL_GetWindowID(native);
    push(gained);
    window.tick();
    REQUIRE(window.scene_tree().text_input_session_active());
    auto check_area = [&](const float scale, const float offset_x) {
        const auto caret = edit->text_input_caret_area();
        SDL_Rect rect {};
        int cursor = -1;
        REQUIRE(SDL_GetTextInputArea(native, &rect, &cursor));
        const auto left = std::floor(offset_x + scale * (20 + caret.get_left()));
        const auto top = std::floor(scale * (30 + caret.get_top()));
        CHECK(rect.x == left);
        CHECK(rect.y == top);
        CHECK(rect.w == std::ceil(offset_x + scale * (20 + caret.get_right())) - left);
        CHECK(rect.h == std::ceil(scale * (30 + caret.get_bottom())) - top);
        CHECK(cursor == 0);
    };
    check_area(scaled ? 2.0F : 1.0F, scaled ? 100.0F : 0.0F);
    // Resize must refresh the viewport mapping, and changing the caret must not need layout.
    REQUIRE(SDL_SetWindowSize(native, 800, 600));
    edit->set_caret(2);
    window.tick();
    check_area(scaled ? 3.0F : 1.0F, scaled ? 100.0F : 0.0F);
    edit.reset();
    root.reset();
    window.close();
}

TEST_CASE(
    "window coalesces consecutive same-frame MouseMoveEvents while preserving button ordering",
    "[app][input][coalesce]"
) {
    using namespace nandina;
    class TraceNode final: public scene::NanControl {
    public:
        std::vector<std::string> events;
        std::vector<foundation::NanPoint> move_positions;
        std::vector<foundation::NanPoint> move_deltas;

        explicit TraceNode(const foundation::NanSize size): scene::NanControl(size) {}

        auto on_input(scene::InputEvent& event) -> bool override {
            if (event.type() == scene::EventType::mouse_move) {
                const auto& move = static_cast<const scene::MouseMoveEvent&>(event);
                events.emplace_back("move");
                move_positions.push_back(move.screen_pos());
                move_deltas.push_back(move.delta());
                event.accept();
                return true;
            }
            if (event.type() == scene::EventType::mouse_button) {
                const auto& button = static_cast<const scene::MouseButtonEvent&>(event);
                events.emplace_back(button.is_pressed() ? "press" : "release");
                event.accept();
                return true;
            }
            return false;
        }
    };

    app::NanApplication application;
    app::WindowConfig config;
    config.width = 400;
    config.height = 240;
    config.vsync = false;
    config.target_fps = 0;
    config.backend = app::WindowBackend::sdl3;
    app::NanWindow window(application, config);
    window.open();

    auto node = std::make_shared<TraceNode>(foundation::NanSize(400.0F, 240.0F));
    window.set_content(node);
    window.tick();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    node->events.clear();
    node->move_positions.clear();
    node->move_deltas.clear();

    auto push_motion = [](const float x, const float y, const float dx, const float dy) {
        SDL_Event motion {};
        motion.type = SDL_EVENT_MOUSE_MOTION;
        motion.motion.x = x;
        motion.motion.y = y;
        motion.motion.xrel = dx;
        motion.motion.yrel = dy;
        push(motion);
    };
    auto push_button = [](const bool down, const float x, const float y) {
        SDL_Event button {};
        button.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        button.button.button = SDL_BUTTON_LEFT;
        button.button.x = x;
        button.button.y = y;
        push(button);
    };

    // Three back-to-back motions coalesce into a single MouseMoveEvent with summed delta.
    push_motion(10.0F, 20.0F, 5.0F, 0.0F);
    push_motion(20.0F, 20.0F, 10.0F, 0.0F);
    push_motion(35.0F, 25.0F, 15.0F, 5.0F);
    // Button press breaks coalescing; subsequent motions coalesce separately before release.
    push_button(true, 35.0F, 25.0F);
    push_motion(45.0F, 25.0F, 10.0F, 0.0F);
    push_motion(60.0F, 30.0F, 15.0F, 5.0F);
    push_button(false, 60.0F, 30.0F);
    window.tick();

    REQUIRE(node->events == std::vector<std::string> {"move", "press", "move", "release"});
    REQUIRE(node->move_positions.size() == 2);
    CHECK(node->move_positions[0].get_x() == Catch::Approx(35.0F));
    CHECK(node->move_positions[0].get_y() == Catch::Approx(25.0F));
    CHECK(node->move_deltas[0].get_x() == Catch::Approx(30.0F));
    CHECK(node->move_deltas[0].get_y() == Catch::Approx(5.0F));
    CHECK(node->move_positions[1].get_x() == Catch::Approx(60.0F));
    CHECK(node->move_positions[1].get_y() == Catch::Approx(30.0F));
    CHECK(node->move_deltas[1].get_x() == Catch::Approx(25.0F));
    CHECK(node->move_deltas[1].get_y() == Catch::Approx(5.0F));

    node.reset();
    window.close();
}

TEST_CASE(
    "window tick lays out using the resized screen_size produced during poll_input",
    "[app][resize][timing]"
) {
    using namespace nandina;

    app::NanApplication application;
    app::WindowConfig config;
    config.width = 400;
    config.height = 240;
    config.vsync = false;
    config.target_fps = 0;

    auto platform = std::make_unique<ResizeOnPollPlatform>();
    auto* raw_platform = platform.get();
    app::NanWindow window(application, config);
    window.set_platform_window(std::move(platform));
    window.open();

    auto content = std::make_shared<scene::NanControl>();
    content->set_width(scene::fill);
    content->set_height(scene::fill);
    window.set_content(content);

    window.tick();
    REQUIRE(content->width() == Catch::Approx(400.0F));
    REQUIRE(content->height() == Catch::Approx(240.0F));

    // Simulate OS resize delivered inside poll_input(): this exact tick must lay out to 640x360.
    raw_platform->apply_resize_on_next_poll = true;
    window.tick();
    CHECK(content->width() == Catch::Approx(640.0F));
    CHECK(content->height() == Catch::Approx(360.0F));

    content.reset();
    window.close();
}

TEST_CASE(
    "window maps pointer input with the viewport refreshed during poll_input",
    "[app][resize][viewport]"
) {
    using namespace nandina;

    app::NanApplication application;
    app::WindowConfig config;
    config.width = 400;
    config.height = 240;
    config.vsync = false;
    config.target_fps = 0;
    // 固定 200x200 设计视口 contain：400x240 窗口下 scale=1.2、offset.x=80。
    config.viewport = app::ViewportScalePolicy {
        .design_size = foundation::NanSize(200.0F, 200.0F),
        .fit = app::ViewportFit::contain,
        .horizontal_anchor = app::ViewportAnchor::center,
        .vertical_anchor = app::ViewportAnchor::center,
    };

    auto platform = std::make_unique<ResizeOnPollPlatform>();
    auto* raw_platform = platform.get();
    app::NanWindow window(application, config);
    window.set_platform_window(std::move(platform));
    window.open();

    auto content = std::make_shared<RecordingContent>();
    content->set_width(scene::fill);
    content->set_height(scene::fill);
    window.set_content(content);

    window.tick();
    REQUIRE(content->width() == Catch::Approx(200.0F));
    REQUIRE(content->height() == Catch::Approx(200.0F));

    // poll_input 内先 resize 到 800x600（scale=3、offset.x=100），再投递同一帧的指针按下。
    // 该事件必须按新映射换算：(400,300) -> (100,100)，而不是沿用上一帧的 scale=1.2/offset=80。
    raw_platform->apply_resize_on_next_poll = true;
    raw_platform->pending_size = foundation::NanSize(800.0F, 600.0F);
    raw_platform->pending_press_screen_pos = foundation::NanPoint(400.0F, 300.0F);
    window.tick();

    REQUIRE(content->presses.size() == 1);
    CHECK(content->presses.front().get_x() == Catch::Approx(100.0F));
    CHECK(content->presses.front().get_y() == Catch::Approx(100.0F));

    content.reset();
    window.close();
}

TEST_CASE(
    "window tick tolerates a zero-sized window reported during poll_input",
    "[app][resize][invalid-size]"
) {
    using namespace nandina;

    app::NanApplication application;
    app::WindowConfig config;
    config.width = 400;
    config.height = 240;
    config.vsync = false;
    config.target_fps = 0;

    auto platform = std::make_unique<ResizeOnPollPlatform>();
    auto* raw_platform = platform.get();
    app::NanWindow window(application, config);
    window.set_platform_window(std::move(platform));
    window.open();

    auto content = std::make_shared<scene::NanControl>();
    content->set_width(scene::fill);
    content->set_height(scene::fill);
    window.set_content(content);

    window.tick();
    REQUIRE(content->width() == Catch::Approx(400.0F));
    REQUIRE(content->height() == Catch::Approx(240.0F));

    // Windows 允许把 client 区域拖成零高；平台契约此时返回零尺寸。tick 不能把
    // make_window_metrics() 的 invalid_argument 抛进宿主主循环，也不能用 0 尺寸排版。
    raw_platform->apply_resize_on_next_poll = true;
    raw_platform->pending_size = foundation::NanSize(400.0F, 0.0F);
    CHECK_NOTHROW(window.tick());
    CHECK(content->width() == Catch::Approx(400.0F));
    CHECK(content->height() == Catch::Approx(240.0F));

    // 尺寸恢复后同一帧重新排版（保留的上一帧结果不应粘住）。
    raw_platform->apply_resize_on_next_poll = true;
    raw_platform->pending_size = foundation::NanSize(640.0F, 360.0F);
    window.tick();
    CHECK(content->width() == Catch::Approx(640.0F));
    CHECK(content->height() == Catch::Approx(360.0F));

    content.reset();
    window.close();
}

TEST_CASE(
    "window tick tolerates a zero-sized window when a fixed viewport is configured",
    "[app][resize][invalid-size]"
) {
    using namespace nandina;

    app::NanApplication application;
    app::WindowConfig config;
    config.width = 400;
    config.height = 240;
    config.vsync = false;
    config.target_fps = 0;
    config.viewport = app::ViewportScalePolicy {
        .design_size = foundation::NanSize(200.0F, 200.0F),
        .fit = app::ViewportFit::contain,
        .horizontal_anchor = app::ViewportAnchor::center,
        .vertical_anchor = app::ViewportAnchor::center,
    };

    auto platform = std::make_unique<ResizeOnPollPlatform>();
    auto* raw_platform = platform.get();
    app::NanWindow window(application, config);
    window.set_platform_window(std::move(platform));
    window.open();

    auto content = std::make_shared<scene::NanControl>();
    content->set_width(scene::fill);
    content->set_height(scene::fill);
    window.set_content(content);

    window.tick();
    REQUIRE(content->width() == Catch::Approx(200.0F));

    // viewport 配置下旧的 update_viewport_mapping() 会对 0 尺寸抛错：
    // 采样必须在把尺寸交给 make_viewport_mapping() 之前完成合法性判定。
    raw_platform->apply_resize_on_next_poll = true;
    raw_platform->pending_size = foundation::NanSize(0.0F, 0.0F);
    CHECK_NOTHROW(window.tick());
    CHECK(content->width() == Catch::Approx(200.0F));
    CHECK(content->height() == Catch::Approx(200.0F));

    raw_platform->apply_resize_on_next_poll = true;
    raw_platform->pending_size = foundation::NanSize(800.0F, 600.0F);
    CHECK_NOTHROW(window.tick());
    CHECK(content->width() == Catch::Approx(200.0F));

    content.reset();
    window.close();
}

TEST_CASE(
    "window tick prepares platform frame target with resized dimensions before begin_frame",
    "[app][resize][prepare-frame]"
) {
    using namespace nandina;

    app::NanApplication application;
    app::WindowConfig config;
    config.width = 400;
    config.height = 240;
    config.vsync = false;
    config.target_fps = 0;
    const auto custom_bg = foundation::NanColor::from(
        foundation::NanHexRgb {.red = 18, .green = 34, .blue = 56, .alpha = 255}
    );
    config.background = custom_bg;

    auto platform = std::make_unique<ResizeOnPollPlatform>();
    auto* raw_platform = platform.get();
    app::NanWindow window(application, config);
    window.set_platform_window(std::move(platform));
    window.open();

    auto content = std::make_shared<scene::NanControl>();
    content->set_width(scene::fill);
    content->set_height(scene::fill);
    window.set_content(content);

    raw_platform->apply_resize_on_next_poll = true;
    raw_platform->pending_size = foundation::NanSize(640.0F, 360.0F);
    window.tick();

    REQUIRE(raw_platform->prepare_calls == 1);
    CHECK(raw_platform->prepare_order < raw_platform->begin_frame_order);
    CHECK(raw_platform->begin_frame_order < raw_platform->clear_order);
    CHECK(raw_platform->prepared_drawable_size.get_width() == Catch::Approx(640.0F));
    CHECK(raw_platform->prepared_drawable_size.get_height() == Catch::Approx(360.0F));
    CHECK(raw_platform->prepared_clear_color.approx_equals(custom_bg));

    // Zero-sized frame must skip prepare_frame and begin_frame altogether.
    raw_platform->apply_resize_on_next_poll = true;
    raw_platform->pending_size = foundation::NanSize(640.0F, 0.0F);
    window.tick();
    CHECK(raw_platform->prepare_calls == 1);

    content.reset();
    window.close();
}
