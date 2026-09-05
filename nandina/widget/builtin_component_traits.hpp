//
// widget/builtin_component_traits - authoring adapters for common built-in controls.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP

#include "build_context.hpp"
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
#include "radio_button.hpp"
#include "radio_group.hpp"
#include "select.hpp"
#include "slider.hpp"
#include "switch.hpp"
#include "tabs.hpp"
#include "text_field.hpp"
#include "tooltip.hpp"

namespace nandina::widget
{
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
    struct ComponentTraits<Tooltip> {
        template<typename Control>
            requires std::derived_from<Control, scene::NanControl>
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string text,
            std::shared_ptr<Control> trigger
        ) -> authoring::NodeBuilder<Tooltip> {
            return authoring::make<Tooltip>(std::move(text), std::move(trigger), ui.theme());
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
                .configure([selected](Select& select) { select.set_selected_index(selected); });
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
                    [title = std::move(title), content = std::move(content)](
                        Dialog& dialog
                    ) mutable {
                        dialog.set_title(std::move(title));
                        if (content) {
                            (void)dialog.set_content(std::move(content));
                        }
                    }
                );
        }
    };
}

#endif // NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP
