#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/app/platform_window.hpp>
#include <nandina/app/system_preferences.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/theme_observation.hpp>

#if defined(_WIN32)
    #include <nandina/app/detail/windows_system_text_scale.hpp>
#elif defined(__linux__)
    #include <nandina/app/detail/linux_system_preferences.hpp>
    #include <unistd.h>
#endif

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    /// 可编程的 OS 偏好来源：`pending` 控制下一次 poll 是否报告变化。
    class FakeSource final: public app::ISystemPreferencesSource {
    public:
        FakeSource() = default;

        [[nodiscard]] auto snapshot() const -> app::SystemPreferencesSnapshot override {
            ++snapshot_calls;
            return {
                .text_scale = value,
                .appearance = appearance,
                .reduced_motion = reduced_motion,
            };
        }

        [[nodiscard]] auto poll_changes()
            -> std::optional<app::SystemPreferencesSnapshot> override {
            ++poll_calls;
            if (!pending) {
                return std::nullopt;
            }
            pending = false;
            return app::SystemPreferencesSnapshot {
                .text_scale = value,
                .appearance = appearance,
                .reduced_motion = reduced_motion,
            };
        }

        std::optional<float> value;
        std::optional<theme::ColorAppearance> appearance;
        std::optional<bool> reduced_motion;
        bool pending = false;
        mutable int snapshot_calls = 0;
        int poll_calls = 0;
    };

    class NullRenderDevice final: public render::IRenderDevice {
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

    class FakePreferencesPlatformWindow final: public app::IPlatformWindow {
    public:
        explicit FakePreferencesPlatformWindow(
            app::SystemPreferencesSnapshot initial,
            const bool fail_render_device = false
        ):
            initial_(initial),
            fail_render_device_(fail_render_device) {}

        auto create(const app::WindowConfig&) -> bool override {
            ready_ = true;
            source_ = std::make_unique<FakeSource>();
            source_->value = initial_.text_scale;
            source_->appearance = initial_.appearance;
            source_->reduced_motion = initial_.reduced_motion;
            return true;
        }

        void destroy() override {
            source_.reset();
            ready_ = false;
        }

        [[nodiscard]] auto is_ready() const -> bool override {
            return ready_;
        }
        [[nodiscard]] auto should_close() const -> bool override {
            return !ready_;
        }
        [[nodiscard]] auto screen_size() const -> foundation::NanSize override {
            return foundation::NanSize(320.0F, 240.0F);
        }
        [[nodiscard]] auto drawable_size() const -> foundation::NanSize override {
            return foundation::NanSize(320.0F, 240.0F);
        }
        [[nodiscard]] auto frame_time() const -> float override {
            return 1.0F / 60.0F;
        }
        [[nodiscard]] auto display_scale() const -> float override {
            return 1.0F;
        }
        void poll_input(app::FrameInput&) override {}
        void present() override {}
        void set_clipboard_text(std::string_view) override {}
        [[nodiscard]] auto clipboard_text() const -> std::string override {
            return {};
        }
        [[nodiscard]] auto make_render_device() -> std::unique_ptr<render::IRenderDevice> override {
            if (fail_render_device_) {
                return nullptr;
            }
            return std::make_unique<NullRenderDevice>();
        }
        [[nodiscard]] auto name() const -> std::string_view override {
            return "fake-prefs";
        }
        [[nodiscard]] auto system_preferences_source() -> app::ISystemPreferencesSource* override {
            return source_.get();
        }

        [[nodiscard]] auto raw_source() const noexcept -> FakeSource* {
            return source_.get();
        }

        void mark_not_ready() noexcept {
            ready_ = false;
        }

    private:
        app::SystemPreferencesSnapshot initial_;
        bool fail_render_device_ = false;
        std::unique_ptr<FakeSource> source_;
        bool ready_ = false;
    };

#if defined(__linux__)
    class ClosedStandardDescriptors {
    public:
        ClosedStandardDescriptors(const int first, const int second):
            first_(first),
            second_(second),
            saved_first_(dup(first)),
            saved_second_(dup(second)) {
            REQUIRE(saved_first_ >= 0);
            REQUIRE(saved_second_ >= 0);
            REQUIRE(close(first_) == 0);
            REQUIRE(close(second_) == 0);
        }

        ~ClosedStandardDescriptors() {
            (void)dup2(saved_first_, first_);
            (void)dup2(saved_second_, second_);
            (void)close(saved_first_);
            (void)close(saved_second_);
        }

        ClosedStandardDescriptors(const ClosedStandardDescriptors&) = delete;
        auto operator=(const ClosedStandardDescriptors&) -> ClosedStandardDescriptors& = delete;

    private:
        int first_;
        int second_;
        int saved_first_;
        int saved_second_;
    };
#endif

    [[nodiscard]] auto scaled_tokens(const theme::ThemeManager& manager)
        -> const theme::NanTypeScale& {
        return manager.design_system().tokens.typography.scale;
    }
} // namespace

