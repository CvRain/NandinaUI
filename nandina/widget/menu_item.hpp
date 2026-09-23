//
// widget/menu_item - 菜单族共享的条目模型与勾选规则。
//
// 阶段 4 的 DropdownMenu / ContextMenu / Combobox / CommandPalette 面向同一批需求：
// 一列带文本、快捷键提示、禁用与勾选状态的条目，需要方向键漫游、typeahead 与单选/多选。
// 本文件把「条目长什么样」与「哪些条目能被聚焦/激活、勾选如何变化」收敛成一份模型，
// 组件只负责渲染与定位，不再各自定义选项结构。
//
// 设计规则见 docs/references/menu_model.md。本文件不依赖 scene / render，可脱离窗口单测。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_MENU_ITEM_HPP
#define NANDINA_EXPERIMENT_WIDGET_MENU_ITEM_HPP

#include "../reactive/event.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::widget
{
    /**
     * 菜单条目的语义种类。
     *
     * 种类决定该条目是否参与键盘漫游与选择，而不决定它长什么样：外观由组件的配方
     * 与状态层表达，同一 kind 在不同组件里可以有不同造型。
     */
    enum class MenuItemKind: std::uint8_t {
        /// 触发一个动作（默认）。不承载勾选状态。
        action,
        /// 独立可勾选项：`checked` 有意义，多选语义。
        checkbox,
        /// 单选组内的可勾选项：`checked` 有意义，同层互斥。
        radio,
        /// 展开子菜单；子项放在 `children` 中。
        submenu,
        /// 分隔线：不可聚焦、不可激活、不需要 label。
        separator,
        /// 分组标题：不可聚焦、不可激活，只有展示语义。
        label,
    };

    /**
     * 一个菜单条目。纯值类型：复制即得到独立副本，便于组件保存与比较。
     *
     * `id` 是稳定身份，选择状态、keying 与事件都以它为准；`label` 同时是 typeahead 的
     * 匹配文本。`shortcut` 只用于展示，不参与匹配（见 menu_model.md）。
     */
    struct MenuItem {
        std::string id;
        std::string label;
        /// 仅供展示的快捷键提示（例如 "Ctrl+S"），不参与 typeahead。
        std::string shortcut;
        /**
         * 可选图标：资源系统中的资源名，由视图通过 ResourceManager 解析。
         * 模型不持有场景节点，因此图标槽位不属于模型（需要任意控件时由组件提供槽位）。
         */
        std::string icon;
        MenuItemKind kind = MenuItemKind::action;
        bool disabled = false;
        /// 仅 checkbox / radio 有意义的初始勾选状态。唯一事实来源（见 MenuSelection）。
        bool checked = false;
        /// kind == submenu 时的子项。
        std::vector<MenuItem> children;
    };

    /**
     * 该条目是否参与键盘漫游。
     *
     * separator / label 是结构性条目，方向键必须跳过它们。**disabled 条目仍然可聚焦**：
     * 与 ARIA menu 一致 —— 用户需要能用方向键走到禁用项上，才能知道它存在以及为什么不可用；
     * 「不可聚焦」与「不可激活」是两件事。
     */
    [[nodiscard]] auto menu_item_is_focusable(const MenuItem& item) noexcept -> bool;

    /// 该条目是否可被用户激活（Enter / Space / 点击）。可聚焦且未禁用即可激活。
    [[nodiscard]] auto menu_item_is_activatable(const MenuItem& item) noexcept -> bool;

    /**
     * 该条目用于 typeahead 的匹配文本。
     *
     * 只有 label 参与匹配：`shortcut` 是展示提示，把 "Ctrl+S" 也纳入匹配会让输入 'c'
     * 意外命中多个条目。结构性条目返回空串 —— 它们在漫游中本就不可聚焦，因此不会被命中。
     */
    [[nodiscard]] auto menu_item_typeahead_text(const MenuItem& item) noexcept -> std::string_view;

    /**
     * 在一层条目中按 id 查找。只查给定这一层，不递归 children。
     *
     * 子菜单是独立的一层：调用方手上已经有正确的层级，递归查找会让「同 id 出现在
     * 父子两层」产生歧义。
     */
    [[nodiscard]] auto find_menu_item(std::vector<MenuItem>& items, std::string_view id)
        -> MenuItem*;
    [[nodiscard]] auto find_menu_item(const std::vector<MenuItem>& items, std::string_view id)
        -> const MenuItem*;

    /// 勾选语义：决定勾选如何被用户操作改变。
    enum class MenuSelectionMode: std::uint8_t {
        /// 纯动作菜单：不保留勾选状态，toggle 一律不生效。
        none,
        /// 单选：勾选一项会清除同层的其他 radio 项。
        single,
        /// 多选：各项独立。
        multiple,
    };

    /**
     * 勾选状态模型。
     *
     * **不额外保存一份状态**：`MenuItem::checked` 是唯一事实来源，本类只提供变更规则
     * （single 的互斥清理、disabled 与结构性条目的拒绝）与变更通知。这样组件渲染时直接读
     * 条目自身的 `checked`，不会出现「模型说选中、条目说没选中」的分裂。
     */
    class MenuSelection {
    public:
        MenuSelection() = default;
        explicit MenuSelection(MenuSelectionMode mode) noexcept:
            mode_(mode) {}

        void set_mode(MenuSelectionMode mode) noexcept {
            mode_ = mode;
        }
        [[nodiscard]] auto mode() const noexcept -> MenuSelectionMode {
            return mode_;
        }

        /**
         * 用户切换某个条目的勾选状态，并触发 changed(id)。
         *
         * @return 状态是否真的发生了变化。以下情况返回 false 且不改动任何条目：
         *         - mode 为 none（该菜单没有勾选语义）；
         *         - id 不存在；
         *         - 条目不是 checkbox / radio；
         *         - 条目被 disabled。
         *
         * single 模式下选中一项会清除**同层**的其他 radio 项；checkbox 项不受影响
         * （同一层可以既有 radio 又有 checkbox）。
         */
        auto toggle(std::vector<MenuItem>& items, std::string_view id) -> bool;

        /// 静默设置勾选状态：不触发事件，也不做 single 互斥清理。程序侧同步用。
        static auto set_checked(std::vector<MenuItem>& items, std::string_view id, bool checked)
            -> bool;

        [[nodiscard]] static auto is_checked(const std::vector<MenuItem>& items, std::string_view id)
            -> bool;
        /// 本层所有 checkbox / radio 中处于勾选状态的 id，按条目顺序。
        [[nodiscard]] static auto checked_ids(const std::vector<MenuItem>& items)
            -> std::vector<std::string>;

        /// 勾选状态被用户改变时触发，携带条目 id。
        [[nodiscard]] auto changed() const -> const reactive::Event<std::string>& {
            return changed_;
        }

    private:
        MenuSelectionMode mode_ = MenuSelectionMode::none;
        reactive::Event<std::string> changed_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_MENU_ITEM_HPP
