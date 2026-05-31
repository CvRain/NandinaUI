module;

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <thorvg-1/thorvg.h>

export module nandina.widgets.field;

import nandina.foundation.color;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.text.nan_font;
import nandina.widgets.label;
import nandina.widgets.text_field;

export namespace nandina::widgets {

    /**
     * Field — 围绕输入控件的语义容器。
     *
     * 遵循 shadcn / primitives 风格，将 label / control / helper / error
     * 组织为统一的表单行结构。
     *
     * 布局：
     *   ┌──────────────────────────────────┐
     *   │  Label (optional)                │
     *   │  4px gap                         │
     *   │  ┌─ control slot ──────────────┐ │
     *   │  │  (TextField / 其他输入控件)   │ │
     *   │  └──────────────────────────────┘ │
     *   │  4px gap                         │
     *   │  Helper / Error (optional)        │
     *   └──────────────────────────────────┘
     *
     * 职责：
     * - label 文案展示
     * - control slot 包裹
     * - helper / error 文本切换（invalid=true 时显示 error，隐藏 helper）
     * - required / invalid / disabled 语义组织
     *
     * 用法：
     *   auto field = Field::create();
     *   field->set_label("Email");
     *   field->set_control(TextField::create());
     *   field->set_helper_text("We'll never share your email.");
     *   field->set_error_text("Invalid email address.");
     */
    class Field final : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Field>;

        ~Field() override = default;

        static auto create() -> Ptr {
            return Ptr{new Field()};
        }

        // ── 输入属性 ──────────────────────────────────────────

