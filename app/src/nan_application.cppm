module;

#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <thorvg-1/thorvg.h>
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

export namespace nandina::app {
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

        template<typename T>
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
            if (auto* widget = dynamic_cast<nandina::widgets::Card*>(m_widget.get())) {
                widget->set_title(std::string{value});
            } else if (auto* widget = dynamic_cast<nandina::widgets::Panel*>(m_widget.get())) {
                widget->set_title(value);
            }
            return std::move(*this);
        }

        auto bg_color(const nandina::NanColor& value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Surface*>(m_widget.get())) {
                widget->set_bg_color(value);
            }
            return std::move(*this);
        }

        auto corner_radius(const float value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Surface*>(m_widget.get())) {
                widget->set_corner_radius(value);
            }
            return std::move(*this);
        }

        auto text(std::string_view value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(m_widget.get())) {
                widget->set_text(value);
            } else if (auto* widget = dynamic_cast<nandina::widgets::Button*>(m_widget.get())) {
                widget->set_text(value);
            }
            return std::move(*this);
        }

        auto font_size(const float value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(m_widget.get())) {
                widget->set_font_size(value);
            }
            return std::move(*this);
        }

        auto color(const nandina::NanColor& value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(m_widget.get())) {
                widget->set_color(value);
            }
            return std::move(*this);
        }

        auto align(const nandina::widgets::TextAlign value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(m_widget.get())) {
                widget->set_align(value);
            }
            return std::move(*this);
        }

        auto vertical_align(const nandina::widgets::TextVerticalAlign value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Label*>(m_widget.get())) {
                widget->set_vertical_align(value);
            }
            return std::move(*this);
        }

        auto colors(const nandina::widgets::ButtonColors& value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Button*>(m_widget.get())) {
                widget->set_colors(value);
            }
            return std::move(*this);
        }

        auto on_click(std::function<void()> handler) && -> Node {
            if (auto* widget = dynamic_cast<nandina::widgets::Button*>(m_widget.get())) {
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
            if (auto* widget = dynamic_cast<nandina::layout::SizedBox*>(m_widget.get())) {
                widget->width(value);
            }
            return std::move(*this);
        }

        auto height(const float value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::layout::SizedBox*>(m_widget.get())) {
                widget->height(value);
            }
            return std::move(*this);
        }

        auto size(const geometry::NanSize& value) && -> Node {
            if (auto* widget = dynamic_cast<nandina::layout::SizedBox*>(m_widget.get())) {
                widget->size(value);
            }
            return std::move(*this);
        }

    private:
        using RefBinder = std::move_only_function<void()>;
        using RefResetter = std::move_only_function<void()>;

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

    template<typename... Nodes>
    inline auto children(Nodes... nodes) -> Children {
        Children result;
        (result.append(std::move(nodes)), ...);
        return result;
    }

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
                m_root_widget->set_bounds(x, y, w, h);
                flush_root_layout();
            }
            return *this;
        }

        void draw(tvg::SwCanvas& canvas) override {
            flush_root_layout();
            NanComponent::draw(canvas);
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return m_root_widget ? m_root_widget->preferred_size() : geometry::NanSize{};
        }

    private:
        auto flush_root_layout() -> void {
            if (!m_root_widget) {
                return;
            }

            m_root_widget->measure(geometry::NanConstraints::tight(width(), height()));
            m_root_widget->layout();
        }

        Node m_root_node;
        runtime::NanWidget* m_root_widget{nullptr};
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
        int width = 1280;
        int height = 720;
        bool resizable = true;
        bool high_dpi = true;
    };

    class NanAppWindow {
    public:
        explicit NanAppWindow(const AppConfig &config) : m_config(config) {
        }

        virtual ~NanAppWindow() {
        }

        auto set_root_component(NanComponent::Ptr component) -> void {
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

        virtual void on_update(double delta_seconds) {
            (void)delta_seconds;
        }

        virtual void on_draw(tvg::SwCanvas &canvas) {
            if (m_root_component) {
                ensure_root_component_layout();
                m_root_component->draw(canvas);
            }
        }

    private:
        auto ensure_root_component_layout() -> void {
            if (!m_root_component) {
                return;
            }

            if (!m_root_component->is_layout_dirty()) {
                return;
            }

            const float width = static_cast<float>(m_active_runtime_window ? m_active_runtime_window->width() : m_config.width);
            const float height = static_cast<float>(m_active_runtime_window ? m_active_runtime_window->height() : m_config.height);
            m_root_component->measure(geometry::NanConstraints::tight(width, height));
            m_root_component->layout();
        }

        auto sync_root_component_bounds() -> void {
            if (!m_root_component) {
                return;
            }

            const float width = static_cast<float>(m_active_runtime_window ? m_active_runtime_window->width() : m_config.width);
            const float height = static_cast<float>(m_active_runtime_window ? m_active_runtime_window->height() : m_config.height);
            m_root_component->set_bounds(0.0f, 0.0f, width, height);
            m_root_component->mark_layout_dirty();
            ensure_root_component_layout();
        }

        AppConfig m_config;
        runtime::NanWindow *m_active_runtime_window{nullptr};
        NanComponent::Ptr m_root_component{nullptr};

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
            }

            void on_draw(tvg::SwCanvas &canvas) override {
                m_owner.on_draw(canvas);
            }

            void on_resize(int, int) override {
                m_owner.sync_root_component_bounds();
            }

            void on_pointer_move(const runtime::PointerMoveEvent& event) override {
                if (m_owner.m_root_component) {
                    auto* hit = m_owner.m_root_component->hit_test(
                        static_cast<float>(event.x), static_cast<float>(event.y));
                    if (hit) {
                        hit->dispatch_event(runtime::Event{event});
                    }
                }
            }

            void on_pointer_down(const runtime::PointerButtonEvent& event) override {
                if (m_owner.m_root_component) {
                    auto* hit = m_owner.m_root_component->hit_test(
                        static_cast<float>(event.x), static_cast<float>(event.y));
                    if (hit) {
                        hit->dispatch_event(runtime::Event{event});
                    }
                }
            }

            void on_pointer_up(const runtime::PointerButtonEvent& event) override {
                if (m_owner.m_root_component) {
                    auto* hit = m_owner.m_root_component->hit_test(
                        static_cast<float>(event.x), static_cast<float>(event.y));
                    if (hit) {
                        hit->dispatch_event(runtime::Event{event});
                    }
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