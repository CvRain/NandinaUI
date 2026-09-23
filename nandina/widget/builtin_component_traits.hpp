//
// widget/builtin_component_traits - authoring adapters for common built-in controls.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP

#include "build_context.hpp"
#include "alert.hpp"
#include "avatar.hpp"
#include "badge.hpp"
#include "button.hpp"
#include "card.hpp"
#include "checkbox.hpp"
#include "chip.hpp"
#include "dialog.hpp"
#include "divider.hpp"
#include "label.hpp"
#include "image.hpp"
#include "progress_bar.hpp"
#include "spinner.hpp"
#include "skeleton.hpp"
#include "empty_state.hpp"
#include "breadcrumb.hpp"
#include "button_group.hpp"
#include "pagination.hpp"
#include "pointer_area.hpp"
#include "gesture_area.hpp"
#include "radio_button.hpp"
#include "radio_group.hpp"
#include "select.hpp"
#include "slider.hpp"
#include "switch.hpp"
#include "tabs.hpp"
#include "toggle.hpp"
#include "toggle_group.hpp"
#include "text_field.hpp"
#include "text_area.hpp"
#include "tooltip.hpp"
#include "popover.hpp"
#include "dropdown_menu.hpp"

namespace nandina::widget
{
    template<>
    struct ComponentTraits<PointerArea> {
        [[nodiscard]] static auto make(const BuildContext&) -> authoring::NodeBuilder<PointerArea> {
            return authoring::make<PointerArea>();
        }
    };

    template<>
    struct ComponentTraits<GestureArea> {
        [[nodiscard]] static auto make(const BuildContext&) -> authoring::NodeBuilder<GestureArea> {
            return authoring::make<GestureArea>();
        }
    };

