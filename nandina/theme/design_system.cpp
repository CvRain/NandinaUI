/**
 * theme/design_system — 组件级解析与框架默认配方。
 *
 * 解析策略：
 *   1. 从 `base` 配方出发，按 selector 顺序应用配方书规则（后匹配者胜）；
 *   2. 跨切面状态变换由解析器代码完成；Button 的交互状态色保留为独立叠加片段，
 *      disabled 透明度仍在解析末尾应用；
 *   3. 输出为片段组合的解析结果（ResolvedButtonStyle 等）。
 *
 * 遗留 NanStyle 规则经 ThemeManager::merge_style 转为配方规则，追加在配方书尾部，
 * 因此仍然生效（后匹配者胜）。tone / treatment / size / state 的语义全部来自
 * default_design_system() 的 base + rules。
 */

#include "design_system.hpp"

namespace nandina::theme
{
    namespace
    {
        /** ButtonRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const ButtonRecipe& recipe,
            const ButtonTone tone
        ) -> ResolvedButtonStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .label = resolve(system, appearance, recipe.label),
                .focus = resolve(system, appearance, recipe.focus),
                .state_layer = {
                    .hover = resolve_color(system, appearance, recipe.state_layer.hover, tone),
                    .pressed = resolve_color(system, appearance, recipe.state_layer.pressed, tone),
                },
                .ripple = {
                    .color = resolve_color(system, appearance, recipe.ripple.color, tone),
                    .duration = resolve_scalar(system, appearance, recipe.ripple.duration),
                },
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** CheckboxRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const CheckboxRecipe& recipe
        ) -> ResolvedCheckboxStyle {
            return {
                .indicator = resolve(system, appearance, recipe.indicator),
                .check = resolve_color(system, appearance, recipe.check),
                .label = resolve(system, appearance, recipe.label),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** SliderRecipe → 解析后的片段组合。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const SliderRecipe& recipe
        ) -> ResolvedSliderStyle {
            return {
                .inactive_track = resolve(system, appearance, recipe.inactive_track),
                .active_track = resolve(system, appearance, recipe.active_track),
                .thumb = resolve(system, appearance, recipe.thumb),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        // ─── disabled 状态变换（跨切面，由解析器按 token 应用） ──────────────

        /** 按因子缩放已解析颜色的 alpha。 */
        void scale_alpha(NanColor& color, const float factor) {
            color = color.with_alpha(color.alpha() * factor);
        }

