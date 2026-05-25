module;

#include <chrono>
#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

export module nandina.app.application;

import nandina.layout.container;
import nandina.layout.flex_widgets;
export import nandina.layout.positioned;
import nandina.log;
import nandina.foundation.color;
import nandina.reactive.effect;
import nandina.runtime.nan_window;
import nandina.runtime.nan_widget;
import nandina.widgets.button;
import nandina.widgets.card;
import nandina.widgets.label;
import nandina.widgets.panel;
import nandina.widgets.surface;
import nandina.theme;

export namespace nandina::app {
    namespace detail {
        using SteadyClock = std::chrono::steady_clock;

        [[nodiscard]] inline auto elapsed_ms(
            const SteadyClock::time_point start,
            const SteadyClock::time_point end) noexcept -> double {
            return std::chrono::duration<double, std::milli>(end - start).count();
        }

        inline constexpr double k_slow_layout_threshold_ms = 4.0;
    }

    class NanComponent : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<NanComponent>;

        ~NanComponent() override = default;
    };

    class Node;
    class Children;
    class LabelNode;
    class ButtonNode;
    template <typename W>
    class WidgetNode;

    class NanAppWindow;

    [[nodiscard]] inline auto adopt(runtime::NanWidget::Ptr widget) -> Node;

    [[nodiscard]] inline auto label(std::string_view text = {}) -> LabelNode;

    [[nodiscard]] inline auto button(std::string_view text = {}) -> ButtonNode;

    [[nodiscard]] inline auto card(Children children) -> Node;

    [[nodiscard]] inline auto panel(Children children) -> Node;

    [[nodiscard]] inline auto positioned(Children children) -> Node;

    template <typename T>
    class Ref {
    public:
        Ref() = default;

        [[nodiscard]] auto get() const noexcept -> T* {
            return m_ptr;
        }

        [[nodiscard]] auto operator->() const noexcept -> T* {
            return m_ptr;
        }

        [[nodiscard]] auto operator*() const noexcept -> T& {
            return *m_ptr;
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return m_ptr != nullptr;
        }

    private:
        friend class Node;

        auto bind(runtime::NanWidget* widget) noexcept -> void {
            m_ptr = dynamic_cast<T*>(widget);
        }

        auto reset() noexcept -> void {
            m_ptr = nullptr;
        }

        T* m_ptr{nullptr};
    };

    class Node {
    public:
        Node() = default;

        Node(Node&&) noexcept = default;

        auto operator=(Node&&) noexcept -> Node& = default;

        Node(const Node&) = delete;

        auto operator=(const Node&) -> Node& = delete;

        [[nodiscard]] auto empty() const noexcept -> bool {
            return m_widget == nullptr;
        }

        template <typename Self, typename T>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto bind(this Self&& self, Ref<T>& ref) -> Self&& {
            auto& node   = static_cast<Node&>(self);
            auto* widget = node.m_widget.get();
            node.m_ref_binders.push_back([&ref, widget]() {
                ref.bind(widget);
            });
            node.m_ref_resetters.push_back([&ref]() {
                ref.reset();
            });
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto key(this Self&& self, std::string_view value) -> Self&& {
            auto& node = static_cast<Node&>(self);
            node.m_key.assign(value);
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto title(this Self&& self, std::string_view value) -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::widgets::Card*>(node.unwrapped())) {
                widget->set_title(std::string{value});
            } else if (auto* widget = dynamic_cast<nandina::widgets::Panel*>(node.unwrapped())) {
                widget->set_title(value);
            }
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto bg_color(this Self&& self, const nandina::NanColor& value) -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::widgets::Surface*>(node.unwrapped())) {
                widget->set_bg_color(value);
            }
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto corner_radius(this Self&& self, const float value) -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::widgets::Surface*>(node.unwrapped())) {
                widget->set_corner_radius(value);
            }
            return std::forward<Self>(self);
        }

        /// padding — 设置内边距（支持三种重载）。
        ///
        /// 重载：
        ///   .padding(value)                     — 四边均等
        ///   .padding(horizontal, vertical)      — 水平/垂直分别设置
        ///   .padding(left, top, right, bottom)  — 四边独立设置
        ///
        /// 示例：
        ///   column(children(...)).padding(16)        // 四边 16px
        ///   column(children(...)).padding(24, 16)    // 左右 24px，上下 16px
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto padding(this Self&& self, const float value) -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::layout::Padding*>(node.m_widget.get())) {
                widget->padding(value);
            } else if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(node.m_widget.get())) {
                widget->padding(value);
            }
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto padding(this Self&& self, const float horizontal, const float vertical) -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::layout::Padding*>(node.m_widget.get())) {
                widget->padding(horizontal, vertical);
            } else if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(node.m_widget.get())) {
                widget->padding(horizontal, vertical);
            }
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto padding(this Self&& self, const float left, const float top, const float right, const float bottom)
            -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::layout::Padding*>(node.m_widget.get())) {
                widget->padding(left, top, right, bottom);
            } else if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(node.m_widget.get())) {
                widget->padding(left, top, right, bottom);
            }
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto padding_top(this Self&& self, const float value) -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(node.m_widget.get())) {
                widget->padding_top(value);
            }
            return std::forward<Self>(self);
        }

        /// gap — 设置 Row / Column 中子组件之间的间距（像素）。
        ///
        /// @code
        /// 示例：
        /// row(children(...)).gap(12)
        /// @endcode
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto gap(this Self&& self, const float value) -> Self&& {
            const auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(node.m_widget.get())) {
                widget->gap(value);
            }
            return std::forward<Self>(self);
        }

        /// align_items — 设置交叉轴方向的子组件对齐方式。
        ///
        /// 对 Row 生效的是垂直对齐；对 Column 生效的是水平对齐。
        ///
        /// 常用值：
        ///   LayoutAlignment::start    — 靠起始端（默认） <br>
        ///   LayoutAlignment::center   — 居中 <br>
        ///   LayoutAlignment::end      — 靠末端 <br>
        ///   LayoutAlignment::stretch  — 拉伸填满交叉轴 <br>
        ///
        /// @code
        /// // 子组件撑满列宽
        /// column(children(...)).align_items(LayoutAlignment::stretch)
        /// // 垂直居中
        /// row(children(...)).align_items(LayoutAlignment::center)
        /// @endcode
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto align_items(this Self&& self, const nandina::layout::LayoutAlignment value) -> Self&& {
            const auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(node.m_widget.get())) {
                widget->align_items(value);
            }
            return std::forward<Self>(self);
        }

        /// justify_content — 设置主轴方向的子组件分布方式。
        ///
        /// 对 Row 生效的是水平分布；对 Column 生效的是垂直分布。
        ///
        /// 常用值：
        ///   LayoutAlignment::start         — 从起始端排列（默认）
        ///   LayoutAlignment::center        — 居中排列
        ///   LayoutAlignment::end           — 从末端排列
        ///   LayoutAlignment::space_between — 两端对齐，子组件等间距
        ///   LayoutAlignment::space_around  — 子组件两侧等间距
        ///
        /// 示例：
        ///   row(children(btn_a, btn_b)).justify_content(LayoutAlignment::space_between)
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto justify_content(this Self&& self, const nandina::layout::LayoutAlignment value)
            -> Self&& {
            const auto& node = static_cast<Node&>(self);
            if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(node.m_widget.get())) {
                widget->justify_content(value);
            }
            return std::forward<Self>(self);
        }

        /// width — 限定节点宽度（像素）；若当前节点不是 SizedBox，自动包裹一层。
        ///
        /// 示例：
        ///   column(children(...)).width(360)   // 将列限宽为 360px
        ///   label("文本").width(120)            // 限制标签最大宽度
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto width(this Self&& self, const float value) -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(node.m_widget.get())) {
                sb->width(value);
            } else {
                // 自动包裹到 SizedBox：label("X").width(200) 等价于 sized_box(label("X")).width(200)
                auto wrapper = nandina::layout::SizedBox::Create();
                wrapper->width(value);
                wrapper->add_child(std::move(node.m_widget));
                node.m_widget = std::move(wrapper);
            }
            return std::forward<Self>(self);
        }

        /// height — 限定节点高度（像素）；若当前节点不是 SizedBox，自动包裹一层。
        ///
        /// 示例：
        ///   button("OK").height(48)   // 固定按钮高度为 48px
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto height(this Self&& self, const float value) -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(node.m_widget.get())) {
                sb->height(value);
            } else {
                auto wrapper = nandina::layout::SizedBox::Create();
                wrapper->height(value);
                wrapper->add_child(std::move(node.m_widget));
                node.m_widget = std::move(wrapper);
            }
            return std::forward<Self>(self);
        }

        /// size — 同时设置宽度和高度（像素）；等价于 .width(w).height(h)。
        ///
        /// 示例：
        ///   button("提交").size({120, 40})
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto size(this Self&& self, const geometry::NanSize& value) -> Self&& {
            auto& node = static_cast<Node&>(self);
            if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(node.m_widget.get())) {
                sb->size(value);
            } else {
                auto wrapper = nandina::layout::SizedBox::Create();
                wrapper->size(value);
                wrapper->add_child(std::move(node.m_widget));
                node.m_widget = std::move(wrapper);
            }
            return std::forward<Self>(self);
        }

        // ── Anchor 定位属性（用于 positioned() 容器内）─────────────────────────
        // 这些方法设置子组件在 Positioned 容器中的 anchor 约束。
        // 在 Row/Column/Stack 容器内这些属性无效（被忽略）。

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_left(this Self&& self, float v) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.left = v;
            return std::forward<Self>(self);
        }

        template <typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_left(this Self&& self, Fn fn) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.left =
                std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_top(this Self&& self, float v) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.top = v;
            return std::forward<Self>(self);
        }

        template <typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_top(this Self&& self, Fn fn) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.top =
                std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_right(this Self&& self, float v) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.right = v;
            return std::forward<Self>(self);
        }

        template <typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_right(this Self&& self, Fn fn) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.right =
                std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_bottom(this Self&& self, float v) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.bottom = v;
            return std::forward<Self>(self);
        }

        template <typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_bottom(this Self&& self, Fn fn) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.bottom =
                std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_width(this Self&& self, float v) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.width = v;
            return std::forward<Self>(self);
        }

        template <typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_width(this Self&& self, Fn fn) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.width =
                std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_height(this Self&& self, float v) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.height = v;
            return std::forward<Self>(self);
        }

        template <typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_height(this Self&& self, Fn fn) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.height =
                std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        /// 填满父容器宽度（左右锚点均为 0）
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto fill_width(this Self&& self) -> Self&& {
            auto& node                    = static_cast<Node&>(self);
            node.m_positioned_props.left  = 0.0f;
            node.m_positioned_props.right = 0.0f;
            return std::forward<Self>(self);
        }

        /// 填满父容器高度（上下锚点均为 0）
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto fill_height(this Self&& self) -> Self&& {
            auto& node                     = static_cast<Node&>(self);
            node.m_positioned_props.top    = 0.0f;
            node.m_positioned_props.bottom = 0.0f;
            return std::forward<Self>(self);
        }

        /// 填满父容器（左右上下锚点均为 0）
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto fill(this Self&& self) -> Self&& {
            auto& node                     = static_cast<Node&>(self);
            node.m_positioned_props.left   = 0.0f;
            node.m_positioned_props.right  = 0.0f;
            node.m_positioned_props.top    = 0.0f;
            node.m_positioned_props.bottom = 0.0f;
            return std::forward<Self>(self);
        }

        /// 将此子组件的布置结果写入 GeomRef（供后续兄弟节点 anchor lambda 引用）
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto bind_geom(this Self&& self, nandina::layout::GeomRef& ref) -> Self&& {
            static_cast<Node&>(self).m_positioned_props.geom_ref = &ref;
            return std::forward<Self>(self);
        }

    private:
        using RefBinder   = std::move_only_function<void()>;
        using RefResetter = std::move_only_function<void()>;

        /// 穿透 SizedBox 包裹层找到真实组件
        [[nodiscard]] auto unwrapped() const -> runtime::NanWidget* {
            if (!m_widget)
                return nullptr;
            if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(m_widget.get())) {
                runtime::NanWidget* result = nullptr;
                sb->for_each_child([&](runtime::NanWidget& c) {
                    if (!result)
                        result = &c;
                });
                return result ? result : m_widget.get();
            }
            return m_widget.get();
        }

        friend class MountedNodeComponent;
        friend class LabelNode;
        friend class ButtonNode;
        template <typename W>
        friend class WidgetNode;

        friend auto mount(Node root) -> NanComponent::Ptr;

        friend class NanAppWindow;
        friend class Children;

        friend auto row(Children children) -> Node;

        friend auto column(Children children) -> Node;

        friend auto stack(Children children) -> Node;

        friend auto spacer(int flex) -> Node;

        template <typename N>
            requires std::derived_from<std::decay_t<N>, Node>
        friend auto expanded(N&& child) -> Node;

        template <typename N>
            requires std::derived_from<std::decay_t<N>, Node>
        friend auto expanded(N&& child, int flex) -> Node;

        friend auto padding(Node child) -> Node;

        friend auto center(Node child) -> Node;

        friend auto sized_box(Node child) -> Node;

        friend auto adopt(runtime::NanWidget::Ptr widget) -> Node;

        friend auto label(std::string_view text) -> LabelNode;

        friend auto button(std::string_view text) -> ButtonNode;

        friend auto card(Children children) -> Node;

        friend auto panel(Children children) -> Node;

        friend auto positioned(Children children) -> Node;

        explicit Node(std::unique_ptr<runtime::NanWidget> widget)
            : m_widget(std::move(widget)) {
        }

        [[nodiscard]] auto take_widget() && -> std::unique_ptr<runtime::NanWidget> {
            return std::move(m_widget);
        }

        [[nodiscard]] auto take_positioned_props() && -> nandina::layout::PositionedProps {
            return std::move(m_positioned_props);
        }

        auto append_ref_binder(RefBinder binder, RefResetter resetter) -> void {
            m_ref_binders.push_back(std::move(binder));
            m_ref_resetters.push_back(std::move(resetter));
        }

        auto absorb(Node child) -> void {
            for (auto& binder : child.m_ref_binders) {
                m_ref_binders.push_back(std::move(binder));
            }
            for (auto& resetter : child.m_ref_resetters) {
                m_ref_resetters.push_back(std::move(resetter));
            }
        }

        auto bind_refs() -> void {
            for (auto& binder : m_ref_binders) {
                binder();
            }
        }

        auto reset_refs() -> void {
            for (auto& resetter : m_ref_resetters) {
                resetter();
            }
        }

        std::unique_ptr<runtime::NanWidget> m_widget;
        std::string m_key;
        std::vector<RefBinder> m_ref_binders;
        std::vector<RefResetter> m_ref_resetters;
        nandina::layout::PositionedProps m_positioned_props;
    };

    // ── WidgetNode<W> — 类型安全组件节点基类 ───────────────────────────
    // 外来开发者创建自定义节点只需继承此类：
    //   class SliderNode : public WidgetNode<widgets::Slider> { ... };
    template <typename W>
    class WidgetNode : public Node {
    public:
        // 类型安全访问器
        [[nodiscard]] auto widget() & noexcept -> W& {
            return *m_typed;
        }

        [[nodiscard]] auto widget() const & noexcept -> const W& {
            return *m_typed;
        }

        // 逃生口：直接操作底层 widget，保持链式返回
        // 适用于 builder 方法未覆盖的属性读/写：
        //   button("OK").configure([](Button& b) { b.set_bg_color(...); })
        template <typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, WidgetNode<W>>
                     && std::invocable<Fn, W&>
        auto configure(this Self&& self, Fn&& fn) -> Self&& {
            std::invoke(std::forward<Fn>(fn), *static_cast<WidgetNode<W>&>(self).m_typed);
            return std::forward<Self>(self);
        }

    protected:
        explicit WidgetNode(W::Ptr w)
            : Node(std::move(w))
              , m_typed(static_cast<W*>(Node::unwrapped())) {
        }

        W* m_typed{nullptr};
    };

    class LabelNode : public WidgetNode<nandina::widgets::Label> {
    public:
        explicit LabelNode(nandina::widgets::Label::Ptr widget)
            : WidgetNode<nandina::widgets::Label>(std::move(widget)) {
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto text(this Self&& self, std::string_view value) -> Self&& {
            self.m_typed->set_text(value);
            return std::forward<Self>(self);
        }

        template <typename Self, typename F>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode> &&
                     std::invocable<F> &&
                     std::convertible_to<std::invoke_result_t<F>, std::string_view> &&
                     (!std::convertible_to<F, std::string_view>)
        auto text(this Self&& self, F fn) -> Self&& {
            auto* w = self.m_typed;
            w->add_opaque_cleanup(
                std::make_shared<nandina::reactive::Effect>(
                    [w, fn = std::move(fn)] {
                        w->set_text(fn());
                    }
                    )
                );
            return std::forward<Self>(self);
        }

        // ── getter ──────────────────────────────────────────────────────────

        [[nodiscard]] auto text() const -> const std::string& {
            return m_typed->text();
        }

        [[nodiscard]] auto font() const -> const nandina::text::NanFont& {
            return m_typed->font();
        }

        [[nodiscard]] auto disabled() const -> bool {
            return m_typed->disabled();
        }

        // ── setter ──────────────────────────────────────────────────────────

        /// color — 快捷设置文本颜色；等价于 .font([](auto& f){ f.color(v); })
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto color(this Self&& self, const nandina::NanColor& value) -> Self&& {
            self.m_typed->set_color(value);
            return std::forward<Self>(self);
        }

        /// font(NanFont) — 替换整个字体配置。
        ///
        /// 示例：label("标题").font(NanFont{}.size(20).weight(NanFontWeight::bold))
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
                  && (!std::invocable<Self, nandina::text::NanFont&>)
        auto font(this Self&& self, nandina::text::NanFont font_config) -> Self&& {
            self.m_typed->set_font(std::move(font_config));
            return std::forward<Self>(self);
        }

        /// font(Fn) — 局部更新字体属性；Fn: (NanFont&) -> void
        ///
        /// 示例：
        ///   label("文本").font([](auto& f){ f.size(16).color(NanColor::white()); })
        ///   label("文本").font([](auto& f){ f.size(f.size() + 2); })  // 在现有基础上调整
        template <typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
                  && std::invocable<Fn, nandina::text::NanFont&>
                  && (!std::convertible_to<std::remove_cvref_t<Fn>, nandina::text::NanFont>)
        auto font(this Self&& self, Fn&& fn) -> Self&& {
            auto updated = self.m_typed->font();
            std::invoke(std::forward<Fn>(fn), updated);
            self.m_typed->set_font(std::move(updated));
            return std::forward<Self>(self);
        }

        /// disabled — 禁用标签；对应 shadcn peer-disabled:opacity-70，文本渲染降至 70% 透明度
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto disabled(this Self&& self, const bool value) -> Self&& {
            self.m_typed->set_disabled(value);
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto align(this Self&& self, const nandina::widgets::TextAlign value) -> Self&& {
            self.m_typed->set_align(value);
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto vertical_align(this Self&& self, const nandina::widgets::TextVerticalAlign value)
            -> Self&& {
            self.m_typed->set_vertical_align(value);
            return std::forward<Self>(self);
        }
    };

    class ButtonNode : public WidgetNode<nandina::widgets::Button> {
    public:
        using Node::size;

        explicit ButtonNode(nandina::widgets::Button::Ptr widget)
            : WidgetNode<nandina::widgets::Button>(std::move(widget)) {
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto text(this Self&& self, std::string_view value) -> Self&& {
            self.m_typed->set_text(value);
            return std::forward<Self>(self);
        }

        template <typename Self, typename F>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode> &&
                     std::invocable<F> &&
                     std::convertible_to<std::invoke_result_t<F>, std::string_view> &&
                     (!std::convertible_to<F, std::string_view>)
        auto text(this Self&& self, F fn) -> Self&& {
            auto* w = self.m_typed;
            w->add_opaque_cleanup(
                std::make_shared<nandina::reactive::Effect>(
                    [w, fn = std::move(fn)] {
                        w->set_text(fn());
                    }
                    )
                );
            return std::forward<Self>(self);
        }

        // ── getter ──────────────────────────────────────────────────────────

        [[nodiscard]] auto text() const -> const std::string& {
            return m_typed->text();
        }

        [[nodiscard]] auto font() const -> const nandina::text::NanFont& {
            return m_typed->font();
        }

        // ── setter ──────────────────────────────────────────────────────────

        /// font(NanFont) — 替换整个字体配置。
        ///
        /// 示例：button("提交").font(NanFont{}.size(14).weight(NanFontWeight::medium))
        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto font(this Self&& self, nandina::text::NanFont font_config) -> Self&& {
            self.m_typed->set_font(std::move(font_config));
            return std::forward<Self>(self);
        }

        /// font(Fn) — 局部更新字体属性；Fn: (NanFont&) -> void
        ///
        /// 示例：
        ///   button("+").font([](auto& f){ f.size(18).weight(NanFontWeight::bold); })
        ///   button("OK").font([](auto& f){ f.size(f.size() + 2); })  // 在现有基础上调整
        template <typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
                  && std::invocable<Fn, nandina::text::NanFont&>
                  && (!std::convertible_to<std::remove_cvref_t<Fn>, nandina::text::NanFont>)
        auto font(this Self&& self, Fn&& fn) -> Self&& {
            auto updated = self.m_typed->font();
            std::invoke(std::forward<Fn>(fn), updated);
            self.m_typed->set_font(std::move(updated));
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto button_style(this Self&& self, const theme::NanButtonStyle& style) -> Self&& {
            self.m_typed->variant(style.variant);
            self.m_typed->size(style.size);

            auto font = nandina::text::NanFont{}
                .size(style.font_size)
                .weight(style.font_weight)
                .color(style.font_color)
                .overflow(style.overflow)
                .single_line(style.single_line);
            self.m_typed->set_font(std::move(font));
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto variant(this Self&& self, nandina::widgets::ButtonVariant value) -> Self&& {
            self.m_typed->variant(value);
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto size(this Self&& self, nandina::widgets::ButtonSize value) -> Self&& {
            self.m_typed->size(value);
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_click(this Self&& self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_click(std::move(handler));
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_press(this Self&& self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_press(std::move(handler));
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_release(this Self&& self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_release(std::move(handler));
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_hover(this Self&& self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_hover(std::move(handler));
            return std::forward<Self>(self);
        }

        template <typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_leave(this Self&& self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_leave(std::move(handler));
            return std::forward<Self>(self);
        }
    };

    class Children {
    public:
        Children() = default;

        auto append(Node node) -> Children& {
            m_nodes.push_back(std::move(node));
            return *this;
        }

        [[nodiscard]] auto size() const noexcept -> std::size_t {
            return m_nodes.size();
        }

        [[nodiscard]] auto take() && -> std::vector<Node> {
            return std::move(m_nodes);
        }

    private:
        std::vector<Node> m_nodes;
    };

    inline auto children() -> Children {
        return {};
    }

    /// 变参模板：接受任意多个 Node（lvalue 或 rvalue），内部统一移动
    template <typename... Nodes>
        requires (std::derived_from<std::decay_t<Nodes>, Node> && ...)
    inline auto children(Nodes&&... nodes) -> Children {
        Children result;
        (result.append(std::move(nodes)), ...);
        return result;
    }

    class MountedNodeComponent final : public NanComponent {
    public:
        explicit MountedNodeComponent(Node root)
            : m_root_node(std::move(root)) {
            auto widget   = std::move(m_root_node).take_widget();
            m_root_widget = add_child(std::move(widget));
            m_root_node.bind_refs();
        }

        ~MountedNodeComponent() override {
            m_root_node.reset_refs();
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            if (m_root_widget) {
                m_root_widget->runtime::NanWidget::set_bounds(x, y, w, h);
                m_pending_root_layout = true;
            }
            return *this;
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            if (!m_root_widget) {
                const geometry::NanSize empty_size{};
                set_measured_layout_state(constraints, constraints.constrain(empty_size));
                return;
            }

            if (constraints.is_tight()) {
                set_measured_layout_state(
                    constraints,
                    geometry::NanSize{constraints.max_width(), constraints.max_height()});
                return;
            }

            m_root_widget->measure(constraints);
            auto measured = m_root_widget->measured_size();
            if (measured.width() <= 0.0f && measured.height() <= 0.0f) {
                measured = m_root_widget->preferred_size();
            }
            set_measured_layout_state(constraints, constraints.constrain(measured));
        }

        auto layout() -> void override {
            if (m_root_widget) {
                m_root_widget->runtime::NanWidget::set_bounds(x(), y(), width(), height());
                m_pending_root_layout = true;
                flush_root_layout("component-layout");
            }
            clear_layout_dirty();
        }

        void draw(tvg::SwCanvas& canvas) override {
            if (m_root_widget && (m_pending_root_layout || m_root_widget->is_layout_dirty())) {
                flush_root_layout(m_pending_root_layout ? "draw-pending" : "draw-child-dirty");
            }
            NanComponent::draw(canvas);
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return m_root_widget ? m_root_widget->preferred_size() : geometry::NanSize{};
        }

    private:
        auto flush_root_layout(std::string_view cause) -> void {
            if (!m_root_widget) {
                return;
            }

            auto log         = nandina::log::get("app.mounted");
            const auto start = detail::SteadyClock::now();
            m_root_widget->measure(geometry::NanConstraints::tight(width(), height()));
            const auto measure_done = detail::SteadyClock::now();
            m_root_widget->layout();
            const auto layout_done = detail::SteadyClock::now();
            m_pending_root_layout  = false;

            // 递归 flush 子树中所有嵌套的 MountedNodeComponent，
            // 避免在后续 draw() 遍历中再次触发重复的 measure+layout
            flush_nested_mounted(*m_root_widget);

            const auto measure_ms = detail::elapsed_ms(start, measure_done);
            const auto layout_ms  = detail::elapsed_ms(measure_done, layout_done);
            const auto total_ms   = detail::elapsed_ms(start, layout_done);

            if (total_ms >= detail::k_slow_layout_threshold_ms) {
                auto [fast_cnt, slow_cnt] = nandina::widgets::Label::measure_diag();

                auto surf_n                                = nandina::widgets::Surface::s_measure_count;
                nandina::widgets::Surface::s_measure_count = 0;

                auto lc_n                                         = nandina::layout::LayoutContainer::s_measure_count;
                nandina::layout::LayoutContainer::s_measure_count = 0;

                log.warn(
                    "Slow mounted flush: total={:.2f}ms measure={:.2f}ms layout={:.2f}ms cause={} root_type={} "
                    "size={:.0f}x{:.0f} label_fast={} label_slow={} surf_n={} lc_n={}",
                    total_ms,
                    measure_ms,
                    layout_ms,
                    cause,
                    typeid(*m_root_widget).name(),
                    width(),
                    height(),
                    fast_cnt,
                    slow_cnt,
                    surf_n,
                    lc_n);
            }
        }

        /// 递归遍历子树，立即 flush 所有嵌套 MountedNodeComponent
        static auto flush_nested_mounted(runtime::NanWidget& root) -> void {
            root.for_each_child([](runtime::NanWidget& child) {
                if (auto* nested = dynamic_cast<MountedNodeComponent*>(&child)) {
                    if (nested->m_pending_root_layout) {
                        nested->flush_root_layout("draw-pending");
                    }
                }
                MountedNodeComponent::flush_nested_mounted(child);
            });
        }

        Node m_root_node;
        runtime::NanWidget* m_root_widget{nullptr};
        bool m_pending_root_layout{false};
    };

    [[nodiscard]] inline auto mount(Node root) -> NanComponent::Ptr {
        if (root.empty()) {
            return nullptr;
        }
        return std::make_unique<MountedNodeComponent>(std::move(root));
    }

    [[nodiscard]] inline auto adopt(runtime::NanWidget::Ptr widget) -> Node {
        return Node{std::move(widget)};
    }

    [[nodiscard]] inline auto label(std::string_view text) -> LabelNode {
        auto widget = nandina::widgets::Label::create();
        if (!text.empty()) {
            widget->set_text(text);
        }
        return LabelNode{std::move(widget)};
    }

    [[nodiscard]] inline auto button(std::string_view text) -> ButtonNode {
        auto widget = nandina::widgets::Button::create();
        if (!text.empty()) {
            widget->set_text(text);
        }
        return ButtonNode{std::move(widget)};
    }

    [[nodiscard]] inline auto card(Children children) -> Node {
        auto widget = nandina::widgets::Card::create();
        Node result{std::move(widget)};
        for (auto& child : std::move(children).take()) {
            auto child_widget = std::move(child).take_widget();
            if (child_widget) {
                result.m_widget->add_child(std::move(child_widget));
            }
            result.absorb(std::move(child));
        }
        return result;
    }

    [[nodiscard]] inline auto panel(Children children) -> Node {
        auto widget = nandina::widgets::Panel::create();
        Node result{std::move(widget)};
        for (auto& child : std::move(children).take()) {
            auto child_widget = std::move(child).take_widget();
            if (child_widget) {
                result.m_widget->add_child(std::move(child_widget));
            }
            result.absorb(std::move(child));
        }
        return result;
    }

    // ── positioned() ────────────────────────────────────────────────────────
    // 自由定位容器：每个子节点通过 anchor_* 方法声明其相对于父容器（或先声明的兄弟节点）的位置。
    //
    // 示例：
    //   positioned(children(
    //       label("背景").fill(),                              // 填满父容器
    //       button("确认").anchor_right(16).anchor_bottom(16)  // 固定到右下角
    //           .anchor_width(80).anchor_height(36)
    //   ))
    [[nodiscard]] inline auto positioned(Children children) -> Node {
        auto widget = nandina::layout::Positioned::Create();
        Node result{std::move(widget)};
        for (auto& child : std::move(children).take()) {
            auto props        = std::move(child.m_positioned_props); // move before take_widget()
            auto child_widget = std::move(child).take_widget();
            if (child_widget) {
                static_cast<nandina::layout::Positioned*>(result.m_widget.get())
                    ->add_positioned_child(std::move(child_widget), std::move(props));
            }
            result.absorb(std::move(child));
        }
        return result;
    }

    /// row — 水平排列子组件（主轴：→，交叉轴：↓）。
    ///
    /// 常用链式方法：
    ///   .gap(12)                                        — 子组件间距
    ///   .align_items(LayoutAlignment::center)           — 交叉轴对齐（垂直居中）
    ///   .justify_content(LayoutAlignment::space_between) — 主轴分布
    ///   .padding(16)                                    — 内边距
    ///
    /// 示例：
    ///   row(children(btn_a, btn_b)).gap(8).align_items(LayoutAlignment::center)
    [[nodiscard]] inline auto row(Children children = {}) -> Node {
        auto widget = nandina::layout::Row::Create();
        Node result{std::move(widget)};
        for (auto& child : std::move(children).take()) {
            auto child_widget = std::move(child).take_widget();
            if (child_widget) {
                static_cast<nandina::layout::Row*>(result.m_widget.get())->add(std::move(child_widget));
            }
            result.absorb(std::move(child));
        }
        return result;
    }

    /// column — 垂直排列子组件（主轴：↓，交叉轴：→）。
    ///
    /// 常用链式方法：
    ///   .gap(12)                               — 子组件间距
    ///   .align_items(LayoutAlignment::stretch) — 子组件撑满列宽
    ///   .justify_content(LayoutAlignment::center) — 主轴居中
    ///   .padding(16, 24)                       — 水平/垂直内边距
    ///   .width(360)                            — 限定列宽（自动包裹 SizedBox）
    ///
    /// 示例：
    ///   column(children(header, body, footer))
    ///     .gap(12)
    ///     .align_items(LayoutAlignment::stretch)
    ///     .width(360)
    [[nodiscard]] inline auto column(Children children = {}) -> Node {
        auto widget = nandina::layout::Column::Create();
        Node result{std::move(widget)};
        for (auto& child : std::move(children).take()) {
            auto child_widget = std::move(child).take_widget();
            if (child_widget) {
                static_cast<nandina::layout::Column*>(result.m_widget.get())->add(std::move(child_widget));
            }
            result.absorb(std::move(child));
        }
        return result;
    }

    /// stack — Z 轴堆叠容器，子组件按声明顺序从下到上叠放，均相对容器左上角对齐。
    ///
    /// 常用于叠加背景层与前景层，如底图 + 浮动按钮。
    ///
    /// 示例：
    ///   stack(children(
    ///       surface().fill(),           // 底层背景色块
    ///       label("覆盖文字")            // 前景文字
    ///   ))
    [[nodiscard]] inline auto stack(Children children = {}) -> Node {
        auto widget = nandina::layout::Stack::Create();
        Node result{std::move(widget)};
        for (auto& child : std::move(children).take()) {
            auto child_widget = std::move(child).take_widget();
            if (child_widget) {
                static_cast<nandina::layout::Stack*>(result.m_widget.get())->add(std::move(child_widget));
            }
            result.absorb(std::move(child));
        }
        return result;
    }

    /// spacer — 弹性空白占位，在 Row / Column 中吸收剩余空间。
    ///
    /// @param flex  弹性系数，默认 1；与 expanded 共享剩余空间时按系数比例分配
    ///
    /// 示例：
    ///   // 将 reset 按钮在 Row 中水平居中
    ///   row(children(spacer(), reset_btn, spacer()))
    ///
    ///   // 将两个按钮推到 Row 的两端
    ///   row(children(btn_left, spacer(), btn_right))
    [[nodiscard]] inline auto spacer(const int flex = 1) -> Node {
        return Node{nandina::layout::Spacer::Create(flex)};
    }

    /// expanded — 让子组件在 Row / Column 的主轴方向上弹性填充剩余空间。
    ///
    /// @param child  被包裹的子节点（可直接传命名变量，无需显式 std::move）
    /// @param flex   弹性系数，默认 1；多个 expanded 按系数比例分配剩余空间
    ///
    /// @code
    /// 示例：
    ///   // Row 中两个按钮各占 50%
    ///   row(children(expanded(btn_a), expanded(btn_b))).gap(8)
    ///
    ///   // Row 中左侧占 2/3、右侧占 1/3
    ///   row(children(expanded(left_panel, 2), expanded(right_panel, 1)))
    ///  @endcode
    template <typename N>
        requires std::derived_from<std::decay_t<N>, Node>
    [[nodiscard]] inline auto expanded(N&& child) -> Node {
        return expanded(std::forward<N>(child), 1);
    }

    template <typename N>
        requires std::derived_from<std::decay_t<N>, Node>
    [[nodiscard]] inline auto expanded(N&& child, const int flex) -> Node {
        auto widget = nandina::layout::Expanded::Create(flex);
        Node result{std::move(widget)};
        Node child_node   = std::move(child); // 无论传入左值还是右值，均安全移动
        auto child_widget = std::move(child_node).take_widget();
        if (child_widget) {
            static_cast<nandina::layout::Expanded*>(result.m_widget.get())->child(std::move(child_widget));
        }
        result.absorb(std::move(child_node));
        return result;
    }

    /// padding — 为子组件添加内边距包装层（Padding 容器）。
    ///
    /// 也可在 row / column 上直接调用 .padding() 方法，效果相同。
    ///
    /// 示例：
    ///   padding(label("内容")).padding(16, 8)   // 水平 16px，垂直 8px
    [[nodiscard]] inline auto padding(Node child) -> Node {
        auto widget = nandina::layout::Padding::Create();
        Node result{std::move(widget)};
        auto child_widget = std::move(child).take_widget();
        if (child_widget) {
            static_cast<nandina::layout::Padding*>(result.m_widget.get())->child(std::move(child_widget));
        }
        result.absorb(std::move(child));
        return result;
    }

    /// center — 将子组件在可用空间内水平 + 垂直居中。
    ///
    /// 常用于页面根节点，使整体内容块居中显示。
    ///
    /// 示例：
    ///   return mount(center(column(children(...)).width(360)));
    [[nodiscard]] inline auto center(Node child) -> Node {
        auto widget = nandina::layout::Center::Create();
        Node result{std::move(widget)};
        auto child_widget = std::move(child).take_widget();
        if (child_widget) {
            static_cast<nandina::layout::Center*>(result.m_widget.get())->child(std::move(child_widget));
        }
        result.absorb(std::move(child));
        return result;
    }

    /// sized_box — 强制子组件具有指定的宽度和/或高度。
    ///
    /// 注意：直接在节点上调用 .width() / .height() / .size() 会自动包裹 SizedBox，
    /// 无需手动调用此函数。
    ///
    /// 示例：
    ///   // 以下两种写法等价：
    ///   sized_box(label("文本")).width(200).height(48)
    ///   label("文本").width(200).height(48)
    [[nodiscard]] inline auto sized_box(Node child) -> Node {
        auto widget = nandina::layout::SizedBox::Create();
        Node result{std::move(widget)};
        auto child_widget = std::move(child).take_widget();
        if (child_widget) {
            static_cast<nandina::layout::SizedBox*>(result.m_widget.get())->child(std::move(child_widget));
        }
        result.absorb(std::move(child));
        return result;
    }

    struct AppConfig {
        std::string title = "NandinaUI";
        int width         = 1280;
        int height        = 720;
        bool resizable    = true;
        bool high_dpi     = true;
        /// 默认背景色（NanColor::from(NanRgb{0,0,0,0}) 表示透明，不绘制背景）
        NanColor bg_color = NanColor::from(NanRgb{21, 24, 32});
    };

    class NanAppWindow {
    public:
        explicit NanAppWindow(const AppConfig& config) : m_config(config) {
        }

        virtual ~NanAppWindow() {
        }

        auto set_root_component(NanComponent::Ptr component) -> void {
            m_hovered_widget = nullptr;
            m_root_component = std::move(component);
            request_root_reflow(true);
        }

        auto set_root(Node root) -> void {
            set_root_component(mount(std::move(root)));
        }

        auto replace_root(Node root) -> void {
            set_root(std::move(root));
        }

        auto run() -> void;

        [[nodiscard]] auto width() const noexcept -> int {
            return m_active_runtime_window ? m_active_runtime_window->width() : m_config.width;
        }

        [[nodiscard]] auto height() const noexcept -> int {
            return m_active_runtime_window ? m_active_runtime_window->height() : m_config.height;
        }

    protected:
        virtual void on_ready() {
        }

        /// 子类可重写 on_update。基类在此处以节流方式处理 resize。
        virtual void on_update(const double delta_seconds) {
            (void) delta_seconds;
            apply_throttled_resize();
        }

        virtual void on_draw(tvg::SwCanvas& canvas) {
            // ── 背景层（Surface 组件，统一使用 NanColor） ──
            if (m_background) {
                m_background->draw(canvas);
            }

            if (m_root_component) {
                auto log         = nandina::log::get("app.window");
                const auto start = detail::SteadyClock::now();
                consume_root_reflow();
                const auto layout_done = detail::SteadyClock::now();
                m_root_component->draw(canvas);
                const auto draw_done = detail::SteadyClock::now();
                m_root_component->clear_dirty_recursive();

                const auto layout_ms = detail::elapsed_ms(start, layout_done);
                const auto draw_ms   = detail::elapsed_ms(layout_done, draw_done);
                const auto total_ms  = detail::elapsed_ms(start, draw_done);

                if (total_ms >= detail::k_slow_layout_threshold_ms) {
                    log.warn(
                        "Slow app on_draw: "
                        "total={:.2f}ms root_layout={:.2f}ms root_draw={:.2f}ms root_type={} size={}x{}",
                        total_ms,
                        layout_ms,
                        draw_ms,
                        typeid(*m_root_component).name(),
                        width(),
                        height());
                }
            }
        }

        [[nodiscard]] auto hit_test_root_component(const float x, const float y) -> runtime::NanWidget* {
            if (!m_root_component) {
                return nullptr;
            }

            consume_root_reflow();

            auto* hit = m_root_component->hit_test(x, y);
            return hit && hit->is_interactive() ? hit : nullptr;
        }

    private:
        [[nodiscard]] auto needs_redraw() const noexcept -> bool {
            return m_root_component && m_root_component->dirty();
        }

        static auto pointer_move_from_button(const runtime::PointerButtonEvent& event) noexcept
            -> runtime::PointerMoveEvent {
            return runtime::PointerMoveEvent{
                .x = event.x,
                .y = event.y,
                .delta_x = 0.0,
                .delta_y = 0.0,
            };
        }

        auto sync_hover_target(runtime::NanWidget* next, const runtime::PointerMoveEvent& event) -> bool {
            if (m_hovered_widget == next) {
                return false;
            }

            if (m_hovered_widget) {
                m_hovered_widget->dispatch_pointer_leave(event);
            }

            m_hovered_widget = next;

            if (m_hovered_widget) {
                m_hovered_widget->dispatch_pointer_enter(event);
            }

            return true;
        }

        auto clear_hover_target(const runtime::PointerMoveEvent& event) -> void {
            sync_hover_target(nullptr, event);
        }

        auto on_window_focus_lost() -> void {
            clear_hover_target(runtime::PointerMoveEvent{});
        }

        auto consume_root_reflow() -> void {
            if (!m_root_component) {
                return;
            }

            if (m_pending_root_bounds_sync) {
                apply_root_component_bounds();
                m_pending_root_bounds_sync = false;
            }

            if (!m_root_component->is_layout_dirty()) {
                return;
            }

            auto log         = nandina::log::get("app.window");
            const auto width = static_cast<float>(m_active_runtime_window
                                                      ? m_active_runtime_window->width()
                                                      : m_config.width);
            const auto height = static_cast<float>(m_active_runtime_window
                                                       ? m_active_runtime_window->height()
                                                       : m_config.height);
            const auto start = detail::SteadyClock::now();
            m_root_component->measure(geometry::NanConstraints::tight(width, height));
            const auto measure_done = detail::SteadyClock::now();
            m_root_component->layout();
            const auto layout_done = detail::SteadyClock::now();

            const auto measure_ms = detail::elapsed_ms(start, measure_done);
            const auto layout_ms  = detail::elapsed_ms(measure_done, layout_done);
            const auto total_ms   = detail::elapsed_ms(start, layout_done);

            if (total_ms >= detail::k_slow_layout_threshold_ms) {
                log.warn(
                    "Slow root layout: total={:.2f}ms measure={:.2f}ms layout={:.2f}ms root_type={} size={:.0f}x{:.0f}",
                    total_ms,
                    measure_ms,
                    layout_ms,
                    typeid(*m_root_component).name(),
                    width,
                    height);
            }
        }

        auto apply_root_component_bounds() -> void {
            if (!m_root_component) {
                return;
            }

            const auto width = static_cast<float>(m_active_runtime_window
                                                      ? m_active_runtime_window->width()
                                                      : m_config.width);
            const auto height = static_cast<float>(m_active_runtime_window
                                                       ? m_active_runtime_window->height()
                                                       : m_config.height);

            m_root_component->runtime::NanWidget::set_bounds(0.0f, 0.0f, width, height);
            m_root_component->mark_layout_dirty();
        }

        auto request_root_reflow(const bool sync_bounds = false) -> void {
            sync_background_bounds();

            if (!m_root_component) {
                return;
            }

            m_pending_root_bounds_sync = m_pending_root_bounds_sync || sync_bounds;
            m_root_component->mark_layout_dirty();
        }

        auto sync_background_bounds() -> void {
            if (!m_background)
                return;
            const auto w = static_cast<float>(m_active_runtime_window
                                                  ? m_active_runtime_window->width()
                                                  : m_config.width);
            const auto h = static_cast<float>(m_active_runtime_window
                                                  ? m_active_runtime_window->height()
                                                  : m_config.height);
            m_background->set_bounds(0.0f, 0.0f, w, h);
        }

        /// resize 节流：以最小间隔（~16ms）应用最新窗口尺寸，
        /// 避免拖拽时每个 SDL resize 事件都触发完整 measure→layout→draw
        auto apply_throttled_resize() -> void {
            if (!m_pending_resize)
                return;

            const auto now        = detail::SteadyClock::now();
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_last_resize_apply_time).count();
            if (elapsed_ms < static_cast<long long>(k_resize_throttle_ms))
                return;

            m_last_resize_apply_time = now;
            m_pending_resize         = false;
            request_root_reflow(true);
        }

        auto on_resize_pending() -> void {
            m_pending_resize = true;
        }

        AppConfig m_config;
        runtime::NanWindow* m_active_runtime_window{nullptr};
        NanComponent::Ptr m_root_component{nullptr};
        runtime::NanWidget* m_hovered_widget{nullptr};
        bool m_pending_root_bounds_sync{false};

        // ── 背景层（Surface 组件，非 widget 树成员） ──────
        widgets::Surface::Ptr m_background{nullptr};

        // ── resize 节流状态 ─────────────────────────────────
        bool m_pending_resize{false};
        detail::SteadyClock::time_point m_last_resize_apply_time{};
        static constexpr std::uint64_t k_resize_throttle_ms = 16; // ~60fps 间隔

        class BridgeWindow final : public runtime::NanWindow {
        public:
            BridgeWindow(NanAppWindow& owner,
                const runtime::NanWindow::Config& config) : runtime::NanWindow(config),
                                                            m_owner(owner) {
            }

        protected:
            void on_ready() override {
                m_owner.on_ready();
            }

            void on_update(double delta) override {
                m_owner.on_update(delta);
                // 确保 resize 节流在每一帧都会检查，即使用户重写 on_update 时未调用基类
                m_owner.apply_throttled_resize();
            }

            void on_draw(tvg::SwCanvas& canvas) override {
                m_owner.on_draw(canvas);
            }

            [[nodiscard]] auto should_present_frame() const noexcept -> bool override {
                return m_owner.needs_redraw();
            }

            void on_resize(int, int) override {
                // 不立即 sync_root_component_bounds，改为标记 pending，
                // 由 on_update → apply_throttled_resize 以 16ms 间隔节流应用
                m_owner.on_resize_pending();
            }

            void on_focus_lost() override {
                m_owner.on_window_focus_lost();
            }

            void on_pointer_move(const runtime::PointerMoveEvent& event) override {
                auto* hit = m_owner.hit_test_root_component(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                const bool changed = m_owner.sync_hover_target(hit, event);
                if (hit && !changed) {
                    hit->dispatch_event(event);
                }
            }

            void on_pointer_enter(const runtime::PointerMoveEvent& event) override {
                auto* hit = m_owner.hit_test_root_component(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                m_owner.sync_hover_target(hit, event);
            }

            void on_pointer_leave(const runtime::PointerMoveEvent& event) override {
                m_owner.clear_hover_target(event);
            }

            void on_pointer_down(const runtime::PointerButtonEvent& event) override {
                auto* hit = m_owner.hit_test_root_component(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                m_owner.sync_hover_target(hit, pointer_move_from_button(event));
                if (hit) {
                    hit->dispatch_event(event, runtime::EventType::PointerDown);
                }
            }

            void on_pointer_up(const runtime::PointerButtonEvent& event) override {
                auto* hit = m_owner.hit_test_root_component(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                m_owner.sync_hover_target(hit, pointer_move_from_button(event));
                if (hit) {
                    hit->dispatch_event(event, runtime::EventType::PointerUp);
                }
            }

        private:
            NanAppWindow& m_owner;
        };
    };

    class NanApplication {
    public:
        NanApplication() {
            log::init("nandina", log::Level::Debug);
        }

        virtual ~NanApplication() {
            log::shutdown();
        }

        // Take ownership via unique_ptr in implementation but maybe GCC dislikes it in header?
        // Let's keep it but make sure the TU where it is used is clean.
        auto run(std::unique_ptr<NanAppWindow> window) -> void {
            if (!window)
                return;
            m_window = std::move(window);
            m_window->run();
        }

    private:
        std::unique_ptr<NanAppWindow> m_window;
    };

    inline auto NanAppWindow::run() -> void {
        auto logger = log::get("app.window");

        // ── 初始化背景层 ──────────────────────────────────
        if (m_config.bg_color.to<NanRgb>().alpha() > 0) {
            m_background = widgets::Surface::create();
            m_background->set_bg_color(m_config.bg_color);
            m_background->set_corner_radius(0.0f);
        }

        runtime::NanWindow::Config runtime_config{
            .title = m_config.title,
            .width = m_config.width,
            .height = m_config.height,
            .resizable = m_config.resizable,
            .high_dpi = m_config.high_dpi
        };

        BridgeWindow window{*this, runtime_config};
        m_active_runtime_window = &window;

        request_root_reflow(true);

        logger.info("Window starting: {} ({}x{})", m_config.title, m_config.width, m_config.height);
        window.run();
        logger.info("Window stopped");

        m_active_runtime_window = nullptr;
    }
} // namespace nandina::app