TEST_CASE("Windows accessibility percent maps to a text scale factor", "[app][system-text-scale]") {
    const auto factor = [](const std::uint32_t percent) {
        return app::text_scale_from_percent(percent);
    };
    REQUIRE(factor(100).has_value());
    REQUIRE(*factor(100) == Catch::Approx(1.0F));
    REQUIRE(*factor(125) == Catch::Approx(1.25F));
    REQUIRE(*factor(150) == Catch::Approx(1.5F));
    REQUIRE(*factor(225) == Catch::Approx(2.25F));
    // Values outside the documented Windows range are treated as "no OS value", not clamped.
    REQUIRE_FALSE(factor(0).has_value());
    REQUIRE_FALSE(factor(99).has_value());
    REQUIRE_FALSE(factor(226).has_value());
    REQUIRE_FALSE(factor(0xFFFFFFFFU).has_value());
}

TEST_CASE(
    "Desktop factor and DPI helpers validate bounds without clamping",
    "[app][system-preferences]"
) {
    REQUIRE(app::text_scale_from_factor(0.5) == Catch::Approx(0.5F));
    REQUIRE(app::text_scale_from_factor(1.0) == Catch::Approx(1.0F));
    REQUIRE(app::text_scale_from_factor(1.25) == Catch::Approx(1.25F));
    REQUIRE(app::text_scale_from_factor(3.0) == Catch::Approx(3.0F));
    REQUIRE_FALSE(app::text_scale_from_factor(0.0).has_value());
    REQUIRE_FALSE(app::text_scale_from_factor(0.49).has_value());
    REQUIRE_FALSE(app::text_scale_from_factor(3.01).has_value());
    REQUIRE_FALSE(app::text_scale_from_factor(-1.0).has_value());
    REQUIRE_FALSE(
        app::text_scale_from_factor(std::numeric_limits<double>::quiet_NaN()).has_value()
    );
    REQUIRE_FALSE(app::text_scale_from_factor(std::numeric_limits<double>::infinity()).has_value());
    REQUIRE_FALSE(
        app::text_scale_from_factor(-std::numeric_limits<double>::infinity()).has_value()
    );

    REQUIRE(app::text_scale_from_dpi(48.0) == Catch::Approx(0.5F));
    REQUIRE(app::text_scale_from_dpi(96.0) == Catch::Approx(1.0F));
    REQUIRE(app::text_scale_from_dpi(120.0) == Catch::Approx(1.25F));
    REQUIRE(app::text_scale_from_dpi(144.0) == Catch::Approx(1.5F));
    REQUIRE(app::text_scale_from_dpi(288.0) == Catch::Approx(3.0F));
    REQUIRE_FALSE(app::text_scale_from_dpi(0.0).has_value());
    REQUIRE_FALSE(app::text_scale_from_dpi(-96.0).has_value());
    REQUIRE_FALSE(app::text_scale_from_dpi(47.0).has_value());
    REQUIRE_FALSE(app::text_scale_from_dpi(289.0).has_value());
    REQUIRE_FALSE(app::text_scale_from_dpi(std::numeric_limits<double>::quiet_NaN()).has_value());
}

TEST_CASE("application bridges OS text scale into the theme manager", "[app][system-text-scale]") {
    app::NanApplication application;
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
    REQUIRE(application.system_preferences_source() == nullptr);

    FakeSource source;
    source.value = 1.25F;
    application.attach_system_preferences(&source);
    REQUIRE(application.system_preferences_source() == &source);
    REQUIRE(application.sync_system_preferences());
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.25F));
    REQUIRE(scaled_tokens(application.theme_manager()).text_scale == Catch::Approx(1.25F));

    // Re-syncing the same snapshot must not publish another theme revision.
    const auto revision = application.theme_manager().revision();
    REQUIRE_FALSE(application.sync_system_preferences());
    REQUIRE(application.theme_manager().revision() == revision);

    // Polling only reacts to a reported change.
    REQUIRE_FALSE(application.poll_system_preferences());
    source.value = 1.5F;
    source.pending = true;
    REQUIRE(application.poll_system_preferences());
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.5F));
    REQUIRE(scaled_tokens(application.theme_manager()).text_scale == Catch::Approx(1.5F));
    REQUIRE(application.theme_manager().revision() > revision);

    // An explicit override wins over OS updates until it is cleared.
    application.set_text_scale_override(2.0F, 28.0F);
    REQUIRE(application.effective_text_scale() == Catch::Approx(2.0F));
    REQUIRE(application.text_scale_override() == Catch::Approx(2.0F));
    REQUIRE(scaled_tokens(application.theme_manager()).max_text_px == Catch::Approx(28.0F));
    source.value = 1.25F;
    source.pending = true;
    (void)application.poll_system_preferences();
    REQUIRE(application.effective_text_scale() == Catch::Approx(2.0F));

    // Clearing the override returns to the OS value; invalid overrides fall back to 1.0.
    application.set_text_scale_override(std::nullopt);
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.25F));
    application.set_text_scale_override(-2.0F);
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
    application.set_text_scale_override(std::nullopt);

    // A source with no usable value (malformed OS setting) falls back to 1.0.
    source.value = std::nullopt;
    source.pending = true;
    (void)application.poll_system_preferences();
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));

    // The first non-null source wins; detaching stops OS polling and returns to 1.0.
    FakeSource second;
    second.value = 1.75F;
    application.attach_system_preferences(&second);
    REQUIRE(application.system_preferences_source() == &source);
    application.detach_system_preferences();
    REQUIRE(application.system_preferences_source() == nullptr);
    source.value = 2.0F;
    source.pending = true;
    REQUIRE_FALSE(application.poll_system_preferences());
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
}