        void apply_checkbox_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedCheckboxStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.indicator.fill, alpha);
            scale_alpha(style.indicator.border, alpha);
            scale_alpha(style.check, alpha);
            scale_alpha(style.label.color, alpha);
        }

        void apply_slider_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedSliderStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.inactive_track.box.fill, alpha);
            scale_alpha(style.active_track.box.fill, alpha);
            scale_alpha(style.thumb.box.fill, alpha);
            scale_alpha(style.focus.color, alpha);
        }

        /** TextFieldRecipe → 解析后的片段组合。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const TextFieldRecipe& recipe
        ) -> ResolvedTextFieldStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .value = resolve(system, appearance, recipe.value),
                .placeholder = resolve(system, appearance, recipe.placeholder),
                .selection = resolve_color(system, appearance, recipe.selection),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        void apply_text_field_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedTextFieldStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.container.fill, alpha);
            scale_alpha(style.container.border, alpha);
            scale_alpha(style.value.color, alpha);
            scale_alpha(style.placeholder.color, alpha);
        }

        /** SwitchRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const SwitchRecipe& recipe
        ) -> ResolvedSwitchStyle {
            return {
                .track = resolve(system, appearance, recipe.track),
                .thumb = resolve(system, appearance, recipe.thumb.box),
                .label = resolve(system, appearance, recipe.label),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** Switch disabled 变换：轨道 / 拇指 / 文本颜色 ×opacity.disabled。 */
        void apply_switch_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedSwitchStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.track.fill, alpha);
            scale_alpha(style.track.border, alpha);
            scale_alpha(style.thumb.fill, alpha);
            scale_alpha(style.label.color, alpha);
        }

        /** BadgeRecipe → 解析后的片段组合（纯展示，无状态变换）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const BadgeRecipe& recipe
        ) -> ResolvedBadgeStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .label = resolve(system, appearance, recipe.label),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** CardRecipe → 解析后的片段组合（纯容器，无状态变换）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const CardRecipe& recipe
        ) -> ResolvedCardStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .shadow = resolve(system, appearance, recipe.shadow),
                .metrics = {
                    .padding_x = resolve_scalar(system, appearance, recipe.metrics.padding_x),
                    .padding_y = resolve_scalar(system, appearance, recipe.metrics.padding_y),
                    .min_height = resolve_scalar(system, appearance, recipe.metrics.min_height),
                },
            };
        }

        /** ProgressBarRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const ProgressBarRecipe& recipe
        ) -> ResolvedProgressBarStyle {
            return {
                .track = resolve(system, appearance, recipe.track),
                .fill = resolve(system, appearance, recipe.fill),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** ProgressBar disabled 变换：轨道 / 填充颜色 ×opacity.disabled。 */
        void apply_progress_bar_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedProgressBarStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.track.fill, alpha);
            scale_alpha(style.track.border, alpha);
            scale_alpha(style.fill.fill, alpha);
            scale_alpha(style.fill.border, alpha);
        }

        /** RadioButtonRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const RadioButtonRecipe& recipe
        ) -> ResolvedRadioButtonStyle {
            return {
                .indicator = resolve(system, appearance, recipe.indicator),
                .dot = resolve_color(system, appearance, recipe.dot),
                .label = resolve(system, appearance, recipe.label),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** RadioButton disabled 变换：指示器 / 内点 / 文本颜色 ×opacity.disabled。 */
        void apply_radio_button_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedRadioButtonStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.indicator.fill, alpha);
            scale_alpha(style.indicator.border, alpha);
            scale_alpha(style.dot, alpha);
            scale_alpha(style.label.color, alpha);
        }

        /** TabsRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const TabsRecipe& recipe
        ) -> ResolvedTabsStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .selected_background = resolve(system, appearance, recipe.selected_background),
                .label = resolve(system, appearance, recipe.label),
                .label_selected = resolve(system, appearance, recipe.label_selected),
                .indicator = resolve_color(system, appearance, recipe.indicator),
                .indicator_thickness =
                    resolve_scalar(system, appearance, recipe.indicator_thickness),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** TooltipRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const TooltipRecipe& recipe
        ) -> ResolvedTooltipStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .label = resolve(system, appearance, recipe.label),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** SelectRecipe → 解析后的片段组合（配方即事实来源）。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const SelectRecipe& recipe
        ) -> ResolvedSelectStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .popup = resolve(system, appearance, recipe.popup),
                .value = resolve(system, appearance, recipe.value),
                .option = resolve(system, appearance, recipe.option),
                .option_selected = resolve(system, appearance, recipe.option_selected),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** Select disabled 变换：字段 / 值文本颜色 ×opacity.disabled。 */
        void apply_select_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedSelectStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.container.fill, alpha);
            scale_alpha(style.container.border, alpha);
            scale_alpha(style.value.color, alpha);
        }

        /** DividerRecipe → 解析后的片段组合。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const DividerRecipe& recipe
        ) -> ResolvedDividerStyle {
            return {
                .color = resolve_color(system, appearance, recipe.color),
                .thickness = resolve_scalar(system, appearance, recipe.thickness),
                .preferred_length = resolve_scalar(system, appearance, recipe.preferred_length),
            };
        }

        /** AvatarRecipe → 解析后的片段组合。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const AvatarRecipe& recipe
        ) -> ResolvedAvatarStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .label = resolve(system, appearance, recipe.label),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** ChipRecipe → 解析后的片段组合。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const ChipRecipe& recipe
        ) -> ResolvedChipStyle {
            return {
                .container = resolve(system, appearance, recipe.container),
                .label = resolve(system, appearance, recipe.label),
                .remove_color = resolve_color(system, appearance, recipe.remove_color),
                .focus = resolve(system, appearance, recipe.focus),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** DialogRecipe → 解析后的片段组合。 */
        [[nodiscard]] auto resolve_recipe(
            const DesignSystem& system,
            const ColorAppearance appearance,
            const DialogRecipe& recipe
        ) -> ResolvedDialogStyle {
            return {
                .scrim = resolve_color(system, appearance, recipe.scrim),
                .panel = resolve(system, appearance, recipe.panel),
                .title = resolve(system, appearance, recipe.title),
                .metrics = resolve(system, appearance, recipe.metrics),
            };
        }

        /** Tabs disabled 变换：标签 / 指示条颜色 ×opacity.disabled。 */
        void apply_tabs_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedTabsStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.label.color, alpha);
            scale_alpha(style.label_selected.color, alpha);
            scale_alpha(style.indicator, alpha);
        }

        /** Button disabled 变换：全部颜色 ×opacity.disabled，焦点环隐去。 */
        void apply_button_disabled(
            const DesignSystem& system,
            const ColorAppearance appearance,
            ResolvedButtonStyle& style
        ) {
            const float alpha = resolve_scalar(
                system,
                appearance,
                ThemeScalar::token(ScalarToken::opacity_disabled)
            );
            scale_alpha(style.container.fill, alpha);
            scale_alpha(style.container.border, alpha);
            scale_alpha(style.label.color, alpha);
            style.focus.color = style.focus.color.with_alpha(0.0F);
        }
    } // namespace

    auto button_state_layer_color(
        const ResolvedButtonStyle& style,
        const ButtonVisualState state
    ) -> NanColor {
        if (state == ButtonVisualState::hovered || state == ButtonVisualState::focused) {
            return style.state_layer.hover;
        }
        if (state == ButtonVisualState::pressed) {
            return style.state_layer.pressed;
        }
        return style.state_layer.hover.with_alpha(0.0F);
    }

    /**
     * 解析 Button 配方。
     *
     * 流程：base → 配方书规则（treatment / size / tone / state，后匹配者胜）→
     * 独立状态层保留在解析结果中（不改写 container.fill）→ disabled 变换。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param tone       语义色家族（accent 引用与状态层表达式依赖）
     * @param treatment  视觉处理方式
     * @param size       尺寸档位
     * @param state      交互状态
     * @return 片段组合的解析结果
     */
    auto resolve_button(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const ButtonTone tone,
        const ButtonTreatment treatment,
        const ButtonSize size,
        const ButtonVisualState state
    ) -> ResolvedButtonStyle {
        auto style = resolve_recipe(system, appearance, system.components.button.base, tone);
        for (const auto& rule: system.components.button.rules) {
            if (rule.selector.matches(tone, treatment, size, state)) {
                apply_rule(system, appearance, style, rule, tone);
            }
        }
        if (state == ButtonVisualState::disabled) {
            apply_button_disabled(system, appearance, style);
        }
        return style;
    }

    /**
     * 解析 Checkbox 配方。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param checked    勾选状态（决定指示器 filled / outline）
     * @param state      交互状态
     * @return 片段组合的解析结果
     */
    auto resolve_checkbox(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const bool checked,
        const CheckboxVisualState state
    ) -> ResolvedCheckboxStyle {
        auto style = resolve_recipe(system, appearance, system.components.checkbox.base);
        for (const auto& rule: system.components.checkbox.rules) {
            if ((rule.checked && *rule.checked != checked) || (rule.state && *rule.state != state)) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (state == CheckboxVisualState::disabled) {
            apply_checkbox_disabled(system, appearance, style);
        }
        return style;
    }

    /**
     * 解析 Slider 配方。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param state      交互状态
     * @return 片段组合的解析结果
     */
    auto resolve_slider(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const SliderVisualState state
    ) -> ResolvedSliderStyle {
        auto style = resolve_recipe(system, appearance, system.components.slider.base);
        for (const auto& rule: system.components.slider.rules) {
            if (rule.state && *rule.state != state) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (state == SliderVisualState::disabled) {
            apply_slider_disabled(system, appearance, style);
        }
        return style;
    }

    /**
     * 解析 TextField 配方。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param state      位掩码交互状态（focused / disabled / invalid）
     * @return 片段组合的解析结果
     */
    auto resolve_text_field(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const TextFieldVisualState state
    ) -> ResolvedTextFieldStyle {
        auto style = resolve_recipe(system, appearance, system.components.text_field.base);
        for (const auto& rule: system.components.text_field.rules) {
            if (rule.state && !has_text_field_state(state, *rule.state)) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (has_text_field_state(state, TextFieldVisualState::disabled)) {
            apply_text_field_disabled(system, appearance, style);
        }
        return style;
    }

    /**
     * 解析 Switch 配方。
     *
     * 流程：base → 配方书规则（checked + state 选择器，后匹配者胜）→ disabled 变换。
     *
     * @param system     设计系统快照
     * @param appearance 当前外观
     * @param checked    勾选状态（决定轨道 filled / outline）
     * @param state      交互状态
     * @return 片段组合的解析结果
     */
    auto resolve_switch(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const bool checked,
        const SwitchVisualState state
    ) -> ResolvedSwitchStyle {
        auto style = resolve_recipe(system, appearance, system.components.switch_component.base);
        for (const auto& rule: system.components.switch_component.rules) {
            if ((rule.checked && *rule.checked != checked) || (rule.state && *rule.state != state)) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (state == SwitchVisualState::disabled) {
            apply_switch_disabled(system, appearance, style);
        }
        return style;
    }

    /**
     * 解析 Badge 配方（纯展示：base → 规则列表，后匹配者胜，无状态变换）。
     */
    auto resolve_badge(
        const DesignSystem& system,
        const ColorAppearance appearance
    ) -> ResolvedBadgeStyle {
        auto style = resolve_recipe(system, appearance, system.components.badge.base);
        for (const auto& rule: system.components.badge.rules) {
            apply_rule(system, appearance, style, rule);
        }
        return style;
    }

    /**
     * 解析 Card 配方（纯容器：base → 规则列表，后匹配者胜，无状态变换）。
     */
    auto resolve_card(
        const DesignSystem& system,
        const ColorAppearance appearance
    ) -> ResolvedCardStyle {
        auto style = resolve_recipe(system, appearance, system.components.card.base);
        for (const auto& rule: system.components.card.rules) {
            apply_rule(system, appearance, style, rule);
        }
        return style;
    }

    /**
     * 解析 ProgressBar 配方（base → 规则列表，后匹配者胜 → disabled 变换）。
     */
    auto resolve_progress_bar(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const ProgressBarVisualState state
    ) -> ResolvedProgressBarStyle {
        auto style = resolve_recipe(system, appearance, system.components.progress_bar.base);
        for (const auto& rule: system.components.progress_bar.rules) {
            if (rule.state && *rule.state != state) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (state == ProgressBarVisualState::disabled) {
            apply_progress_bar_disabled(system, appearance, style);
        }
        return style;
    }

    /**
     * 解析 RadioButton 配方（base → 规则列表，checked + state 选择器，后匹配者胜 →
     * disabled 变换）。
     */
    auto resolve_radio_button(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const bool checked,
        const RadioButtonVisualState state
    ) -> ResolvedRadioButtonStyle {
        auto style = resolve_recipe(system, appearance, system.components.radio_button.base);
        for (const auto& rule: system.components.radio_button.rules) {
            if ((rule.checked && *rule.checked != checked) || (rule.state && *rule.state != state)) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (state == RadioButtonVisualState::disabled) {
            apply_radio_button_disabled(system, appearance, style);
        }
        return style;
    }

    /**
     * 解析 Tabs 配方（base → 规则列表，state 选择器，后匹配者胜 → disabled 变换）。
     */
    auto resolve_tabs(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const TabsVisualState state
    ) -> ResolvedTabsStyle {
        auto style = resolve_recipe(system, appearance, system.components.tabs.base);
        for (const auto& rule: system.components.tabs.rules) {
            if (rule.state && *rule.state != state) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (state == TabsVisualState::disabled) {
            apply_tabs_disabled(system, appearance, style);
        }
        return style;
    }

    /**
     * 解析 Tooltip 配方（纯展示：base → 规则列表，后匹配者胜）。
     */
    auto resolve_tooltip(
        const DesignSystem& system,
        const ColorAppearance appearance
    ) -> ResolvedTooltipStyle {
        auto style = resolve_recipe(system, appearance, system.components.tooltip.base);
        for (const auto& rule: system.components.tooltip.rules) {
            apply_rule(system, appearance, style, rule);
        }
        return style;
    }

    /**
     * 解析 Select 配方（base → 规则列表，state 选择器，后匹配者胜 → disabled 变换）。
     */
    auto resolve_select(
        const DesignSystem& system,
        const ColorAppearance appearance,
        const SelectVisualState state
    ) -> ResolvedSelectStyle {
        auto style = resolve_recipe(system, appearance, system.components.select.base);
        for (const auto& rule: system.components.select.rules) {
            if (rule.state && *rule.state != state) {
                continue;
            }
            apply_rule(system, appearance, style, rule);
        }
        if (state == SelectVisualState::disabled) {
            apply_select_disabled(system, appearance, style);
        }
        return style;
    }

    /**
     * 解析 Divider 配方（纯展示：base → 规则列表，后匹配者胜）。
     */
    auto resolve_divider(
        const DesignSystem& system,
        const ColorAppearance appearance
    ) -> ResolvedDividerStyle {
        auto style = resolve_recipe(system, appearance, system.components.divider.base);
        for (const auto& rule: system.components.divider.rules) {
            apply_rule(system, appearance, style, rule);
        }
        return style;
    }

    /**
     * 解析 Avatar 配方（纯展示：base → 规则列表，后匹配者胜）。
     */
    auto resolve_avatar(
        const DesignSystem& system,
        const ColorAppearance appearance
    ) -> ResolvedAvatarStyle {
        auto style = resolve_recipe(system, appearance, system.components.avatar.base);
        for (const auto& rule: system.components.avatar.rules) {
            apply_rule(system, appearance, style, rule);
        }
        return style;
    }

    /**
     * 解析 Chip 配方（纯展示：base → 规则列表，后匹配者胜）。
     */
    auto resolve_chip(
        const DesignSystem& system,
        const ColorAppearance appearance
    ) -> ResolvedChipStyle {
        auto style = resolve_recipe(system, appearance, system.components.chip.base);
        for (const auto& rule: system.components.chip.rules) {
            apply_rule(system, appearance, style, rule);
        }
        return style;
    }

    /**
     * 解析 Dialog 配方（纯展示：base → 规则列表，后匹配者胜）。
     */
    auto resolve_dialog(
        const DesignSystem& system,
        const ColorAppearance appearance
    ) -> ResolvedDialogStyle {
        auto style = resolve_recipe(system, appearance, system.components.dialog.base);
        for (const auto& rule: system.components.dialog.rules) {
            apply_rule(system, appearance, style, rule);
        }
        return style;
    }

    // ─── apply_rule：把配方规则应用到已解析的配方（widget set_override 复用） ───

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedButtonStyle& style,
        const ButtonRecipeRule& rule,
        const ButtonTone tone
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill, tone);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border, tone);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color, tone);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color, tone);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.metrics_height)
            style.metrics.height = resolve_scalar(system, appearance, *rule.metrics_height);
        if (rule.metrics_padding_x)
            style.metrics.padding_x = resolve_scalar(system, appearance, *rule.metrics_padding_x);
        if (rule.state_layer_hover) {
            style.state_layer.hover =
                resolve_color(system, appearance, *rule.state_layer_hover, tone);
        }
        if (rule.state_layer_pressed) {
            style.state_layer.pressed =
                resolve_color(system, appearance, *rule.state_layer_pressed, tone);
        }
        if (rule.ripple_color) {
            style.ripple.color = resolve_color(system, appearance, *rule.ripple_color, tone);
        }
        if (rule.ripple_duration) {
            style.ripple.duration = resolve_scalar(system, appearance, *rule.ripple_duration);
        }
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedCheckboxStyle& style,
        const CheckboxRecipeRule& rule
    ) {
        if (rule.indicator_fill)
            style.indicator.fill = resolve_color(system, appearance, *rule.indicator_fill);
        if (rule.indicator_border)
            style.indicator.border = resolve_color(system, appearance, *rule.indicator_border);
        if (rule.indicator_border_width) {
            style.indicator.border_width =
                resolve_scalar(system, appearance, *rule.indicator_border_width);
        }
        if (rule.indicator_radius)
            style.indicator.radius = resolve_scalar(system, appearance, *rule.indicator_radius);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.metrics_gap)
            style.metrics.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
        if (rule.metrics_box_size)
            style.metrics.box_size = resolve_scalar(system, appearance, *rule.metrics_box_size);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedSliderStyle& style,
        const SliderRecipeRule& rule
    ) {
        if (rule.track_inactive_fill) {
            style.inactive_track.box.fill =
                resolve_color(system, appearance, *rule.track_inactive_fill);
        }
        if (rule.track_active_fill) {
            style.active_track.box.fill = resolve_color(system, appearance, *rule.track_active_fill);
        }
        if (rule.track_thickness) {
            style.inactive_track.thickness = resolve_scalar(system, appearance, *rule.track_thickness);
            style.active_track.thickness = resolve_scalar(system, appearance, *rule.track_thickness);
        }
        if (rule.thumb_fill)
            style.thumb.box.fill = resolve_color(system, appearance, *rule.thumb_fill);
        if (rule.thumb_radius)
            style.thumb.box.radius = resolve_scalar(system, appearance, *rule.thumb_radius);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedTextFieldStyle& style,
        const TextFieldRecipeRule& rule
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.value_color)
            style.value.color = resolve_color(system, appearance, *rule.value_color);
        if (rule.placeholder_color)
            style.placeholder.color = resolve_color(system, appearance, *rule.placeholder_color);
        if (rule.selection_color)
            style.selection = resolve_color(system, appearance, *rule.selection_color);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.font_size) {
            style.value.font_size = resolve_scalar(system, appearance, *rule.font_size);
            style.placeholder.font_size = resolve_scalar(system, appearance, *rule.font_size);
        }
        if (rule.metrics_height)
            style.metrics.height = resolve_scalar(system, appearance, *rule.metrics_height);
        if (rule.metrics_padding_x)
            style.metrics.padding_x = resolve_scalar(system, appearance, *rule.metrics_padding_x);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedSwitchStyle& style,
        const SwitchRecipeRule& rule
    ) {
        if (rule.track_fill)
            style.track.fill = resolve_color(system, appearance, *rule.track_fill);
        if (rule.track_border)
            style.track.border = resolve_color(system, appearance, *rule.track_border);
        if (rule.track_border_width) {
            style.track.border_width =
                resolve_scalar(system, appearance, *rule.track_border_width);
        }
        if (rule.track_radius)
            style.track.radius = resolve_scalar(system, appearance, *rule.track_radius);
        if (rule.thumb_fill)
            style.thumb.fill = resolve_color(system, appearance, *rule.thumb_fill);
        if (rule.thumb_radius)
            style.thumb.radius = resolve_scalar(system, appearance, *rule.thumb_radius);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.metrics_track_width) {
            style.metrics.track_width =
                resolve_scalar(system, appearance, *rule.metrics_track_width);
        }
        if (rule.metrics_track_height) {
            style.metrics.track_height =
                resolve_scalar(system, appearance, *rule.metrics_track_height);
        }
        if (rule.metrics_thumb_size) {
            style.metrics.thumb_size =
                resolve_scalar(system, appearance, *rule.metrics_thumb_size);
        }
        if (rule.metrics_gap)
            style.metrics.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedBadgeStyle& style,
        const BadgeRecipeRule& rule
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.metrics_height)
            style.metrics.height = resolve_scalar(system, appearance, *rule.metrics_height);
        if (rule.metrics_padding_x)
            style.metrics.padding_x = resolve_scalar(system, appearance, *rule.metrics_padding_x);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedCardStyle& style,
        const CardRecipeRule& rule
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.shadow_color)
            style.shadow.color = resolve_color(system, appearance, *rule.shadow_color);
        if (rule.shadow_offset_x) {
            style.shadow.offset_x =
                resolve_scalar(system, appearance, *rule.shadow_offset_x);
        }
        if (rule.shadow_offset_y) {
            style.shadow.offset_y =
                resolve_scalar(system, appearance, *rule.shadow_offset_y);
        }
        if (rule.shadow_spread)
            style.shadow.spread = resolve_scalar(system, appearance, *rule.shadow_spread);
        if (rule.metrics_padding_x) {
            style.metrics.padding_x =
                resolve_scalar(system, appearance, *rule.metrics_padding_x);
        }
        if (rule.metrics_padding_y) {
            style.metrics.padding_y =
                resolve_scalar(system, appearance, *rule.metrics_padding_y);
        }
        if (rule.metrics_min_height) {
            style.metrics.min_height =
                resolve_scalar(system, appearance, *rule.metrics_min_height);
        }
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedProgressBarStyle& style,
        const ProgressBarRecipeRule& rule
    ) {
        if (rule.track_fill)
            style.track.fill = resolve_color(system, appearance, *rule.track_fill);
        if (rule.track_border)
            style.track.border = resolve_color(system, appearance, *rule.track_border);
        if (rule.track_border_width) {
            style.track.border_width =
                resolve_scalar(system, appearance, *rule.track_border_width);
        }
        if (rule.track_radius)
            style.track.radius = resolve_scalar(system, appearance, *rule.track_radius);
        if (rule.fill_fill)
            style.fill.fill = resolve_color(system, appearance, *rule.fill_fill);
        if (rule.fill_border)
            style.fill.border = resolve_color(system, appearance, *rule.fill_border);
        if (rule.fill_radius)
            style.fill.radius = resolve_scalar(system, appearance, *rule.fill_radius);
        if (rule.metrics_height)
            style.metrics.height = resolve_scalar(system, appearance, *rule.metrics_height);
        if (rule.metrics_min_height) {
            style.metrics.min_height =
                resolve_scalar(system, appearance, *rule.metrics_min_height);
        }
        if (rule.metrics_preferred_width) {
            style.metrics.preferred_width =
                resolve_scalar(system, appearance, *rule.metrics_preferred_width);
        }
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedRadioButtonStyle& style,
        const RadioButtonRecipeRule& rule
    ) {
        if (rule.indicator_fill)
            style.indicator.fill = resolve_color(system, appearance, *rule.indicator_fill);
        if (rule.indicator_border)
            style.indicator.border = resolve_color(system, appearance, *rule.indicator_border);
        if (rule.indicator_border_width) {
            style.indicator.border_width =
                resolve_scalar(system, appearance, *rule.indicator_border_width);
        }
        if (rule.indicator_radius)
            style.indicator.radius = resolve_scalar(system, appearance, *rule.indicator_radius);
        if (rule.dot_color)
            style.dot = resolve_color(system, appearance, *rule.dot_color);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.metrics_gap)
            style.metrics.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
        if (rule.metrics_box_size)
            style.metrics.box_size = resolve_scalar(system, appearance, *rule.metrics_box_size);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedTabsStyle& style,
        const TabsRecipeRule& rule
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.selected_background_fill) {
            style.selected_background.fill =
                resolve_color(system, appearance, *rule.selected_background_fill);
        }
        if (rule.selected_background_radius) {
            style.selected_background.radius =
                resolve_scalar(system, appearance, *rule.selected_background_radius);
        }
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.label_selected_color) {
            style.label_selected.color =
                resolve_color(system, appearance, *rule.label_selected_color);
        }
        if (rule.label_selected_font_size) {
            style.label_selected.font_size =
                resolve_scalar(system, appearance, *rule.label_selected_font_size);
        }
        if (rule.indicator_color)
            style.indicator = resolve_color(system, appearance, *rule.indicator_color);
        if (rule.indicator_thickness) {
            style.indicator_thickness =
                resolve_scalar(system, appearance, *rule.indicator_thickness);
        }
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.metrics_gap)
            style.metrics.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
        if (rule.metrics_padding_x) {
            style.metrics.padding_x =
                resolve_scalar(system, appearance, *rule.metrics_padding_x);
        }
        if (rule.metrics_min_height) {
            style.metrics.min_height =
                resolve_scalar(system, appearance, *rule.metrics_min_height);
        }
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedTooltipStyle& style,
        const TooltipRecipeRule& rule
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.metrics_padding_x) {
            style.metrics.padding_x =
                resolve_scalar(system, appearance, *rule.metrics_padding_x);
        }
        if (rule.metrics_gap)
            style.metrics.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
        if (rule.metrics_min_height) {
            style.metrics.min_height =
                resolve_scalar(system, appearance, *rule.metrics_min_height);
        }
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedSelectStyle& style,
        const SelectRecipeRule& rule
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.popup_fill)
            style.popup.fill = resolve_color(system, appearance, *rule.popup_fill);
        if (rule.popup_border)
            style.popup.border = resolve_color(system, appearance, *rule.popup_border);
        if (rule.popup_border_width) {
            style.popup.border_width =
                resolve_scalar(system, appearance, *rule.popup_border_width);
        }
        if (rule.popup_radius)
            style.popup.radius = resolve_scalar(system, appearance, *rule.popup_radius);
        if (rule.value_color)
            style.value.color = resolve_color(system, appearance, *rule.value_color);
        if (rule.option_color)
            style.option.color = resolve_color(system, appearance, *rule.option_color);
        if (rule.option_selected_color) {
            style.option_selected.color =
                resolve_color(system, appearance, *rule.option_selected_color);
        }
        if (rule.option_font_size) {
            style.option.font_size = resolve_scalar(system, appearance, *rule.option_font_size);
            style.option_selected.font_size =
                resolve_scalar(system, appearance, *rule.option_font_size);
        }
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.metrics_height)
            style.metrics.height = resolve_scalar(system, appearance, *rule.metrics_height);
        if (rule.metrics_padding_x) {
            style.metrics.padding_x =
                resolve_scalar(system, appearance, *rule.metrics_padding_x);
        }
        if (rule.metrics_gap)
            style.metrics.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
        if (rule.metrics_min_height) {
            style.metrics.min_height =
                resolve_scalar(system, appearance, *rule.metrics_min_height);
        }
        if (rule.metrics_preferred_width) {
            style.metrics.preferred_width =
                resolve_scalar(system, appearance, *rule.metrics_preferred_width);
        }
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedDividerStyle& style,
        const DividerRecipeRule& rule
    ) {
        if (rule.color)
            style.color = resolve_color(system, appearance, *rule.color);
        if (rule.thickness)
            style.thickness = resolve_scalar(system, appearance, *rule.thickness);
        if (rule.preferred_length) {
            style.preferred_length =
                resolve_scalar(system, appearance, *rule.preferred_length);
        }
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedAvatarStyle& style,
        const AvatarRecipeRule& rule
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.metrics_box_size)
            style.metrics.box_size = resolve_scalar(system, appearance, *rule.metrics_box_size);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedChipStyle& style,
        const ChipRecipeRule& rule
    ) {
        if (rule.container_fill)
            style.container.fill = resolve_color(system, appearance, *rule.container_fill);
        if (rule.container_border)
            style.container.border = resolve_color(system, appearance, *rule.container_border);
        if (rule.container_border_width) {
            style.container.border_width =
                resolve_scalar(system, appearance, *rule.container_border_width);
        }
        if (rule.container_radius)
            style.container.radius = resolve_scalar(system, appearance, *rule.container_radius);
        if (rule.label_color)
            style.label.color = resolve_color(system, appearance, *rule.label_color);
        if (rule.label_font_size)
            style.label.font_size = resolve_scalar(system, appearance, *rule.label_font_size);
        if (rule.remove_color)
            style.remove_color = resolve_color(system, appearance, *rule.remove_color);
        if (rule.focus_ring_color)
            style.focus.color = resolve_color(system, appearance, *rule.focus_ring_color);
        if (rule.focus_ring_width)
            style.focus.width = resolve_scalar(system, appearance, *rule.focus_ring_width);
        if (rule.metrics_height)
            style.metrics.height = resolve_scalar(system, appearance, *rule.metrics_height);
        if (rule.metrics_padding_x) {
            style.metrics.padding_x =
                resolve_scalar(system, appearance, *rule.metrics_padding_x);
        }
        if (rule.metrics_gap)
            style.metrics.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
    }

    void apply_rule(
        const DesignSystem& system,
        const ColorAppearance appearance,
        ResolvedDialogStyle& style,
        const DialogRecipeRule& rule
    ) {
        if (rule.scrim)
            style.scrim = resolve_color(system, appearance, *rule.scrim);
        if (rule.panel_fill)
            style.panel.fill = resolve_color(system, appearance, *rule.panel_fill);
        if (rule.panel_border)
            style.panel.border = resolve_color(system, appearance, *rule.panel_border);
        if (rule.panel_border_width) {
            style.panel.border_width =
                resolve_scalar(system, appearance, *rule.panel_border_width);
        }
        if (rule.panel_radius)
            style.panel.radius = resolve_scalar(system, appearance, *rule.panel_radius);
        if (rule.title_color)
            style.title.color = resolve_color(system, appearance, *rule.title_color);
        if (rule.title_font_size)
            style.title.font_size = resolve_scalar(system, appearance, *rule.title_font_size);
        if (rule.metrics_panel_width) {
            style.metrics.panel_width =
                resolve_scalar(system, appearance, *rule.metrics_panel_width);
        }
        if (rule.metrics_padding_x) {
            style.metrics.padding_x =
                resolve_scalar(system, appearance, *rule.metrics_padding_x);
        }
        if (rule.metrics_padding_y) {
            style.metrics.padding_y =
                resolve_scalar(system, appearance, *rule.metrics_padding_y);
        }
        if (rule.metrics_gap)
            style.metrics.gap = resolve_scalar(system, appearance, *rule.metrics_gap);
        if (rule.metrics_min_height) {
            style.metrics.min_height =
                resolve_scalar(system, appearance, *rule.metrics_min_height);
        }
    }

    /** @return 框架默认 Button 配方（normal 态通用语义；treatment/size 由规则覆盖）。 */
    auto default_button_recipe() -> ButtonRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::transparent(ColorToken::surface),
                .border = ThemeColor::transparent(accent_ref),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::token(ScalarToken::radius_sm),
            },
            .label = TypeStyle {
                .color = ThemeColor::accent(),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::token(ScalarToken::border_focus_ring),
            },
            // 状态层回退：无可见覆盖（treatment 规则按各自语义覆盖）。
            .state_layer = StateLayerStyle {
                .hover = ThemeColor::transparent(ColorToken::surface),
                .pressed = ThemeColor::transparent(ColorToken::surface),
            },
            .ripple = RippleStyle {
                .color = ThemeColor::with_alpha(
                    on_accent_ref, ThemeScalar::literal(0.20F)
                ),
                .duration = ThemeScalar::token(ScalarToken::motion_medium_duration),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(36.0F),
                .padding_x = ThemeScalar::token(ScalarToken::spacing_md),
                .gap = ThemeScalar::literal(0.0F),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Checkbox 配方（未勾选：透明指示器 + outline 边框）。 */
    auto default_checkbox_recipe() -> CheckboxRecipe {
        return {
            .indicator = BoxStyle {
                .fill = ThemeColor::transparent(ColorToken::surface),
                .border = ThemeColor::token(ColorToken::outline),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::literal(5.0F), // 与现状 radius.sm * 0.5 一致
            },
            .check = ThemeColor::token(ColorToken::on_primary),
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(0.0F),
                .padding_x = ThemeScalar::literal(0.0F),
                .gap = ThemeScalar::token(ScalarToken::spacing_sm),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(20.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Slider 配方。 */
    auto default_slider_recipe() -> SliderRecipe {
        return {
            .inactive_track = TrackStyle {
                .box = BoxStyle {
                    .fill = ThemeColor::token(ColorToken::outline_variant),
                    .border = ThemeColor::token(ColorToken::outline_variant),
                    .border_width = ThemeScalar::literal(0.0F),
                    .radius = ThemeScalar::token(ScalarToken::radius_full),
                },
                .thickness = ThemeScalar::literal(4.0F),
            },
            .active_track = TrackStyle {
                .box = BoxStyle {
                    .fill = ThemeColor::token(ColorToken::primary),
                    .border = ThemeColor::token(ColorToken::primary),
                    .border_width = ThemeScalar::literal(0.0F),
                    .radius = ThemeScalar::token(ScalarToken::radius_full),
                },
                .thickness = ThemeScalar::literal(4.0F),
            },
            .thumb = ThumbStyle {
                .box = BoxStyle {
                    .fill = ThemeColor::token(ColorToken::primary),
                    .border = ThemeColor::token(ColorToken::primary),
                    .border_width = ThemeScalar::literal(0.0F),
                    .radius = ThemeScalar::literal(9.0F), // dragging 11 / hovered 10 由规则覆盖
                },
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(0.0F),
                .padding_x = ThemeScalar::literal(0.0F),
                .gap = ThemeScalar::literal(0.0F),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(240.0F),
            },
        };
    }

    /** @return 框架默认 TextField 配方（normal 状态；focused/invalid 由规则覆盖）。 */
    auto default_text_field_recipe() -> TextFieldRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface_variant),
                .border = ThemeColor::token(ColorToken::outline_variant),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::token(ScalarToken::radius_sm),
            },
            .value = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .placeholder = TypeStyle {
                .color = ThemeColor::with_alpha(
                    ColorToken::on_surface_variant,
                    ThemeScalar::literal(0.72F)
                ),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .selection = ThemeColor::token(ColorToken::selection),
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(40.0F),
                .padding_x = ThemeScalar::token(ScalarToken::spacing_md),
                .gap = ThemeScalar::literal(0.0F),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Switch 配方（未勾选：outline_variant 轨道 + surface 拇指）。 */
    auto default_switch_recipe() -> SwitchRecipe {
        return {
            .track = BoxStyle {
                .fill = ThemeColor::token(ColorToken::outline_variant),
                .border = ThemeColor::transparent(ColorToken::outline_variant),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::token(ScalarToken::radius_full),
            },
            .thumb = ThumbStyle {
                .box = BoxStyle {
                    .fill = ThemeColor::token(ColorToken::surface),
                    .border = ThemeColor::transparent(ColorToken::surface),
                    .border_width = ThemeScalar::literal(0.0F),
                    .radius = ThemeScalar::token(ScalarToken::radius_full),
                },
            },
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
            },
            .metrics = SwitchMetrics {
                .track_width = ThemeScalar::literal(40.0F),
                .track_height = ThemeScalar::literal(24.0F),
                .thumb_size = ThemeScalar::literal(16.0F),
                .gap = ThemeScalar::token(ScalarToken::spacing_sm),
                .min_height = ThemeScalar::literal(32.0F),
            },
        };
    }

    /** @return 框架默认 Badge 配方（pill 展示标签，无交互）。 */
    auto default_badge_recipe() -> BadgeRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface_variant),
                .border = ThemeColor::transparent(ColorToken::surface_variant),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::token(ScalarToken::radius_full),
            },
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface_variant),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_sm),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(22.0F),
                .padding_x = ThemeScalar::token(ScalarToken::spacing_sm),
                .gap = ThemeScalar::literal(0.0F),
                .min_height = ThemeScalar::literal(20.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Card 配方（surface 卡片容器，单子内容；默认无阴影）。 */
    auto default_card_recipe() -> CardRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface),
                .border = ThemeColor::token(ColorToken::outline_variant),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::token(ScalarToken::radius_md),
            },
            .shadow = ShadowStyle {
                .color = ThemeColor::literal(NanColor::from_hex(0x000000, 0.0F)),
                .offset_x = ThemeScalar::literal(0.0F),
                .offset_y = ThemeScalar::literal(0.0F),
                .spread = ThemeScalar::literal(0.0F),
            },
            .metrics = CardMetrics {
                .padding_x = ThemeScalar::token(ScalarToken::spacing_md),
                .padding_y = ThemeScalar::token(ScalarToken::spacing_md),
                .min_height = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 ProgressBar 配方（outline_variant 轨道 + primary 填充）。 */
    auto default_progress_bar_recipe() -> ProgressBarRecipe {
        return {
            .track = BoxStyle {
                .fill = ThemeColor::token(ColorToken::outline_variant),
                .border = ThemeColor::transparent(ColorToken::outline_variant),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::token(ScalarToken::radius_full),
            },
            .fill = BoxStyle {
                .fill = ThemeColor::token(ColorToken::primary),
                .border = ThemeColor::transparent(ColorToken::primary),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::token(ScalarToken::radius_full),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(8.0F),
                .padding_x = ThemeScalar::literal(0.0F),
                .gap = ThemeScalar::literal(0.0F),
                .min_height = ThemeScalar::literal(8.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(240.0F),
            },
        };
    }

    /** @return 框架默认 RadioButton 配方（未选中：透明指示器 + outline 边框）。 */
    auto default_radio_button_recipe() -> RadioButtonRecipe {
        return {
            .indicator = BoxStyle {
                .fill = ThemeColor::transparent(ColorToken::surface),
                .border = ThemeColor::token(ColorToken::outline),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::token(ScalarToken::radius_full),
            },
            .dot = ThemeColor::token(ColorToken::primary),
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(0.0F),
                .padding_x = ThemeScalar::literal(0.0F),
                .gap = ThemeScalar::token(ScalarToken::spacing_sm),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(20.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Tabs 配方（下划线风格：无容器背景/pill，选中 primary + 下划线）。 */
    auto default_tabs_recipe() -> TabsRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::transparent(ColorToken::surface),
                .border = ThemeColor::transparent(ColorToken::surface),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::literal(0.0F),
            },
            .selected_background = BoxStyle {
                .fill = ThemeColor::transparent(ColorToken::surface),
                .border = ThemeColor::transparent(ColorToken::surface),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::literal(0.0F),
            },
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface_variant),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .label_selected = TypeStyle {
                .color = ThemeColor::token(ColorToken::primary),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .indicator = ThemeColor::token(ColorToken::primary),
            .indicator_thickness = ThemeScalar::literal(2.0F),
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(0.0F),
                .padding_x = ThemeScalar::literal(0.0F),
                .gap = ThemeScalar::token(ScalarToken::spacing_lg),
                .min_height = ThemeScalar::literal(40.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Tooltip 配方（primary 气泡 + on_primary 文本）。 */
    auto default_tooltip_recipe() -> TooltipRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::primary),
                .border = ThemeColor::transparent(ColorToken::primary),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::token(ScalarToken::radius_sm),
            },
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_primary),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_sm),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(0.0F),
                .padding_x = ThemeScalar::token(ScalarToken::spacing_sm),
                .gap = ThemeScalar::token(ScalarToken::spacing_sm),
                .min_height = ThemeScalar::literal(24.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Select 配方（surface_variant 字段 + surface 弹窗 + primary 选中）。 */
    auto default_select_recipe() -> SelectRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface_variant),
                .border = ThemeColor::token(ColorToken::outline_variant),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::token(ScalarToken::radius_sm),
            },
            .popup = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface),
                .border = ThemeColor::token(ColorToken::outline_variant),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::token(ScalarToken::radius_sm),
            },
            .value = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .option = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .option_selected = TypeStyle {
                .color = ThemeColor::token(ColorToken::primary),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::literal(0.0F), // focused 规则按需开启
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(40.0F),
                .padding_x = ThemeScalar::token(ScalarToken::spacing_md),
                .gap = ThemeScalar::literal(4.0F),
                .min_height = ThemeScalar::literal(32.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(160.0F),
            },
        };
    }

    /** @return 框架默认 Divider 配方（outline_variant 1px 线）。 */
    auto default_divider_recipe() -> DividerRecipe {
        return {
            .color = ThemeColor::token(ColorToken::outline_variant),
            .thickness = ThemeScalar::token(ScalarToken::border_thin),
            .preferred_length = ThemeScalar::literal(0.0F),
        };
    }

    /** @return 框架默认 Avatar 配方（surface_variant 圆形 + on_surface_variant 首字母）。 */
    auto default_avatar_recipe() -> AvatarRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface_variant),
                .border = ThemeColor::transparent(ColorToken::surface_variant),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::token(ScalarToken::radius_full),
            },
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface_variant),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(0.0F),
                .padding_x = ThemeScalar::literal(0.0F),
                .gap = ThemeScalar::literal(0.0F),
                .min_height = ThemeScalar::literal(0.0F),
                .box_size = ThemeScalar::literal(40.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Chip 配方（surface_variant pill + on_surface_variant 文本 + on_surface_variant 移除）。 */
    auto default_chip_recipe() -> ChipRecipe {
        return {
            .container = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface_variant),
                .border = ThemeColor::transparent(ColorToken::surface_variant),
                .border_width = ThemeScalar::literal(0.0F),
                .radius = ThemeScalar::token(ScalarToken::radius_full),
            },
            .label = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface_variant),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_sm),
            },
            .remove_color = ThemeColor::token(ColorToken::on_surface_variant),
            .focus = FocusRingStyle {
                .color = ThemeColor::token(ColorToken::focus_ring),
                .width = ThemeScalar::token(ScalarToken::border_focus_ring),
            },
            .metrics = ControlMetrics {
                .height = ThemeScalar::literal(28.0F),
                .padding_x = ThemeScalar::token(ScalarToken::spacing_sm),
                .gap = ThemeScalar::literal(6.0F),
                .min_height = ThemeScalar::literal(24.0F),
                .box_size = ThemeScalar::literal(0.0F),
                .preferred_width = ThemeScalar::literal(0.0F),
            },
        };
    }

    /** @return 框架默认 Dialog 配方（半透明 scrim + surface 面板 + on_surface 标题）。 */
    auto default_dialog_recipe() -> DialogRecipe {
        return {
            .scrim = ThemeColor::with_alpha(
                ColorOperand {ColorToken::on_surface},
                ThemeScalar::literal(0.40F)
            ),
            .panel = BoxStyle {
                .fill = ThemeColor::token(ColorToken::surface),
                .border = ThemeColor::token(ColorToken::outline_variant),
                .border_width = ThemeScalar::token(ScalarToken::border_thin),
                .radius = ThemeScalar::token(ScalarToken::radius_md),
            },
            .title = TypeStyle {
                .color = ThemeColor::token(ColorToken::on_surface),
                .font_size = ThemeScalar::token(ScalarToken::typography_label_lg),
            },
            .metrics = DialogMetrics {
                .panel_width = ThemeScalar::literal(360.0F),
                .padding_x = ThemeScalar::token(ScalarToken::spacing_lg),
                .padding_y = ThemeScalar::token(ScalarToken::spacing_md),
                .gap = ThemeScalar::token(ScalarToken::spacing_md),
                .min_height = ThemeScalar::literal(120.0F),
            },
        };
    }

    /**
     * 框架默认设计系统。
     *
     * 片段携带无默认构造的 ThemeValue（没有 "unset" 态），因此整棵树只能通过
     * 聚合初始化构建——本函数就是那个种子。品牌主题从拷贝出发修改字段后原子提交。
     *
     * @return 与遗留 default_theme() 语义对齐的默认快照
     */
    auto default_design_system() -> DesignSystem {
        return DesignSystem {
            .tokens = NanTokens {},
            // 框架默认亮/暗两套语义色（Skeleton 参考，见 phase7 文档 Step 2）：
            // 品牌色两模式同值，明暗差异集中在中性色。
            .light = default_light_palette(),
            .dark = default_dark_palette(),
            .typography = TypographyRoles {
                .label_sm = TypeStyle {
                    .color = ThemeColor::token(ColorToken::on_surface),
                    .font_size = ThemeScalar::token(ScalarToken::typography_label_sm),
                },
                .label_md = TypeStyle {
                    .color = ThemeColor::token(ColorToken::on_surface),
                    .font_size = ThemeScalar::token(ScalarToken::typography_label_md),
                },
                .label_lg = TypeStyle {
                    .color = ThemeColor::token(ColorToken::on_surface),
                    .font_size = ThemeScalar::token(ScalarToken::typography_label_lg),
                },
            },
            .components = ComponentRecipes {
                .button = ButtonRecipes {
                    .base = default_button_recipe(),
                    .rules = {
                        // 尺寸档位（对齐遗留解析器：small 32 / medium 40 / large 48）
                        ButtonRecipeRule {
                            .selector = {.size = ButtonSize::small},
                            .label_font_size = ThemeScalar::token(ScalarToken::typography_label_sm),
                            .metrics_height = ThemeScalar::literal(32.0F),
                            .metrics_padding_x = ThemeScalar::token(ScalarToken::spacing_sm),
                        },
                        ButtonRecipeRule {
                            .selector = {.size = ButtonSize::medium},
                            .label_font_size = ThemeScalar::token(ScalarToken::typography_label_md),
                            .metrics_height = ThemeScalar::literal(40.0F),
                            .metrics_padding_x = ThemeScalar::token(ScalarToken::spacing_md),
                        },
                        ButtonRecipeRule {
                            .selector = {.size = ButtonSize::large},
                            .container_radius = ThemeScalar::token(ScalarToken::radius_md),
                            .label_font_size = ThemeScalar::token(ScalarToken::typography_label_lg),
                            .metrics_height = ThemeScalar::literal(48.0F),
                            .metrics_padding_x = ThemeScalar::token(ScalarToken::spacing_lg),
                        },
                        // 视觉处理方式（normal 态语义 + 独立状态叠加色）。filled 在强调色
                        // 上叠加 on_accent，其余 treatment 在自身基础容器上叠加 accent。
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::filled},
                            .container_fill = ThemeColor::accent(),
                            .container_border = ThemeColor::transparent(accent_ref),
                            .label_color = ThemeColor::on_accent(),
                            .state_layer_hover = ThemeColor::with_alpha(
                                on_accent_ref,
                                ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                            ),
                            .state_layer_pressed = ThemeColor::with_alpha(
                                on_accent_ref,
                                ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                            ),
                        },
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::tonal},
                            .container_fill = ThemeColor::mix(
                                ColorOperand {ColorToken::surface_variant},
                                accent_ref,
                                ThemeScalar::literal(0.35F)
                            ),
                            .container_border = ThemeColor::transparent(accent_ref),
                            .label_color = ThemeColor::accent(),
                            .state_layer_hover = ThemeColor::with_alpha(
                                accent_ref,
                                ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                            ),
                            .state_layer_pressed = ThemeColor::with_alpha(
                                accent_ref,
                                ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                            ),
                        },
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::outlined},
                            .container_fill = ThemeColor::transparent(ColorToken::surface),
                            .container_border = ThemeColor::accent(),
                            .container_border_width = ThemeScalar::token(ScalarToken::border_thin),
                            .label_color = ThemeColor::accent(),
                            .state_layer_hover = ThemeColor::with_alpha(
                                accent_ref,
                                ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                            ),
                            .state_layer_pressed = ThemeColor::with_alpha(
                                accent_ref,
                                ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                            ),
                        },
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::ghost},
                            .container_fill = ThemeColor::transparent(ColorToken::surface),
                            .container_border = ThemeColor::transparent(accent_ref),
                            .label_color = ThemeColor::accent(),
                            .state_layer_hover = ThemeColor::with_alpha(
                                accent_ref,
                                ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                            ),
                            .state_layer_pressed = ThemeColor::with_alpha(
                                accent_ref,
                                ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                            ),
                        },
                        ButtonRecipeRule {
                            .selector = {.treatment = ButtonTreatment::link},
                            .container_fill = ThemeColor::transparent(ColorToken::surface),
                            .container_border = ThemeColor::transparent(accent_ref),
                            .label_color = ThemeColor::accent(),
                            .metrics_padding_x = ThemeScalar::literal(0.0F),
                            .state_layer_hover = ThemeColor::transparent(ColorToken::surface),
                            .state_layer_pressed = ThemeColor::transparent(ColorToken::surface),
                        },
                    },
                },
                .checkbox = CheckboxRecipes {
                    .base = default_checkbox_recipe(),
                    .rules = {
                        CheckboxRecipeRule {
                            .checked = true,
                            .indicator_fill = ThemeColor::token(ColorToken::primary),
                            .indicator_border = ThemeColor::token(ColorToken::primary),
                        },
                        CheckboxRecipeRule {
                            .checked = false,
                            .state = CheckboxVisualState::hovered,
                            .indicator_fill = ThemeColor::with_alpha(
                                ColorToken::primary,
                                ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                            ),
                        },
                        CheckboxRecipeRule {
                            .checked = false,
                            .state = CheckboxVisualState::pressed,
                            .indicator_fill = ThemeColor::with_alpha(
                                ColorToken::primary,
                                ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                            ),
                        },
                        CheckboxRecipeRule {
                            .state = CheckboxVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                    },
                },
                .slider = SliderRecipes {
                    .base = default_slider_recipe(),
                    .rules = {
                        SliderRecipeRule {
                            .state = SliderVisualState::dragging,
                            .thumb_radius = ThemeScalar::literal(11.0F),
                        },
                        SliderRecipeRule {
                            .state = SliderVisualState::hovered,
                            .thumb_radius = ThemeScalar::literal(10.0F),
                        },
                        SliderRecipeRule {
                            .state = SliderVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                    },
                },
                .text_field = TextFieldRecipes {
                    .base = default_text_field_recipe(),
                    .rules = {
                        TextFieldRecipeRule {
                            .state = TextFieldVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                        TextFieldRecipeRule {
                            .state = TextFieldVisualState::invalid,
                            .container_border = ThemeColor::token(ColorToken::error),
                            .focus_ring_color = ThemeColor::token(ColorToken::error),
                        },
                    },
                },
                .switch_component = SwitchRecipes {
                    .base = default_switch_recipe(),
                    .rules = {
                        // 勾选：primary 轨道 + surface 拇指（亮色下近白，比 on_primary 浅色更好看）。
                        SwitchRecipeRule {
                            .checked = true,
                            .track_fill = ThemeColor::token(ColorToken::primary),
                            .track_border = ThemeColor::transparent(ColorToken::primary),
                            .thumb_fill = ThemeColor::token(ColorToken::surface),
                        },
                        // 未勾选交互：轨道向 primary 轻微着色。
                        SwitchRecipeRule {
                            .checked = false,
                            .state = SwitchVisualState::hovered,
                            .track_fill = ThemeColor::mix(
                                ColorOperand {ColorToken::outline_variant},
                                ColorOperand {ColorToken::primary},
                                ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                            ),
                        },
                        SwitchRecipeRule {
                            .checked = false,
                            .state = SwitchVisualState::pressed,
                            .track_fill = ThemeColor::mix(
                                ColorOperand {ColorToken::outline_variant},
                                ColorOperand {ColorToken::primary},
                                ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                            ),
                        },
                        // 勾选交互：primary 轨道向 on_primary 着色（镜像 filled 状态层）。
                        SwitchRecipeRule {
                            .checked = true,
                            .state = SwitchVisualState::hovered,
                            .track_fill = ThemeColor::mix(
                                ColorOperand {ColorToken::primary},
                                ColorOperand {ColorToken::on_primary},
                                ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                            ),
                        },
                        SwitchRecipeRule {
                            .checked = true,
                            .state = SwitchVisualState::pressed,
                            .track_fill = ThemeColor::mix(
                                ColorOperand {ColorToken::primary},
                                ColorOperand {ColorToken::on_primary},
                                ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                            ),
                        },
                        // 聚焦：焦点环开启（checked / unchecked 均适用）。
                        SwitchRecipeRule {
                            .state = SwitchVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                    },
                },
                .badge = BadgeRecipes {
                    .base = default_badge_recipe(),
                    .rules = {},
                },
                .card = CardRecipes {
                    .base = default_card_recipe(),
                    .rules = {},
                },
                .progress_bar = ProgressBarRecipes {
                    .base = default_progress_bar_recipe(),
                    .rules = {},
                },
                .radio_button = RadioButtonRecipes {
                    .base = default_radio_button_recipe(),
                    .rules = {
                        // 选中：primary 指示器边框 + primary 内点。
                        RadioButtonRecipeRule {
                            .checked = true,
                            .indicator_border = ThemeColor::token(ColorToken::primary),
                        },
                        // 未选中交互：指示器向 primary 轻微着色。
                        RadioButtonRecipeRule {
                            .checked = false,
                            .state = RadioButtonVisualState::hovered,
                            .indicator_fill = ThemeColor::with_alpha(
                                ColorToken::primary,
                                ThemeScalar::token(ScalarToken::opacity_hover_overlay)
                            ),
                        },
                        RadioButtonRecipeRule {
                            .checked = false,
                            .state = RadioButtonVisualState::pressed,
                            .indicator_fill = ThemeColor::with_alpha(
                                ColorToken::primary,
                                ThemeScalar::token(ScalarToken::opacity_pressed_overlay)
                            ),
                        },
                        // 聚焦：焦点环开启（checked / unchecked 均适用）。
                        RadioButtonRecipeRule {
                            .state = RadioButtonVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                    },
                },
                .tabs = TabsRecipes {
                    .base = default_tabs_recipe(),
                    .rules = {
                        TabsRecipeRule {
                            .state = TabsVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                    },
                },
                .tooltip = TooltipRecipes {
                    .base = default_tooltip_recipe(),
                    .rules = {},
                },
                .select = SelectRecipes {
                    .base = default_select_recipe(),
                    .rules = {
                        SelectRecipeRule {
                            .state = SelectVisualState::focused,
                            .focus_ring_width =
                                ThemeScalar::token(ScalarToken::border_focus_ring),
                        },
                    },
                },
                .divider = DividerRecipes {
                    .base = default_divider_recipe(),
                    .rules = {},
                },
                .avatar = AvatarRecipes {
                    .base = default_avatar_recipe(),
                    .rules = {},
                },
                .chip = ChipRecipes {
                    .base = default_chip_recipe(),
                    .rules = {},
                },
                .dialog = DialogRecipes {
                    .base = default_dialog_recipe(),
                    .rules = {},
                },
            },
        };
    }

    /**
     * 从遗留 NanTheme 构建 DesignSystem：tokens + 单调色板（light/dark 同值），
     * 配方沿用 default_design_system() 的框架默认。
     */
    auto design_system_from_theme(const NanTheme& theme) -> DesignSystem {
        auto system = default_design_system();
        system.tokens = theme.tokens;
        system.light = theme.palette;
        system.dark = theme.palette;
        return system;
    }

} // namespace nandina::theme
