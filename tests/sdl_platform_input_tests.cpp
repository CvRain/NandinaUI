#include <nandina/app/detail/sdl_platform_window.hpp>

#include <SDL3/SDL.h>

#include <catch2/catch_test_macros.hpp>

#include <variant>
#include <vector>

namespace
{
    using namespace nandina;

    // Meson supplies the dummy video/software renderer environment: this is the real platform
    // poller, not a replacement IPlatformWindow. Pushed events intentionally disagree with SDL's
    // global modifier state, which SDL_PushEvent does not update.
    class PlatformInputFixture {
    public:
        explicit PlatformInputFixture(const SDL_Keymod initial_modifiers = SDL_KMOD_NONE) {
            REQUIRE(SDL_Init(SDL_INIT_VIDEO));
            SDL_SetModState(initial_modifiers);
            app::WindowConfig config;
            config.width = 160;
            config.height = 120;
            config.vsync = false;
            config.target_fps = 0;
            REQUIRE(window.create(config));
            SDL_PumpEvents();
            SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
            int count = 0;
            SDL_Window** windows = SDL_GetWindows(&count);
            REQUIRE(windows != nullptr);
            REQUIRE(count == 1);
            window_id = SDL_GetWindowID(windows[0]);
            SDL_free(windows);
        }

        void
        key(const SDL_EventType type,
            const SDL_Keycode code,
            const SDL_Keymod mods,
            const bool repeat = false,
            const SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN) const {
            SDL_Event event {};
            event.type = type;
            event.key.windowID = window_id;
            event.key.key = code;
            event.key.scancode = scancode;
            event.key.mod = mods;
            event.key.repeat = repeat;
            event.key.down = type == SDL_EVENT_KEY_DOWN;
            REQUIRE(SDL_PushEvent(&event));
        }

        void button(const float x, const float y, const bool down = true) const {
            SDL_Event event {};
            event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
            event.button.windowID = window_id;
            event.button.button = SDL_BUTTON_LEFT;
            event.button.down = down;
            event.button.x = x;
            event.button.y = y;
            REQUIRE(SDL_PushEvent(&event));
        }

        void wheel(const float x, const float y) const {
            SDL_Event event {};
            event.type = SDL_EVENT_MOUSE_WHEEL;
            event.wheel.windowID = window_id;
            event.wheel.mouse_x = x;
            event.wheel.mouse_y = y;
            event.wheel.y = 1.0F;
            REQUIRE(SDL_PushEvent(&event));
        }

        void focus(const SDL_EventType type, const SDL_WindowID id) const {
            SDL_Event event {};
            event.type = type;
            event.window.windowID = id;
            REQUIRE(SDL_PushEvent(&event));
        }

        [[nodiscard]] auto poll() -> app::FrameInput {
            app::FrameInput input;
            window.poll_input(input);
            return input;
        }

        app::detail::SdlPlatformWindow window;
        SDL_WindowID window_id = 0;
    };

    template<typename T>
    auto events_of(const app::FrameInput& input) -> std::vector<T> {
        std::vector<T> result;
        for (const auto& frame_event: input.events) {
            if (const auto* typed = std::get_if<T>(&frame_event)) {
                result.push_back(*typed);
            }
        }
        return result;
    }

    auto keys_of(const app::FrameInput& input) -> std::vector<scene::KeyEvent> {
        return events_of<scene::KeyEvent>(input);
    }
    auto buttons_of(const app::FrameInput& input) -> std::vector<scene::MouseButtonEvent> {
        return events_of<scene::MouseButtonEvent>(input);
    }
    auto wheels_of(const app::FrameInput& input) -> std::vector<scene::MouseWheelEvent> {
        return events_of<scene::MouseWheelEvent>(input);
    }

    void require_modifiers(
        const scene::KeyModifiers actual,
        const bool shift = false,
        const bool ctrl = false,
        const bool alt = false,
        const bool super = false
    ) {
        CHECK(actual.shift == shift);
        CHECK(actual.ctrl == ctrl);
        CHECK(actual.alt == alt);
        CHECK(actual.super == super);
    }

    void require_position(const scene::MouseEvent& event, const float x, const float y) {
        CHECK(event.screen_pos().get_x() == x);
        CHECK(event.screen_pos().get_y() == y);
    }
} // namespace