TEST_CASE(
    "application bridges OS appearance and reduced motion into ThemeManager",
    "[app][system-preferences]"
) {
    app::NanApplication application;
    auto& themes = application.theme_manager();
    REQUIRE(themes.appearance() == theme::ColorAppearance::light);
    REQUIRE_FALSE(themes.reduced_motion());

    FakeSource source;
    source.appearance = theme::ColorAppearance::dark;
    source.reduced_motion = true;
    application.attach_system_preferences(&source);

    const auto rev_before_sync = themes.revision();
    REQUIRE(application.sync_system_preferences());
    REQUIRE(themes.appearance() == theme::ColorAppearance::dark);
    REQUIRE(themes.reduced_motion());
    REQUIRE(
        themes.system_preferences()
        == theme::SystemPreferences {
            .appearance = theme::ColorAppearance::dark,
            .reduced_motion = true,
        }
    );
    // Appearance + reduced_motion commit together in one set_system_preferences() call.
    REQUIRE(themes.revision() == rev_before_sync + 1);

    // Re-syncing the same snapshot produces zero extra revisions.
    const auto rev_after_sync = themes.revision();
    REQUIRE_FALSE(application.sync_system_preferences());
    REQUIRE(themes.revision() == rev_after_sync);

    // Live polling updates appearance and reduced motion dynamically.
    source.appearance = theme::ColorAppearance::light;
    source.reduced_motion = false;
    source.pending = true;
    REQUIRE(application.poll_system_preferences());
    REQUIRE(themes.appearance() == theme::ColorAppearance::light);
    REQUIRE_FALSE(themes.reduced_motion());
    REQUIRE(themes.revision() == rev_after_sync + 1);

    // Explicit user light preference is preserved when OS switches to dark,
    // while ThemeManager still records the latest OS preference for later system mode.
    const auto light_id = theme::ThemeSchemeId::parse("nandina:default/light").value();
    const auto dark_id = theme::ThemeSchemeId::parse("nandina:default/dark").value();
    REQUIRE(themes.select(
        theme::AppearanceThemeSelection {light_id, dark_id, theme::ThemePreference::light}
    ));
    const auto rev_fixed_light = themes.revision();
    source.appearance = theme::ColorAppearance::dark;
    source.pending = true;
    REQUIRE(application.poll_system_preferences());
    REQUIRE(themes.appearance() == theme::ColorAppearance::light);
    REQUIRE(themes.system_preferences().appearance == theme::ColorAppearance::dark);
    REQUIRE(themes.revision() == rev_fixed_light);

    // Switching back to ThemePreference::system immediately picks up the recorded dark OS state.
    REQUIRE(themes.select(
        theme::AppearanceThemeSelection {light_id, dark_id, theme::ThemePreference::system}
    ));
    REQUIRE(themes.appearance() == theme::ColorAppearance::dark);

    // Detaching restores defaults (light + reduced_motion=false + text_scale=1.0).
    source.value = 1.5F;
    source.reduced_motion = true;
    source.pending = true;
    REQUIRE(application.poll_system_preferences());
    REQUIRE(themes.reduced_motion());
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.5F));

    // Explicit MotionPreference and FixedThemeSelection are respected while recording OS state.
    themes.set_motion_preference(theme::MotionPreference::full);
    REQUIRE_FALSE(themes.reduced_motion());
    themes.set_motion_preference(theme::MotionPreference::reduced);
    REQUIRE(themes.reduced_motion());
    themes.set_motion_preference(theme::MotionPreference::system);
    REQUIRE(themes.reduced_motion());

    REQUIRE(themes.select(theme::FixedThemeSelection {light_id}));
    const auto rev_fixed_scheme = themes.revision();
    source.appearance = theme::ColorAppearance::light;
    source.pending = true;
    REQUIRE(application.poll_system_preferences());
    REQUIRE(themes.appearance() == theme::ColorAppearance::light);
    REQUIRE(themes.revision() == rev_fixed_scheme);
    REQUIRE(themes.select(
        theme::AppearanceThemeSelection {light_id, dark_id, theme::ThemePreference::system}
    ));

    // Malformed NaN / Inf / non-positive text_scale values from custom sources fall back to 1.0.
    source.value = std::numeric_limits<float>::quiet_NaN();
    source.pending = true;
    REQUIRE(application.poll_system_preferences());
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
    source.value = std::numeric_limits<float>::infinity();
    source.pending = true;
    REQUIRE_FALSE(application.poll_system_preferences());
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));

    application.detach_system_preferences();
    REQUIRE(themes.appearance() == theme::ColorAppearance::light);
    REQUIRE_FALSE(themes.reduced_motion());
    REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
}

