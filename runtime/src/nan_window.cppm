module;

// ============================================================
// 全局模块片段
// ThorVG 在接口层可见——on_frame 回调参数类型涉及 tvg::SwCanvas。
// 消费者如需在回调中调用 ThorVG API，须在其 GMF 中 include ThorVG。
// SDL 类型完全隐藏在实现单元 nan_window.cpp 中。
// ============================================================
#include <thorvg-1/thorvg.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

export module nandina.runtime.nan_window;

// ============================================================
// 导出接口
// ============================================================
export namespace nandina::runtime {
    enum class PointerButton : std::uint8_t {
        Unknown = 0,
        Left = 1,
        Middle = 2,
        Right = 3,
        X1 = 4,
        X2 = 5,
    };

    struct PointerMoveEvent {
        double x{0.0};
        double y{0.0};
        double delta_x{0.0};
        double delta_y{0.0};
    };

    struct PointerButtonEvent {
        PointerButton button{PointerButton::Unknown};
        double x{0.0};
        double y{0.0};
        bool is_repeat{false};
    };

    struct PointerWheelEvent {
        double x{0.0};
        double y{0.0};
    };

    struct KeyEvent {
        std::int32_t key_code{0};
        bool is_repeat{false};
    };


    inline constexpr int kDefaultWindowWidth = 1280;
    inline constexpr int kDefaultWindowHeight = 720;

    // ──────────────────────────────────────────────────────────
    // NanWindow — 可继承窗口基类（SDL3 + ThorVG 软件渲染）
    //
    // 渲染管线：
    //   on_draw()（用户绘制 ThorVG 图形）
    //     → tvg::SwCanvas（CPU 矢量光栅化）
    //     → pixel_buffer（ARGB8888 线性内存）
    //     → SDL_Texture（STREAMING，CPU→GPU 上传通道）
    //     → SDL_Renderer → SDL_Window（屏幕呈现）
    //
    // 使用模式 A（推荐，Godot 风格）：
    //   class DemoWindow final : public NanWindow {
    //   public:
    //       using NanWindow::NanWindow;
    //   protected:
    //       void on_draw(tvg::SwCanvas& canvas) override { /* 添加 ThorVG 图元 */ }
    //   };
    //   DemoWindow win{NanWindow::Builder{}.set_title("Demo").set_size(1280, 720).to_config()};
    //   win.run();
    //
    // 使用模式 B（手动驱动）：
    //   while (!window.should_close()) {
    //       window.poll_events();
    //       window.present_frame();
    //   }
    //
    // 设计约束：
    //   - 接口层零 SDL 类型（PIMPL 隔离，便于未来后端替换）
    //   - ThorVG 类型仅出现在 on_draw 签名，不侵入其余接口
    //   - Builder 用于配置，继承类通过 Config 构造
    //   - 对应 Issue 009 / 010 / 017（M1）
    // ──────────────────────────────────────────────────────────
    class NanWindow {
    public:
        struct Config {
            std::string title = "NandinaUI";
            int width = kDefaultWindowWidth;
            int height = kDefaultWindowHeight;
            bool resizable = true;
            bool high_dpi = true;
        };

        // ── Builder ───────────────────────────────────────────
        // 链式参数配置，可转为 Config 或直接 build()。
        class Builder {
        public:
            [[nodiscard]] auto set_title(std::string_view title) noexcept -> Builder&;

            [[nodiscard]] auto set_width(int width) noexcept -> Builder&;

            [[nodiscard]] auto set_height(int height) noexcept -> Builder&;

            // 同时设置宽高的便捷方法
            [[nodiscard]] auto set_size(int width, int height) noexcept -> Builder&;

            [[nodiscard]] auto set_resizable(bool resizable) noexcept -> Builder&;

            [[nodiscard]] auto set_high_dpi(bool high_dpi) noexcept -> Builder&;

            // Getters（供调试或测试读取配置）
            [[nodiscard]] auto get_title() const noexcept -> std::string_view;

            [[nodiscard]] auto get_width() const noexcept -> int;

