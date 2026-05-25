//
// nan_button.cppm — Button 组件接口
//
// shadcn/ui 风格按钮：variant + size + icon + event 全覆盖。
//
// 内部结构：
//   Button (Surface)      ← bg/corner/padding, 直接处理 hover/click
//     └── Row (align_items=center)
//           ├── Icon (optional, left)
//           ├── Label (text)
//           └── Icon (optional, right)
//
// 使用示例：
//   button("OK").variant(ButtonVariant::default).size(ButtonSize::lg)
//   button("Save").icon_left(IconType::Check).on_click([] { ... })
//   button().icon(IconType::ArrowUp).size(ButtonSize::icon)
//

module;

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <thorvg-1/thorvg.h>

export module nandina.widgets.button;

import nandina.widgets.surface;
import nandina.widgets.label;
import nandina.widgets.icon;
import nandina.layout.flex_widgets;
import nandina.layout.container;
import nandina.foundation.nan_insets;
import nandina.foundation.color;
import nandina.reactive.event_signal;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.text.nan_font;
import nandina.foundation.nan_types;
import nandina.theme.nan_style;  // for ButtonVariant / ButtonSize

export namespace nandina::widgets {

    // ── 导入 theme 层的类型别名 ──────────────────────────
    using ButtonVariant = nandina::theme::ButtonVariant;
    using ButtonSize    = nandina::theme::ButtonSize;

    // ═══════════════════════════════════════════════════════════
    // § Button — 完整按钮组件
    // ═══════════════════════════════════════════════════════════

    class Button : public Surface {
    public:
        using Ptr      = std::unique_ptr<Button>;
        using Callback = std::function<void()>;

        ~Button() override = default;

        static auto create() -> Ptr;

        // ── 文本 ──────────────────────────────────────────
        auto set_text(std::string_view text) -> Button&;

        [[nodiscard]] auto text() const noexcept -> const std::string&;

        // ── 变体 / 尺寸 ──────────────────────────────────
        auto variant(ButtonVariant v) -> Button&;

        [[nodiscard]] auto variant() const noexcept -> ButtonVariant;

        auto size(ButtonSize s) -> Button&;

        [[nodiscard]] auto size() const noexcept -> ButtonSize;

        // ── 图标 ──────────────────────────────────────────
        auto icon(IconType type) -> Button&;

        auto icon_left(IconType type) -> Button&;

        auto icon_right(IconType type) -> Button&;

        [[nodiscard]] auto icon_type() const noexcept -> std::optional<IconType>;

        // ── 字体（委托给内部 Label）───────────────────────
        auto font_size(float pt) -> Button&;

        [[nodiscard]] auto font_size() const noexcept -> float;

        auto font_color(const nandina::NanColor& color) -> Button&;

        [[nodiscard]] auto font_color() const noexcept -> const nandina::NanColor&;

        auto font_weight(text::NanFontWeight w) -> Button&;

        [[nodiscard]] auto font_weight() const noexcept -> text::NanFontWeight;

        auto font_family(std::string family) -> Button&;

        [[nodiscard]] auto font_family() const noexcept -> const std::string&;

        auto set_font(text::NanFont font) -> Button&;

        [[nodiscard]] auto font() const noexcept -> const text::NanFont&;
        /// 局部更新字体属性；Fn: (NanFont&) -> void
        ///
        /// 示例：
        ///   button_ref->update_font([](auto& f){ f.color(NanColor::white()); });
        template <typename Fn>
            requires std::invocable<Fn, text::NanFont&>
        auto update_font(Fn&& fn) -> Button& {
            auto updated = font();
            std::invoke(std::forward<Fn>(fn), updated);
            return set_font(std::move(updated));
        }
        // ── 回调 ──────────────────────────────────────────
        auto on_click(Callback cb) -> Button&;

        auto on_press(Callback cb) -> Button&;

        auto on_release(Callback cb) -> Button&;

        auto on_hover(Callback cb) -> Button&;

        auto on_leave(Callback cb) -> Button&;

        // ── 信号 ──────────────────────────────────────────
        [[nodiscard]] auto clicked() -> reactive::EventSignal<>&;
        [[nodiscard]] auto hovered() -> reactive::EventSignal<>&;
        [[nodiscard]] auto left() -> reactive::EventSignal<>&;
        [[nodiscard]] auto pressed() -> reactive::EventSignal<>&;
        [[nodiscard]] auto released() -> reactive::EventSignal<>&;

        // ── 禁用 / 加载 ───────────────────────────────────
        auto set_disabled(bool disabled) -> Button&;

        [[nodiscard]] auto is_disabled() const noexcept -> bool;

        auto set_loading(bool loading) -> Button&;

        [[nodiscard]] auto is_loading() const noexcept -> bool;

        // ── 布局协议 ──────────────────────────────────────
        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override;

        // ── 交互标识 ─────────────────────────────────────
        [[nodiscard]] auto is_interactive() const noexcept -> bool override {
            return true;
        }

        /// 访问内部 Label，供开发者自定义文本属性
        [[nodiscard]] auto label() noexcept -> Label& {
            return *m_label;
        }

        [[nodiscard]] auto label() const noexcept -> const Label& {
            return *m_label;
        }

    protected:
        // ── 事件（直接处理，不依赖 Pressable 子节点）─────
        auto on_pointer_move(const runtime::PointerMoveEvent& event) -> bool override;

        auto on_pointer_enter(const runtime::PointerMoveEvent& event) -> bool override;

        auto on_pointer_leave(const runtime::PointerMoveEvent& event) -> bool override;

        auto on_pointer_down(const runtime::PointerButtonEvent& event) -> bool override;

        auto on_pointer_up(const runtime::PointerButtonEvent& event) -> bool override;

        void on_draw(tvg::SwCanvas& canvas) override;

    private:
        Button();

        auto apply_variant() -> void;

        auto apply_size() -> void;

        auto update_visual_state() -> void;

        // ── 状态 ──────────────────────────────────────
        ButtonVariant m_variant{ButtonVariant::default_variant};
        ButtonSize    m_size{ButtonSize::md};
        std::optional<IconType> m_icon_type;

        bool m_hovered{false};
        bool m_pressed{false};
        bool m_disabled{false};
        bool m_loading{false};

        // ── 信号 ──────────────────────────────────────────
        reactive::EventSignal<> m_clicked_signal;
        reactive::EventSignal<> m_hovered_signal;
        reactive::EventSignal<> m_left_signal;
        reactive::EventSignal<> m_pressed_signal;
        reactive::EventSignal<> m_released_signal;

        // ── 子节点 ────────────────────────────────────
        layout::Row*   m_content_row{nullptr};
        Icon*           m_icon_left{nullptr};
        Label*          m_label{nullptr};
    };
} // namespace nandina::widgets