TEST_CASE(
    "detach_system_preferences and text-scale rollback stay safe during theme publication",
    "[app][system-preferences][lifecycle]"
) {
    SECTION("detaching during theme notification does not terminate and clears source pointer") {
        app::NanApplication application;
        auto& themes = application.theme_manager();

        FakeSource source;
        source.value = 1.5F;
        source.appearance = theme::ColorAppearance::dark;
        source.reduced_motion = true;
        application.attach_system_preferences(&source);
        REQUIRE(application.sync_system_preferences());

        struct ReentrantDetachObserver final: theme::ThemeObserver {
            app::NanApplication* app = nullptr;
            bool called = false;
            bool scale_threw = false;

            void on_theme_revision_changed(const theme::ThemeManager&) override {
                if (called) {
                    return;
                }
                called = true;
                try {
                    app->set_text_scale_override(2.0F);
                }
                catch (const std::logic_error&) {
                    scale_threw = true;
                }
                // detach_system_preferences() is noexcept and must not std::terminate()
                // when ThemeManager::require_idle() rejects nested mutations.
                app->detach_system_preferences();
            }

            void on_theme_manager_destroyed(const theme::ThemeManager&) noexcept override {}
        } observer;
        observer.app = &application;
        themes.add_observer(observer);

        source.value = 1.75F;
        source.pending = true;
        REQUIRE(application.poll_system_preferences());
        REQUIRE(observer.called);
        REQUIRE(observer.scale_threw);
        REQUIRE(application.system_preferences_source() == nullptr);
        // Rollback preserved ThemeManager coherence: subsequent sync converges cleanly.
        themes.remove_observer(observer);
        REQUIRE(application.sync_system_preferences());
        REQUIRE(application.effective_text_scale() == Catch::Approx(2.0F));
        REQUIRE(themes.text_scale() == Catch::Approx(2.0F));
        REQUIRE(themes.appearance() == theme::ColorAppearance::light);
        REQUIRE_FALSE(themes.reduced_motion());
    }

    SECTION(
        "throwing theme observer during detach does not terminate and still resets text scale"
    ) {
        app::NanApplication application;
        auto& themes = application.theme_manager();

        FakeSource source;
        source.value = 1.5F;
        source.appearance = theme::ColorAppearance::dark;
        application.attach_system_preferences(&source);
        REQUIRE(application.sync_system_preferences());

        struct ThrowingObserver final: theme::ThemeObserver {
            bool armed = false;
            void on_theme_revision_changed(const theme::ThemeManager&) override {
                if (armed) {
                    throw std::runtime_error("observer failure during detach");
                }
            }

            void on_theme_manager_destroyed(const theme::ThemeManager&) noexcept override {}
        } observer;
        themes.add_observer(observer);
        observer.armed = true;

        application.detach_system_preferences();
        themes.remove_observer(observer);

        REQUIRE(application.system_preferences_source() == nullptr);
        REQUIRE(themes.appearance() == theme::ColorAppearance::light);
        REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
        REQUIRE(themes.text_scale() == Catch::Approx(1.0F));
    }
}

TEST_CASE(
    "NanWindow attaches platform preferences on open and detaches them before destroy",
    "[app][system-preferences][window]"
) {
    SECTION("auto-attached platform source is detached on close") {
        app::NanApplication application;
        app::NanWindow window(application, {});
        auto platform =
            std::make_unique<FakePreferencesPlatformWindow>(app::SystemPreferencesSnapshot {
                .text_scale = 1.25F,
                .appearance = theme::ColorAppearance::dark,
                .reduced_motion = true,
            });
        auto* raw_platform = platform.get();
        window.set_platform_window(std::move(platform));

        window.open();
        REQUIRE(raw_platform->raw_source() != nullptr);
        REQUIRE(application.system_preferences_source() == raw_platform->raw_source());
        REQUIRE(application.effective_text_scale() == Catch::Approx(1.25F));
        REQUIRE(application.theme_manager().appearance() == theme::ColorAppearance::dark);
        REQUIRE(application.theme_manager().reduced_motion());

        window.close();
        REQUIRE(application.system_preferences_source() == nullptr);
        REQUIRE_FALSE(application.poll_system_preferences());
        REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
        REQUIRE(application.theme_manager().appearance() == theme::ColorAppearance::light);
        REQUIRE_FALSE(application.theme_manager().reduced_motion());
    }

    SECTION("app-injected source wins over platform source and survives window close") {
        app::NanApplication application;
        FakeSource app_source;
        app_source.value = 1.5F;
        app_source.appearance = theme::ColorAppearance::light;
        application.attach_system_preferences(&app_source);

        app::NanWindow window(application, {});
        window.set_platform_window(
            std::make_unique<FakePreferencesPlatformWindow>(app::SystemPreferencesSnapshot {
                .text_scale = 2.0F,
                .appearance = theme::ColorAppearance::dark,
                .reduced_motion = true,
            })
        );

        window.open();
        REQUIRE(application.system_preferences_source() == &app_source);
        REQUIRE(application.effective_text_scale() == Catch::Approx(1.5F));
        REQUIRE(application.theme_manager().appearance() == theme::ColorAppearance::light);

        window.close();
        REQUIRE(application.system_preferences_source() == &app_source);
        REQUIRE(application.effective_text_scale() == Catch::Approx(1.5F));
    }

    SECTION("failed open detaches auto-attached platform source before destroying platform") {
        app::NanApplication application;
        app::NanWindow window(application, {});
        window.set_platform_window(
            std::make_unique<FakePreferencesPlatformWindow>(
                app::SystemPreferencesSnapshot {
                    .text_scale = 1.5F,
                    .appearance = theme::ColorAppearance::dark,
                    .reduced_motion = true,
                },
                /*fail_render_device=*/true
            )
        );

        REQUIRE_THROWS_AS(window.open(), std::runtime_error);
        REQUIRE(application.system_preferences_source() == nullptr);
        REQUIRE_FALSE(application.poll_system_preferences());
        REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
        REQUIRE(application.theme_manager().appearance() == theme::ColorAppearance::light);
        REQUIRE_FALSE(application.theme_manager().reduced_motion());
    }

    SECTION("destructor detaches platform source even if platform is no longer ready") {
        app::NanApplication application;
        {
            app::NanWindow window(application, {});
            auto platform =
                std::make_unique<FakePreferencesPlatformWindow>(app::SystemPreferencesSnapshot {
                    .text_scale = 1.25F,
                    .appearance = theme::ColorAppearance::dark,
                    .reduced_motion = true,
                });
            auto* raw_platform = platform.get();
            window.set_platform_window(std::move(platform));

            window.open();
            REQUIRE(application.system_preferences_source() == raw_platform->raw_source());
            raw_platform->mark_not_ready();
        }
        REQUIRE(application.system_preferences_source() == nullptr);
        REQUIRE_FALSE(application.poll_system_preferences());
        REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
    }

    SECTION("multiple windows keep first window source until first closes without dangling") {
        app::NanApplication application;
        app::NanWindow first(application, {});
        auto first_platform =
            std::make_unique<FakePreferencesPlatformWindow>(app::SystemPreferencesSnapshot {
                .text_scale = 1.25F,
                .appearance = theme::ColorAppearance::dark,
            });
        auto* raw_first = first_platform.get();
        first.set_platform_window(std::move(first_platform));

        app::NanWindow second(application, {});
        second.set_platform_window(
            std::make_unique<FakePreferencesPlatformWindow>(app::SystemPreferencesSnapshot {
                .text_scale = 1.75F,
                .appearance = theme::ColorAppearance::light,
            })
        );

        first.open();
        second.open();
        REQUIRE(application.system_preferences_source() == raw_first->raw_source());
        REQUIRE(application.effective_text_scale() == Catch::Approx(1.25F));

        // Closing the second window must not detach the first window's active source.
        second.close();
        REQUIRE(application.system_preferences_source() == raw_first->raw_source());

        first.close();
        REQUIRE(application.system_preferences_source() == nullptr);
        REQUIRE(application.effective_text_scale() == Catch::Approx(1.0F));
    }
}