TEST_CASE("SDL input preserves same-frame event positions and modifier snapshots", "[sdl][input]") {
    PlatformInputFixture fixture;
    SDL_SetModState(SDL_KMOD_ALT);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_A, SDL_KMOD_SHIFT);
    fixture.button(12.0F, 23.0F);
    fixture.wheel(34.0F, 45.0F);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_B, SDL_KMOD_CTRL);
    fixture.button(56.0F, 67.0F, false);
    fixture.wheel(78.0F, 89.0F);
    fixture.key(SDL_EVENT_KEY_UP, SDLK_B, SDL_KMOD_NONE);

    const auto input = fixture.poll();
    REQUIRE(keys_of(input).size() == 3);
    REQUIRE(buttons_of(input).size() == 2);
    REQUIRE(wheels_of(input).size() == 2);
    // 跨类型事件保留 SDL 队列顺序，不按类型重新分组。
    REQUIRE(input.events.size() == 7);
    CHECK(std::holds_alternative<scene::KeyEvent>(input.events[0]));
    CHECK(std::holds_alternative<scene::MouseButtonEvent>(input.events[1]));
    CHECK(std::holds_alternative<scene::MouseWheelEvent>(input.events[2]));
    CHECK(std::holds_alternative<scene::KeyEvent>(input.events[3]));
    CHECK(std::holds_alternative<scene::MouseButtonEvent>(input.events[4]));
    CHECK(std::holds_alternative<scene::MouseWheelEvent>(input.events[5]));
    CHECK(std::holds_alternative<scene::KeyEvent>(input.events[6]));
    CHECK(keys_of(input)[0].logical_key() == foundation::LogicalKey::a);
    CHECK(keys_of(input)[1].logical_key() == foundation::LogicalKey::b);
    CHECK(keys_of(input)[2].logical_key() == foundation::LogicalKey::b);
    CHECK_FALSE(keys_of(input)[2].is_pressed());
    CHECK_FALSE(keys_of(input)[2].is_press_or_repeat());
    require_modifiers(keys_of(input)[2].modifiers());
    require_modifiers(keys_of(input)[0].modifiers(), true);
    require_modifiers(keys_of(input)[1].modifiers(), false, true);
    require_modifiers(buttons_of(input)[0].modifiers(), true);
    require_modifiers(buttons_of(input)[1].modifiers(), false, true);
    require_modifiers(wheels_of(input)[0].modifiers(), true);
    require_modifiers(wheels_of(input)[1].modifiers(), false, true);
    require_position(buttons_of(input)[0], 12.0F, 23.0F);
    require_position(buttons_of(input)[1], 56.0F, 67.0F);
    require_position(wheels_of(input)[0], 34.0F, 45.0F);
    require_position(wheels_of(input)[1], 78.0F, 89.0F);
    CHECK_FALSE(buttons_of(input)[1].is_pressed());
    CHECK(input.pointer.get_x() == 78.0F);
    CHECK(input.pointer.get_y() == 89.0F);
    // The frame summary remains current SDL state, not the last queued key snapshot.
    require_modifiers(input.modifiers, false, false, true);
}

TEST_CASE(
    "SDL modifier snapshots persist across frames and advance for every key lifecycle event",
    "[sdl][input]"
) {
    PlatformInputFixture fixture;
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_A, SDL_KMOD_SHIFT);
    const auto first = fixture.poll();
    REQUIRE(keys_of(first).size() == 1);
    SDL_SetModState(SDL_KMOD_NONE);
    fixture.button(1.0F, 2.0F);
    fixture.wheel(3.0F, 4.0F);
    const auto second = fixture.poll();
    REQUIRE(buttons_of(second).size() == 1);
    REQUIRE(wheels_of(second).size() == 1);
    require_modifiers(buttons_of(second)[0].modifiers(), true);
    require_modifiers(wheels_of(second)[0].modifiers(), true);
    require_modifiers(second.modifiers);

    const auto unknown_before = fixture.window.unknown_key_count();
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_UNKNOWN, SDL_KMOD_CTRL);
    fixture.button(5.0F, 6.0F);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_A, SDL_KMOD_GUI, true);
    fixture.wheel(7.0F, 8.0F);
    fixture.key(SDL_EVENT_KEY_UP, SDLK_UNKNOWN, SDL_KMOD_NONE);
    fixture.button(9.0F, 10.0F, false);
    const auto third = fixture.poll();
    REQUIRE(keys_of(third).size() == 3);
    CHECK(keys_of(third)[0].logical_key() == foundation::LogicalKey::unknown);
    CHECK(keys_of(third)[0].physical_key() == foundation::PhysicalKey::unknown);
    CHECK(keys_of(third)[0].is_pressed());
    CHECK(keys_of(third)[1].logical_key() == foundation::LogicalKey::a);
    CHECK_FALSE(keys_of(third)[1].is_pressed());
    CHECK(keys_of(third)[1].is_press_or_repeat());
    CHECK_FALSE(keys_of(third)[2].is_press_or_repeat());
    CHECK(fixture.window.unknown_key_count() == unknown_before + 2);
    REQUIRE(buttons_of(third).size() == 2);
    REQUIRE(wheels_of(third).size() == 1);
    require_modifiers(buttons_of(third)[0].modifiers(), false, true);
    require_modifiers(wheels_of(third)[0].modifiers(), false, false, false, true);
    require_modifiers(buttons_of(third)[1].modifiers());

    fixture.wheel(11.0F, 12.0F);
    const auto fourth = fixture.poll();
    REQUIRE(wheels_of(fourth).size() == 1);
    require_modifiers(wheels_of(fourth)[0].modifiers());
}

TEST_CASE("SDL modifier snapshots synchronize at creation and focus boundaries", "[sdl][input]") {
    PlatformInputFixture fixture(SDL_KMOD_CTRL);
    SDL_SetModState(SDL_KMOD_NONE);
    fixture.button(1.0F, 2.0F);
    auto input = fixture.poll();
    REQUIRE(buttons_of(input).size() == 1);
    require_modifiers(buttons_of(input)[0].modifiers(), false, true);

    fixture.focus(SDL_EVENT_WINDOW_FOCUS_LOST, fixture.window_id);
    fixture.button(3.0F, 4.0F);
    input = fixture.poll();
    REQUIRE(buttons_of(input).size() == 1);
    require_modifiers(buttons_of(input)[0].modifiers());

    SDL_SetModState(SDL_KMOD_ALT);
    fixture.focus(SDL_EVENT_WINDOW_FOCUS_GAINED, fixture.window_id);
    fixture.wheel(5.0F, 6.0F);
    input = fixture.poll();
    REQUIRE(wheels_of(input).size() == 1);
    require_modifiers(wheels_of(input)[0].modifiers(), false, false, true);

    fixture.focus(SDL_EVENT_WINDOW_FOCUS_LOST, fixture.window_id + 1);
    fixture.button(7.0F, 8.0F);
    input = fixture.poll();
    REQUIRE(buttons_of(input).size() == 1);
    require_modifiers(buttons_of(input)[0].modifiers(), false, false, true);
}

TEST_CASE(
    "SDL logical identities follow layouts independently of physical positions",
    "[sdl][input][keys]"
) {
    PlatformInputFixture fixture;
    // Synthetic SDL layout results exercise the real poller, not a real keyboard layout device.
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_Q, SDL_KMOD_NONE, false, SDL_SCANCODE_Q);
    fixture.key(SDL_EVENT_KEY_UP, SDLK_Q, SDL_KMOD_NONE, false, SDL_SCANCODE_Q);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_A, SDL_KMOD_NONE, false, SDL_SCANCODE_Q); // French AZERTY
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_Z, SDL_KMOD_NONE, false, SDL_SCANCODE_Y); // German QWERTZ
    fixture.key(SDL_EVENT_KEY_DOWN, 0x00E9, SDL_KMOD_NONE, false, SDL_SCANCODE_2);
    fixture.key(SDL_EVENT_KEY_DOWN, 0x1F642, SDL_KMOD_NONE);
    const auto input = fixture.poll();
    REQUIRE(keys_of(input).size() == 6);
    CHECK(keys_of(input)[0].logical_key() == foundation::LogicalKey::q);
    CHECK(keys_of(input)[2].logical_key() == foundation::LogicalKey::a);
    CHECK(keys_of(input)[0].physical_key() == foundation::PhysicalKey::q);
    CHECK(keys_of(input)[2].physical_key() == foundation::PhysicalKey::q);
    CHECK(keys_of(input)[3].logical_key() == foundation::LogicalKey::z);
    CHECK(keys_of(input)[3].physical_key() == foundation::PhysicalKey::y);
    CHECK(keys_of(input)[4].logical_key() == static_cast<foundation::LogicalKey>(0x00E9));
    CHECK(keys_of(input)[4].physical_key() == foundation::PhysicalKey::num_2);
    CHECK(keys_of(input)[5].logical_key() == static_cast<foundation::LogicalKey>(0x1F642));
    CHECK(keys_of(input)[5].physical_key() == foundation::PhysicalKey::unknown);
    CHECK(fixture.window.unknown_key_count() == 0);
}

