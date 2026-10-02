#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace nandina::showcase
{
    enum class ComponentCategory {
        content,
        input,
        navigation,
        overlay,
        interaction,
    };

    enum class ComponentId {
        label,
        image,
        avatar,
        badge,
        chip,
        divider,
        progress_bar,
        spinner,
        skeleton,
        empty_state,
        card,
        button,
        checkbox,
        switch_control,
        toggle,
        toggle_group,
        button_group,
        radio_button,
        slider,
        text_field,
        text_area,
        select,
        combobox,
        tabs,
        breadcrumb,
        pagination,
        dialog,
        popover,
        dropdown_menu,
        context_menu,
        command_palette,
        hover_card,
        alert_dialog,
        tooltip,
        alert,
        pointer_area,
        gesture_area,
        count,
    };

    struct ComponentCategoryInfo {
        ComponentCategory id;
        std::string_view name;
    };

    struct ComponentInfo {
        ComponentId id;
        ComponentCategory category;
        std::string_view name;
        std::string_view description;
        bool experimental = false;
    };

    inline constexpr auto kComponentCategories = std::array {
        ComponentCategoryInfo {ComponentCategory::content, "内容与展示"},
        ComponentCategoryInfo {ComponentCategory::input, "输入与选择"},
        ComponentCategoryInfo {ComponentCategory::navigation, "导航"},
        ComponentCategoryInfo {ComponentCategory::overlay, "浮层与反馈"},
        ComponentCategoryInfo {ComponentCategory::interaction, "交互扩展"},
    };

    // 与 builtin_component_traits.hpp 中面向应用的 ComponentTraits 保持同步。
    inline constexpr auto kComponentCatalog = std::array {
        ComponentInfo {
            ComponentId::label,
            ComponentCategory::content,
            "Label",
            "展示文本并支持响应式文本绑定"
        },
        ComponentInfo {
            ComponentId::image,
            ComponentCategory::content,
            "Image",
            "从资源系统加载和展示图片"
        },
        ComponentInfo {
            ComponentId::avatar,
            ComponentCategory::content,
            "Avatar",
            "展示头像占位与名称语义"
        },
        ComponentInfo {
            ComponentId::badge,
            ComponentCategory::content,
            "Badge",
            "展示轻量状态或分类"
        },
        ComponentInfo {ComponentId::chip, ComponentCategory::content, "Chip", "展示可移除标签"},
        ComponentInfo {ComponentId::divider, ComponentCategory::content, "Divider", "分隔内容区域"},
        ComponentInfo {
            ComponentId::progress_bar,
            ComponentCategory::content,
            "ProgressBar",
            "展示确定性进度"
        },
        ComponentInfo {
            ComponentId::spinner,
            ComponentCategory::content,
            "Spinner",
            "展示未知时长的等待状态"
        },
        ComponentInfo {
            ComponentId::skeleton,
            ComponentCategory::content,
            "Skeleton",
            "在内容加载前保留最终版式"
        },
        ComponentInfo {
            ComponentId::empty_state,
            ComponentCategory::content,
            "EmptyState",
            "为空集合提供占位与操作引导"
        },
        ComponentInfo {ComponentId::card, ComponentCategory::content, "Card", "承载一组相关内容"},
        ComponentInfo {ComponentId::button, ComponentCategory::input, "Button", "触发一个语义操作"},
        ComponentInfo {
            ComponentId::checkbox,
            ComponentCategory::input,
            "Checkbox",
            "进行独立的布尔选择"
        },
        ComponentInfo {
            ComponentId::switch_control,
            ComponentCategory::input,
            "Switch",
            "即时启用或关闭设置"
        },
        ComponentInfo {
            ComponentId::toggle,
            ComponentCategory::input,
            "Toggle",
            "提供按钮外观的两态开关"
        },
        ComponentInfo {
            ComponentId::toggle_group,
            ComponentCategory::input,
            "ToggleGroup",
            "协调一组 Toggle 的选择与键盘漫游"
        },
        ComponentInfo {
            ComponentId::button_group,
            ComponentCategory::input,
            "ButtonGroup",
            "排列一组相关按钮"
        },
        ComponentInfo {
            ComponentId::radio_button,
            ComponentCategory::input,
            "RadioButton",
            "在 RadioGroup 中完成单项选择"
        },
        ComponentInfo {
            ComponentId::slider,
            ComponentCategory::input,
            "Slider",
            "在给定范围内选择数值"
        },
        ComponentInfo {
            ComponentId::text_field,
            ComponentCategory::input,
            "TextField",
            "接收单行文本输入"
        },
        ComponentInfo {
            ComponentId::text_area,
            ComponentCategory::input,
            "TextArea",
            "接收多行纯文本输入"
        },
        ComponentInfo {
            ComponentId::select,
            ComponentCategory::input,
            "Select",
            "从字符串选项中单选"
        },
        ComponentInfo {
            ComponentId::combobox,
            ComponentCategory::input,
            "Combobox",
            "筛选并选择下拉选项"
        },
        ComponentInfo {
            ComponentId::tabs,
            ComponentCategory::navigation,
            "Tabs",
            "在同级内容视图间切换"
        },
        ComponentInfo {
            ComponentId::breadcrumb,
            ComponentCategory::navigation,
            "Breadcrumb",
            "展示当前页面的层级路径"
        },
        ComponentInfo {
            ComponentId::pagination,
            ComponentCategory::navigation,
            "Pagination",
            "在分页内容间导航"
        },
        ComponentInfo {
            ComponentId::dialog,
            ComponentCategory::overlay,
            "Dialog",
            "展示模态内容并限制焦点"
        },
        ComponentInfo {
            ComponentId::popover,
            ComponentCategory::overlay,
            "Popover",
            "在触发控件附近展示非模态浮层"
        },
        ComponentInfo {
            ComponentId::dropdown_menu,
            ComponentCategory::overlay,
            "DropdownMenu",
            "展示锚定的动作或选择菜单"
        },
        ComponentInfo {
            ComponentId::context_menu,
            ComponentCategory::overlay,
            "ContextMenu",
            "通过右键或菜单键打开上下文菜单"
        },
        ComponentInfo {
            ComponentId::command_palette,
            ComponentCategory::overlay,
            "CommandPalette",
            "筛选并执行键盘优先的命令"
        },
        ComponentInfo {
            ComponentId::hover_card,
            ComponentCategory::overlay,
            "HoverCard",
            "悬停后展示可交互补充内容"
        },
        ComponentInfo {
            ComponentId::alert_dialog,
            ComponentCategory::overlay,
            "AlertDialog",
            "要求用户明确确认或取消"
        },
        ComponentInfo {
            ComponentId::tooltip,
            ComponentCategory::overlay,
            "Tooltip",
            "悬停后展示简短提示"
        },
        ComponentInfo {
            ComponentId::alert,
            ComponentCategory::overlay,
            "Alert",
            "以内联消息传达状态或反馈"
        },
        ComponentInfo {
            ComponentId::pointer_area,
            ComponentCategory::interaction,
            "PointerArea",
            "为任意子控件观察原始指针输入",
            true
        },
        ComponentInfo {
            ComponentId::gesture_area,
            ComponentCategory::interaction,
            "GestureArea",
            "识别点击、长按与拖拽手势",
            true
        },
    };

    [[nodiscard]] consteval auto valid_component_catalog() -> bool {
        if (kComponentCatalog.size() != static_cast<std::size_t>(ComponentId::count)) {
            return false;
        }

        for (std::size_t index = 0; index < kComponentCatalog.size(); ++index) {
            const auto& component = kComponentCatalog[index];
            if (static_cast<std::size_t>(component.id) != index || component.name.empty()
                || component.description.empty()
                || static_cast<std::size_t>(component.category) >= kComponentCategories.size())
            {
                return false;
            }
        }
        return true;
    }

    static_assert(valid_component_catalog(), "component catalog is incomplete or out of order");

    [[nodiscard]] constexpr auto component_info(const ComponentId id) -> const ComponentInfo& {
        return kComponentCatalog[static_cast<std::size_t>(id)];
    }

    [[nodiscard]] constexpr auto category_info(const ComponentCategory category)
        -> const ComponentCategoryInfo& {
        return kComponentCategories[static_cast<std::size_t>(category)];
    }
} // namespace nandina::showcase
