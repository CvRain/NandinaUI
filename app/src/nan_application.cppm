module;

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>
#include <typeinfo>
#include <utility>
#include <vector>

export module nandina.app.application;

import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.log;
import nandina.foundation.color;
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

    [[nodiscard]] inline auto adopt(runtime::NanWidget::Ptr widget) -> Node;

    [[nodiscard]] inline auto label(std::string_view text = {}) -> Node;

    [[nodiscard]] inline auto button(std::string_view text = {}) -> Node;

    [[nodiscard]] inline auto card(Children children) -> Node;

    [[nodiscard]] inline auto panel(Children children) -> Node;

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

        template <typename T>
        auto bind(Ref<T>& ref) && -> Node {
            auto* widget = m_widget.get();
            m_ref_binders.push_back([&ref, widget]() {
                ref.bind(widget);
            });
            m_ref_resetters.push_back([&ref]() {
                ref.reset();
            });
            return std::move(*this);
        }

        auto key(std::string_view value) && -> Node {
            m_key.assign(value);
            return std::move(*this);
        }

        auto title(std::string_view value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Card*>(unwrapped())) {
                widget->set_title(std::string{value});
            } else if (auto* widget = dynamic_cast<nandina::widgets::Panel*>(unwrapped())) {
                widget->set_title(value);
            }
            return std::move(*this);
        }

        auto bg_color(const nandina::NanColor& value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Surface*>(unwrapped())) {
                widget->set_bg_color(value);
            }
            return std::move(*this);
        }

        auto corner_radius(const float value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Surface*>(unwrapped())) {
                widget->set_corner_radius(value);
            }
            return std::move(*this);
        }

        auto text(std::string_view value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(unwrapped())) {
                widget->set_text(value);
            } else if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->set_text(value);
            }
            return std::move(*this);
        }

        auto font_size(const float value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(unwrapped())) {
                widget->set_font_size(value);
            } else if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->font_size(value);
            }
            return std::move(*this);
        }

        auto color(const nandina::NanColor& value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(unwrapped())) {
                widget->set_color(value);
            }
            return std::move(*this);
        }

        // ── 字体链式配置 ───────────────────────────────────

        /// 设置字体颜色（Label / Button）
        auto font_color(const nandina::NanColor& value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(unwrapped())) {
                widget->set_color(value);
            } else if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->font_color(value);
            }
            return std::move(*this);
        }

        /// 设置字体粗细（Label / Button）
        auto font_weight(nandina::text::NanFontWeight value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(unwrapped())) {
                widget->set_font_weight(value);
            } else if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->font_weight(value);
            }
            return std::move(*this);
        }

        /// 设置字体族名（Label / Button）
        auto font_family(std::string_view value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(unwrapped())) {
                widget->set_font_family(std::string{value});
            } else if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->font_family(std::string{value});
            }
            return std::move(*this);
        }

        /// 通过 NanFont 一次性应用字体配置（Label / Button）
        auto font(nandina::text::NanFont font_config) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(unwrapped())) {
                widget->set_font(std::move(font_config));
            } else if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->set_font(std::move(font_config));
            }
            return std::move(*this);
        }

        /// 通过 button_style 应用预设样式（Button）
        auto button_style(const theme::NanButtonStyle& style) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->variant(style.variant);
                widget->size(style.size);

                auto font = nandina::text::NanFont{}
                    .size(style.font_size)
                    .weight(style.font_weight)
                    .color(style.font_color)
                    .overflow(style.overflow)
                    .single_line(style.single_line);
                widget->set_font(std::move(font));
            }
            return std::move(*this);
        }

        auto align(const nandina::widgets::TextAlign value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(unwrapped())) {
                widget->set_align(value);
            }
            return std::move(*this);
        }

        auto vertical_align(const nandina::widgets::TextVerticalAlign value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(unwrapped())) {
                widget->set_vertical_align(value);
            }
            return std::move(*this);
        }

        /// 设置 Button 变体（default/secondary/outline/ghost/destructive/link）
        auto button_variant(nandina::widgets::ButtonVariant value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->variant(value);
            }
            return std::move(*this);
        }

        /// 设置 Button 尺寸预设（xs/sm/md/lg/icon）
        auto button_size(nandina::widgets::ButtonSize value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->size(value);
            }
            return std::move(*this);
        }

        auto on_click(std::function<void()> handler) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Button*>(unwrapped())) {
                widget->on_click(std::move(handler));
            }
            return std::move(*this);
        }

        auto padding(const float value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::layout::Padding*>(m_widget.get())) {
                widget->padding(value);
            } else if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(m_widget.get())) {
                widget->padding(value);
            }
            return std::move(*this);
        }

        auto padding(const float horizontal, const float vertical) && -> Node {
            if (auto* widget = dynamic_cast<nandina::layout::Padding*>(m_widget.get())) {
                widget->padding(horizontal, vertical);
            } else if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(m_widget.get())) {
                widget->padding(horizontal, vertical);
            }
            return std::move(*this);
        }

        auto padding(const float left, const float top, const float right, const float bottom) && -> Node {
            if (auto* widget = dynamic_cast<nandina::layout::Padding*>(m_widget.get())) {
                widget->padding(left, top, right, bottom);
            } else if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(m_widget.get())) {
                widget->padding(left, top, right, bottom);
            }
            return std::move(*this);
        }

        auto padding_top(const float value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(m_widget.get())) {
                widget->padding_top(value);
            }
            return std::move(*this);
        }

        auto gap(const float value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(m_widget.get())) {
                widget->gap(value);
            }
            return std::move(*this);
        }

        auto align_items(const nandina::layout::LayoutAlignment value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(m_widget.get())) {
                widget->align_items(value);
            }
            return std::move(*this);
        }

        auto justify_content(const nandina::layout::LayoutAlignment value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::layout::LayoutContainer*>(m_widget.get())) {
                widget->justify_content(value);
            }
            return std::move(*this);
        }

        auto width(const float value) && -> Node {
            if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(m_widget.get())) {
                sb->width(value);
            } else {
                // 自动包裹到 SizedBox：label("X").width(200) 等价于 sized_box(label("X")).width(200)
                auto wrapper = nandina::layout::SizedBox::Create();
                wrapper->width(value);
                wrapper->add_child(std::move(m_widget));
                m_widget = std::move(wrapper);
            }
            return std::move(*this);
        }

        auto height(const float value) && -> Node {
            if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(m_widget.get())) {
                sb->height(value);
            } else {
                auto wrapper = nandina::layout::SizedBox::Create();
                wrapper->height(value);
                wrapper->add_child(std::move(m_widget));
                m_widget = std::move(wrapper);
            }
            return std::move(*this);
        }

        auto size(const geometry::NanSize& value) && -> Node {
            if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(m_widget.get())) {
                sb->size(value);
            } else {
                auto wrapper = nandina::layout::SizedBox::Create();
                wrapper->size(value);
                wrapper->add_child(std::move(m_widget));
                m_widget = std::move(wrapper);
            }
            return std::move(*this);
        }

    private:
        using RefBinder   = std::move_only_function<void()>;
        using RefResetter = std::move_only_function<void()>;

        /// 穿透 SizedBox 包裹层找到真实组件
        [[nodiscard]] auto unwrapped() -> runtime::NanWidget* {
            if (!m_widget) return nullptr;
            if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(m_widget.get())) {
                runtime::NanWidget* result = nullptr;
                sb->for_each_child([&](runtime::NanWidget& c) {
                    if (!result) result = &c;
                });
                return result ? result : m_widget.get();
            }
            return m_widget.get();
        }

        friend class MountedNodeComponent;

        friend auto mount(Node root) -> NanComponent::Ptr;

        friend class NanAppWindow;
        friend class Children;

        friend auto row(Children children) -> Node;

        friend auto column(Children children) -> Node;

        friend auto stack(Children children) -> Node;

        friend auto spacer(int flex) -> Node;

        friend auto expanded(Node child, int flex) -> Node;

        friend auto padding(Node child) -> Node;

        friend auto center(Node child) -> Node;

        friend auto sized_box(Node child) -> Node;

        friend auto adopt(runtime::NanWidget::Ptr widget) -> Node;

        friend auto label(std::string_view text) -> Node;

        friend auto button(std::string_view text) -> Node;

        friend auto card(Children children) -> Node;

        friend auto panel(Children children) -> Node;

        explicit Node(std::unique_ptr<runtime::NanWidget> widget)
            : m_widget(std::move(widget)) {
        }

        [[nodiscard]] auto take_widget() && -> std::unique_ptr<runtime::NanWidget> {
            return std::move(m_widget);
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
        requires (std::same_as<std::decay_t<Nodes>, Node> && ...)
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
                    "Slow mounted flush: total={:.2f}ms measure={:.2f}ms layout={:.2f}ms cause={} root_type={} size={:.0f}x{:.0f} label_fast={} label_slow={} surf_n={} lc_n={}",
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

    [[nodiscard]] inline auto label(std::string_view text) -> Node {
        auto widget = nandina::widgets::Label::create();
        if (!text.empty()) {
            widget->set_text(text);
        }
        return adopt(std::move(widget));
    }

    [[nodiscard]] inline auto button(std::string_view text) -> Node {
        auto widget = nandina::widgets::Button::create();
        if (!text.empty()) {
            widget->set_text(text);
        }
        return adopt(std::move(widget));
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

    [[nodiscard]] inline auto spacer(const int flex = 1) -> Node {
        return Node{nandina::layout::Spacer::Create(flex)};
    }

    [[nodiscard]] inline auto expanded(Node child, const int flex = 1) -> Node {
        auto widget = nandina::layout::Expanded::Create(flex);
        Node result{std::move(widget)};
        auto child_widget = std::move(child).take_widget();
        if (child_widget) {
            static_cast<nandina::layout::Expanded*>(result.m_widget.get())->child(std::move(child_widget));
        }
        result.absorb(std::move(child));
        return result;
    }

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
            sync_root_component_bounds();
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
                ensure_root_component_layout();
                const auto layout_done = detail::SteadyClock::now();
                m_root_component->draw(canvas);
                const auto draw_done = detail::SteadyClock::now();
                m_root_component->clear_dirty_recursive();

                const auto layout_ms = detail::elapsed_ms(start, layout_done);
                const auto draw_ms   = detail::elapsed_ms(layout_done, draw_done);
                const auto total_ms  = detail::elapsed_ms(start, draw_done);

                if (total_ms >= detail::k_slow_layout_threshold_ms) {
                    log.warn(
                        "Slow app on_draw: total={:.2f}ms root_layout={:.2f}ms root_draw={:.2f}ms root_type={} size={}x{}",
                        total_ms,
                        layout_ms,
                        draw_ms,
                        typeid(*m_root_component).name(),
                        width(),
                        height());
                }
            }
        }

    private:
        [[nodiscard]] auto needs_redraw() const noexcept -> bool {
            return m_root_component && m_root_component->dirty();
        }

        [[nodiscard]] auto resolve_interactive_hit(const float x, const float y) -> runtime::NanWidget* {
            if (!m_root_component) {
                return nullptr;
            }

            auto* hit = m_root_component->hit_test(x, y);
            return hit && hit->is_interactive() ? hit : nullptr;
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

        auto ensure_root_component_layout() -> void {
            if (!m_root_component) {
                return;
            }

            if (!m_root_component->is_layout_dirty()) {
                return;
            }

            auto log          = nandina::log::get("app.window");
            const float width = static_cast<float>(m_active_runtime_window
                                                       ? m_active_runtime_window->width()
                                                       : m_config.width);
            const float height = static_cast<float>(m_active_runtime_window
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

        auto sync_root_component_bounds(const bool immediate = true) -> void {
            // ── 同步背景层 bounds ──────────────────────────
            sync_background_bounds();

            if (!m_root_component) {
                return;
            }

            const float width = static_cast<float>(m_active_runtime_window
                                                       ? m_active_runtime_window->width()
                                                       : m_config.width);
            const float height = static_cast<float>(m_active_runtime_window
                                                        ? m_active_runtime_window->height()
                                                        : m_config.height);

            if (immediate) {
                m_root_component->set_bounds(0.0f, 0.0f, width, height);
                return;
            }

            m_root_component->runtime::NanWidget::set_bounds(0.0f, 0.0f, width, height);
            m_root_component->mark_layout_dirty();
        }

        auto sync_background_bounds() -> void {
            if (!m_background)
                return;
            const float w = static_cast<float>(m_active_runtime_window
                                                   ? m_active_runtime_window->width()
                                                   : m_config.width);
            const float h = static_cast<float>(m_active_runtime_window
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
            sync_root_component_bounds(false);
        }

        auto on_resize_pending() -> void {
            m_pending_resize = true;
        }

        AppConfig m_config;
        runtime::NanWindow* m_active_runtime_window{nullptr};
        NanComponent::Ptr m_root_component{nullptr};
        runtime::NanWidget* m_hovered_widget{nullptr};

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
                auto* hit = m_owner.resolve_interactive_hit(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                const bool changed = m_owner.sync_hover_target(hit, event);
                if (hit && !changed) {
                    hit->dispatch_event(event);
                }
            }

            void on_pointer_enter(const runtime::PointerMoveEvent& event) override {
                auto* hit = m_owner.resolve_interactive_hit(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                m_owner.sync_hover_target(hit, event);
            }

            void on_pointer_leave(const runtime::PointerMoveEvent& event) override {
                m_owner.clear_hover_target(event);
            }

            void on_pointer_down(const runtime::PointerButtonEvent& event) override {
                auto* hit = m_owner.resolve_interactive_hit(
                    static_cast<float>(event.x), static_cast<float>(event.y));
                m_owner.sync_hover_target(hit, pointer_move_from_button(event));
                if (hit) {
                    hit->dispatch_event(event, runtime::EventType::PointerDown);
                }
            }

            void on_pointer_up(const runtime::PointerButtonEvent& event) override {
                auto* hit = m_owner.resolve_interactive_hit(
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

        sync_root_component_bounds();

        logger.info("Window starting: {} ({}x{})", m_config.title, m_config.width, m_config.height);
        window.run();
        logger.info("Window stopped");

        m_active_runtime_window = nullptr;
    }
} // namespace nandina::app