TEST_CASE(
    "SDL preserves independently unknown identities and counts only both unknown",
    "[sdl][input][keys]"
) {
    PlatformInputFixture fixture;
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_UNKNOWN, SDL_KMOD_NONE, false, SDL_SCANCODE_A);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_B, SDL_KMOD_NONE);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_UNKNOWN, SDL_KMOD_NONE);
    fixture.key(SDL_EVENT_KEY_UP, SDLK_UNKNOWN, SDL_KMOD_NONE);
    const auto input = fixture.poll();
    REQUIRE(keys_of(input).size() == 4);
    CHECK(keys_of(input)[0].logical_key() == foundation::LogicalKey::unknown);
    CHECK(keys_of(input)[0].physical_key() == foundation::PhysicalKey::a);
    CHECK(keys_of(input)[1].logical_key() == foundation::LogicalKey::b);
    CHECK(keys_of(input)[1].physical_key() == foundation::PhysicalKey::unknown);
    for (const auto index: {2, 3}) {
        CHECK(keys_of(input)[index].logical_key() == foundation::LogicalKey::unknown);
        CHECK(keys_of(input)[index].physical_key() == foundation::PhysicalKey::unknown);
    }
    CHECK(fixture.window.unknown_key_count() == 2);
}

TEST_CASE(
    "SDL preserves left right modifier identities and aggregates snapshots",
    "[sdl][input][keys]"
) {
    PlatformInputFixture fixture;
    struct ModifierCase {
        SDL_Keycode code;
        SDL_Scancode scan;
        SDL_Keymod mod;
        foundation::LogicalKey logical;
        foundation::PhysicalKey physical;
    };
    const ModifierCase cases[] {
        {SDLK_LSHIFT,
         SDL_SCANCODE_LSHIFT,
         SDL_KMOD_LSHIFT,
         foundation::LogicalKey::left_shift,
         foundation::PhysicalKey::left_shift},
        {SDLK_RSHIFT,
         SDL_SCANCODE_RSHIFT,
         SDL_KMOD_RSHIFT,
         foundation::LogicalKey::right_shift,
         foundation::PhysicalKey::right_shift},
        {SDLK_LCTRL,
         SDL_SCANCODE_LCTRL,
         SDL_KMOD_LCTRL,
         foundation::LogicalKey::left_ctrl,
         foundation::PhysicalKey::left_ctrl},
        {SDLK_RCTRL,
         SDL_SCANCODE_RCTRL,
         SDL_KMOD_RCTRL,
         foundation::LogicalKey::right_ctrl,
         foundation::PhysicalKey::right_ctrl},
        {SDLK_LALT,
         SDL_SCANCODE_LALT,
         SDL_KMOD_LALT,
         foundation::LogicalKey::left_alt,
         foundation::PhysicalKey::left_alt},
        {SDLK_RALT,
         SDL_SCANCODE_RALT,
         SDL_KMOD_RALT,
         foundation::LogicalKey::right_alt,
         foundation::PhysicalKey::right_alt},
        {SDLK_LGUI,
         SDL_SCANCODE_LGUI,
         SDL_KMOD_LGUI,
         foundation::LogicalKey::left_super,
         foundation::PhysicalKey::left_super},
        {SDLK_RGUI,
         SDL_SCANCODE_RGUI,
         SDL_KMOD_RGUI,
         foundation::LogicalKey::right_super,
         foundation::PhysicalKey::right_super},
    };
    for (const auto& item: cases) {
        fixture.key(SDL_EVENT_KEY_DOWN, item.code, item.mod, false, item.scan);
    }
    fixture.key(
        SDL_EVENT_KEY_DOWN,
        SDLK_A,
        SDL_Keymod(SDL_KMOD_CTRL | SDL_KMOD_MODE),
        false,
        SDL_SCANCODE_A
    );
    const auto input = fixture.poll();
    REQUIRE(keys_of(input).size() == 9);
    for (unsigned i = 0; i < 8; ++i) {
        CHECK(keys_of(input)[i].logical_key() == cases[i].logical);
        CHECK(keys_of(input)[i].physical_key() == cases[i].physical);
        require_modifiers(
            keys_of(input)[i].modifiers(),
            i < 2,
            i >= 2 && i < 4,
            i >= 4 && i < 6,
            i >= 6
        );
    }
    require_modifiers(keys_of(input)[8].modifiers(), false, true, true);
}