#if defined(__linux__)
TEST_CASE(
    "Linux XDG portal and desktop parsers handle valid and malformed inputs",
    "[app][linux][system-preferences]"
) {
    SECTION("XDG Desktop Portal ReadAll JSON") {
        constexpr std::string_view gnome_json = R"json({
            "type": "a{sa{sv}}",
            "data": [{
                "org.freedesktop.appearance": {
                    "color-scheme": {"type": "u", "data": 1},
                    "reduced-motion": {"type": "u", "data": 0}
                },
                "org.gnome.desktop.interface": {
                    "text-scaling-factor": {"type": "d", "data": 1.25},
                    "enable-animations": {"type": "b", "data": true},
                    "color-scheme": {"type": "s", "data": "prefer-dark"}
                }
            }]
        })json";
        const auto parsed = app::detail::parse_xdg_portal_settings_json(gnome_json);
        REQUIRE(parsed.appearance == theme::ColorAppearance::dark);
        REQUIRE(parsed.reduced_motion == false);
        REQUIRE(parsed.text_scale.has_value());
        REQUIRE(*parsed.text_scale == Catch::Approx(1.25F));

        // Legacy portal reporting reduced-motion=0 while GNOME explicitly disables animations.
        constexpr std::string_view disabled_anim_json = R"json({
            "type": "a{sa{sv}}",
            "data": [{
                "org.freedesktop.appearance": {
                    "color-scheme": {"type": "u", "data": 2},
                    "reduced-motion": {"type": "u", "data": 0}
                },
                "org.gnome.desktop.interface": {
                    "text-scaling-factor": {"type": "d", "data": 5.0},
                    "enable-animations": {"type": "b", "data": false}
                }
            }]
        })json";
        const auto disabled_anim = app::detail::parse_xdg_portal_settings_json(disabled_anim_json);
        REQUIRE(disabled_anim.appearance == theme::ColorAppearance::light);
        REQUIRE(disabled_anim.reduced_motion == true);
        REQUIRE_FALSE(disabled_anim.text_scale.has_value());

        // color-scheme=0 (no preference) falls back to GNOME string color-scheme.
        constexpr std::string_view fallback_scheme_json = R"json({
            "type": "a{sa{sv}}",
            "data": [{
                "org.freedesktop.appearance": {
                    "color-scheme": {"type": "u", "data": 0},
                    "reduced-motion": {"type": "u", "data": 1}
                },
                "org.gnome.desktop.interface": {
                    "color-scheme": {"type": "s", "data": "prefer-dark"}
                }
            }]
        })json";
        const auto fallback_scheme =
            app::detail::parse_xdg_portal_settings_json(fallback_scheme_json);
        REQUIRE(fallback_scheme.appearance == theme::ColorAppearance::dark);
        REQUIRE(fallback_scheme.reduced_motion == true);

        // Integer text-scaling-factor, nested variants, and unknown enum values.
        constexpr std::string_view edge_portal_json = R"json({
            "type": "a{sa{sv}}",
            "data": [{
                "org.freedesktop.appearance": {
                    "color-scheme": {"type": "v", "data": {"type": "u", "data": 99}},
                    "reduced-motion": {"type": "u", "data": 99}
                },
                "org.gnome.desktop.interface": {
                    "text-scaling-factor": {"type": "d", "data": 2}
                }
            }]
        })json";
        const auto edge_parsed = app::detail::parse_xdg_portal_settings_json(edge_portal_json);
        REQUIRE_FALSE(edge_parsed.appearance.has_value());
        REQUIRE_FALSE(edge_parsed.reduced_motion.has_value());
        REQUIRE(edge_parsed.text_scale == Catch::Approx(2.0F));

        REQUIRE(
            app::detail::parse_xdg_portal_settings_json("not json")
            == app::SystemPreferencesSnapshot {}
        );
        REQUIRE(
            app::detail::parse_xdg_portal_settings_json("{}") == app::SystemPreferencesSnapshot {}
        );
        REQUIRE(
            app::detail::parse_xdg_portal_settings_json(R"({"data":[]})")
            == app::SystemPreferencesSnapshot {}
        );
        REQUIRE(
            app::detail::parse_xdg_portal_settings_json(R"({"data":[null]})")
            == app::SystemPreferencesSnapshot {}
        );
    }

    SECTION("gsettings outputs") {
        REQUIRE(app::detail::parse_gsettings_text_scale("1.25\n") == Catch::Approx(1.25F));
        REQUIRE(app::detail::parse_gsettings_text_scale(" 0.75 ") == Catch::Approx(0.75F));
        REQUIRE_FALSE(app::detail::parse_gsettings_text_scale("0.2").has_value());
        REQUIRE_FALSE(app::detail::parse_gsettings_text_scale("3.5").has_value());
        REQUIRE_FALSE(app::detail::parse_gsettings_text_scale("1.25abc").has_value());
        REQUIRE_FALSE(app::detail::parse_gsettings_text_scale("").has_value());

        REQUIRE(
            app::detail::parse_gsettings_color_scheme("'prefer-dark'\n")
            == theme::ColorAppearance::dark
        );
        REQUIRE(
            app::detail::parse_gsettings_color_scheme("'prefer-light'")
            == theme::ColorAppearance::light
        );
        REQUIRE(
            app::detail::parse_gsettings_color_scheme("prefer-dark") == theme::ColorAppearance::dark
        );
        REQUIRE_FALSE(app::detail::parse_gsettings_color_scheme("'default'\n").has_value());

        REQUIRE(app::detail::parse_gsettings_enable_animations("true\n") == false);
        REQUIRE(app::detail::parse_gsettings_enable_animations("false\n") == true);
        REQUIRE_FALSE(app::detail::parse_gsettings_enable_animations("maybe").has_value());
    }

    SECTION("KDE kcmfonts and kdeglobals INI") {
        constexpr std::string_view kcmfonts_120 = "[General]\nforceFontDPI=120\n";
        REQUIRE(app::detail::parse_kde_kcmfonts(kcmfonts_120) == Catch::Approx(1.25F));
        REQUIRE_FALSE(app::detail::parse_kde_kcmfonts("[General]\nforceFontDPI=0\n").has_value());
        REQUIRE_FALSE(app::detail::parse_kde_kcmfonts("[Other]\nforceFontDPI=120\n").has_value());

        // Last assignment wins across duplicate keys and repeated sections; UTF-8 BOM is tolerated.
        constexpr std::string_view kcmfonts_dup =
            "\xEF\xBB\xBF[General]\r\nforceFontDPI=96\r\nforceFontDPI=144\r\n";
        REQUIRE(app::detail::parse_kde_kcmfonts(kcmfonts_dup) == Catch::Approx(1.5F));
        constexpr std::string_view kcmfonts_reset =
            "[General]\nforceFontDPI=144\n[Other]\nfoo=1\n[General]\nforceFontDPI=0\n";
        REQUIRE_FALSE(app::detail::parse_kde_kcmfonts(kcmfonts_reset).has_value());

        REQUIRE(
            app::detail::parse_kde_globals_reduced_motion("[KDE]\nAnimationDurationFactor=0\n")
            == true
        );
        REQUIRE(
            app::detail::parse_kde_globals_reduced_motion("[KDE]\nAnimationDurationFactor=1.0\n")
            == false
        );
        REQUIRE(
            app::detail::parse_kde_globals_reduced_motion(
                "[KDE]\nAnimationDurationFactor=1.0\nAnimationDurationFactor=0\n"
            )
            == true
        );
        REQUIRE_FALSE(
            app::detail::parse_kde_globals_reduced_motion("[KDE]\nAnimationDurationFactor=-1\n")
                .has_value()
        );
    }

    SECTION("GTK settings.ini") {
        constexpr std::string_view gtk_ini =
            "[Settings]\n"
            "gtk-application-prefer-dark-theme=1\n"
            "gtk-enable-animations=false\n"
            "gtk-xft-dpi=122880\n";
        const auto parsed = app::detail::parse_gtk_settings_ini(gtk_ini);
        REQUIRE(parsed.appearance == theme::ColorAppearance::dark);
        REQUIRE(parsed.reduced_motion == true);
        REQUIRE(parsed.text_scale == Catch::Approx(1.25F));

        // Duplicate keys and repeated [Settings] blocks use last-assignment-wins semantics.
        constexpr std::string_view gtk_override =
            "[Settings]\n"
            "gtk-application-prefer-dark-theme=1\n"
            "gtk-enable-animations=false\n"
            "gtk-xft-dpi=147456\n"
            "[Other]\n"
            "gtk-xft-dpi=98304\n"
            "[Settings]\n"
            "gtk-application-prefer-dark-theme=0\n"
            "gtk-enable-animations=true\n"
            "gtk-xft-dpi=122880\n";
        const auto parsed_override = app::detail::parse_gtk_settings_ini(gtk_override);
        REQUIRE_FALSE(parsed_override.appearance.has_value());
        REQUIRE(parsed_override.reduced_motion == false);
        REQUIRE(parsed_override.text_scale == Catch::Approx(1.25F));

        // prefer-dark-theme=0 means "do not force dark", not explicit light; -1 DPI means unset.
        constexpr std::string_view gtk_default =
            "[Settings]\n"
            "gtk-application-prefer-dark-theme=0\n"
            "gtk-enable-animations=true\n"
            "gtk-xft-dpi=-1\n";
        const auto parsed_default = app::detail::parse_gtk_settings_ini(gtk_default);
        REQUIRE_FALSE(parsed_default.appearance.has_value());
        REQUIRE(parsed_default.reduced_motion == false);
        REQUIRE_FALSE(parsed_default.text_scale.has_value());
    }
}

