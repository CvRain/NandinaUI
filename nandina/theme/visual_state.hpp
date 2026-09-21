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

    /**
     * TextArea 交互状态（位掩码：focused / disabled 可组合）。
     *
     * read-only 不进状态枚举：它由 `TextAreaRecipeRule::read_only` 选择器单独表达，
     * 与视觉状态位正交（只读控件仍可获得焦点）。
     */
    enum class TextAreaVisualState : unsigned char {
        normal = 0,
        focused = 1 << 0,
        disabled = 1 << 1,
    };

    [[nodiscard]] constexpr auto operator|(TextAreaVisualState lhs, TextAreaVisualState rhs)
        -> TextAreaVisualState {
        return static_cast<TextAreaVisualState>(
            static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)
        );
    }

    /** 判断位掩码状态是否包含指定状态位。 */
    [[nodiscard]] constexpr auto
    has_text_area_state(TextAreaVisualState value, TextAreaVisualState state) noexcept -> bool {
        if (state == TextAreaVisualState::normal)
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

    /** Toggle 交互状态（checked 是解析入参，不是状态枚举成员，与 Checkbox 同款）。 */
    enum class ToggleVisualState: std::uint8_t {
        normal,
        hovered,
        pressed,
        focused,
        disabled,
    };

    /**
     * ToggleGroup 交互状态（组只做协调与布局，自身不接受输入，仅 normal）。
     *
     * 与 Skeleton / EmptyState 同款：状态枚举先占位，规则解析器已经按 state 过滤，
     * 将来若组需要 disabled 之类的状态，只需在此加成员并播下规则。
     */
    enum class ToggleGroupVisualState: std::uint8_t {
        normal,
    };

    /**
     * ButtonGroup 交互状态（组只做排列与间距协调，自身不接受输入，仅 normal）。
     *
     * 与 ToggleGroup 同款：状态枚举先占位，规则解析器已经按 state 过滤。
     */
    enum class ButtonGroupVisualState: std::uint8_t {
        normal,
    };

    /**
     * Breadcrumb 交互状态（容器只做排布与绘制，交互由内部链接条目承载，仅 normal）。
     */
    enum class BreadcrumbVisualState: std::uint8_t {
        normal,
    };

    /** Pagination 交互状态（整条分页器可聚焦，页码槽位的 hover 由组件按位置表达）。 */
    enum class PaginationVisualState: std::uint8_t {
        normal,
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
