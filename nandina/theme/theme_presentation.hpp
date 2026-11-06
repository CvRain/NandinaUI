/**
 * theme/theme_presentation — 只读、受作用域保护的主题呈现窄投影（D2 首批）。
 *
 * 载荷只有两项：当前**有效**外观（不是用户偏好）与不透明呈现修订标记。写入仍统一经
 * ThemeManager；本适配器只观察，不提供任何写入口，也不冒充选择状态或纯主题内容事件。
 * 契约见 docs/references/theme_authoring_and_state.md §5.2–§5.5。
 *
 * 生命周期：投影必须由 ReactiveScope 持有（`ui.theme_state()` 或 `scope.own<T>()`），
 * 随作用域销毁并注销观察者；ThemeManager 先销毁时投影只失效、不悬垂访问。
 */

#ifndef NANDINA_EXPERIMENT_THEME_THEME_PRESENTATION_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_PRESENTATION_HPP

#include "../reactive/event.hpp"
#include "../reactive/signal.hpp"
#include "theme_manager.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

namespace nandina::theme
{
    /**
     * §5.5 首批窄投影载荷。
     *
     * - `appearance` 是当前有效外观，不是 preference；
     * - `revision` 是 manager 的不透明呈现修订标记，包含仅动效变化，不持久化、不推断
     *   变化类别、不假定每次只加一。
     */
    struct ThemePresentationState {
        ColorAppearance appearance = ColorAppearance::light;
        std::uint64_t revision = 0;

        friend auto operator==(const ThemePresentationState&, const ThemePresentationState&)
            -> bool = default;
    };

    /**
     * ThemeManager 呈现状态的只读响应式投影。
     *
     * - `get()` 返回当前载荷；`readonly()` 是借用的只读源，供 computed/绑定消费；
     * - `subscribe(handler)` 返回 RAII 订阅，订阅本身不补齐一次“初始变化”；
     * - 等价状态去重：观察值未变化时不通知；
     * - 作用域或 manager 任一先销毁都安全；manager 先销毁后 `valid()` 为 false，
     *   `get()` 仍返回最后一个已发布载荷。
     */
    class ThemePresentation final {
    public:
        ThemePresentation(ThemeManager& manager, reactive::Graph& graph);
        ~ThemePresentation();

        ThemePresentation(const ThemePresentation&) = delete;
        auto operator=(const ThemePresentation&) -> ThemePresentation& = delete;
        ThemePresentation(ThemePresentation&&) = delete;
        auto operator=(ThemePresentation&&) -> ThemePresentation& = delete;

        /// manager 仍存活时为 true；manager 销毁后转为 false。
        [[nodiscard]] auto valid() const noexcept -> bool;
        /// 读取参与依赖追踪：在 computed/effect 中调用会订阅呈现变化。
        [[nodiscard]] auto get() const -> ThemePresentationState;
        [[nodiscard]] auto appearance() const -> ColorAppearance;
        [[nodiscard]] auto revision() const -> std::uint64_t;

        /// 借用视图；投影及其 Graph 必须活过此视图的所有读取，无写入入口。
        [[nodiscard]] auto readonly() const -> reactive::ReadSignal<ThemePresentationState>;

        /// 侧效应订阅；仅在观察值真正变化时回调。订阅本身不伪造初始变化。
        [[nodiscard]] auto subscribe(std::function<void(ThemePresentationState)> handler)
            -> reactive::Subscription;

    private:
        struct Impl;
        std::shared_ptr<Impl> impl_;
    };
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_PRESENTATION_HPP