TEST_CASE(
    "Linux bounded command reader enforces timeout and exit status",
    "[app][linux][system-preferences]"
) {
    REQUIRE(
        app::detail::read_command_output_with_timeout(
            {"/bin/sh", "-c", "printf 'ok-value'"},
            std::chrono::milliseconds(500)
        )
        == "ok-value"
    );
    REQUIRE_FALSE(
        app::detail::read_command_output_with_timeout(
            {"/bin/sh", "-c", "exit 3"},
            std::chrono::milliseconds(500)
        )
            .has_value()
    );

    const auto start = std::chrono::steady_clock::now();
    const auto timed_out = app::detail::read_command_output_with_timeout(
        {"/bin/sh", "-c", "exec sleep 5"},
        std::chrono::milliseconds(50)
    );
    const auto elapsed = std::chrono::steady_clock::now() - start;
    REQUIRE_FALSE(timed_out.has_value());
    REQUIRE(elapsed < std::chrono::milliseconds(1000));

    // Child closing stdout early then hanging must still be killed by wait_until_deadline.
    const auto eof_hang_start = std::chrono::steady_clock::now();
    const auto eof_hung = app::detail::read_command_output_with_timeout(
        {"/bin/sh", "-c", "printf partial; exec >&-; exec sleep 5"},
        std::chrono::milliseconds(50)
    );
    const auto eof_hang_elapsed = std::chrono::steady_clock::now() - eof_hang_start;
    REQUIRE_FALSE(eof_hung.has_value());
    REQUIRE(eof_hang_elapsed < std::chrono::milliseconds(1000));

    // Output exceeding the 256 KiB cap is rejected cleanly.
    REQUIRE_FALSE(
        app::detail::read_command_output_with_timeout(
            {"/bin/sh", "-c", "head -c 300000 /dev/zero | tr '\\0' 'a'"},
            std::chrono::milliseconds(500)
        )
            .has_value()
    );

    // Closed standard descriptors (e.g. GUI launcher) are repaired without corrupting the pipe.
    std::optional<std::string> repaired;
    {
        ClosedStandardDescriptors descriptors(STDIN_FILENO, STDERR_FILENO);
        repaired = app::detail::read_command_output_with_timeout(
            {"/bin/sh", "-c", "test -r /dev/stdin && test -w /dev/stderr && printf repaired"},
            std::chrono::milliseconds(500)
        );
    }
    REQUIRE(repaired == "repaired");
}

