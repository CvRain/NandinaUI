//
// widget/alert_dialog - modal dialog constrained to "the user must choose".
//
// AlertDialog 是 **Dialog 加语义约束**，不是另一个弹窗实现：它内部组合一个 `Dialog`，
// 把 `set_dismissible(false)` 与 `set_alert_semantics(true)` 固定下来，因此：
//
//   * 外部点击与 Escape 都**不会**关掉它 —— 用户必须按其中一个按钮；
//   * 读屏听到的 role 是 `alertdialog`（断言式）而不是 `dialog`。
//
// 外观、布局、定位、焦点限制、遮罩、淡入淡出全部来自 `Dialog`，包括 `DialogRecipe` ——
// 所以本组件**没有自己的配方**。要可关闭的对话框请直接用 `Dialog`。
//
// 两条输入约束是刻意的，都在构造期与 setter 上强制（`std::invalid_argument`）：
//
//   1. **描述不能为空**。alertdialog 的读屏契约要求它有描述：只有标题时用户听到的是
//      "某个标题 + 两个按钮"，不知道自己在确认什么。这不是风格问题，是可用性问题。
//   2. **确认按钮文案不能为空**。它不可关闭，如果没有确认动作，用户会被困在这个模态里。
//      取消按钮可以留空（表示只有确认一条出路）。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_ALERT_DIALOG_HPP
#define NANDINA_EXPERIMENT_WIDGET_ALERT_DIALOG_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "menu_item.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace nandina::scene
{
    class OverlayHost;
} // namespace nandina::scene

namespace nandina::widget
{
    template<typename Component>
    struct ComponentTraits;

    class Button;
    class Dialog;
    class Flex;

    namespace primitives
    {
        class Text;
    } // namespace primitives

    /**
     * 断言式模态对话框：标题 + 说明 + 取消 / 确认。
     *
     * 用于不可逆或需要明确确认的动作。因为不可被外部关闭，"必须给出一个答案"由结构保证。
     */
    class AlertDialog: public scene::NanControl {
    public:
        explicit AlertDialog(
            std::string title,
            std::string description,
            theme::NanTheme theme = theme::default_theme()
        );
        ~AlertDialog() override;

        [[nodiscard]] static auto create(
            std::string title,
            std::string description,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<AlertDialog>;

        // ─── 文本 ────────────────────────────────────────────────────────

        void set_title(std::string title);
        [[nodiscard]] auto title() const -> std::string_view;
        /// 说明文本；空字符串会被拒绝（见文件头第 1 条约束）。
        void set_description(std::string description);
        [[nodiscard]] auto description() const -> std::string_view;

        // ─── 动作 ────────────────────────────────────────────────────────

        /// 取消按钮文案（空字符串 = 隐藏取消按钮，只留确认一条出路）。
        void set_cancel_text(std::string text);
        [[nodiscard]] auto cancel_text() const -> std::string_view;
        /// 确认按钮文案；空字符串会被拒绝（见文件头第 2 条约束）。
        void set_confirm_text(std::string text);
        [[nodiscard]] auto confirm_text() const -> std::string_view;
        /// 确认按钮的语气（默认 `primary`；破坏性操作用 `danger`）。
        void set_confirm_tone(theme::ButtonTone tone);
        [[nodiscard]] auto confirm_tone() const -> theme::ButtonTone;

        /// 用户按下确认。组件随后关闭对话框。
        void set_on_confirm(std::function<void()> callback);
        /// 用户按下取消（或只有确认、程序化关闭时不会触发）。组件随后关闭对话框。
        void set_on_cancel(std::function<void()> callback);
        /// 对话框关闭后触发（无论走哪个按钮）。
        void set_on_close(std::function<void()> callback);

        // ─── 状态 ────────────────────────────────────────────────────────

        void open();
        void close();
        [[nodiscard]] auto is_open() const -> bool;

        /// 语义约束是固定的：AlertDialog 不能被外部点击 / Escape 关闭。
        /// 需要可关闭的对话框请用 `Dialog`。
        [[nodiscard]] static constexpr auto dismissible() noexcept -> bool {
            return false;
        }

        // ─── 主题（沿用 DialogRecipe，AlertDialog 没有自己的配方）─────────

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::DialogRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedDialogStyle;

        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        /// 树内回退时提升 z 序，使模态面板压过后续兄弟。
        [[nodiscard]] auto z_index_hint() const -> int override;
        void on_exit_tree() override;

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;

    private:
        friend struct ComponentTraits<AlertDialog>;

        /// Internal: bind the owning window's overlay portal. Called by
        /// `ComponentTraits<AlertDialog>` and forwarded to the inner Dialog.
        void set_overlay_service(scene::OverlayHost* host) noexcept;

        /// 刷新说明文本的排版样式（沿用 `DialogRecipe::description`）。
        void apply_text_styles();
        /// 重建底部动作行（文案 / 语气 / 可见性变化时调用）。
        void rebuild_actions();

        std::shared_ptr<Dialog> dialog_;
        std::shared_ptr<Flex> actions_;
        std::shared_ptr<Button> cancel_button_;
        std::shared_ptr<Button> confirm_button_;
        std::shared_ptr<primitives::Text> description_text_;

        std::string title_;
        std::string description_;
        std::string cancel_text_ = "取消";
        std::string confirm_text_ = "确定";
        theme::ButtonTone confirm_tone_ = theme::ButtonTone::primary;

        std::function<void()> on_confirm_;
        std::function<void()> on_cancel_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_ALERT_DIALOG_HPP
