//
// scene/focus_hint — input modality and focus-hint request context (D1 phase 1).
//
// 输入上下文与焦点提示策略归 scene：平台与宿主只上报“发生了哪类有意义的交互”，
// 组件只声明自己是否必须有提示；是否绘制焦点提示由 NanSceneTree::should_show_focus_hint()
// 统一回答，组件不各自维护第二份设备状态。
//

#ifndef NANDINA_EXPERIMENT_SCENE_FOCUS_HINT_HPP
#define NANDINA_EXPERIMENT_SCENE_FOCUS_HINT_HPP

#include <cstdint>
#include <optional>

namespace nandina::scene
{
    /// 最近一次“有意义”的交互类型。被动事件（移动、悬停、滚轮、单独修饰键、提交文本）
    /// 不改变它；unknown 表示尚无交互历史，默认显示焦点提示。
    enum class InputModality : std::uint8_t {
        unknown,
        pointer,
        keyboard,
    };

    /// 本次焦点转移的原因。原因说明“为什么转移”，交互上下文说明“属于指向还是键盘”；
    /// restore 本身不决定提示是否显示。
    enum class FocusRequestReason : std::uint8_t {
        programmatic,
        pointer,
        navigation,
        restore,
        popup,
        accessibility,
    };

    /// 组件的提示需求。\c always_required 用于可编辑输入：指向聚焦也必须显示明确编辑提示。
    enum class FocusHintRequirement : std::uint8_t {
        context_dependent,
        always_required,
    };

    /// 一次焦点请求携带的上下文。\c modality 缺省时沿用当前窗口最近一次有意义交互；
    /// 显式上下文优先于最近交互，但不越过窗口失活与“始终显示”的优先级。
    struct FocusRequestContext {
        FocusRequestReason reason = FocusRequestReason::programmatic;
        std::optional<InputModality> modality {};
    };
} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_SCENE_FOCUS_HINT_HPP