TEST_CASE(
    "LinuxSystemPreferencesSource throttles polls and skips queries when config stamps are unchanged",
    "[app][linux][system-preferences]"
) {
    int query_count = 0;
    app::SystemPreferencesSnapshot live {
        .text_scale = 1.0F,
        .appearance = theme::ColorAppearance::light,
        .reduced_motion = false,
    };
    std::vector<app::detail::LinuxConfigFileStamp> stamps {
        {.exists = true, .device = 1, .inode = 10, .size = 100, .mtime_sec = 1000, .mtime_nsec = 0}
    };
    auto now = std::chrono::steady_clock::time_point(std::chrono::seconds(10));

    app::detail::LinuxSystemPreferencesSource source(
        [&] {
            ++query_count;
            return live;
        },
        [&] { return stamps; },
        [&] { return now; },
        std::chrono::milliseconds(500)
    );

    const auto initial = source.snapshot();
    REQUIRE(query_count == 1);
    REQUIRE(initial == live);

    // Before throttle expires: no query, no change.
    now += std::chrono::milliseconds(200);
    stamps[0].mtime_sec = 1001;
    REQUIRE_FALSE(source.poll_changes().has_value());
    REQUIRE(query_count == 1);

    // Reset stamp back to initial and expire throttle: stat-gating skips external query.
    stamps[0].mtime_sec = 1000;
    now += std::chrono::milliseconds(400);
    REQUIRE_FALSE(source.poll_changes().has_value());
    REQUIRE(query_count == 1);

    // Stamp changes with same snapshot values: one query, no change event emitted.
    now += std::chrono::milliseconds(500);
    stamps[0].mtime_sec = 1002;
    REQUIRE_FALSE(source.poll_changes().has_value());
    REQUIRE(query_count == 2);

    // Stamp changes with new text scale + dark mode: emits updated snapshot once.
    now += std::chrono::milliseconds(500);
    stamps[0].mtime_sec = 1003;
    live.text_scale = 1.25F;
    live.appearance = theme::ColorAppearance::dark;
    const auto changed = source.poll_changes();
    REQUIRE(query_count == 3);
    REQUIRE(changed.has_value());
    REQUIRE(*changed == live);

    // Unset HOME/XDG_CONFIG_HOME yields an empty watch stamp list; stat-gating must still
    // suppress repeated external queries instead of spawning subprocesses every interval.
    int empty_watch_queries = 0;
    auto empty_now = std::chrono::steady_clock::time_point(std::chrono::seconds(10));
    app::detail::LinuxSystemPreferencesSource empty_watch_source(
        [&] {
            ++empty_watch_queries;
            return app::SystemPreferencesSnapshot {};
        },
        [] { return std::vector<app::detail::LinuxConfigFileStamp> {}; },
        [&] { return empty_now; },
        std::chrono::milliseconds(500)
    );
    (void)empty_watch_source.snapshot();
    REQUIRE(empty_watch_queries == 1);
    empty_now += std::chrono::milliseconds(600);
    REQUIRE_FALSE(empty_watch_source.poll_changes().has_value());
    REQUIRE(empty_watch_queries == 1);
}

