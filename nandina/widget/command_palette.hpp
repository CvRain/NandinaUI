//
// widget/command_palette - modal command launcher (Ctrl/Cmd+K) over the MenuItem model.
//
// 结构：CommandPalette 组合一套模态浮层设施（与 Dialog 同款）与一个查询输入框：
//
//     DismissLayer（遮罩 / Escape / 外部点击 / 输入阻断）
//     └── FocusScope（焦点限制与关闭后恢复）
//         └── internal::CommandPaletteShell（面板定位：靠上而非居中）
//             ├── TextField（查询行）
//             └── internal::CommandPaletteResultSurface（过滤后的结果列表）
//
// 为什么不用 Popover：`Popover` 是"锚定在触发控件旁的非模态浮层"，而命令面板没有触发
// 控件、需要遮罩并阻断下层输入，语义上更接近 Dialog。承载方式、detached 回退与关闭后
// 焦点恢复因此都复用 Dialog 已经验证过的那条路径。
//
// 焦点模型与 Combobox 一致：**焦点始终留在查询输入框**，结果列表不接收焦点。列表行不
// 参与命中测试（`contains_point` 返回 false），指针一律由结果表面按 y 坐标解析到行下标，
// 于是悬停 / 点击与键盘高亮走同一条 `active_index` 路径。
//
// 键盘采用捕获阶段拦截（`on_input_capture` 在 TextField 之前运行）：只消费导航与提交键，
// 其余（可打印字符、Backspace、左右键、Ctrl+A …）一律返回 false 落到 TextField。查询本身
// 就是过滤器，因此漫游**不启用 typeahead**（`menu_item_typeahead_text` 一律返回空串），
// 否则输入的字母会既进输入框又跳转高亮。
//
// 条目模型与规则见 docs/references/menu_model.md：复用 `MenuItem` 与 `MenuSelection`，
// 不新增平行的选项类型；`disabled` 条目仍可聚焦但不可激活。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_COMMAND_PALETTE_HPP
#define NANDINA_EXPERIMENT_WIDGET_COMMAND_PALETTE_HPP

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
    class OverlayHandle;
    class OverlayHost;
    class InputEvent;
    class KeyEvent;
} // namespace nandina::scene

namespace nandina::widget
{
    template<typename Component>
    struct ComponentTraits;

    class TextField;

    namespace internal
    {
        class CommandPalettePanel;
        class CommandPaletteResultSurface;
        class CommandPaletteShell;
        class DismissLayer;
        class FocusScope;
    } // namespace internal

    /**
     * 命令面板：一个查询输入框 + 过滤后的可执行条目列表，以模态浮层呈现。
     *
     * 与菜单族共用同一份 `MenuItem` 模型与 `MenuSelection` 规则：`action` 条目激活后
     * 关闭整块面板并触发 `on_select`，`checkbox` / `radio` 条目按 `selection_mode`
     * 原地切换并保持面板打开（与 DropdownMenu 同规则）。
     *
     * 过滤按 `label` 做 ASCII 大小写不敏感子串匹配（与 Combobox 同一套规则）；
     * `shortcut` 只用于展示，不参与匹配（`menu_model.md` 规则 2）。分组结构
     * （`MenuItemKind::label` 分组标题与 `separator` 分隔线）在过滤后保留 —— 整组都
     * 无匹配时，该组连同它的标题与分隔线一起消失。
     */
    class CommandPalette: public scene::NanControl {
    public:
        explicit CommandPalette(
            std::vector<MenuItem> items = {},
            std::string placeholder = {},
            theme::NanTheme theme = theme::default_theme()
        );
        ~CommandPalette() override;

