module;

#include <memory>
#include <thorvg-1/thorvg.h>
#include <variant>
#include <vector>

export module nandina.runtime.nan_widget;

export import nandina.runtime.nan_event;

export namespace nandina::runtime {
    /**
     * @brief NanWidget — 所有 UI 元素的基类 (M1)
     */
    class NanWidget {
    public:
        using Ptr = std::unique_ptr<NanWidget>;

        NanWidget()          = default; // Avoid = default for now to dodge GCC module bugs
        virtual ~NanWidget() = default;

        // ── 空间属性 ──────────────────────────────────────────
        [[nodiscard]] auto x() const noexcept -> float {
            return m_x;
        }

        [[nodiscard]] auto y() const noexcept -> float {
            return m_y;
        }

        [[nodiscard]] auto width() const noexcept -> float {
            return m_width;
        }

        [[nodiscard]] auto height() const noexcept -> float {
            return m_height;
        }

        auto set_position(const float x, const float y) noexcept -> void {
            m_x = x;
            m_y = y;
        }

        auto set_size(const float w, const float h) noexcept -> void {
            m_width  = w;
            m_height = h;
        }

        // ── 组合能力 ──────────────────────────────────────────
        auto add_child(Ptr child) -> NanWidget* {
            if (!child)
                return nullptr;
            auto* ptr = child.get();
            m_children.push_back(std::move(child));
            return ptr;
        }

        auto clear_children() -> void {
            m_children.clear();
        }

        // ── 绘制 ──────────────────────────────────────────────
        virtual void draw(tvg::SwCanvas& canvas) {
            on_draw(canvas);
            for (const auto& child : m_children) {
                child->draw(canvas);
            }
        }

        // ── 事件分发（Issue 011/015）─────────────────────────
        // 将 Event 分发给对应的 on_xxx 虚方法。
        // 返回 true 表示事件已被消费。
        virtual auto dispatch_event(const Event& ev) -> bool {
            switch (event_type(ev)) {
            case EventType::PointerMove:
                return on_pointer_move(std::get<PointerMoveEvent>(ev));
            case EventType::PointerDown:
                return on_pointer_down(std::get<PointerButtonEvent>(ev));
            case EventType::PointerUp:
                return on_pointer_up(std::get<PointerButtonEvent>(ev));
            case EventType::PointerClick:
                return on_pointer_click(std::get<PointerButtonEvent>(ev));
            case EventType::PointerWheel:
                return on_pointer_wheel(std::get<PointerWheelEvent>(ev));
            case EventType::KeyDown:
                return on_key_down(std::get<KeyEvent>(ev));
            case EventType::KeyUp:
                return on_key_up(std::get<KeyEvent>(ev));
            case EventType::TextInput:
                return on_text_input(std::get<TextInputEvent>(ev));
            case EventType::FocusIn:
                return on_focus_in();
            case EventType::FocusOut:
                return on_focus_out();
            case EventType::WindowResize:
                return on_window_resize(std::get<WindowResizeEvent>(ev));
            case EventType::WindowClose:
                return on_window_close();
            }
            return false;
        }

        // ── 子事件冒泡 ────────────────────────────────────────
        // 将事件分发给所有子节点，直到某个子节点消费。
        // 返回 true 表示事件被某个子节点消费。
        [[nodiscard]] auto bubble_event(const Event& ev) const -> bool {
            for (auto& child : m_children) {
                if (child->dispatch_event(ev)) {
                    return true;
                }
            }
            return false;
        }

    protected:
        virtual void on_draw(tvg::SwCanvas& /*canvas*/) {
        }

        // ── 事件处理虚方法（默认不做响应）────────────────────
        virtual auto on_pointer_move(const PointerMoveEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_pointer_down(const PointerButtonEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_pointer_up(const PointerButtonEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_pointer_click(const PointerButtonEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_pointer_wheel(const PointerWheelEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_key_down(const KeyEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_key_up(const KeyEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_text_input(const TextInputEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_focus_in() -> bool {
            return false;
        }

        virtual auto on_focus_out() -> bool {
            return false;
        }

        virtual auto on_window_resize(const WindowResizeEvent& /*event*/) -> bool {
            return false;
        }

        virtual auto on_window_close() -> bool {
            return false;
        }

    private:
        float m_x{0.0f};
        float m_y{0.0f};
        float m_width{0.0f};
        float m_height{0.0f};

        std::vector<Ptr> m_children;
    };
} // namespace nandina::runtime
