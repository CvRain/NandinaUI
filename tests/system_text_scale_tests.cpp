#include <nandina/app/nan_application.hpp>
#include <nandina/app/system_preferences.hpp>
#include <nandina/theme/design_system.hpp>

#if defined(_WIN32)
    #include <nandina/app/detail/windows_system_text_scale.hpp>
#endif

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>

using namespace nandina;

namespace
{
    /// 可编程的 OS 偏好来源：`pending` 控制下一次 poll 是否报告变化。
    class FakeSource final: public app::ISystemPreferencesSource {
    public:
        FakeSource() = default;

        [[nodiscard]] auto snapshot() const -> app::SystemPreferencesSnapshot override {
            ++snapshot_calls;
            return {.text_scale = value};
        }

        [[nodiscard]] auto poll_changes()
            -> std::optional<app::SystemPreferencesSnapshot> override {
            ++poll_calls;
            if (!pending) {
                return std::nullopt;
            }
            pending = false;
            return app::SystemPreferencesSnapshot {.text_scale = value};
        }

        std::optional<float> value;
        bool pending = false;
        mutable int snapshot_calls = 0;
        int poll_calls = 0;
    };

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
