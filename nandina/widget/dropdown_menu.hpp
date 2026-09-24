//
// widget/dropdown_menu - menu anchored to a trigger, built on Popover + MenuItem.
//
// 结构：DropdownMenu 组合一个 Popover（单子），触发控件交给 Popover，条目列表由内部
// 的 `internal::MenuSurface` 渲染并作为 Popover 的 content。打开 / 关闭、锚点定位、
// DismissLayer（外部点击 / Escape）与 FocusScope（初始焦点与关闭后焦点恢复）全部复用
// Popover，本组件不重复实现浮层设施。
//
// 键盘模型：打开时焦点落在 MenuSurface（容器自身可聚焦），方向键走共享的
// `RovingFocus`（selection_only：容器保持焦点，active_index 就是高亮项，不改变任何
// 条目的 checked）。这与 Select 弹出列表的模型一致，见 docs/components/selection_and_navigation.md。
//
// 子菜单模型：`MenuItemKind::submenu` 的 children 会作为独立浮层递归展开。指针悬停，
// Enter / Space / Right 都可进入子层；Left / Escape 返回父层。动作条目关闭整棵菜单，
// checkbox / radio 则保持当前层打开，便于连续调整。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_DROPDOWN_MENU_HPP
#define NANDINA_EXPERIMENT_WIDGET_DROPDOWN_MENU_HPP

#include "../reactive/event.hpp"
#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "internal/anchored_positioner.hpp"
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
} // namespace nandina::scene

namespace nandina::widget
{
    template<typename Component>
    struct ComponentTraits;

    class Popover;
    class ContextMenu;

    namespace internal
    {
        class MenuSurface;
    } // namespace internal

    /**
     * 锚定在触发控件旁的菜单：复用 Popover 托管与 menu_item 模型。
     *
     * 勾选状态的事实来源是 `items()`（`MenuItem::checked`），本组件只按
     * `MenuSelectionMode` 决定用户激活时如何变更。`none` 模式下 checkbox / radio
     * 条目仍然可激活并触发 `on_select`，只是不会被勾选（同 MenuSelection 的规则）。
     */
    class DropdownMenu: public scene::NanControl {
    public:
        explicit DropdownMenu(
            std::shared_ptr<scene::NanControl> trigger = nullptr,
            std::vector<MenuItem> items = {},
            theme::NanTheme theme = theme::default_theme()
        );
        ~DropdownMenu() override;

        [[nodiscard]] static auto create(
            std::shared_ptr<scene::NanControl> trigger = nullptr,
            std::vector<MenuItem> items = {},
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<DropdownMenu>;

        /// 替换触发控件（空指针拒绝；Popover 单子机制同款）。
        auto set_trigger(std::shared_ptr<scene::NanControl> trigger) -> DropdownMenu&;
        [[nodiscard]] auto trigger() const -> std::shared_ptr<scene::NanControl>;

        /// 替换条目列表。打开状态下会重建条目视图并重新呈现浮层（面板需要重新测量）。
        void set_items(std::vector<MenuItem> items);
        [[nodiscard]] auto items() const -> const std::vector<MenuItem>&;
        [[nodiscard]] auto item_count() const -> std::size_t;

        /// 勾选语义模式（默认 none = 纯动作菜单）。模式变更不回溯清理已有勾选。
        void set_selection_mode(MenuSelectionMode mode);
        [[nodiscard]] auto selection_mode() const -> MenuSelectionMode;
        /// 当前勾选条目的 id（顺序 = 条目顺序）。勾选状态的事实来源是 items()。
        [[nodiscard]] auto checked_ids() const -> std::vector<std::string>;
        /// 程序化静默设置勾选（不触发事件、不做 single 互斥清理）。
        auto set_checked(std::string_view id, bool checked) -> bool;

        void open();
        void close();
        void toggle();
        [[nodiscard]] auto is_open() const -> bool;

        /// 高亮项索引（-1 = 无）。打开时默认落在第一个可聚焦条目。
        [[nodiscard]] auto active_index() const -> int;

        void set_on_select(std::function<void(std::string_view id)> callback);
        [[nodiscard]] auto item_selected() const -> const reactive::Event<std::string>&;
        void set_on_submenu(std::function<void(std::string_view id)> callback);
        void set_on_close(std::function<void()> callback);

        void set_placement(internal::OverlayPlacement placement);
        void set_alignment(internal::OverlayAlignment alignment);
        void set_gap(float gap);

        /// 高级接口：以完整 NanTheme 覆盖控件主题（面板与条目一并覆盖，不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::DropdownMenuRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedDropdownMenuStyle;

        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        [[nodiscard]] auto z_index_hint() const -> int override;
        auto on_input(scene::InputEvent& event) -> bool override;
        void on_process(float dt) override;
        void on_exit_tree() override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        friend struct ComponentTraits<DropdownMenu>;
        friend class ContextMenu;

        /// Internal: bind the owning window's overlay portal. Called by
        /// `ComponentTraits<DropdownMenu>` so page authors never create an OverlayHost.
        void set_overlay_service(scene::OverlayHost* host) noexcept;
        /// ContextMenu positions this menu at a pointer rect while retaining the
        /// wrapped target as the focus/overlay owner.
        void set_external_anchor_rect(
            const std::shared_ptr<scene::NanControl>& owner,
            foundation::NanRect anchor
        ) noexcept;

        /// 用户激活某条目后的落地：按 kind 分派（见 menu_model.md 的规则）。
        void handle_activate(std::string_view id);
        /// 指针进入 submenu 条目时打开子层；进入普通条目时收起现有子层。
        void handle_hover(std::string_view id);
        void sync_submenu_items();
        /// 从任意深度关闭最外层菜单，交由浮层父子关系递归清理所有子层。
        void close_menu_tree();
        /// 触发 on_select 与 item_selected（用户激活路径）。
        void notify_select(std::string_view id);
        /// Popover 关闭回调：复位 typeahead / hover 并转发用户回调。
        void handle_closed();
        /// 把当前解析出的条目配方下发给 MenuSurface。
        void sync_surface_style();
        /// 打开状态下让浮层按新的条目列表重新测量；树遍历期间延迟到安全提交点。
        void reflow_if_open();

        /// 菜单表面（Popover 的 content）。持有条目模型并负责渲染 / 键盘。
        std::shared_ptr<internal::MenuSurface> surface_;
        /// 浮层基座：唯一子节点，触发控件挂在它下面。
        std::shared_ptr<Popover> popover_;
        std::shared_ptr<DropdownMenu> submenu_;
        std::weak_ptr<scene::OverlayHost> overlay_service_;
        std::string submenu_parent_id_;
        DropdownMenu* parent_menu_ = nullptr;
        bool nested_ = false;
        MenuSelection selection_;
        std::function<void(std::string_view)> on_select_;
        std::function<void(std::string_view)> on_submenu_;
        std::function<void()> on_close_;
        reactive::Event<std::string> item_selected_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::DropdownMenuRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_DROPDOWN_MENU_HPP
