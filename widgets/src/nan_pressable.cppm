//
// Created by cvrain on 2026/4/28.
//

module;

#include <memory>
#include <functional>

export module nandina.widgets.pressable;

import nandina.runtime.nan_widget;
import nandina.runtime.nan_event;
import nandina.foundation.nan_types;
import nandina.reactive.state;
import nandina.reactive.event_signal;

/**
 * nandina.widgets.pressable
 *
 * Pressable — 交互状态机组件。
 *
 * 职责：
 * - 封装 hover / pressed / focused / disabled 四个交互状态
 * - 响应鼠标事件更新内部状态
 * - 提供回调接口（on_press / on_release / on_click / on_hover / on_leave）
 *
 * 状态转换：
 *   disabled → 所有事件忽略
 *   enabled + hover → hover=true
 *   hover + PointerDown → pressed=true
 *   pressed + PointerUp → pressed=false, fire on_click
 *
 * 用法：
 *   auto btn = Pressable::create()
 *       .on_click([] { log("clicked"); })
 *       .set_disabled(false);
 *   // 通过 state() 获取当前交互状态驱动子组件的视觉表现
 */
export namespace nandina::widgets {

    // ── 交互状态快照 ──────────────────────────────────
    struct PressableState {
        bool hovered{false};
        bool pressed{false};
        bool focused{false};
        bool disabled{false};

        [[nodiscard]] auto is_active() const noexcept -> bool {
            return pressed && !disabled;
        }

        [[nodiscard]] auto is_interactive() const noexcept -> bool {
            return hovered && !disabled;
        }
    };

    /**
     * Pressable — 交互状态封装
     *
     * 通过继承 NanWidget 获得事件分发能力。
     * 不负责绘制，仅供子类（如 Button）获取 PressableState 进行视觉呈现。
     */
    class Pressable : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Pressable>;
        using Callback = std::function<void()>;

        ~Pressable() override = default;

        static auto create() -> Ptr {
            return Ptr{new Pressable()};
        }

        // ── 回调注册（链式） ──────────────────────────
        auto on_click(Callback cb) -> Pressable& {
            m_on_click = std::move(cb);
            return *this;
        }

        auto on_press(Callback cb) -> Pressable& {
            m_on_press = std::move(cb);
            return *this;
        }

        auto on_release(Callback cb) -> Pressable& {
            m_on_release = std::move(cb);
            return *this;
        }

        auto on_hover(Callback cb) -> Pressable& {
            m_on_hover = std::move(cb);
            return *this;
        }

        auto on_leave(Callback cb) -> Pressable& {
            m_on_leave = std::move(cb);
            return *this;
        }

        // ── 属性 ────────────────────────────────────────
        auto set_disabled(bool disabled) -> void {
            m_disabled.set(disabled);
        }

        [[nodiscard]] auto is_disabled() const noexcept -> bool {
            return m_disabled.get();
        }

        [[nodiscard]] auto state() const noexcept -> PressableState {
            return PressableState{
                .hovered = m_hovered,
                .pressed = m_pressed_inside,
                .focused = m_focused,
                .disabled = m_disabled.get()
            };
        }

        // ── 事件信号 ──────────────────────────────────
        // 提供 Signal 版本，支持批量连接
        [[nodiscard]] auto clicked() -> reactive::EventSignal<>& {
            return m_clicked_signal;
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            for_each_child([&](runtime::NanWidget& child) {
                child.set_bounds(x, y, w, h);
            });
            return *this;
        }

    protected:
        // ── 事件处理 ──────────────────────────────────
        auto on_pointer_move(const nandina::runtime::PointerMoveEvent& /*event*/) -> bool override {
            if (m_disabled.get()) return false;

            const bool was_hovered = m_hovered;
            m_hovered = true;
            mark_dirty();

            if (!was_hovered && m_on_hover) {
                m_on_hover();
            }
            return true;
        }

        auto on_pointer_down(const nandina::runtime::PointerButtonEvent& event) -> bool override {
            if (m_disabled.get()) return false;
            if (event.button != nandina::types::PointerButton::Left) return false;

            m_pressed_inside = true;
            mark_dirty();

            if (m_on_press) {
                m_on_press();
            }
            return true;
        }

        auto on_pointer_up(const nandina::runtime::PointerButtonEvent& event) -> bool override {
            if (m_disabled.get()) return false;
            if (event.button != nandina::types::PointerButton::Left) return false;

            const bool was_pressed = m_pressed_inside;
            m_pressed_inside = false;
            mark_dirty();

            if (was_pressed && m_on_release) {
                m_on_release();
            }

            // 如果释放时鼠标仍在区域内，视为 click
            if (was_pressed && m_hovered) {
                if (m_on_click) {
                    m_on_click();
                }
                m_clicked_signal.emit();
            }

            return true;
        }

        auto on_focus_in() -> bool override {
            m_focused = true;
            mark_dirty();
            return true;
        }

        auto on_focus_out() -> bool override {
            m_focused = false;
            m_hovered = false;
            m_pressed_inside = false;
            mark_dirty();

            if (m_on_leave) {
                m_on_leave();
            }
            return true;
        }

    private:
        Pressable() = default;

        bool m_hovered{false};
        bool m_pressed_inside{false};
        bool m_focused{false};
        reactive::State<bool> m_disabled{false};

        Callback m_on_click;
        Callback m_on_press;
        Callback m_on_release;
        Callback m_on_hover;
        Callback m_on_leave;

        reactive::EventSignal<> m_clicked_signal;
    };

} // namespace nandina::widgets