TEST_CASE(
    "Linux live desktop source returns plausible preferences",
    "[app][linux][system-preferences]"
) {
    app::detail::LinuxSystemPreferencesSource source;
    const auto snapshot = source.snapshot();
    if (snapshot.text_scale.has_value()) {
        REQUIRE(*snapshot.text_scale >= 0.5F);
        REQUIRE(*snapshot.text_scale <= 3.0F);
    }
    // Immediate poll right after snapshot is throttled and must return nullopt.
    REQUIRE_FALSE(source.poll_changes().has_value());

    app::NanApplication application;
    application.attach_system_preferences(&source);
    (void)application.sync_system_preferences();
    REQUIRE(
        application.effective_text_scale() == Catch::Approx(snapshot.text_scale.value_or(1.0F))
    );
    REQUIRE(
        application.theme_manager().system_preferences().appearance
        == snapshot.appearance.value_or(theme::ColorAppearance::light)
    );
    REQUIRE(
        application.theme_manager().system_preferences().reduced_motion
        == snapshot.reduced_motion.value_or(false)
    );

    if (const char* expected_scale = std::getenv("NANDINA_EXPECT_TEXT_SCALE");
        expected_scale != nullptr)
    {
        REQUIRE(application.effective_text_scale() == Catch::Approx(std::stof(expected_scale)));
    }
    if (const char* expected_appearance = std::getenv("NANDINA_EXPECT_APPEARANCE");
        expected_appearance != nullptr)
    {
        const auto wanted = std::string_view(expected_appearance) == "dark"
            ? theme::ColorAppearance::dark
            : theme::ColorAppearance::light;
        REQUIRE(application.theme_manager().appearance() == wanted);
    }
    if (const char* expected_motion = std::getenv("NANDINA_EXPECT_REDUCED_MOTION");
        expected_motion != nullptr)
    {
        REQUIRE(
            application.theme_manager().reduced_motion()
            == (std::string_view(expected_motion) == "1")
        );
    }
}
#endif

#if defined(_WIN32)
TEST_CASE("Windows OS text-scale source reads a plausible value", "[app][windows][text-scale]") {
    // This exercises the real registry read path. The value depends on the machine's
    // accessibility setting, so assert the documented range instead of an exact factor.
    app::detail::WindowsSystemTextScaleSource source;
    const auto snapshot = source.snapshot();
    if (snapshot.text_scale.has_value()) {
        REQUIRE(*snapshot.text_scale >= 1.0F);
        REQUIRE(*snapshot.text_scale <= 2.25F);
    }
    // The first poll either reports the initial value or (when absent) nothing; both are valid.
    (void)source.poll_changes();
    const auto second = source.poll_changes();
    if (second.has_value() && second->text_scale.has_value()) {
        REQUIRE(*second->text_scale >= 1.0F);
        REQUIRE(*second->text_scale <= 2.25F);
    }
}

TEST_CASE(
    "the real Windows source drives the application text scale",
    "[app][windows][text-scale]"
) {
    app::detail::WindowsSystemTextScaleSource source;
    const auto snapshot = source.snapshot();

    app::NanApplication application;
    application.attach_system_preferences(&source);
    REQUIRE(application.sync_system_preferences());
    const float expected = snapshot.text_scale.value_or(1.0F);
    REQUIRE(application.effective_text_scale() == Catch::Approx(expected));
    REQUIRE(scaled_tokens(application.theme_manager()).text_scale == Catch::Approx(expected));

    // Manual-acceptance hook: change the Windows accessibility text size (e.g. 150%), then run
    // with NANDINA_EXPECT_TEXT_SCALE=1.5 to prove the OS -> source -> app -> theme chain.
    if (const char* expected_env = std::getenv("NANDINA_EXPECT_TEXT_SCALE");
        expected_env != nullptr)
    {
        REQUIRE(application.effective_text_scale() == Catch::Approx(std::stof(expected_env)));
    }
}
#endif
