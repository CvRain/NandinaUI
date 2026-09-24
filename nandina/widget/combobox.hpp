//
// widget/combobox - filterable text input with a dropdown list of options.
//
// 结构：Combobox 组合一个 `TextField`（可编辑文本：光标 / 选区 / 输入法 / 文本绘制
// 全部复用）与一个 `Popover`（浮层托管、锚定、外部点击关闭、焦点作用域）。条目列表由
// 内部的 `internal::ComboboxListSurface` 渲染，并作为 Popover 的 content。
//
// 与 DropdownMenu 的分工一致：Popover 负责面板，组件负责条目列表语义。列表只呈现
// **过滤后**的 `MenuItem` 副本（匹配 label 的 ASCII 大小写不敏感子串，空查询列出全部
// 可聚焦条目），因此 `set_items()` 换掉容器时列表不会留下悬垂引用。
//
// 焦点模型与本仓库其它组合控件不同：Combobox 自身**不可聚焦**，焦点落在内部的
// TextField 上（否则 Tab 会在同一个控件上停两次，也会隐藏光标）。打开浮层时
// FocusScope 会短暂接管焦点，组件随后把焦点交还 TextField，因此输入过滤在浮层打开
// 期间持续可用；浮层里的行不接收焦点，点击行通过 `focus_delegate()` 仍然落在输入框。
//
// 键盘采用捕获阶段拦截（`on_input_capture` 在 TextField 之前运行）：只消费导航 /
// 提交键，其余（可打印字符、Backspace、左右键、Ctrl+A …）一律返回 false 落到 TextField。
// 空格键码也被消费，目的是阻止 Popover 的“触发键切换”把输入空格当成开合开关；字符本身
// 由平台的 TextInputEvent 送达（EditableText 只从 text_input 插入文本）。
//
// 条目模型与规则见 docs/references/menu_model.md。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_COMBOBOX_HPP
#define NANDINA_EXPERIMENT_WIDGET_COMBOBOX_HPP

#include "../reactive/event.hpp"
#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "menu_item.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::scene
{
    class OverlayHost;
    class KeyEvent;
} // namespace nandina::scene

namespace nandina::widget
{
    template<typename Component>
    struct ComponentTraits;

    class Popover;
    class TextField;

    namespace internal
    {
        class ComboboxListSurface;
    } // namespace internal

    /**
     * 可过滤的输入框 + 选项浮层（type-ahead 选择控件）。
     *
     * 文本与选中保持一致：选中一项后 `text()` 等于其 label、`selected_id()` 等于其 id；
     * 之后继续输入会清空 `selected_id()`（除非新文本恰好等于某个可激活条目的 label）。
     * 选择状态的事实来源始终是这份一致性，而不是另一份并行状态。
     */
    class Combobox: public scene::NanControl {
    public:
        explicit Combobox(
            std::vector<MenuItem> items = {},
            std::string value = {},
            std::string placeholder = {},
            theme::NanTheme theme = theme::default_theme()
        );
        ~Combobox() override;

