//
// Created by cvrain on 2026/4/28.
//
// nan_button.cpp — Button 组件实现（PIMPL style: 完整类型仅在此可见）
//

module;

#include <string>
#include <string_view>
#include <functional>

#include <thorvg-1/thorvg.h>

module nandina.widgets.button;

import nandina.widgets.pressable;
import nandina.widgets.label;

namespace nandina::widgets {

    // ── 工厂 ────────────────────────────────────────────────
    auto Button::create() -> Ptr {
        return Ptr{new Button()};
    }

    // ── 私有构造 ────────────────────────────────────────────
    Button::Button() {
        // 初始化默认样式
        set_corner_radius(m_colors.corner_radius);
        set_padding(m_colors.padding);
        set_bg_color(m_colors.bg);

        // 创建 Pressable（交互状态机）作为 direct child
        auto pressable = Pressable::create();
        m_pressable = pressable.get();
        add_child(std::move(pressable));

        // 创建 Label（文本显示）作为 pressable 的子节点
        auto label = Label::create();
        m_label = label.get();
        m_pressable->add_child(std::move(label));
    }

    // ── 文本 ────────────────────────────────────────────────
    auto Button::set_text(std::string_view text) -> Button& {
        m_label->set_text(text);
        mark_dirty();
        return *this;
    }

    auto Button::text() const noexcept -> const std::string& {
        return m_label->text();
    }

    // ── 回调 ────────────────────────────────────────────────
    auto Button::on_click(Callback cb) -> Button& {
        m_pressable->on_click(std::move(cb));
        return *this;
    }

    auto Button::on_press(Callback cb) -> Button& {
        m_pressable->on_press(std::move(cb));
        return *this;
    }

    auto Button::on_release(Callback cb) -> Button& {
        m_pressable->on_release(std::move(cb));
        return *this;
    }

    // ── 信号 ────────────────────────────────────────────────
    auto Button::clicked() -> reactive::EventSignal<>& {
        return m_pressable->clicked();
    }

    // ── 禁用状态 ────────────────────────────────────────────
    auto Button::set_disabled(bool disabled) -> Button& {
        m_pressable->set_disabled(disabled);
        update_visual_state();
        mark_dirty();
        return *this;
    }

    auto Button::is_disabled() const noexcept -> bool {
        return m_pressable->is_disabled();
    }

    // ── 样式 ────────────────────────────────────────────────
    auto Button::set_colors(const ButtonColors& colors) -> Button& {
        m_colors = colors;
        update_visual_state();
        return *this;
    }

    auto Button::colors() const noexcept -> const ButtonColors& {
        return m_colors;
    }

    // ── 首选尺寸 ────────────────────────────────────────────
    auto Button::preferred_size() const noexcept -> geometry::NanSize {
        const auto label_pref = m_label->preferred_size();
        const auto& pad = padding();
        return geometry::NanSize{
            label_pref.width() + pad.left() + pad.right(),
            label_pref.height() + pad.top() + pad.bottom()
        };
    }

    // ── 绘制 ────────────────────────────────────────────────
    void Button::on_draw(tvg::SwCanvas& canvas) {
        update_visual_state();
        // 先由 Surface 绘制背景
        Surface::on_draw(canvas);
        // 子节点（Pressable + Label）由基类 draw() 遍历绘制
    }

    // ── 视觉状态 ────────────────────────────────────────────
    auto Button::update_visual_state() -> void {
        const auto state = m_pressable->state();

        if (state.disabled) {
            set_bg_color(m_colors.bg_disabled);
            m_label->set_color(m_colors.text_disabled);
        } else if (state.pressed) {
            set_bg_color(m_colors.bg_pressed);
            m_label->set_color(m_colors.text);
        } else if (state.hovered) {
            set_bg_color(m_colors.bg_hover);
            m_label->set_color(m_colors.text);
        } else {
            set_bg_color(m_colors.bg);
            m_label->set_color(m_colors.text);
        }
    }

} // namespace nandina::widgets