        [[nodiscard]] static auto create(
            std::vector<MenuItem> items = {},
            std::string placeholder = {},
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<CommandPalette>;

        // ─── 条目 ────────────────────────────────────────────────────────

        /// 替换条目模型。面板打开时会重新过滤并**就地**更新列表，不重建浮层，
        /// 因此不会打断已输入的查询与焦点。
        void set_items(std::vector<MenuItem> items);
        [[nodiscard]] auto items() const -> const std::vector<MenuItem>&;
        [[nodiscard]] auto item_count() const -> std::size_t;
        /// 当前过滤后的条目 id（按列表顺序，只含可聚焦条目）。
        [[nodiscard]] auto filtered_ids() const -> std::vector<std::string>;

        /// 勾选语义模式（默认 none = 纯动作面板）。模式变更不回溯清理已有勾选。
        void set_selection_mode(MenuSelectionMode mode);
        [[nodiscard]] auto selection_mode() const -> MenuSelectionMode;
        /// 当前勾选条目的 id（事实来源是 `items()` 的 `MenuItem::checked`）。
        [[nodiscard]] auto checked_ids() const -> std::vector<std::string>;

        // ─── 查询 ────────────────────────────────────────────────────────

        /// 程序化设置查询文本（会重新过滤，但不触发 `on_query_change`）。
        void set_query(std::string query);
        [[nodiscard]] auto query() const -> std::string_view;
        void clear_query();
        void set_placeholder(std::string placeholder);
        [[nodiscard]] auto placeholder() const -> std::string_view;

        // ─── 结果上限 ────────────────────────────────────────────────────

        /// 最多渲染多少条结果（默认 8）。超出部分不渲染，但 `hidden_result_count()`
        /// 会如实报告数量，让面板能提示"还有 N 条，继续输入以缩小范围"。
        ///
        /// 为什么是上限而不是滚动：仓库里没有"漫游时滚动进视野"的设施，DropdownMenu
        /// 也不滚动；自造一套滚动会把这一版拖进裁剪与偏移的细节里。命令面板的核心
        /// 交互本来就是"输入以缩小范围"，因此这一版用上限 + 提示，而不是假装能滚动。
        void set_max_visible_results(std::size_t count);
        [[nodiscard]] auto max_visible_results() const -> std::size_t;
        /// 因上限而未渲染的结果数量。
        [[nodiscard]] auto hidden_result_count() const -> std::size_t;

        // ─── 开关与高亮 ──────────────────────────────────────────────────

        void open();
        void close();
        void toggle();
        [[nodiscard]] auto is_open() const -> bool;
        /// 高亮项在**过滤后列表**中的下标；-1 表示无。
        [[nodiscard]] auto active_index() const -> int;
        /// 高亮项的条目 id；无高亮时为空。
        [[nodiscard]] auto active_id() const -> std::string_view;

        // ─── 回调 / 事件 ─────────────────────────────────────────────────

        /// 用户激活条目时触发（Enter / 点击）。`checkbox` / `radio` 也会触发。
        void set_on_select(std::function<void(std::string_view id)> callback);
        /// 可订阅的同名事件。
        [[nodiscard]] auto item_selected() const -> const reactive::Event<std::string>&;
        /// 查询文本变化时触发（仅用户输入；`set_query` / `clear_query` 静默）。
        void set_on_query_change(std::function<void(std::string_view query)> callback);
        /// 面板关闭时触发（含 Escape、外部点击与激活后自动关闭）。
        void set_on_close(std::function<void()> callback);

        void set_disabled(bool disabled);
        [[nodiscard]] auto disabled() const -> bool;

        // ─── 主题 ────────────────────────────────────────────────────────

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::CommandPaletteRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedCommandPaletteStyle;

        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        [[nodiscard]] auto z_index_hint() const -> int override;
        auto on_input_capture(scene::InputEvent& event) -> bool override;
        void on_process(float dt) override;
        void on_exit_tree() override;

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

    private:
        friend struct ComponentTraits<CommandPalette>;
        friend class internal::CommandPaletteResultSurface;

        /// Internal: bind the owning window's overlay portal. Called by
        /// `ComponentTraits<CommandPalette>` so page authors never create an OverlayHost.
        void set_overlay_service(scene::OverlayHost* host) noexcept;
        [[nodiscard]] auto resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost>;

        /// 承载方式，在首次打开时确定后固定下来（同 Dialog）。
        enum class MountPhase { closed, opening, opened, closing };
        enum class MountMode { unmounted, tree, overlay };

        [[nodiscard]] auto mount() -> bool;
        void unmount();
        void install_dismiss_callback();
        [[nodiscard]] auto active() const noexcept -> bool {
            return phase_ != MountPhase::closed;
        }

        /// 按当前查询重算过滤结果（含分组保留规则）并灌给结果表面。
        void refresh_results();
        /// 把解析后的配方转成面板与结果表面的样式（并下发给内嵌 TextField）。
        void apply_style();
        void sync_field_style();
        /// 查询文本变化：重新过滤、复位高亮、通知应用。
        void handle_query_changed(std::string_view query);
        /// 把焦点交回查询框。打开时 FocusScope 会接管初始焦点，需要再抢回来。
        [[nodiscard]] auto restore_query_focus() -> bool;
        /// 遮罩淡入淡出：树内用 AnimationHost，detached 直接设目标值。
        void start_fade(float target);
        /// 用户激活（Enter / 点击行）。
        void activate_id(std::string_view id);

        std::vector<MenuItem> items_;
        MenuSelectionMode selection_mode_ = MenuSelectionMode::none;
        /// 过滤 + 分组保留之后的结果（只含可聚焦条目）。
        std::vector<MenuItem> results_;
        /// 因上限而未渲染的数量。
        std::size_t hidden_results_ = 0;
        std::size_t max_visible_results_ = 8;

        std::shared_ptr<internal::DismissLayer> dismiss_layer_;
        std::shared_ptr<internal::FocusScope> focus_scope_;
        std::shared_ptr<internal::CommandPaletteShell> shell_;
        std::shared_ptr<internal::CommandPalettePanel> panel_;
        std::shared_ptr<TextField> text_field_;
        std::shared_ptr<internal::CommandPaletteResultSurface> surface_;

        std::weak_ptr<scene::OverlayHost> overlay_service_;
        std::unique_ptr<scene::OverlayHandle> portal_handle_;
        MountPhase phase_ = MountPhase::closed;
        MountMode mount_mode_ = MountMode::unmounted;
        bool disabled_ = false;
        /// 打开后需要把焦点交还查询框；首次尝试可能早于浮层托管完成。
        bool focus_restore_pending_ = false;

        std::function<void(std::string_view)> on_select_;
        reactive::Event<std::string> item_selected_;
        std::function<void(std::string_view)> on_query_change_;
        std::function<void()> on_close_;

        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::CommandPaletteRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_COMMAND_PALETTE_HPP