    template<>
    struct ComponentTraits<Image> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string source = {})
            -> authoring::NodeBuilder<Image> {
            return authoring::make<Image>(std::move(source))
                .configure([resources = ui.resource_manager()](Image& image) {
                    image.set_resource_manager(resources);
                });
        }
    };

    template<>
    struct ComponentTraits<Label> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string text = {})
            -> authoring::NodeBuilder<Label> {
            return authoring::make<Label>(ui.graph(), std::move(text), ui.theme());
        }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        [[nodiscard]] static auto make(const BuildContext& ui, Source& source)
            -> authoring::NodeBuilder<Label> {
            auto result = make(ui, std::string(source.get()));
            ui.bind(result.build(), &Label::set_text, source);
            return result;
        }
    };

    template<>
    struct ComponentTraits<Button> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string text)
            -> authoring::NodeBuilder<Button> {
            return authoring::make<Button>(std::move(text), ui.theme());
        }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        [[nodiscard]] static auto make(const BuildContext& ui, Source& source)
            -> authoring::NodeBuilder<Button> {
            auto result = make(ui, std::string(source.get()));
            ui.bind(result.build(), &Button::set_text, source);
            return result;
        }
    };

    template<>
    struct ComponentTraits<Checkbox> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string label,
            const bool checked = false
        ) -> authoring::NodeBuilder<Checkbox> {
            return authoring::make<Checkbox>(std::move(label), checked, ui.theme());
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<bool>& checked,
            std::string label
        ) -> authoring::NodeBuilder<Checkbox> {
            auto result = make(ui, std::move(label), checked.get());
            const auto control = result.build();
            ui.bind(control, &Checkbox::set_checked, checked);
            ui.connect(control->checked_changed(), [&checked](const bool current) {
                if (checked.peek() != current) {
                    checked.set(current);
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<Switch> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string label,
            const bool checked = false
        ) -> authoring::NodeBuilder<Switch> {
            return authoring::make<Switch>(std::move(label), checked, ui.theme());
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<bool>& checked,
            std::string label
        ) -> authoring::NodeBuilder<Switch> {
            auto result = make(ui, std::move(label), checked.get());
            const auto control = result.build();
            ui.bind(control, &Switch::set_checked, checked);
            ui.connect(control->checked_changed(), [&checked](const bool current) {
                if (checked.peek() != current) {
                    checked.set(current);
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<Slider> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string label,
            const float value = 0.0F,
            const float minimum = 0.0F,
            const float maximum = 1.0F,
            const float step = 0.01F
        ) -> authoring::NodeBuilder<Slider> {
            return authoring::make<Slider>(
                std::move(label), value, minimum, maximum, step, ui.theme()
            );
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<float>& value,
            std::string label,
            const float minimum = 0.0F,
            const float maximum = 1.0F,
            const float step = 0.01F
        ) -> authoring::NodeBuilder<Slider> {
            auto result = make(ui, std::move(label), value.get(), minimum, maximum, step);
            const auto control = result.build();
            ui.bind(control, &Slider::set_value, value);
            ui.connect(control->value_changed(), [&value](const float current) {
                if (std::abs(value.peek() - current) > foundation::nan_epsilon) {
                    value.set(current);
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<TextField> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string value,
            std::string placeholder
        ) -> authoring::NodeBuilder<TextField> {
            return authoring::make<TextField>(
                std::move(value), std::move(placeholder), ui.theme()
            );
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<std::string>& value,
            std::string placeholder
        ) -> authoring::NodeBuilder<TextField> {
            auto result = make(ui, std::string(value.get()), std::move(placeholder));
            const auto field = result.build();
            ui.bind(field, &TextField::set_value, value);
            ui.connect(field->value_changed(), [&value](const std::string_view current) {
                if (value.peek() != current) {
                    value.set(std::string(current));
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<TextArea> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string value,
            std::string placeholder
        ) -> authoring::NodeBuilder<TextArea> {
            return authoring::make<TextArea>(std::move(value), ui.theme())
                .configure([placeholder = std::move(placeholder)](TextArea& area) mutable {
                    area.set_placeholder(std::move(placeholder));
                });
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<std::string>& value,
            std::string placeholder
        ) -> authoring::NodeBuilder<TextArea> {
            auto result = make(ui, std::string(value.get()), std::move(placeholder));
            const auto area = result.build();
            ui.bind(area, &TextArea::set_value, value);
            ui.connect(area->value_changed(), [&value](const std::string_view current) {
                if (value.peek() != current) {
                    value.set(std::string(current));
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<Badge> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string text)
            -> authoring::NodeBuilder<Badge> {
            return authoring::make<Badge>(std::move(text), ui.theme());
        }
    };

    template<>
    struct ComponentTraits<Card> {
        [[nodiscard]] static auto make(const BuildContext& ui) -> authoring::NodeBuilder<Card> {
            return authoring::make<Card>(ui.theme());
        }
    };

    template<>
    struct ComponentTraits<ProgressBar> {
        [[nodiscard]] static auto make(const BuildContext& ui, const float value = 0.0F)
            -> authoring::NodeBuilder<ProgressBar> {
            return authoring::make<ProgressBar>(value, ui.theme());
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<float>& value
        ) -> authoring::NodeBuilder<ProgressBar> {
            auto result = make(ui, value.get());
            ui.bind(result.build(), &ProgressBar::set_value, value);
            return result;
        }
    };

    template<>
    struct ComponentTraits<Spinner> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string label = {})
            -> authoring::NodeBuilder<Spinner> {
            return authoring::make<Spinner>(ui.theme())
                .configure([label = std::move(label)](Spinner& spinner) mutable {
                    if (!label.empty()) {
                        spinner.set_label(std::move(label));
                    }
                });
        }
    };

    template<>
    struct ComponentTraits<Skeleton> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            const SkeletonVariant variant = SkeletonVariant::text,
            const int lines = 1
        ) -> authoring::NodeBuilder<Skeleton> {
            return authoring::make<Skeleton>(ui.theme())
                .configure([variant, lines](Skeleton& skeleton) {
                    skeleton.set_variant(variant);
                    skeleton.set_lines(lines);
                });
        }
    };

    template<>
    struct ComponentTraits<EmptyState> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string title = {},
            std::string description = {}
        ) -> authoring::NodeBuilder<EmptyState> {
            return authoring::make<EmptyState>(ui.theme())
                .configure([title = std::move(title), description = std::move(description)](
                               EmptyState& empty_state
                           ) mutable {
                    empty_state.set_title(std::move(title));
                    empty_state.set_description(std::move(description));
                });
        }
    };

    template<>
    struct ComponentTraits<Alert> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            theme::AlertTone tone,
            std::string title,
            std::string description = {}
        ) -> authoring::NodeBuilder<Alert> {
            return authoring::make<Alert>(ui.theme())
                .configure([tone, title = std::move(title), description = std::move(description)](
                               Alert& alert
                           ) mutable {
                    alert.set_tone(tone);
                    alert.set_title(std::move(title));
                    alert.set_description(std::move(description));
                });
        }
    };

    template<>
    struct ComponentTraits<RadioButton> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string label)
            -> authoring::NodeBuilder<RadioButton> {
            return authoring::make<RadioButton>(std::move(label), nullptr, ui.theme());
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string label,
            std::shared_ptr<RadioGroup> group
        ) -> authoring::NodeBuilder<RadioButton> {
            return authoring::make<RadioButton>(std::move(label), std::move(group), ui.theme());
        }
    };

    template<>
    struct ComponentTraits<Tabs> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::vector<std::string> labels,
            const int selected = 0
        ) -> authoring::NodeBuilder<Tabs> {
            return authoring::make<Tabs>(std::move(labels), ui.theme())
                .configure([selected](Tabs& tabs) { tabs.set_selected_index(selected); });
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<int>& selected,
            std::vector<std::string> labels
        ) -> authoring::NodeBuilder<Tabs> {
            auto result = make(ui, std::move(labels), selected.get());
            const auto control = result.build();
            ui.bind(control, &Tabs::set_selected_index, selected);
            ui.connect(control->selection_changed(), [&selected](const int index) {
                if (selected.peek() != index) {
                    selected.set(index);
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<Toggle> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string text)
            -> authoring::NodeBuilder<Toggle> {
            return authoring::make<Toggle>(std::move(text), ui.theme());
        }

        /// 组内成员：注册到共享的 ToggleGroup，方向键 / typeahead 由组统一漫游。
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string text,
            std::shared_ptr<ToggleGroup> group
        ) -> authoring::NodeBuilder<Toggle> {
            return authoring::make<Toggle>(std::move(text), std::move(group), ui.theme());
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<bool>& checked,
            std::string text
        ) -> authoring::NodeBuilder<Toggle> {
            auto result = authoring::make<Toggle>(std::move(text), ui.theme())
                              .configure([&checked](Toggle& toggle) {
                                  toggle.set_checked(checked.get());
                              });
            const auto control = result.build();
            ui.bind(control, &Toggle::set_checked, checked);
            ui.connect(control->checked_changed(), [&checked](const bool current) {
                if (checked.peek() != current) {
                    checked.set(current);
                }
            });
            return result;
        }
    };

    /**
     * ToggleGroup 不是场景节点（同 RadioGroup），因此这里返回 `shared_ptr` 而不是
     * NodeBuilder：`BuildContext::make<T>()` 的约束是 `derived_from<T, scene::NanNode>`，
     * 组要走 `ComponentTraits<ToggleGroup>::make(ui)` 或 `ToggleGroup::create()`。
     */
    template<>
    struct ComponentTraits<ToggleGroup> {
        [[nodiscard]] static auto make(const BuildContext&) -> std::shared_ptr<ToggleGroup> {
            return ToggleGroup::create();
        }
    };

    template<>
    struct ComponentTraits<Tooltip> {
        template<typename Control>
            requires std::derived_from<Control, scene::NanControl>
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string text,
            std::shared_ptr<Control> trigger
        ) -> authoring::NodeBuilder<Tooltip> {
            return authoring::make<Tooltip>(std::move(text), std::move(trigger), ui.theme())
                .configure([&ui](Tooltip& tooltip) {
                    tooltip.set_overlay_service(
                        ui.has_overlay_host() ? &ui.overlay_host() : nullptr
                    );
                });
        }
    };

    template<>
    struct ComponentTraits<Popover> {
        template<typename Control = scene::NanControl>
            requires std::derived_from<Control, scene::NanControl>
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::shared_ptr<Control> trigger = nullptr,
            std::shared_ptr<scene::NanControl> content = nullptr
        ) -> authoring::NodeBuilder<Popover> {
            return authoring::make<Popover>(std::move(trigger), std::move(content), ui.theme())
                .configure([&ui](Popover& popover) {
                    popover.set_overlay_service(
                        ui.has_overlay_host() ? &ui.overlay_host() : nullptr
                    );
                });
        }
    };

    /**
     * DropdownMenu 是场景节点：注入覆盖层服务与主题，条目列表在构建后由调用方用
     * `set_items()` 提供或替换（同 Popover 的内容槽位）。
     */
    template<>
    struct ComponentTraits<DropdownMenu> {
        template<typename Control = scene::NanControl>
            requires std::derived_from<Control, scene::NanControl>
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::shared_ptr<Control> trigger = nullptr,
            std::vector<MenuItem> items = {}
        ) -> authoring::NodeBuilder<DropdownMenu> {
            return authoring::make<DropdownMenu>(std::move(trigger), std::move(items), ui.theme())
                .configure([&ui](DropdownMenu& menu) {
                    menu.set_overlay_service(
                        ui.has_overlay_host() ? &ui.overlay_host() : nullptr
                    );
                });
        }
    };

    template<>
    struct ComponentTraits<Select> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::vector<std::string> options,
            const int selected = 0
        ) -> authoring::NodeBuilder<Select> {
            return authoring::make<Select>(std::move(options), ui.theme())
                .configure(
                    [&ui, selected](Select& select) {
                        select.set_selected_index(selected);
                        select.set_overlay_service(
                            ui.has_overlay_host() ? &ui.overlay_host() : nullptr
                        );
                    }
                );
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<int>& selected,
            std::vector<std::string> options
        ) -> authoring::NodeBuilder<Select> {
            auto result = make(ui, std::move(options), selected.get());
            const auto control = result.build();
            ui.bind(control, &Select::set_selected_index, selected);
            ui.connect(control->selection_changed(), [&selected](const int index) {
                if (selected.peek() != index) {
                    selected.set(index);
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<Divider> {
        [[nodiscard]] static auto make(const BuildContext& ui)
            -> authoring::NodeBuilder<Divider> {
            return authoring::make<Divider>(ui.theme());
        }
    };

    template<>
    struct ComponentTraits<Avatar> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string name)
            -> authoring::NodeBuilder<Avatar> {
            return authoring::make<Avatar>(std::move(name), ui.theme());
        }
    };

    template<>
    struct ComponentTraits<Chip> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string text,
            const bool removable = false
        ) -> authoring::NodeBuilder<Chip> {
            return authoring::make<Chip>(std::move(text), removable, ui.theme());
        }
    };

    template<>
    struct ComponentTraits<Dialog> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string title,
            std::shared_ptr<scene::NanControl> content = nullptr
        ) -> authoring::NodeBuilder<Dialog> {
            return authoring::make<Dialog>(ui.theme())
                .configure(
                    [&ui, title = std::move(title), content = std::move(content)](
                        Dialog& dialog
                    ) mutable {
                        dialog.set_title(std::move(title));
                        if (content) {
                            (void)dialog.set_content(std::move(content));
                        }
                        dialog.set_overlay_service(
                            ui.has_overlay_host() ? &ui.overlay_host() : nullptr
                        );
                    }
                );
        }
    };
    /**
     * ButtonGroup 是场景节点，走 NodeBuilder 正常路径；方向复用 `LayoutAxis`。
     */
    template<>
    struct ComponentTraits<ButtonGroup> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            const LayoutAxis axis = LayoutAxis::horizontal
        ) -> authoring::NodeBuilder<ButtonGroup> {
            return authoring::make<ButtonGroup>(axis, ui.theme());
        }
    };

    /** Breadcrumb 是场景节点；条目在构建后由调用方用 `add_item()` 追加。 */
    template<>
    struct ComponentTraits<Breadcrumb> {
        [[nodiscard]] static auto make(const BuildContext& ui)
            -> authoring::NodeBuilder<Breadcrumb> {
            return authoring::make<Breadcrumb>(ui.theme());
        }
    };

    /** Pagination 是场景节点；页码槽位由 page_count / current_page 派生。 */
    template<>
    struct ComponentTraits<Pagination> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            const int page_count = 0,
            const int current_page = 1
        ) -> authoring::NodeBuilder<Pagination> {
            return authoring::make<Pagination>(ui.theme())
                .configure([page_count, current_page](Pagination& pagination) {
                    pagination.set_page_count(page_count);
                    pagination.set_current_page(current_page);
                });
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<int>& current_page,
            const int page_count
        ) -> authoring::NodeBuilder<Pagination> {
            auto result = make(ui, page_count, current_page.get());
            const auto control = result.build();
            ui.bind(control, &Pagination::set_current_page, current_page);
            ui.connect(control->page_changed(), [&current_page](const int page) {
                if (current_page.peek() != page) {
                    current_page.set(page);
                }
            });
            return result;
        }
    };
}

#endif // NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP
