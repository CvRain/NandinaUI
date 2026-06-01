/**
 * @file nan_application.cppm
 * @brief NandinaUI 应用开发层主模块 — Authoring DSL 与 App Shell
 *
 * 本模块是 nandina.app 层的聚合入口，提供：
 *   - **Node 系统**  — 可组合节点（Widget 所有权 + Flutter 风格链式配置）
 *   - **NodeFactory** — 通用节点工厂（消除逐组件手写样板，第三方 Widget 即插即用）
 *   - **组件工厂**   — row / column / center / padding / button / label 等语法糖
 *   - **挂载系统**   — MountedNodeComponent（将 authoring 树挂载到 Widget 树）
 *   - **应用窗口**   — NanAppWindow + BridgeWindow（SDL 窗口封装 + resize 节流）
 *
 * ## 模块拆分计划（待 GCC 模块 bug 修复后执行）
 *
 * 当前因 GCC 16 已知问题保持单文件：
 *   1. `export` 类型跨模块不可重声明（阻止 friend-free 后的模块分离）
 *   2. thorvg + std::unique_ptr 组合在模块分区接口中触发 .gcm 序列化损坏
 *
 * 待编译器修复后拆分为 5 个模块分区：
 *   :node              — Node / Ref / Children / NodeLike / NodeFactory
 *   :widget_nodes      — WidgetNode / LabelNode / ButtonNode
 *   :component         — NanComponent / MountedNodeComponent / mount()
 *   :builtin_factories — row / column / center / padding 等语法糖工厂
 *   :app_window        — NanAppWindow / BridgeWindow / NanApplication
 *
 * @see docs/component-authoring-and-mounting.md
 * @see docs/component-composition-api-v1.md
 */

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
import nandina.widgets.icon;
import nandina.widgets.label;
import nandina.widgets.panel;
import nandina.widgets.progressbar;
import nandina.widgets.sidebar;
import nandina.widgets.sidebar_group;
import nandina.widgets.sidebar_menu_button;
import nandina.widgets.surface;
import nandina.widgets.tag;
import nandina.widgets.text_field;
import nandina.widgets.field;
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
    class TagNode;
    class ButtonNode;
    class TextFieldNode;
    class FieldNode;
    class ProgressBarNode;
    class SidebarMenuButtonNode;
    template<typename W>
    class WidgetNode;

    class NanAppWindow;

    [[nodiscard]] inline auto adopt(runtime::NanWidget::Ptr widget) -> Node;

    [[nodiscard]] inline auto label(std::string_view text = {}) -> LabelNode;

    [[nodiscard]] inline auto tag(std::string_view text = {}) -> TagNode;

    [[nodiscard]] inline auto button(std::string_view text = {}) -> ButtonNode;

    [[nodiscard]] inline auto text_field() -> TextFieldNode;

    [[nodiscard]] inline auto field() -> FieldNode;

    [[nodiscard]] inline auto progress_bar() -> ProgressBarNode;

    [[nodiscard]] inline auto sidebar_menu_button(std::string_view text = {}) -> SidebarMenuButtonNode;

    [[nodiscard]] inline auto card(Children children) -> Node;

    [[nodiscard]] inline auto panel(Children children) -> Node;

    [[nodiscard]] inline auto positioned(Children children) -> Node;

    template<typename T>
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

        /** @internal 绑定到真实 widget */
        auto bind(runtime::NanWidget *widget) noexcept -> void { m_ptr = dynamic_cast<T *>(widget); }
        /** @internal 重置引用 */
        auto reset() noexcept -> void { m_ptr = nullptr; }

        T *m_ptr{nullptr};
    };

    class Node {
    public:
        Node() = default;

        Node(Node &&) noexcept = default;

        auto operator=(Node &&) noexcept -> Node& = default;

        Node(const Node &) = delete;

        auto operator=(const Node &) -> Node& = delete;

        [[nodiscard]] auto empty() const noexcept -> bool {
            return m_widget == nullptr;
        }

        template<typename Self, typename T>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto bind(this Self &&self, Ref<T> &ref) -> Self&& {
            auto &node = static_cast<Node &>(self);
            auto *widget = node.m_widget.get();
            node.m_ref_binders.push_back([&ref, widget]() {
                ref.bind(widget);
            });
            node.m_ref_resetters.push_back([&ref]() {
                ref.reset();
            });
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto key(this Self &&self, std::string_view value) -> Self&& {
            auto &node = static_cast<Node &>(self);
            node.m_key.assign(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto title(this Self &&self, std::string_view value) -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::widgets::Card *>(node.unwrapped())) {
                widget->set_title(std::string{value});
            }
            else if (auto *widget = dynamic_cast<nandina::widgets::Panel *>(node.unwrapped())) {
                widget->set_title(value);
            }
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto bg_color(this Self &&self, const nandina::NanColor &value) -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::widgets::Surface *>(node.unwrapped())) {
                widget->set_bg_color(value);
            }
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto corner_radius(this Self &&self, const float value) -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::widgets::Surface *>(node.unwrapped())) {
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
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto padding(this Self &&self, const float value) -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::layout::Padding *>(node.m_widget.get())) {
                widget->padding(value);
            }
            else if (auto *widget = dynamic_cast<nandina::layout::LayoutContainer *>(node.m_widget.get())) {
                widget->padding(value);
            }
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto padding(this Self &&self, const float horizontal, const float vertical) -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::layout::Padding *>(node.m_widget.get())) {
                widget->padding(horizontal, vertical);
            }
            else if (auto *widget = dynamic_cast<nandina::layout::LayoutContainer *>(node.m_widget.get())) {
                widget->padding(horizontal, vertical);
            }
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto padding(this Self &&self, const float left, const float top, const float right, const float bottom)
            -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::layout::Padding *>(node.m_widget.get())) {
                widget->padding(left, top, right, bottom);
            }
            else if (auto *widget = dynamic_cast<nandina::layout::LayoutContainer *>(node.m_widget.get())) {
                widget->padding(left, top, right, bottom);
            }
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto padding_top(this Self &&self, const float value) -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::layout::LayoutContainer *>(node.m_widget.get())) {
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
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto gap(this Self &&self, const float value) -> Self&& {
            const auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::layout::LayoutContainer *>(node.m_widget.get())) {
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
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto align_items(this Self &&self, const nandina::layout::LayoutAlignment value) -> Self&& {
            const auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::layout::LayoutContainer *>(node.m_widget.get())) {
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
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto justify_content(this Self &&self, const nandina::layout::LayoutAlignment value)
            -> Self&& {
            const auto &node = static_cast<Node &>(self);
            if (auto *widget = dynamic_cast<nandina::layout::LayoutContainer *>(node.m_widget.get())) {
                widget->justify_content(value);
            }
            return std::forward<Self>(self);
        }

        /// width — 限定节点宽度（像素）；若当前节点不是 SizedBox，自动包裹一层。
        ///
        /// 示例：
        ///   column(children(...)).width(360)   // 将列限宽为 360px
        ///   label("文本").width(120)            // 限制标签最大宽度
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto width(this Self &&self, const float value) -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *sb = dynamic_cast<nandina::layout::SizedBox *>(node.m_widget.get())) {
                sb->width(value);
            }
            else {
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
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto height(this Self &&self, const float value) -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *sb = dynamic_cast<nandina::layout::SizedBox *>(node.m_widget.get())) {
                sb->height(value);
            }
            else {
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
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto size(this Self &&self, const geometry::NanSize &value) -> Self&& {
            auto &node = static_cast<Node &>(self);
            if (auto *sb = dynamic_cast<nandina::layout::SizedBox *>(node.m_widget.get())) {
                sb->size(value);
            }
            else {
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

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_left(this Self &&self, float v) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.left = v;
            return std::forward<Self>(self);
        }

        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_left(this Self &&self, Fn fn) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.left =
                    std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_top(this Self &&self, float v) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.top = v;
            return std::forward<Self>(self);
        }

        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_top(this Self &&self, Fn fn) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.top =
                    std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_right(this Self &&self, float v) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.right = v;
            return std::forward<Self>(self);
        }

        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_right(this Self &&self, Fn fn) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.right =
                    std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_bottom(this Self &&self, float v) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.bottom = v;
            return std::forward<Self>(self);
        }

        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_bottom(this Self &&self, Fn fn) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.bottom =
                    std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_width(this Self &&self, float v) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.width = v;
            return std::forward<Self>(self);
        }

        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_width(this Self &&self, Fn fn) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.width =
                    std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto anchor_height(this Self &&self, float v) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.height = v;
            return std::forward<Self>(self);
        }

        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
                     && std::invocable<Fn>
                     && std::convertible_to<std::invoke_result_t<Fn>, float>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, float>)
        auto anchor_height(this Self &&self, Fn fn) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.height =
                    std::function<float()>{std::move(fn)};
            return std::forward<Self>(self);
        }

        /// 填满父容器宽度（左右锚点均为 0）
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto fill_width(this Self &&self) -> Self&& {
            auto &node = static_cast<Node &>(self);
            node.m_positioned_props.left = 0.0f;
            node.m_positioned_props.right = 0.0f;
            return std::forward<Self>(self);
        }

        /// 填满父容器高度（上下锚点均为 0）
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto fill_height(this Self &&self) -> Self&& {
            auto &node = static_cast<Node &>(self);
            node.m_positioned_props.top = 0.0f;
            node.m_positioned_props.bottom = 0.0f;
            return std::forward<Self>(self);
        }

        /// 填满父容器（左右上下锚点均为 0）
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto fill(this Self &&self) -> Self&& {
            auto &node = static_cast<Node &>(self);
            node.m_positioned_props.left = 0.0f;
            node.m_positioned_props.right = 0.0f;
            node.m_positioned_props.top = 0.0f;
            node.m_positioned_props.bottom = 0.0f;
            return std::forward<Self>(self);
        }

        /// 将此子组件的布置结果写入 GeomRef（供后续兄弟节点 anchor lambda 引用）
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, Node>
        auto bind_geom(this Self &&self, nandina::layout::GeomRef &ref) -> Self&& {
            static_cast<Node &>(self).m_positioned_props.geom_ref = &ref;
            return std::forward<Self>(self);
        }

        // ── 内部 API（框架层使用）─────────────────────────────────────────

        /** @internal 从 widget 构造节点 */
        explicit Node(std::unique_ptr<runtime::NanWidget> widget) : m_widget(std::move(widget)) {
        }

        /** @internal 提取内部 widget 所有权（仅右值） */
        [[nodiscard]] auto take_widget() && -> std::unique_ptr<runtime::NanWidget> { return std::move(m_widget); }

        /** @internal 提取定位属性（仅右值） */
        [[nodiscard]] auto take_positioned_props() && -> nandina::layout::PositionedProps {
            return std::move(m_positioned_props);
        }

        /** @internal 吸收子节点的 Ref binder */
        auto absorb(Node child) -> void {
            for (auto &b: child.m_ref_binders) m_ref_binders.push_back(std::move(b));
            for (auto &r: child.m_ref_resetters) m_ref_resetters.push_back(std::move(r));
        }

        /** @internal 绑定所有 Ref<T> */
        auto bind_refs() -> void { for (auto &b: m_ref_binders) b(); }

        /** @internal 重置所有 Ref<T> */
        auto reset_refs() -> void { for (auto &r: m_ref_resetters) r(); }

        /** 获取底层 widget 指针 */
        [[nodiscard]] auto widget_ptr() -> runtime::NanWidget* { return m_widget.get(); }

        /** @internal 穿透 SizedBox 找真实组件 */
        [[nodiscard]] auto unwrapped() const -> runtime::NanWidget* {
            if (!m_widget) return nullptr;
            if (auto *sb = dynamic_cast<nandina::layout::SizedBox *>(m_widget.get())) {
                runtime::NanWidget *result = nullptr;
                sb->for_each_child([&](runtime::NanWidget &c) { if (!result) result = &c; });
                return result ? result : m_widget.get();
            }
            return m_widget.get();
        }

    private:
        using RefBinder = std::move_only_function<void()>;
        using RefResetter = std::move_only_function<void()>;

        auto append_ref_binder(RefBinder b, RefResetter r) -> void {
            m_ref_binders.push_back(std::move(b));
            m_ref_resetters.push_back(std::move(r));
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
    template<typename W>
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
        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, WidgetNode<W>>
                     && std::invocable<Fn, W &>
        auto configure(this Self &&self, Fn &&fn) -> Self&& {
            std::invoke(std::forward<Fn>(fn), *static_cast<WidgetNode<W> &>(self).m_typed);
            return std::forward<Self>(self);
        }

    protected:
        explicit WidgetNode(W::Ptr w)
            : Node(std::move(w))
              , m_typed(static_cast<W *>(Node::unwrapped())) {
        }

        W *m_typed{nullptr};
    };

    class LabelNode : public WidgetNode<nandina::widgets::Label> {
    public:
        explicit LabelNode(nandina::widgets::Label::Ptr widget)
            : WidgetNode<nandina::widgets::Label>(std::move(widget)) {
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto text(this Self &&self, std::string_view value) -> Self&& {
            self.m_typed->set_text(value);
            return std::forward<Self>(self);
        }

        template<typename Self, typename F>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode> &&
                     std::invocable<F> &&
                     std::convertible_to<std::invoke_result_t<F>, std::string_view> &&
                     (!std::convertible_to<F, std::string_view>)
        auto text(this Self &&self, F fn) -> Self&& {
            auto *w = self.m_typed;
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

        [[nodiscard]] auto error() const -> bool {
            return m_typed->error();
        }

        [[nodiscard]] auto required() const -> bool {
            return m_typed->required();
        }

        // ── setter ──────────────────────────────────────────────────────────

        /// color — 快捷设置文本颜色；等价于 .font([](auto& f){ f.color(v); })
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto color(this Self &&self, const nandina::NanColor &value) -> Self&& {
            self.m_typed->set_color(value);
            return std::forward<Self>(self);
        }

        /// font(NanFont) — 替换整个字体配置。
        ///
        /// 示例：label("标题").font(NanFont{}.size(20).weight(NanFontWeight::bold))
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
                     && (!std::invocable<Self, nandina::text::NanFont &>)
        auto font(this Self &&self, nandina::text::NanFont font_config) -> Self&& {
            self.m_typed->set_font(std::move(font_config));
            return std::forward<Self>(self);
        }

        /// font(Fn) — 局部更新字体属性；Fn: (NanFont&) -> void
        ///
        /// 示例：
        ///   label("文本").font([](auto& f){ f.size(16).color(NanColor::white()); })
        ///   label("文本").font([](auto& f){ f.size(f.size() + 2); })  // 在现有基础上调整
        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
                     && std::invocable<Fn, nandina::text::NanFont &>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, nandina::text::NanFont>)
        auto font(this Self &&self, Fn &&fn) -> Self&& {
            auto updated = self.m_typed->font();
            std::invoke(std::forward<Fn>(fn), updated);
            self.m_typed->set_font(std::move(updated));
            return std::forward<Self>(self);
        }

        /// disabled — 禁用标签；对应 shadcn peer-disabled:opacity-70，文本渲染降至 70% 透明度
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto disabled(this Self &&self, const bool value) -> Self&& {
            self.m_typed->set_disabled(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto error(this Self &&self, const bool value) -> Self&& {
            self.m_typed->set_error(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto required(this Self &&self, const bool value) -> Self&& {
            self.m_typed->set_required(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto typography_role(this Self &&self, const nandina::theme::NanTypographyRole value) -> Self&& {
            self.m_typed->set_typography_role(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto align(this Self &&self, const nandina::widgets::TextAlign value) -> Self&& {
            self.m_typed->set_align(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, LabelNode>
        auto vertical_align(this Self &&self, const nandina::widgets::TextVerticalAlign value)
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

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto text(this Self &&self, std::string_view value) -> Self&& {
            self.m_typed->text(value);
            return std::forward<Self>(self);
        }

        template<typename Self, typename F>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode> &&
                     std::invocable<F> &&
                     std::convertible_to<std::invoke_result_t<F>, std::string_view> &&
                     (!std::convertible_to<F, std::string_view>)
        auto text(this Self &&self, F fn) -> Self&& {
            auto *w = self.m_typed;
            w->add_opaque_cleanup(
                std::make_shared<nandina::reactive::Effect>(
                    [w, fn = std::move(fn)] {
                        w->text(fn());
                    }
                )
            );
            return std::forward<Self>(self);
        }

        [[nodiscard]] auto text() const -> const std::string& {
            return m_typed->text();
        }

        [[nodiscard]] auto font() const -> const nandina::text::NanFont& {
            return m_typed->font();
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto font(this Self &&self, nandina::text::NanFont font_config) -> Self&& {
            self.m_typed->font(std::move(font_config));
            return std::forward<Self>(self);
        }

        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
                     && std::invocable<Fn, nandina::text::NanFont &>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, nandina::text::NanFont>)
        auto font(this Self &&self, Fn &&fn) -> Self&& {
            auto updated = self.m_typed->font();
            std::invoke(std::forward<Fn>(fn), updated);
            self.m_typed->font(std::move(updated));
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto button_style(this Self &&self, const theme::NanButtonStyle &style) -> Self&& {
            self.m_typed->color_variant(style.color_variant);
            self.m_typed->variant(style.variant);
            self.m_typed->size(style.size);

            auto font = nandina::text::NanFont{}
                    .size(style.font_size)
                    .weight(style.font_weight)
                    .color(style.font_color)
                    .overflow(style.overflow)
                    .single_line(style.single_line);
            self.m_typed->font(std::move(font));
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto variant(this Self &&self, nandina::widgets::ButtonVariant value) -> Self&& {
            self.m_typed->variant(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto size(this Self &&self, nandina::widgets::ButtonSize value) -> Self&& {
            self.m_typed->size(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto color_variant(this Self &&self, nandina::theme::ColorVariant value) -> Self&& {
            self.m_typed->color_variant(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto icon(this Self &&self, nandina::widgets::IconType value) -> Self&& {
            self.m_typed->icon(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto icon_left(this Self &&self, nandina::widgets::IconType value) -> Self&& {
            self.m_typed->icon_left(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto icon_right(this Self &&self, nandina::widgets::IconType value) -> Self&& {
            self.m_typed->icon_right(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto disabled(this Self &&self, bool value) -> Self&& {
            self.m_typed->disabled(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto loading(this Self &&self, bool value) -> Self&& {
            self.m_typed->loading(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_click(this Self &&self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_click(std::move(handler));
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_press(this Self &&self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_press(std::move(handler));
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_release(this Self &&self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_release(std::move(handler));
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_hover(this Self &&self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_hover(std::move(handler));
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ButtonNode>
        auto on_leave(this Self &&self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_leave(std::move(handler));
            return std::forward<Self>(self);
        }
    };

    class TagNode : public WidgetNode<nandina::widgets::Tag> {
    public:
        using Node::size;

        explicit TagNode(nandina::widgets::Tag::Ptr widget)
            : WidgetNode<nandina::widgets::Tag>(std::move(widget)) {
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TagNode>
        auto text(this Self &&self, std::string_view value) -> Self&& {
            self.m_typed->text(value);
            return std::forward<Self>(self);
        }

        [[nodiscard]] auto text() const -> const std::string& {
            return m_typed->text();
        }

        [[nodiscard]] auto font() const -> const nandina::text::NanFont& {
            return m_typed->font();
        }

        [[nodiscard]] auto disabled() const -> bool {
            return m_typed->disabled();
        }

        [[nodiscard]] auto color_variant() const -> nandina::theme::ColorVariant {
            return m_typed->color_variant();
        }

        [[nodiscard]] auto tag_size() const -> nandina::widgets::TagSize {
            return m_typed->size();
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TagNode>
        auto font(this Self &&self, nandina::text::NanFont font_config) -> Self&& {
            self.m_typed->font(std::move(font_config));
            return std::forward<Self>(self);
        }

        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, TagNode>
                     && std::invocable<Fn, nandina::text::NanFont &>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, nandina::text::NanFont>)
        auto font(this Self &&self, Fn &&fn) -> Self&& {
            auto updated = self.m_typed->font();
            std::invoke(std::forward<Fn>(fn), updated);
            self.m_typed->font(std::move(updated));
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TagNode>
        auto size(this Self &&self, const nandina::widgets::TagSize value) -> Self&& {
            self.m_typed->size(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TagNode>
        auto color_variant(this Self &&self, const nandina::theme::ColorVariant value) -> Self&& {
            self.m_typed->color_variant(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TagNode>
        auto disabled(this Self &&self, const bool value) -> Self&& {
            self.m_typed->disabled(value);
            return std::forward<Self>(self);
        }
    };

    // ═══════════════════════════════════════════════════════════
    // TextFieldNode — 单行输入框链式配置节点
    // ═══════════════════════════════════════════════════════════
    //
    // 用法：
    //   text_field()
    //       .value("alice@example.com")
    //       .placeholder("Email")
    //       .read_only(false)
    //       .on_change([&](std::string_view v) { ... })
    //       .on_submit([&](std::string_view v) { ... })
    //       .bind(ref)

    class TextFieldNode : public WidgetNode<nandina::widgets::TextField> {
    public:
        explicit TextFieldNode(nandina::widgets::TextField::Ptr widget)
            : WidgetNode<nandina::widgets::TextField>(std::move(widget)) {
        }

        // ── value ──────────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TextFieldNode>
        auto value(this Self &&self, std::string_view v) -> Self&& {
            self.m_typed->set_value(std::string{v});
            return std::forward<Self>(self);
        }

        // ── placeholder ────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TextFieldNode>
        auto placeholder(this Self &&self, std::string_view v) -> Self&& {
            self.m_typed->set_placeholder(std::string{v});
            return std::forward<Self>(self);
        }

        // ── disabled ───────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TextFieldNode>
        auto disabled(this Self &&self, bool v) -> Self&& {
            self.m_typed->set_disabled(v);
            return std::forward<Self>(self);
        }

        // ── read_only ──────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TextFieldNode>
        auto read_only(this Self &&self, bool v) -> Self&& {
            self.m_typed->set_read_only(v);
            return std::forward<Self>(self);
        }

        // ── invalid ────────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TextFieldNode>
        auto invalid(this Self &&self, bool v) -> Self&& {
            self.m_typed->set_invalid(v);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TextFieldNode>
        auto color_variant(this Self &&self, nandina::theme::ColorVariant v) -> Self&& {
            self.m_typed->color_variant(v);
            return std::forward<Self>(self);
        }

        // ── on_change ──────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TextFieldNode>
        auto on_change(this Self &&self, std::function<void(std::string_view)> cb) -> Self&& {
            self.m_typed->on_change(std::move(cb));
            return std::forward<Self>(self);
        }

        // ── on_submit ──────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, TextFieldNode>
        auto on_submit(this Self &&self, std::function<void(std::string_view)> cb) -> Self&& {
            self.m_typed->on_submit(std::move(cb));
            return std::forward<Self>(self);
        }
    };

    // ═══════════════════════════════════════════════════════════
    // FieldNode — 表单语义容器链式配置节点
    // ═══════════════════════════════════════════════════════════
    //
    // 用法：
    //   field()
    //       .label("Email")
    //       .helper_text("We'll never share your email.")
    //       .error_text("Invalid email address.")
    //       .control(text_field().placeholder("you@example.com"))
    //       .required(true)
    //       .bind(ref)

    class FieldNode : public WidgetNode<nandina::widgets::Field> {
    public:
        explicit FieldNode(nandina::widgets::Field::Ptr widget)
            : WidgetNode<nandina::widgets::Field>(std::move(widget)) {
        }

        // ── label ──────────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, FieldNode>
        auto label(this Self &&self, std::string_view v) -> Self&& {
            self.m_typed->set_label(std::string{v});
            return std::forward<Self>(self);
        }

        // ── helper_text ────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, FieldNode>
        auto helper_text(this Self &&self, std::string_view v) -> Self&& {
            self.m_typed->set_helper_text(std::string{v});
            return std::forward<Self>(self);
        }

        // ── error_text ─────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, FieldNode>
        auto error_text(this Self &&self, std::string_view v) -> Self&& {
            self.m_typed->set_error_text(std::string{v});
            return std::forward<Self>(self);
        }

        // ── required ───────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, FieldNode>
        auto required(this Self &&self, bool v) -> Self&& {
            self.m_typed->set_required(v);
            return std::forward<Self>(self);
        }

        // ── invalid ────────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, FieldNode>
        auto invalid(this Self &&self, bool v) -> Self&& {
            self.m_typed->set_invalid(v);
            return std::forward<Self>(self);
        }

        // ── disabled ───────────────────────────────────────────────────────

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, FieldNode>
        auto disabled(this Self &&self, bool v) -> Self&& {
            self.m_typed->set_disabled(v);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, FieldNode>
        auto color_variant(this Self &&self, nandina::theme::ColorVariant v) -> Self&& {
            self.m_typed->set_color_variant(v);
            return std::forward<Self>(self);
        }

        // ── control ────────────────────────────────────────────────────────
        //
        // 接收一个 NodeLike 作为输入控件，提取其底层 widget 所有权并注入 Field。
        //
        // 用法：
        //   field().control(text_field().placeholder("Email"))

        template<typename Self, typename NodeLike>
            requires std::derived_from<std::remove_cvref_t<Self>, FieldNode>
        auto control(this Self &&self, NodeLike &&child) -> Self&& {
            auto node = Node(std::forward<NodeLike>(child));
            self.m_typed->set_control(std::move(node).take_widget());
            return std::forward<Self>(self);
        }
    };

    // ═══════════════════════════════════════════════════════════
    // SidebarMenuButtonNode — 侧边栏菜单项链式配置节点
    // ═══════════════════════════════════════════════════════════
    //
    // 用法：
    //   group->add_child(
    //       sidebar_menu_button("Introduction")
    //           .active(true)
    //           .font([](auto& f){ f.size(11.0f); })
    //           .on_click([&]{ router->navigate("intro"); }));
    //
    // 隐式转换 operator NanWidget::Ptr() && 支持直接传入 SidebarGroup::add_child()。

    class SidebarMenuButtonNode : public WidgetNode<nandina::widgets::SidebarMenuButton> {
    public:
        explicit SidebarMenuButtonNode(nandina::widgets::SidebarMenuButton::Ptr widget)
            : WidgetNode<nandina::widgets::SidebarMenuButton>(std::move(widget)) {
        }

        // ── 文本（统一风格，转发 Button::set_text）────────────────────
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, SidebarMenuButtonNode>
        auto text(this Self &&self, std::string_view t) -> Self&& {
            self.m_typed->text(t);
            return std::forward<Self>(self);
        }

        // ── 激活状态（统一 getter/setter 风格）────────────────────────
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, SidebarMenuButtonNode>
        auto active(this Self &&self, bool value) -> Self&& {
            self.m_typed->active(value);
            return std::forward<Self>(self);
        }

        // ── accent 指示条颜色─────────────────────────────────────────
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, SidebarMenuButtonNode>
        auto accent_color(this Self &&self, const nandina::NanColor &color) -> Self&& {
            self.m_typed->accent_color(color);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, SidebarMenuButtonNode>
        auto color_variant(this Self &&self, nandina::theme::ColorVariant value) -> Self&& {
            self.m_typed->color_variant(value);
            return std::forward<Self>(self);
        }

        // ── active_changed 回调───────────────────────────────────────
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, SidebarMenuButtonNode>
        auto on_active_changed(this Self &&self, std::function<void(bool)> cb) -> Self&& {
            self.m_typed->on_active_changed(std::move(cb));
            return std::forward<Self>(self);
        }

        // ── 点击回调─────────────────────────────────────────────────
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, SidebarMenuButtonNode>
        auto on_click(this Self &&self, std::function<void()> handler) -> Self&& {
            self.m_typed->on_click(std::move(handler));
            return std::forward<Self>(self);
        }

        // ── 字体（全量替换）──────────────────────────────────────────
        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, SidebarMenuButtonNode>
        auto font(this Self &&self, nandina::text::NanFont font_config) -> Self&& {
            self.m_typed->font(std::move(font_config));
            return std::forward<Self>(self);
        }

        // ── 字体（局部更新）──────────────────────────────────────────
        template<typename Self, typename Fn>
            requires std::derived_from<std::remove_cvref_t<Self>, SidebarMenuButtonNode>
                     && std::invocable<Fn, nandina::text::NanFont &>
                     && (!std::convertible_to<std::remove_cvref_t<Fn>, nandina::text::NanFont>)
        auto font(this Self &&self, Fn &&fn) -> Self&& {
            auto updated = self.m_typed->font();
            std::invoke(std::forward<Fn>(fn), updated);
            self.m_typed->font(std::move(updated));
            return std::forward<Self>(self);
        }

        // ── 隐式转换 — 支持直接传入 SidebarGroup::add_child(NanWidget::Ptr) ──
        operator nandina::runtime::NanWidget::Ptr() && {
            return static_cast<Node &&>(*this).take_widget();
        }
    };

    class ProgressBarNode : public WidgetNode<nandina::widgets::ProgressBar> {
    public:
        explicit ProgressBarNode(nandina::widgets::ProgressBar::Ptr widget)
            : WidgetNode<nandina::widgets::ProgressBar>(std::move(widget)) {
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ProgressBarNode>
        auto progress(this Self &&self, float value) -> Self&& {
            self.m_typed->set_progress(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ProgressBarNode>
        auto color_variant(this Self &&self, nandina::theme::ColorVariant value) -> Self&& {
            self.m_typed->color_variant(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ProgressBarNode>
        auto bar_color(this Self &&self, const nandina::NanColor& value) -> Self&& {
            self.m_typed->set_bar_color(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ProgressBarNode>
        auto track_color(this Self &&self, const nandina::NanColor& value) -> Self&& {
            self.m_typed->set_track_color(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ProgressBarNode>
        auto bar_height(this Self &&self, float value) -> Self&& {
            self.m_typed->set_bar_height(value);
            return std::forward<Self>(self);
        }

        template<typename Self>
            requires std::derived_from<std::remove_cvref_t<Self>, ProgressBarNode>
        auto corner_radius(this Self &&self, float value) -> Self&& {
            self.m_typed->set_corner_radius(value);
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
    template<typename... Nodes>
        requires (std::derived_from<std::decay_t<Nodes>, Node> && ...)
    inline auto children(Nodes &&... nodes) -> Children {
        Children result;
        (result.append(std::move(nodes)), ...);
        return result;
    }

    // ── NodeLike concept ──────────────────────────────────────────────────
    /** @brief 可接受为 Node 的类型（含派生类） */
    template<typename T>
    concept NodeLike = std::derived_from<std::decay_t<T>, Node>;

    // ── 工厂检测辅助 ────────────────────────────────────────────────────
    namespace detail {
        /** @brief 统一工厂调度：自动检测 Create() 或 create() */
        template<typename W, typename... Args>
        auto make_widget(Args &&... args) -> runtime::NanWidget::Ptr {
            if constexpr (requires { { W::Create(std::forward<Args>(args)...) }; }) {
                return W::Create(std::forward<Args>(args)...);
            }
            else if constexpr (requires { { W::create(std::forward<Args>(args)...) }; }) {
                return W::create(std::forward<Args>(args)...);
            }
            else {
                static_assert(sizeof(W) == 0,
                              "Widget type must have a static factory: Create(...) or create(...)");
            }
        }
    }

    // ── NodeFactory — 通用节点工厂 ──────────────────────────────────────
    /** @brief 通用节点工厂。第三方 Widget 可直接使用，无需修改框架文件。 */
    class NodeFactory {
    public:
        [[nodiscard]] static auto from_widget(runtime::NanWidget::Ptr widget) -> Node {
            return Node{std::move(widget)};
        }

        template<typename W> requires requires { { detail::make_widget<W>() }; }
        [[nodiscard]] static auto container(Children children) -> Node {
            Node result{detail::make_widget<W>()};
            _populate_children<W>(result, std::move(children));
            return result;
        }

        template<typename W> requires requires { { detail::make_widget<W>() }; }
        [[nodiscard]] static auto positioned_container(Children children) -> Node {
            Node result{detail::make_widget<W>()};
            for (auto &child: std::move(children).take()) {
                auto props = std::move(child).take_positioned_props();
                if (auto cw = std::move(child).take_widget())
                    static_cast<W *>(result.widget_ptr())->add_positioned_child(std::move(cw), std::move(props));
                result.absorb(std::move(child));
            }
            return result;
        }

        template<typename W, typename N> requires NodeLike<N> && requires { { detail::make_widget<W>() }; }
        [[nodiscard]] static auto wrapper(N &&child_node) -> Node {
            Node result{detail::make_widget<W>()};
            Node moved = std::move(child_node);
            if (auto cw = std::move(moved).take_widget()) _set_child<W>(*result.widget_ptr(), std::move(cw));
            result.absorb(std::move(moved));
            return result;
        }

        template<typename W, typename N, typename... Args>
            requires NodeLike<N> && requires(Args... args) { { detail::make_widget<W>(std::forward<Args>(args)...) }; }
        [[nodiscard]] static auto wrapper_with(N &&child_node, Args &&... args) -> Node {
            Node result{detail::make_widget<W>(std::forward<Args>(args)...)};
            Node moved = std::move(child_node);
            if (auto cw = std::move(moved).take_widget()) _set_child<W>(*result.widget_ptr(), std::move(cw));
            result.absorb(std::move(moved));
            return result;
        }

    private:
        template<typename W>
        static auto _populate_children(Node &result, Children children) -> void {
            for (auto &child: std::move(children).take()) {
                if (auto cw = std::move(child).take_widget()) {
                    if constexpr (requires(W &w, runtime::NanWidget::Ptr c) { w.add(std::move(c)); })
                        static_cast<W *>(result.widget_ptr())->add(std::move(cw));
                    else result.widget_ptr()->add_child(std::move(cw));
                }
                result.absorb(std::move(child));
            }
        }

        template<typename W>
        static auto _set_child(runtime::NanWidget &w, runtime::NanWidget::Ptr cw) -> void {
            if constexpr (requires(W &w2, runtime::NanWidget::Ptr c) { w2.child(std::move(c)); })
                static_cast<W &>(w).child(std::move(cw));
            else w.add_child(std::move(cw));
        }
    };

    class MountedNodeComponent final : public NanComponent {
    public:
        explicit MountedNodeComponent(Node root)
            : m_root_node(std::move(root)) {
            auto widget = std::move(m_root_node).take_widget();
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

        auto measure(const geometry::NanConstraints &constraints) -> void override {
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

        void draw(tvg::SwCanvas &canvas) override {
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

            auto log = nandina::log::get("app.mounted");
            const auto start = detail::SteadyClock::now();
            m_root_widget->measure(geometry::NanConstraints::tight(width(), height()));
            const auto measure_done = detail::SteadyClock::now();
            m_root_widget->layout();
            const auto layout_done = detail::SteadyClock::now();
            m_pending_root_layout = false;

            // 递归 flush 子树中所有嵌套的 MountedNodeComponent，
            // 避免在后续 draw() 遍历中再次触发重复的 measure+layout
            flush_nested_mounted(*m_root_widget);

            const auto measure_ms = detail::elapsed_ms(start, measure_done);
            const auto layout_ms = detail::elapsed_ms(measure_done, layout_done);
            const auto total_ms = detail::elapsed_ms(start, layout_done);

            if (total_ms >= detail::k_slow_layout_threshold_ms) {
                auto [fast_cnt, slow_cnt] = nandina::widgets::Label::measure_diag();

                auto surf_n = nandina::widgets::Surface::s_measure_count;
                nandina::widgets::Surface::s_measure_count = 0;

                auto lc_n = nandina::layout::LayoutContainer::s_measure_count;
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
        static auto flush_nested_mounted(runtime::NanWidget &root) -> void {
            root.for_each_child([](runtime::NanWidget &child) {
                if (auto *nested = dynamic_cast<MountedNodeComponent *>(&child)) {
                    if (nested->m_pending_root_layout) {
                        nested->flush_root_layout("draw-pending");
                    }
                }
                MountedNodeComponent::flush_nested_mounted(child);
            });
        }

        Node m_root_node;
        runtime::NanWidget *m_root_widget{nullptr};
        bool m_pending_root_layout{false};
    };

    // ═══════════════════════════════════════════════════════════════════════
    // 组件工厂函数（薄包装 → NodeFactory）
    // ═══════════════════════════════════════════════════════════════════════

    /** @brief 将 authoring 节点树挂载为可运行的 NanComponent */
    [[nodiscard]] inline auto mount(Node root) -> NanComponent::Ptr {
        if (root.empty()) return nullptr;
        return std::make_unique<MountedNodeComponent>(std::move(root));
    }

    /** @brief 将任意 NanWidget 子类包装为 Node */
    [[nodiscard]] inline auto adopt(runtime::NanWidget::Ptr widget) -> Node {
        return NodeFactory::from_widget(std::move(widget));
    }

    // ── 叶子组件 ──────────────────────────────────────────────────────

    /** @brief 创建 Label 节点 */
    [[nodiscard]] inline auto label(std::string_view text) -> LabelNode {
        auto w = nandina::widgets::Label::create();
        if (!text.empty()) w->set_text(text);
        return LabelNode{std::move(w)};
    }

    [[nodiscard]] inline auto tag(std::string_view text) -> TagNode {
        auto w = nandina::widgets::Tag::create();
        if (!text.empty()) w->text(text);
        return TagNode{std::move(w)};
    }

    /** @brief 创建 Button 节点 */
    [[nodiscard]] inline auto button(std::string_view text) -> ButtonNode {
        auto w = nandina::widgets::Button::create();
        if (!text.empty()) w->text(text);
        return ButtonNode{std::move(w)};
    }

    /** @brief 创建 TextField 节点 */
    [[nodiscard]] inline auto text_field() -> TextFieldNode {
        return TextFieldNode{nandina::widgets::TextField::create()};
    }

    /** @brief 创建 Field 节点 */
    [[nodiscard]] inline auto field() -> FieldNode {
        return FieldNode{nandina::widgets::Field::create()};
    }

    [[nodiscard]] inline auto progress_bar() -> ProgressBarNode {
        return ProgressBarNode(nandina::widgets::ProgressBar::create());
    }

    // ── 容器组件 ──────────────────────────────────────────────────────

    /** @brief 水平排列容器 */
    [[nodiscard]] inline auto row(Children children = {}) -> Node {
        return NodeFactory::container<nandina::layout::Row>(std::move(children));
    }

    /** @brief 垂直排列容器 */
    [[nodiscard]] inline auto column(Children children = {}) -> Node {
        return NodeFactory::container<nandina::layout::Column>(std::move(children));
    }

    /** @brief Z 轴堆叠容器 */
    [[nodiscard]] inline auto stack(Children children = {}) -> Node {
        return NodeFactory::container<nandina::layout::Stack>(std::move(children));
    }

    /** @brief 卡片容器 */
    [[nodiscard]] inline auto card(Children children) -> Node {
        return NodeFactory::container<nandina::widgets::Card>(std::move(children));
    }

    /** @brief 面板容器 */
    [[nodiscard]] inline auto panel(Children children) -> Node {
        return NodeFactory::container<nandina::widgets::Panel>(std::move(children));
    }

    /** @brief 自由定位容器 */
    [[nodiscard]] inline auto positioned(Children children) -> Node {
        return NodeFactory::positioned_container<nandina::layout::Positioned>(std::move(children));
    }

    // ── 基础节点 ──────────────────────────────────────────────────────

    /** @brief 弹性空白占位 */
    [[nodiscard]] inline auto spacer(const int flex = 1) -> Node {
        return NodeFactory::from_widget(nandina::layout::Spacer::Create(flex));
    }

    // ── 单子节点包装 ──────────────────────────────────────────────────

    /** @brief 水平+垂直居中 */
    [[nodiscard]] inline auto center(NodeLike auto &&child) -> Node {
        return NodeFactory::wrapper<nandina::layout::Center>(std::forward<decltype(child)>(child));
    }

    /** @brief 内边距包装 */
    [[nodiscard]] inline auto padding(NodeLike auto &&child) -> Node {
        return NodeFactory::wrapper<nandina::layout::Padding>(std::forward<decltype(child)>(child));
    }

    /** @brief 固定尺寸包装 */
    [[nodiscard]] inline auto sized_box(NodeLike auto &&child) -> Node {
        return NodeFactory::wrapper<nandina::layout::SizedBox>(std::forward<decltype(child)>(child));
    }

    /** @brief 弹性填充包装 */
    [[nodiscard]] inline auto expanded(NodeLike auto &&child) -> Node {
        return NodeFactory::wrapper_with<nandina::layout::Expanded>(std::forward<decltype(child)>(child), 1);
    }

    /** @brief 弹性填充包装（指定 flex 系数） */
    [[nodiscard]] inline auto expanded(NodeLike auto &&child, const int flex) -> Node {
        return NodeFactory::wrapper_with<nandina::layout::Expanded>(std::forward<decltype(child)>(child), flex);
    }

    // ── Sidebar 组件 ──────────────────────────────────────────────────

    /** @brief 创建侧边栏菜单按钮（返回可链式配置的 SidebarMenuButtonNode）
     *
     *  链式用法：
     *    group->add_child(sidebar_menu_button("Introduction").active(true).font(NanFont{}.size(11)));
     *    group->add_child(sidebar_menu_button("Settings").on_click([&]{ ... }));
     */
    [[nodiscard]] inline auto sidebar_menu_button(std::string_view text) -> SidebarMenuButtonNode {
        auto w = nandina::widgets::SidebarMenuButton::create();
        if (!text.empty()) {
            w->text(text);
        }
        return SidebarMenuButtonNode{std::move(w)};
    }

    /** @brief 创建侧边栏分组容器 */
    [[nodiscard]] inline auto sidebar_group(std::string_view label_text) -> Node {
        auto g = nandina::widgets::SidebarGroup::create();
        if (!label_text.empty()) g->label(label_text);
        return adopt(std::move(g));
    }

    struct AppConfig {
        std::string title = "NandinaUI";
        int width = 1280;
        int height = 720;
        bool resizable = true;
        bool high_dpi = true;
        /// 默认背景色（NanColor::from(NanRgb{0,0,0,0}) 表示透明，不绘制背景）
        NanColor bg_color = NanColor::from(NanRgb{21, 24, 32});
    };

    class NanAppWindow {
    public:
        explicit NanAppWindow(AppConfig config) : m_config(std::move(config)) {
        }

        virtual ~NanAppWindow() = default;

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
            (void)delta_seconds;
            apply_throttled_resize();
        }

        virtual void on_draw(tvg::SwCanvas &canvas) {
            // ── 背景层（Surface 组件，统一使用 NanColor） ──
            if (m_background) {
                m_background->draw(canvas);
            }

            if (m_root_component) {
                auto log = nandina::log::get("app.window");
                const auto start = detail::SteadyClock::now();
                consume_root_reflow();
                const auto layout_done = detail::SteadyClock::now();
                sync_focused_text_input_area();
                m_root_component->draw(canvas);
                const auto draw_done = detail::SteadyClock::now();
                m_root_component->clear_dirty_recursive();

                const auto layout_ms = detail::elapsed_ms(start, layout_done);
                const auto draw_ms = detail::elapsed_ms(layout_done, draw_done);
                const auto total_ms = detail::elapsed_ms(start, draw_done);

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

            auto *hit = m_root_component->hit_test(x, y);
            return hit && hit->is_interactive() ? hit : nullptr;
        }

    private:
        [[nodiscard]] auto needs_redraw() const noexcept -> bool {
            return m_root_component && m_root_component->dirty();
        }

        static auto pointer_move_from_button(const runtime::PointerButtonEvent &event) noexcept
            -> runtime::PointerMoveEvent {
            return runtime::PointerMoveEvent{
                .x = event.x,
                .y = event.y,
                .delta_x = 0.0,
                .delta_y = 0.0,
            };
        }

        auto sync_hover_target(runtime::NanWidget *next, const runtime::PointerMoveEvent &event) -> bool {
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

        auto clear_hover_target(const runtime::PointerMoveEvent &event) -> void {
            sync_hover_target(nullptr, event);
        }

        auto sync_focus_target(runtime::NanWidget *next) -> bool {
            if (m_focused_widget == next) {
                return false;
            }

            if (m_focused_widget) {
                m_focused_widget->dispatch_event(runtime::FocusEvent{.got_focus = false});
            }

            m_focused_widget = nullptr;
            if (next && next->dispatch_event(runtime::FocusEvent{.got_focus = true})) {
                m_focused_widget = next;
            }

            sync_focused_text_input_area();

            return true;
        }

        auto clear_focus_target() -> void {
            sync_focus_target(nullptr);
        }

        auto sync_focused_text_input_area() -> void {
            if (!m_active_runtime_window) {
                return;
            }

            if (m_focused_widget) {
                if (const auto area = m_focused_widget->text_input_area(); area.has_value()) {
                    m_active_runtime_window->set_text_input_area(area->rect, area->cursor);
                    return;
                }
            }

            m_active_runtime_window->clear_text_input_area();
        }

        auto on_window_focus_lost() -> void {
            clear_hover_target(runtime::PointerMoveEvent{});
            clear_focus_target();
            m_pointer_capture_widget = nullptr;
            sync_focused_text_input_area();
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

            auto log = nandina::log::get("app.window");
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

            sync_focused_text_input_area();

            const auto measure_ms = detail::elapsed_ms(start, measure_done);
            const auto layout_ms = detail::elapsed_ms(measure_done, layout_done);
            const auto total_ms = detail::elapsed_ms(start, layout_done);

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

            const auto now = detail::SteadyClock::now();
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_last_resize_apply_time).count();
            if (elapsed_ms < static_cast<long long>(k_resize_throttle_ms))
                return;

            m_last_resize_apply_time = now;
            m_pending_resize = false;
            request_root_reflow(true);
        }

        auto on_resize_pending() -> void {
            m_pending_resize = true;
        }

        AppConfig m_config;
        runtime::NanWindow *m_active_runtime_window{nullptr};
        NanComponent::Ptr m_root_component{nullptr};
        runtime::NanWidget *m_hovered_widget{nullptr};
        runtime::NanWidget *m_focused_widget{nullptr};
        runtime::NanWidget *m_pointer_capture_widget{nullptr};
        bool m_pending_root_bounds_sync{false};

        // ── 背景层（Surface 组件，非 widget 树成员） ──────
        widgets::Surface::Ptr m_background{nullptr};

        // ── resize 节流状态 ─────────────────────────────────
        bool m_pending_resize{false};
        detail::SteadyClock::time_point m_last_resize_apply_time{};
        static constexpr std::uint64_t k_resize_throttle_ms = 16; // ~60fps 间隔

        class BridgeWindow final : public runtime::NanWindow {
        public:
            BridgeWindow(NanAppWindow &owner,
                         const runtime::NanWindow::Config &config) : runtime::NanWindow(config),
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

            void on_draw(tvg::SwCanvas &canvas) override {
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

            void on_pointer_move(const runtime::PointerMoveEvent &event) override {
                auto *hit = m_owner.hit_test_root_component(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                const bool changed = m_owner.sync_hover_target(hit, event);
                if (m_owner.m_pointer_capture_widget) {
                    m_owner.m_pointer_capture_widget->dispatch_event(event);
                } else if (hit && !changed) {
                    hit->dispatch_event(event);
                }
            }

            void on_pointer_enter(const runtime::PointerMoveEvent &event) override {
                auto *hit = m_owner.hit_test_root_component(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                m_owner.sync_hover_target(hit, event);
            }

            void on_pointer_leave(const runtime::PointerMoveEvent &event) override {
                m_owner.clear_hover_target(event);
                m_owner.m_pointer_capture_widget = nullptr;
            }

            void on_pointer_down(const runtime::PointerButtonEvent &event) override {
                auto *hit = m_owner.hit_test_root_component(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                m_owner.sync_hover_target(hit, pointer_move_from_button(event));
                if (hit) {
                    m_owner.sync_focus_target(hit);
                } else {
                    m_owner.clear_focus_target();
                }
                if (event.button == nandina::types::PointerButton::Left) {
                    m_owner.m_pointer_capture_widget = hit;
                }
                if (hit) {
                    hit->dispatch_event(event, runtime::EventType::PointerDown);
                }
            }

            void on_pointer_up(const runtime::PointerButtonEvent &event) override {
                auto *hit = m_owner.hit_test_root_component(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                m_owner.sync_hover_target(hit, pointer_move_from_button(event));
                auto *target = m_owner.m_pointer_capture_widget ? m_owner.m_pointer_capture_widget : hit;
                if (target) {
                    target->dispatch_event(event, runtime::EventType::PointerUp);
                }
                if (event.button == nandina::types::PointerButton::Left) {
                    m_owner.m_pointer_capture_widget = nullptr;
                }
            }

            void on_key_down(const runtime::KeyEvent &event) override {
                if (m_owner.m_focused_widget) {
                    m_owner.m_focused_widget->dispatch_event(event, runtime::EventType::KeyDown);
                }
            }

            void on_key_up(const runtime::KeyEvent &event) override {
                if (m_owner.m_focused_widget) {
                    m_owner.m_focused_widget->dispatch_event(event, runtime::EventType::KeyUp);
                }
            }

            void on_text_input(std::string_view text) override {
                if (m_owner.m_focused_widget && !text.empty()) {
                    m_owner.m_focused_widget->dispatch_event(runtime::TextInputEvent{.text = std::string{text}});
                }
            }

            void on_text_editing(std::string_view text, std::int32_t start, std::int32_t length) override {
                if (m_owner.m_focused_widget) {
                    m_owner.m_focused_widget->dispatch_event(runtime::TextEditingEvent{
                        .text = std::string{text},
                        .start = start,
                        .length = length,
                    });
                }
            }

        private:
            NanAppWindow &m_owner;
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