TEST_CASE(
    "SDL keypad has independent physical positions and layout supplied logical meanings",
    "[sdl][input][keys]"
) {
    PlatformInputFixture fixture;
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_RETURN, SDL_KMOD_NONE, false, SDL_SCANCODE_RETURN);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_KP_ENTER, SDL_KMOD_NONE, false, SDL_SCANCODE_KP_ENTER);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_KP_1, SDL_KMOD_NUM, false, SDL_SCANCODE_KP_1);
    fixture.key(SDL_EVENT_KEY_UP, SDLK_KP_1, SDL_KMOD_NUM, false, SDL_SCANCODE_KP_1);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_END, SDL_KMOD_NONE, false, SDL_SCANCODE_KP_1);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_KP_PLUS, SDL_KMOD_NONE, false, SDL_SCANCODE_KP_PLUS);
    const auto input = fixture.poll();
    REQUIRE(keys_of(input).size() == 6);
    CHECK(keys_of(input)[0].logical_key() == foundation::LogicalKey::enter);
    CHECK(keys_of(input)[1].logical_key() == foundation::LogicalKey::enter);
    CHECK(keys_of(input)[0].physical_key() == foundation::PhysicalKey::enter);
    CHECK(keys_of(input)[1].physical_key() == foundation::PhysicalKey::keypad_enter);
    CHECK(keys_of(input)[2].logical_key() == foundation::LogicalKey::num_1);
    CHECK(keys_of(input)[4].logical_key() == foundation::LogicalKey::end);
    CHECK(keys_of(input)[2].physical_key() == foundation::PhysicalKey::keypad_1);
    CHECK(keys_of(input)[4].physical_key() == foundation::PhysicalKey::keypad_1);
    CHECK(keys_of(input)[5].logical_key() == static_cast<foundation::LogicalKey>('+'));
    CHECK(keys_of(input)[5].physical_key() == foundation::PhysicalKey::keypad_add);
}

TEST_CASE(
    "SDL key lifecycle retains initial logical identity until release or focus loss",
    "[sdl][input][keys]"
) {
    PlatformInputFixture fixture;
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_A, SDL_KMOD_NONE, false, SDL_SCANCODE_Q);
    auto input = fixture.poll();
    REQUIRE(keys_of(input).size() == 1);
    CHECK(keys_of(input)[0].is_pressed());
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_Q, SDL_KMOD_SHIFT, true, SDL_SCANCODE_Q);
    fixture.key(SDL_EVENT_KEY_UP, SDLK_Q, SDL_KMOD_CTRL, false, SDL_SCANCODE_Q);
    input = fixture.poll();
    REQUIRE(keys_of(input).size() == 2);
    for (const auto& event: keys_of(input)) {
        CHECK(event.logical_key() == foundation::LogicalKey::a);
        CHECK(event.physical_key() == foundation::PhysicalKey::q);
        CHECK_FALSE(event.is_pressed());
    }
    CHECK(keys_of(input)[0].is_press_or_repeat());
    CHECK_FALSE(keys_of(input)[1].is_press_or_repeat());
    require_modifiers(keys_of(input)[0].modifiers(), true);
    require_modifiers(keys_of(input)[1].modifiers(), false, true);
    fixture.key(SDL_EVENT_KEY_UP, SDLK_Z, SDL_KMOD_NONE, false, SDL_SCANCODE_Q);
    input = fixture.poll();
    REQUIRE(keys_of(input).size() == 1);
    CHECK(keys_of(input)[0].logical_key() == foundation::LogicalKey::z);

    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_A, SDL_KMOD_NONE, false, SDL_SCANCODE_Q);
    (void)fixture.poll();
    fixture.focus(SDL_EVENT_WINDOW_FOCUS_LOST, fixture.window_id + 1);
    fixture.key(SDL_EVENT_KEY_DOWN, SDLK_Q, SDL_KMOD_NONE, true, SDL_SCANCODE_Q);
    input = fixture.poll();
    REQUIRE(keys_of(input).size() == 1);
    CHECK(keys_of(input)[0].logical_key() == foundation::LogicalKey::a);
    fixture.focus(SDL_EVENT_WINDOW_FOCUS_LOST, fixture.window_id);
    input = fixture.poll();
    CHECK(keys_of(input).empty()); // Focus loss clears state without synthesizing releases.
    fixture.key(SDL_EVENT_KEY_UP, SDLK_Q, SDL_KMOD_NONE, false, SDL_SCANCODE_Q);
    input = fixture.poll();
    REQUIRE(keys_of(input).size() == 1);
    CHECK(keys_of(input)[0].logical_key() == foundation::LogicalKey::q);
    CHECK_FALSE(keys_of(input)[0].is_press_or_repeat());
}