            [[nodiscard]] auto get_height() const noexcept -> int;

            [[nodiscard]] auto to_config() const -> Config;

            // 消费 Builder 并构造 NanWindow，可能抛出 std::runtime_error
            [[nodiscard]] auto build() -> NanWindow;

        private:
            std::string m_title = "NandinaUI";
            int m_width = kDefaultWindowWidth;
            int m_height = kDefaultWindowHeight;
            bool m_resizable = true;
            bool m_high_dpi = true;
        };

        // ── 拷贝/移动语义 ─────────────────────────────────────
        NanWindow(const NanWindow &) = delete;

        NanWindow& operator=(const NanWindow &) = delete;

        NanWindow(NanWindow &&) noexcept;

        NanWindow& operator=(NanWindow &&) noexcept;

        virtual ~NanWindow();

        // ── 属性访问 ──────────────────────────────────────────
        [[nodiscard]] auto title() const noexcept -> std::string_view;

        [[nodiscard]] auto width() const noexcept -> int;

        [[nodiscard]] auto height() const noexcept -> int;

        [[nodiscard]] auto dpi_scale() const noexcept -> float;

        // ── 生命周期状态 ──────────────────────────────────────
        [[nodiscard]] auto should_close() const noexcept -> bool;

        // 主动请求关闭（下一次 poll_events / run 检查时生效）
        auto request_close() noexcept -> void;

        // ── 事件处理（Issue 010 基础通路）────────────────────
        // 消费平台事件队列，翻译并触发对应回调。
        // 返回 true 表示已收到退出信号（should_close 同步置 true）。
        // 应在每帧主循环中首先调用。
        auto poll_events() -> bool;

        // ── 帧渲染（Issue 017 最小渲染闭环）─────────────────
        // 执行完整的单帧渲染流水线：
        //   1. 清空上一帧 ThorVG 绘制指令
        //   2. 调用 on_frame（用户向 canvas 添加图元）
        //   3. ThorVG 光栅化 → pixel_buffer
        //   4. 上传 SDL_Texture → SDL_RenderPresent
        auto present_frame() -> void;

        // ── 主循环阻塞接口 ───────────────────────────────────
        // 等价于：while (!poll_events()) { present_frame(); }
        // 推荐在不需要自定义调度时使用。
        auto run() -> void;

    protected:
        // 继承类使用该构造函数进行初始化
        explicit NanWindow(const Config &config);

        // 生命周期钩子（Godot-like）
        // 仅在 run() 内首次循环前调用一次。
        virtual auto on_ready() -> void;

        // 每帧调用一次，delta_seconds 为上一帧耗时。
        virtual auto on_update(double delta_seconds) -> void;

        // 每帧绘制调用，canvas 已清空。
        virtual auto on_draw(tvg::SwCanvas &canvas) -> void;

        // 窗口逻辑尺寸变化后调用。
        virtual auto on_resize(int new_width, int new_height) -> void;

        // 收到关闭请求时调用。
        virtual auto on_close_requested() -> void;

        // 鼠标移动事件。
        virtual auto on_pointer_move(const PointerMoveEvent &event) -> void;

        // 鼠标按下事件。
        virtual auto on_pointer_down(const PointerButtonEvent &event) -> void;

        // 鼠标抬起事件。
        virtual auto on_pointer_up(const PointerButtonEvent &event) -> void;

        // 鼠标滚轮事件。
        virtual auto on_pointer_wheel(const PointerWheelEvent &event) -> void;

        // 键盘按下事件。
        virtual auto on_key_down(const KeyEvent &event) -> void;

        // 键盘抬起事件。
        virtual auto on_key_up(const KeyEvent &event) -> void;

        // 文本输入事件（IME 结果文本）。
        virtual auto on_text_input(std::string_view text) -> void;

    private:
        explicit NanWindow(std::string_view title, int width, int height, bool resizable, bool high_dpi);

        // PIMPL：SDL 类型完全封装于此，不泄漏至接口层
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace nandina::runtime