        auto set_label(std::string value) -> Field& {
            if (m_label_text == value) {
                return *this;
            }

            m_label_text = std::move(value);
            sync_label_visibility();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto label_text() const noexcept -> const std::string& {
            return m_label_text;
        }

        auto set_helper_text(std::string value) -> Field& {
            if (m_helper_text == value) {
                return *this;
            }

            m_helper_text = std::move(value);
            sync_helper_visibility();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto helper_text() const noexcept -> const std::string& {
            return m_helper_text;
        }

        auto set_error_text(std::string value) -> Field& {
            if (m_error_text == value) {
                return *this;
            }

            m_error_text = std::move(value);
            sync_helper_visibility();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto error_text() const noexcept -> const std::string& {
            return m_error_text;
        }

        auto set_control(runtime::NanWidget::Ptr control) -> Field& {
            if (m_control) {
                // 移除旧 control
                auto* raw = m_control;
                m_control = nullptr;
                remove_child(raw);
            }

            if (control) {
                m_control = control.get();
                m_control->set_hit_test_visible(true);
                add_child(std::move(control));
            }

            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto control() const noexcept -> runtime::NanWidget* {
            return m_control;
        }

        auto set_required(const bool value) -> Field& {
            if (m_required == value) {
                return *this;
            }

            m_required = value;
            if (m_label_widget) {
                m_label_widget->set_required(value);
            }
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto required() const noexcept -> bool {
            return m_required;
        }

        auto set_invalid(const bool value) -> Field& {
            if (m_invalid == value) {
                return *this;
            }

            m_invalid = value;

            // 传播 invalid 到 control
            propagate_invalid_to_control();

            // 切换 helper / error 显隐
            sync_helper_visibility();

            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto invalid() const noexcept -> bool {
            return m_invalid;
        }

        auto set_disabled(const bool value) -> Field& {
            if (m_disabled == value) {
                return *this;
            }

            m_disabled = value;

            // 传播 disabled 到子控件
            propagate_disabled();

            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto disabled() const noexcept -> bool {
            return m_disabled;
        }

        // ── 布局 ──────────────────────────────────────────────

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            float total_height = 0.0f;
            float max_width = 0.0f;

            if (m_label_widget && m_label_widget->visible()) {
                const auto pref = m_label_widget->preferred_size();
                max_width = std::max(max_width, pref.width());
                total_height += pref.height() + k_gap;
            }

            if (m_control) {
                const auto pref = m_control->preferred_size();
                max_width = std::max(max_width, pref.width());
                total_height += pref.height();
            }

            if (auto* active_hint = active_hint_label()) {
                const auto pref = active_hint->preferred_size();
                max_width = std::max(max_width, pref.width());
                total_height += k_gap + pref.height();
            }

            return {max_width, total_height};
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            // 标签
            if (m_label_widget) {
                m_label_widget->measure(constraints.loosen());
            }

            // control
            if (m_control) {
                m_control->measure(constraints.loosen());
            }

            // helper / error
            if (auto* hint = active_hint_label()) {
                hint->measure(constraints.loosen());
            }

            // 计算总高度
            float total_height = 0.0f;
            float max_width = 0.0f;

            auto add_child_size = [&](runtime::NanWidget* child) {
                if (!child || !child->visible()) return;
                const auto ms = child->measured_size();
                max_width = std::max(max_width, ms.width());
                total_height += ms.height();
            };

            if (m_label_widget && m_label_widget->visible()) {
                add_child_size(m_label_widget);
                total_height += k_gap;
            }

            if (m_control) {
                add_child_size(m_control);
            }

            if (auto* hint = active_hint_label()) {
                total_height += k_gap;
                add_child_size(hint);
            }

            const auto measured = constraints.constrain(geometry::NanSize{
                max_width > 0.0f ? max_width : constraints.min_width(),
                total_height > 0.0f ? total_height : constraints.min_height(),
            });

            set_measured_layout_state(constraints, measured);
        }

        auto layout() -> void override {
            float cursor_y = y();
            const float content_width = width();

            // 标签
            if (m_label_widget && m_label_widget->visible()) {
                const auto pref = m_label_widget->measured_size();
                m_label_widget->set_bounds(x(), cursor_y, content_width, pref.height());
                m_label_widget->layout();
                cursor_y += pref.height() + k_gap;
            }

            // control — 撑满剩余宽度，高度由 control 自身决定
            if (m_control) {
                const auto pref = m_control->measured_size();
                const float ctrl_height = std::max(pref.height(), m_control->preferred_size().height());
                m_control->set_bounds(x(), cursor_y, content_width, ctrl_height);
                m_control->layout();
                cursor_y += ctrl_height;
            }

            // helper / error
            if (auto* hint = active_hint_label()) {
                cursor_y += k_gap;
                const auto pref = hint->measured_size();
                hint->set_bounds(x(), cursor_y, content_width, pref.height());
                hint->layout();
            }

            clear_layout_dirty();
        }

        // ── 绘制 ──────────────────────────────────────────────
    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            // Field 自身不绘制背景，委托子节点绘制
            for_each_child([&](runtime::NanWidget& child) {
                child.draw(canvas);
            });
        }

    public:
        [[nodiscard]] auto is_interactive() const noexcept -> bool override {
            return false; // 让 hit_test 穿透到 control
        }

    private:
        Field() {
            // ── Label slot ──
            auto label = Label::create();
            label->set_font(text::NanFont{}
                .size(k_label_font_size)
                .color(k_label_color())
                .single_line(true)
                .overflow(text::TextOverflow::ellipsis));
            label->set_visible(false);
            label->set_hit_test_visible(false);
            m_label_widget = label.get();
            add_child(std::move(label));

            // ── Helper slot ──
            auto helper = Label::create();
            helper->set_font(text::NanFont{}
                .size(k_hint_font_size)
                .color(k_helper_color())
                .single_line(true)
                .overflow(text::TextOverflow::ellipsis));
            helper->set_visible(false);
            helper->set_hit_test_visible(false);
            m_helper_widget = helper.get();
            add_child(std::move(helper));

            // ── Error slot ──
            auto error = Label::create();
            error->set_font(text::NanFont{}
                .size(k_hint_font_size)
                .color(k_error_color())
                .single_line(true)
                .overflow(text::TextOverflow::ellipsis));
            error->set_visible(false);
            error->set_hit_test_visible(false);
            m_error_widget = error.get();
            add_child(std::move(error));
        }

        [[nodiscard]] auto active_hint_label() const noexcept -> Label* {
            if (m_invalid && m_error_widget && m_error_widget->visible()) {
                return m_error_widget;
            }
            if (!m_invalid && m_helper_widget && m_helper_widget->visible()) {
                return m_helper_widget;
            }
            return nullptr;
        }

        auto sync_label_visibility() -> void {
            if (!m_label_widget) return;
            const bool has_label = !m_label_text.empty();
            m_label_widget->set_text(m_label_text);
            m_label_widget->set_visible(has_label);
        }

        auto sync_helper_visibility() -> void {
            if (m_helper_widget) {
                const bool show_helper = !m_invalid && !m_helper_text.empty();
                m_helper_widget->set_text(m_helper_text);
                m_helper_widget->set_visible(show_helper);
            }

            if (m_error_widget) {
                const bool show_error = m_invalid && !m_error_text.empty();
                m_error_widget->set_text(m_error_text);
                m_error_widget->set_visible(show_error);
            }
        }

        auto propagate_invalid_to_control() -> void {
            // 尝试通过 dynamic_cast 把 invalid 状态传给已知 control 类型
            if (auto* tf = dynamic_cast<nandina::widgets::TextField*>(m_control)) {
                tf->set_invalid(m_invalid);
            }
            // 其他 control 类型可根据未来扩展添加
        }

        auto propagate_disabled() -> void {
            if (m_label_widget) {
                m_label_widget->set_disabled(m_disabled);
            }

            // 传播到 control
            if (auto* tf = dynamic_cast<nandina::widgets::TextField*>(m_control)) {
                tf->set_disabled(m_disabled);
            }
            // 未来可扩展其他 control 类型

            // helper / error 的 disabled 样式交由 Label 处理
            if (m_helper_widget) {
                m_helper_widget->set_disabled(m_disabled);
            }
            if (m_error_widget) {
                m_error_widget->set_disabled(m_disabled);
            }
        }

        // ── 移除子节点（内部辅助）──
        auto remove_child(runtime::NanWidget* child) -> void {
            auto& list = children();
            for (auto it = list.begin(); it != list.end(); ++it) {
                if (it->get() == child) {
                    list.erase(it);
                    return;
                }
            }
        }

        // ── 常量 ──
        static constexpr float k_gap = 4.0f;
        static constexpr float k_label_font_size = 13.0f;
        static constexpr float k_hint_font_size = 12.0f;

        static auto k_label_color() -> nandina::NanColor {
            return nandina::NanColor::from(nandina::NanRgb{30, 30, 46});
        }

        static auto k_helper_color() -> nandina::NanColor {
            return nandina::NanColor::from(nandina::NanRgb{154, 157, 180});
        }

        static auto k_error_color() -> nandina::NanColor {
            return nandina::NanColor::from(nandina::NanRgb{231, 130, 132});
        }

        // ── 子 widget（raw ptr，owned 通过 add_child）──
        Label* m_label_widget{nullptr};
        Label* m_helper_widget{nullptr};
        Label* m_error_widget{nullptr};
        runtime::NanWidget* m_control{nullptr};

        std::string m_label_text;
        std::string m_helper_text;
        std::string m_error_text;

        bool m_required{false};
        bool m_invalid{false};
        bool m_disabled{false};
    };

} // namespace nandina::widgets
