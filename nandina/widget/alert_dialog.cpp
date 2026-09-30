//
// widget/alert_dialog - modal dialog constrained to "the user must choose".
//

#include "alert_dialog.hpp"

#include "button.hpp"
#include "dialog.hpp"
#include "layout.hpp"
#include "primitives/text.hpp"

#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        /// 说明文本最多铺几行（与 Alert 的说明同档：够放下两三句，又不至于把面板撑高）。
        constexpr int kDescriptionMaxLines = 4;

        void require_non_empty(const std::string_view value, const char* what) {
            if (value.empty()) {
                throw std::invalid_argument(std::string {"AlertDialog: "} + what + " is required");
            }
        }
    } // namespace

    AlertDialog::AlertDialog(std::string title, std::string description, theme::NanTheme theme):
        title_(std::move(title)),
        description_(std::move(description)) {
        // 约束 1：alertdialog 的读屏契约要求有描述（见文件头）。
        require_non_empty(description_, "description");

        dialog_ = std::make_shared<Dialog>(theme);
        // 约束 3：不可被外部点击 / Escape 关闭 —— 这是本组件与 Dialog 的全部差别。
        dialog_->set_dismissible(false);
        dialog_->set_alert_semantics(true);
        dialog_->set_title(title_);

        description_text_ = std::make_shared<primitives::Text>(description_);
        (void)dialog_->set_content(description_text_);

        cancel_button_ = Button::create(cancel_text_, theme);
        cancel_button_->set_tone(theme::ButtonTone::neutral);
        cancel_button_->set_on_click([this] {
            if (on_cancel_) {
                on_cancel_();
            }
            close();
        });

        confirm_button_ = Button::create(confirm_text_, theme);
        confirm_button_->set_tone(confirm_tone_);
        confirm_button_->set_on_click([this] {
            if (on_confirm_) {
                on_confirm_();
            }
            close();
        });

        actions_ = std::make_shared<Flex>(LayoutAxis::horizontal);
        actions_->set_gap(8.0F);
        actions_->set_main_alignment(LayoutAlignment::end);
        actions_->set_cross_alignment(LayoutAlignment::center);
        actions_->add(cancel_button_);
        actions_->add(confirm_button_);
        (void)dialog_->set_footer(actions_);

        add_child(dialog_);
        apply_text_styles();
        set_visible(true);
    }

    AlertDialog::~AlertDialog() = default;

    auto AlertDialog::create(std::string title, std::string description, theme::NanTheme theme)
        -> std::shared_ptr<AlertDialog> {
        return std::make_shared<AlertDialog>(std::move(title), std::move(description), theme);
    }

    // ─── 文本 ────────────────────────────────────────────────────────────

    void AlertDialog::set_title(std::string title) {
        title_ = std::move(title);
        dialog_->set_title(title_);
    }

    auto AlertDialog::title() const -> std::string_view {
        return title_;
    }

    void AlertDialog::set_description(std::string description) {
        require_non_empty(description, "description");
        description_ = std::move(description);
        description_text_->set_text(description_);
        mark_layout_dirty();
    }

    auto AlertDialog::description() const -> std::string_view {
        return description_;
    }

    // ─── 动作 ────────────────────────────────────────────────────────────

    void AlertDialog::set_cancel_text(std::string text) {
        cancel_text_ = std::move(text);
        rebuild_actions();
    }

    auto AlertDialog::cancel_text() const -> std::string_view {
        return cancel_text_;
    }

    void AlertDialog::set_confirm_text(std::string text) {
        // 约束 2：它不可关闭，没有确认动作就会把用户困在模态里（见文件头）。
        require_non_empty(text, "confirm text");
        confirm_text_ = std::move(text);
        rebuild_actions();
    }

    auto AlertDialog::confirm_text() const -> std::string_view {
        return confirm_text_;
    }

    void AlertDialog::set_confirm_tone(const theme::ButtonTone tone) {
        confirm_tone_ = tone;
        confirm_button_->set_tone(tone);
    }

    auto AlertDialog::confirm_tone() const -> theme::ButtonTone {
        return confirm_tone_;
    }

    void AlertDialog::set_on_confirm(std::function<void()> callback) {
        on_confirm_ = std::move(callback);
    }

    void AlertDialog::set_on_cancel(std::function<void()> callback) {
        on_cancel_ = std::move(callback);
    }

    void AlertDialog::set_on_close(std::function<void()> callback) {
        dialog_->set_on_close(std::move(callback));
    }

    // ─── 状态 ────────────────────────────────────────────────────────────

    void AlertDialog::open() {
        apply_text_styles();
        dialog_->open();
    }

    void AlertDialog::close() {
        dialog_->close();
    }

    auto AlertDialog::is_open() const -> bool {
        return dialog_->is_open();
    }

    // ─── 主题 ────────────────────────────────────────────────────────────

    void AlertDialog::set_theme(theme::NanTheme theme) {
        dialog_->set_theme(theme);
        cancel_button_->set_theme(theme);
        confirm_button_->set_theme(theme);
        apply_text_styles();
    }

    auto AlertDialog::theme_ref() const -> const theme::NanTheme& {
        return dialog_->theme_ref();
    }

    void AlertDialog::set_override(theme::DialogRecipeRule rule) {
        dialog_->set_override(std::move(rule));
        apply_text_styles();
    }

    auto AlertDialog::resolved_style() const -> theme::ResolvedDialogStyle {
        return dialog_->resolved_style();
    }

    void AlertDialog::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        dialog_->on_style_context_changed(context);
        apply_text_styles();
    }

    void AlertDialog::on_theme_changed(const theme::ThemeManager& manager) {
        dialog_->on_theme_changed(manager);
        cancel_button_->on_theme_changed(manager);
        confirm_button_->on_theme_changed(manager);
        apply_text_styles();
    }

    // ─── 生命周期 ────────────────────────────────────────────────────────

    auto AlertDialog::z_index_hint() const -> int {
        return dialog_->z_index_hint();
    }

    void AlertDialog::on_exit_tree() {
        dialog_->on_exit_tree();
    }

    auto AlertDialog::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        // 本节点只是内部 Dialog 在布局里的占位（与 Popover 之于触发控件同理）。
        return constraints.constrain(dialog_->measure_layout(constraints));
    }

    void AlertDialog::on_layout() {
        dialog_->layout_to(local_rect());
    }

    // ─── 内部 ────────────────────────────────────────────────────────────

    void AlertDialog::set_overlay_service(scene::OverlayHost* host) noexcept {
        dialog_->set_overlay_service(host);
    }

    void AlertDialog::apply_text_styles() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        const primitives::TextStyle text_style {
            .color = context.text_color_from_context ? context.text_color : style.description.color,
            .font_size =
                context.font_size_from_context ? context.font_size : style.description.font_size,
            .font = context.font_from_context ? context.font : description_text_->font(),
            .overflow = primitives::TextOverflow::wrap,
            .max_lines = kDescriptionMaxLines,
        };
        if (!description_text_->style().approx_equals(text_style)) {
            description_text_->set_style(text_style);
            mark_layout_dirty();
        }
    }

    void AlertDialog::rebuild_actions() {
        cancel_button_->set_text(cancel_text_);
        confirm_button_->set_text(confirm_text_);
        // 取消文案留空表示"只留确认一条出路"：把按钮从动作行里摘掉，而不是显示一个空按钮。
        const bool want_cancel = !cancel_text_.empty();
        const bool has_cancel = actions_->child_count() > 1;
        if (want_cancel && !has_cancel) {
            (void)actions_->add(cancel_button_);
        }
        else if (!want_cancel && has_cancel) {
            actions_->remove_and_delete(*cancel_button_);
        }
        mark_layout_dirty();
    }
} // namespace nandina::widget
