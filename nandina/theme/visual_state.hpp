/**
 * theme/visual_state — 组件交互状态枚举。
 *
 * 从各组件样式文件中抽出，与具体样式解析解耦；配方规则与控件共享同一组枚举。
 */

#ifndef NANDINA_EXPERIMENT_THEME_VISUAL_STATE_HPP
#define NANDINA_EXPERIMENT_THEME_VISUAL_STATE_HPP

#include <cstdint>

namespace nandina::theme
{
    /** Checkbox 交互状态。 */
    enum class CheckboxVisualState: std::uint8_t {
        normal,
        hovered,
        pressed,
        focused,
        disabled,
    };

    /** Slider 交互状态。 */
    enum class SliderVisualState: std::uint8_t {
        normal,
        hovered,
        dragging,
        focused,
        disabled,
    };

    /** TextField 交互状态（位掩码：focused / disabled / invalid 可组合）。 */
    enum class TextFieldVisualState : unsigned char {
        normal = 0,
        focused = 1 << 0,
        disabled = 1 << 1,
        invalid = 1 << 2,
    };

    [[nodiscard]] constexpr auto operator|(TextFieldVisualState lhs, TextFieldVisualState rhs)
        -> TextFieldVisualState {
        return static_cast<TextFieldVisualState>(
            static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)
        );
    }

    /** 判断位掩码状态是否包含指定状态位。 */
    [[nodiscard]] constexpr auto
    has_text_field_state(TextFieldVisualState value, TextFieldVisualState state) noexcept -> bool {
        if (state == TextFieldVisualState::normal)
            return value == state;
        return (static_cast<unsigned char>(value) & static_cast<unsigned char>(state))
            == static_cast<unsigned char>(state);
    }

    /** Button 语义色家族。 */
    enum class ButtonTone: std::uint8_t {
        primary,
        secondary,
        neutral,
        danger,
    };

    /** Button 视觉处理方式。 */
    enum class ButtonTreatment: std::uint8_t {
        filled,
        tonal,
        outlined,
        ghost,
        link,
    };

    /** Button 尺寸档位。 */
    enum class ButtonSize: std::uint8_t {
        small,
        medium,
        large,
    };

    /**
     * Alert 语义色家族：每个 tone 对应一对语义色角色（`info` / `info_foreground` 等）。
     *
     * 只表达"这条消息是什么性质"，不表达外观处理方式；容器填充 / 边框由各 tone 的
     * 规则在默认设计系统里给出。
     */
    enum class AlertTone: std::uint8_t {
        info,
        success,
        warning,
        error,
    };

    /** Button 交互状态。 */
    enum class ButtonVisualState: std::uint8_t {
        normal,
        hovered,
        pressed,
        focused,
        disabled,
    };

    /** Switch 交互状态。 */
    enum class SwitchVisualState: std::uint8_t {
        normal,
        hovered,
        pressed,
        focused,
        disabled,
    };

    /** ProgressBar 交互状态（确定性进度条：非交互，仅 normal / disabled）。 */
    enum class ProgressBarVisualState: std::uint8_t {
        normal,
        disabled,
    };

    /** Spinner 交互状态（不定量进度指示：非交互，仅 normal / disabled）。 */
    enum class SpinnerVisualState: std::uint8_t {
        normal,
        disabled,
    };

    /** Skeleton 交互状态（纯展示加载占位，无交互，仅 normal）。 */
    enum class SkeletonVisualState: std::uint8_t {
        normal,
    };

    /** EmptyState 交互状态（纯展示空状态，交互由 action 槽位承载，仅 normal）。 */
    enum class EmptyStateVisualState: std::uint8_t {
        normal,
    };

    /**
     * Alert 交互状态（纯展示消息条，交互由 action / dismiss 承载，仅 normal）。
     *
     * 刻意不提供 `disabled`：Alert 自身不接受任何输入，禁用它并不改变容器造型；
     * 想弱化展示请用实例 override 或换 tone，而不是加一个没有视觉落点的死状态。
     */
    enum class AlertVisualState: std::uint8_t {
        normal,
    };

    /** RadioButton 交互状态。 */
    enum class RadioButtonVisualState: std::uint8_t {
        normal,
        hovered,
        pressed,
        focused,
        disabled,
    };

    /** Tabs 交互状态。 */
    enum class TabsVisualState: std::uint8_t {
        normal,
        focused,
        disabled,
    };

    /** Select 交互状态。 */
    enum class SelectVisualState: std::uint8_t {
        normal,
        focused,
        disabled,
    };

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_VISUAL_STATE_HPP