        [[nodiscard]] static auto create(
            std::vector<MenuItem> items = {},
            std::string value = {},
            std::string placeholder = {},
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Combobox>;

        // ─── 条目 ────────────────────────────────────────────────────────

        /// 替换条目模型。打开状态下会重新过滤并**就地**更新列表（面板靠脏标记重算尺寸，
        /// 不重建浮层，因此不会打断输入焦点）。
        void set_items(std::vector<MenuItem> items);
        [[nodiscard]] auto items() const -> const std::vector<MenuItem>&;
        [[nodiscard]] auto item_count() const -> std::size_t;
        /// 当前过滤后的条目 id（按列表顺序）。
        [[nodiscard]] auto filtered_ids() const -> std::vector<std::string>;

        // ─── 文本 / 值 ───────────────────────────────────────────────────

        /// 程序化设置输入框文本（不触发 on_change；文本与某个条目 label 完全相同时
        /// 会静默恢复该条目的选中状态）。
        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;
        void set_placeholder(std::string placeholder);
        [[nodiscard]] auto placeholder() const -> std::string_view;

        /// 当前选中的条目 id；没有匹配选中项时为空。
        [[nodiscard]] auto selected_id() const -> std::string_view;
        /// 程序化选中：设置文本为条目 label，不触发事件；未知 id 或不可激活条目返回 false。
        auto set_selected_id(std::string_view id) -> bool;
        /// 清空选中与文本。
        void clear_selection();

        /// 允许自由文本（默认 false）：true 时失焦 / 关闭不强制匹配，on_change 也会带出
        /// 未匹配文本；false 时失焦 / 关闭会把未匹配文本回退到最后一次提交的标签。
        void set_allow_custom_value(bool allow);
        [[nodiscard]] auto allow_custom_value() const -> bool;

        // ─── 浮层 ────────────────────────────────────────────────────────

        void open();
        void close();
        [[nodiscard]] auto is_open() const -> bool;
        /// 高亮项在**过滤后列表**中的下标；-1 表示无。
        [[nodiscard]] auto active_index() const -> int;

        // ─── 回调 / 事件 ─────────────────────────────────────────────────

        /// 文本或选中发生变化时触发：携带当前文本与选中 id（未匹配时为空串）。
        /// 用户输入、选中条目与自定义值提交都会触发；`set_text` / `set_selected_id` /
        /// `clear_selection` 静默。
        void set_on_change(std::function<void(std::string_view text, std::string_view id)> callback);
        /// 用户选中条目时触发，携带条目 id。
        [[nodiscard]] auto item_selected() const -> const reactive::Event<std::string>&;
        void set_on_close(std::function<void()> callback);

        void set_disabled(bool disabled);
        [[nodiscard]] auto disabled() const -> bool;

        // ─── 主题 ────────────────────────────────────────────────────────

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::ComboboxRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedComboboxStyle;
        [[nodiscard]] auto visual_state() const -> theme::ComboboxVisualState;

        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        [[nodiscard]] auto z_index_hint() const -> int override;
        auto on_input_capture(scene::InputEvent& event) -> bool override;
        void on_process(float dt) override;
        void on_exit_tree() override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

    private:
        friend struct ComponentTraits<Combobox>;

        /// Internal: bind the owning window's overlay portal. Called by
        /// `ComponentTraits<Combobox>` so page authors never create an OverlayHost.
        void set_overlay_service(scene::OverlayHost* host) noexcept;

        /// 按当前文本重算过滤结果并灌给列表（就地替换，面板靠脏标记重算尺寸）。
        void refresh_filter();
        /// 把解析后的配方转成 TextField 的实例覆盖：外壳与文本都由组合的输入框绘制。
        void sync_field_style();
        void sync_surface_style();
        /// 把配方的间距下发给 Popover（负值 / 非有限值钳到 0，避免 set_gap 抛错）。
        void sync_popover_gap();
        /// 打开浮层后把焦点交还输入框（FocusScope 会在托管时接管焦点）。
        [[nodiscard]] auto restore_field_focus() -> bool;

        void handle_field_text_changed(std::string_view text);
        /// 捕获阶段消费导航键：只处理开合 / 高亮 / 提交，其余交还 TextField。
        auto handle_capture_key(scene::KeyEvent& event) -> bool;
        void handle_capture_enter();
        /// 用户选中（Enter / 点击行）：写文本、写选中、通知并关闭。
        void activate_id(std::string_view id);
        /// Popover 关闭回调：提交未匹配文本、复位瞬态并转发用户回调。
        void handle_closed();
        /// 失焦 / 关闭时的一致性收尾（见 allow_custom_value 的规则）。
        void commit_unmatched();
        /// 文本变化后重算选中：完全等于某个可激活条目的 label 才会重新选中。
        [[nodiscard]] auto sync_selected_from_text() -> bool;
        /// 静默把文本恢复到最近一次提交的标签。
        void restore_committed_text();

        void notify_change(std::string_view text, std::string_view id);

        std::vector<MenuItem> items_;
        std::shared_ptr<TextField> text_field_;
        std::shared_ptr<Popover> popover_;
        std::shared_ptr<internal::ComboboxListSurface> surface_;

        std::string selected_id_;
        /// 最近一次提交的标签：未匹配文本在失焦 / 关闭时回退到它。
        std::string committed_label_;
        /// 上一次通知的 (text, id)：输入与收尾都走 notify_change，用它去重。
        std::string last_change_text_;
        std::string last_change_id_;
        bool change_emitted_ = false;

        bool allow_custom_value_ = false;
        bool disabled_ = false;
        bool focused_ = false;
        /// 打开浮层后需要把焦点交还输入框；首次尝试可能早于浮层托管完成。
        bool focus_restore_pending_ = false;

        std::function<void(std::string_view, std::string_view)> on_change_;
        reactive::Event<std::string> item_selected_;
        std::function<void()> on_close_;

        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::ComboboxRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_COMBOBOX_HPP
