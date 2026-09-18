//
// window_lifecycle_tests — 窗口关闭路径的约束。
//
// 背景：在 `on_frame()` 里直接调用 `close()` 会立刻销毁 render device 与原生窗口，
// 而 `tick()` 在 `on_frame()` 返回后仍要 `device_->begin_frame()` / `clear()` / 绘制，
// 于是必然解引用空指针（实测 SIGSEGV 于 nan_window.cpp 的 device_->clear）。
// 因此新增 `request_close()`：只置标志，`tick()` 在整帧（含绘制与帧末提交）结束后
// 才真正 `close()`。
//
// 本文件用一个真实窗口跑若干帧，验证：
//   * 在 on_frame 里 request_close() 能干净退出（退出码 0，不 SIGSEGV）；
//   * 关闭发生在帧末——关闭前窗口仍完成了若干帧绘制。
//

#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/theme/builtin_themes.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>

using namespace nandina;

namespace
{
    /// 在第 `close_on_frame` 帧的 on_frame 里请求关闭。
    class ClosingWindow final: public app::NanWindow {
    public:
        ClosingWindow(
            app::NanApplication& application,
            app::WindowConfig config,
            int close_on_frame
        ):
            app::NanWindow(application, std::move(config)),
            close_on_frame_(close_on_frame) {}

        [[nodiscard]] auto frames() const noexcept -> int {
            return frames_;
        }

        [[nodiscard]] auto close_requested_at_frame() const noexcept -> int {
            return requested_at_;
        }

    protected:
        void on_setup() override {
            auto column = widget::Column::create();
            column->add(widget::Label::create(graph(), "lifecycle", theme::default_theme()));
            set_content(column);
        }

        void on_frame(float /*dt*/) override {
            ++frames_;
            if (frames_ >= close_on_frame_ && requested_at_ < 0) {
                requested_at_ = frames_;
                // 这里必须是 request_close()，不能是 close()。
                request_close();
            }
        }

    private:
        int close_on_frame_ = 3;
        int frames_ = 0;
        int requested_at_ = -1;
    };
} // namespace

TEST_CASE("request_close from on_frame shuts the window down cleanly", "[app][window][lifecycle]") {
#ifdef NANDINA_SKIP_WINDOW_TESTS
    // 见 tests/meson.build：该配置的构建环境无法初始化 raylib/GL。
    SKIP("window tests are skipped in this build configuration");
#else
    app::NanApplication application(
        app::NanApplicationConfig::for_process("com.nandina.lifecycle_test")
    );
    theme::register_default_theme_families(application.theme_manager());

    ClosingWindow window {
        application,
        app::WindowConfig {
            .title = "lifecycle test",
            .width = 320,
            .height = 240,
        },
        3,
    };

    const auto exit_code = application.run(window);

    REQUIRE(exit_code == 0);
    // 关闭请求在第 3 帧发出，且之后不再有帧。
    REQUIRE(window.close_requested_at_frame() == 3);
    REQUIRE(window.frames() == 3);
#endif
}
