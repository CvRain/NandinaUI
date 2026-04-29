//
// Created by cvrain on 2026/4/28.
//

module;

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <thorvg-1/thorvg.h>

export module nandina.widgets.button;

import nandina.widgets.surface;
import nandina.widgets.pressable;
import nandina.widgets.label;
import nandina.foundation.nan_insets;
import nandina.foundation.color;
import nandina.reactive.event_signal;

/**
 * nandina.widgets.button
 *
 * Button — 按钮组件（组合 Surface + Pressable + Label）。
 *
 * 设计：
 * - Button 继承自 Surface（获得背景/圆角/padding/描边能力）
 * - Button 包含一个 Pressable 子节点（获得交互状态机）
 * - Label 作为文本显示子节点
 * - 交互状态变化时自动更新视觉呈现（通过每帧 update_visual_state）
 * - 链式 API：Button::create().set_text("Click").on_click([] { ... })
 *
 * 视觉状态映射：
 *   normal   → bg: accent色(99,102,241) / text: white
 *   hover    → bg: 略亮(120,123,255)
 *   pressed  → bg: 略暗(80,82,200)
 *   disabled → bg: 灰(60,62,80) / text: dim(110,112,130)
 */
export namespace nandina::widgets {
    // ── 按钮样式 ──────────────────────────────────────────────
    struct ButtonColors {
        nandina::NanColor bg{nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};
        nandina::NanColor bg_hover{nandina::NanColor::from(nandina::NanRgb{120, 123, 255})};
        nandina::NanColor bg_pressed{nandina::NanColor::from(nandina::NanRgb{80, 82, 200})};
        nandina::NanColor bg_disabled{nandina::NanColor::from(nandina::NanRgb{60, 62, 80})};
        nandina::NanColor text{nandina::NanColor::from(nandina::NanRgb{255, 255, 255})};
        nandina::NanColor text_disabled{nandina::NanColor::from(nandina::NanRgb{110, 112, 130})};
        float corner_radius{6.0f};
        geometry::NanInsets padding{12.0f, 8.0f, 12.0f, 8.0f};
    };

    /**
     * Button — 按钮组件
     *
     * 组合 Surface（背景绘制）+ Pressable（交互状态）+ Label（文本）。
     *
     * 实现位于 nan_button.cpp。
     */
    class Button : public Surface {
    public:
        using Ptr = std::unique_ptr<Button>;
        using Callback = std::function<void()>;

        ~Button() override = default;

        static auto create() -> Ptr;

        // ── 文本 ──────────────────────────────────────────
        auto set_text(std::string_view text) -> Button&;
        [[nodiscard]] auto text() const noexcept -> const std::string&;

        // ── 回调注册 ──────────────────────────────────────
        auto on_click(Callback cb) -> Button&;
        auto on_press(Callback cb) -> Button&;
        auto on_release(Callback cb) -> Button&;

        // ── 信号 ──────────────────────────────────────────
        [[nodiscard]] auto clicked() -> reactive::EventSignal<>&;

        // ── 禁用状态 ──────────────────────────────────────
        auto set_disabled(bool disabled) -> Button&;
        [[nodiscard]] auto is_disabled() const noexcept -> bool;

        // ── 样式 ──────────────────────────────────────────
        auto set_colors(const ButtonColors& colors) -> Button&;
        [[nodiscard]] auto colors() const noexcept -> const ButtonColors&;

        // ── 布局协议 ──────────────────────────────────────
        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override;

    protected:
        void on_draw(tvg::SwCanvas& canvas) override;

    private:
        Button();

        auto update_visual_state() -> void;

        ButtonColors m_colors;
        Pressable* m_pressable{nullptr};
        Label* m_label{nullptr};
    };
} // namespace nandina::widgets