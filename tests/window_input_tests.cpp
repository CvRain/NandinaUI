#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/scene/control.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/primitives/editable_text.hpp>

#include <SDL3/SDL.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <memory>
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
