//
// app/nan_router — keep-alive page stack router implementation.
//

#include "nan_router.hpp"

#include "../animation/animated_property.hpp"
#include "../animation/animation_host.hpp"
#include "../animation/behavior.hpp"
#include "../scene/scene_tree.hpp"

#include <cmath>

namespace nandina::app
{
    /// 每页的转场包装：承载淡入淡出 opacity 并透传布局。
    class PageFrame final: public scene::NanControl {
    public:
        animation::AnimatedProperty<float> opacity {0.0F};

        [[nodiscard]] auto local_opacity() const -> float override {
            return NanControl::local_opacity() * opacity.value();
        }

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override {
            return constraints.constrain(size());
        }

        auto on_layout() -> void override {
            for (std::size_t i = 0; i < child_count(); ++i) {
                auto* child = get_child(i) != nullptr ? get_child(i)->as_control() : nullptr;
                if (!child) {
                    continue;
                }
                (void)child->measure_layout(scene::LayoutConstraints::tight(size()));
                child->layout_to(local_rect());
            }
        }
    };

    namespace
    {
        class PageHost final: public scene::NanControl {
        public:
            std::function<void()> on_tick;

            void on_process(const float dt) override {
                (void)dt;
                if (on_tick) {
                    on_tick();
                }
            }

        protected:
            [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                return constraints.constrain(size());
            }

            auto on_layout() -> void override {
                for (std::size_t i = 0; i < child_count(); ++i) {
                    auto* child = get_child(i) != nullptr ? get_child(i)->as_control() : nullptr;
                    if (!child || !child->visible()) {
                        continue;
                    }
                    (void)child->measure_layout(scene::LayoutConstraints::tight(size()));
                    child->layout_to(local_rect());
                }
            }
        };
    } // namespace

    NanRouter::NanRouter(
        reactive::Graph& graph,
        const theme::NanTheme& theme,
        NanStore* store,
        NanTypeKey store_key,
        resource::ResourceManager* resources,
        text::FontLoader* font_loader,
        text::FontFamilyRegistry* font_families,
        UiDispatcher* dispatcher,
        BackgroundExecutor* background_executor
    ):
        graph_(&graph),
        theme_(&theme),
        store_(store),
        store_key_(store_key),
        resources_(resources),
        font_loader_(font_loader),
        font_families_(font_families),
        dispatcher_(dispatcher),
        background_executor_(background_executor),
        host_(std::make_shared<PageHost>()) {
        static_cast<PageHost*>(host_.get())->on_tick = [this] { drop_completed_exits(); };
    }

    NanRouter::NanRouter(
        reactive::Graph& graph,
        theme::ThemeManager& theme_manager,
        NanStore* store,
        NanTypeKey store_key,
        resource::ResourceManager* resources,
        text::FontLoader* font_loader,
        text::FontFamilyRegistry* font_families,
        UiDispatcher* dispatcher,
        BackgroundExecutor* background_executor
    ):
        NanRouter(
            graph,
            theme_manager.theme(),
            store,
            store_key,
            resources,
            font_loader,
            font_families,
            dispatcher,
            background_executor
        ) {
        theme_manager_ = &theme_manager;
    }

    auto NanRouter::host() -> std::shared_ptr<scene::NanControl> {
        return host_;
    }

    auto NanRouter::graph() -> reactive::Graph& {
        return *graph_;
    }

    auto NanRouter::theme() const -> const theme::NanTheme& {
        return theme_manager_ != nullptr ? theme_manager_->theme() : *theme_;
    }

    auto NanRouter::store_base() -> NanStore* {
        return store_;
    }

    auto NanRouter::depth() const -> std::size_t {
        return frames_.size();
    }

    auto NanRouter::empty() const -> bool {
        return frames_.empty();
    }

    auto NanRouter::current_key() const -> std::string_view {
        return frames_.empty() ? std::string_view {} : std::string_view {frames_.back().key};
    }

    auto NanRouter::can_pop() const -> bool {
        return frames_.size() > 1;
    }

    void NanRouter::clear_store() {
        store_ = nullptr;
        store_key_ = nullptr;
    }

    auto NanRouter::pop() -> bool {
        if (frames_.size() <= 1) {
            return false;
        }

        remove_top();
        sync_visibility();
        return true;
    }

    auto NanRouter::pop_to(std::string_view route_key) -> bool {
        if (frames_.empty()) {
            return false;
        }

        std::size_t target = frames_.size();
        for (std::size_t i = frames_.size(); i > 0; --i) {
            if (frames_[i - 1].key == route_key) {
                target = i - 1;
                break;
            }
        }
        if (target == frames_.size()) {
            return false;
        }

        while (frames_.size() > target + 1) {
            drop_frame(frames_.back());
            frames_.pop_back();
        }
        sync_visibility();
        return true;
    }

    void NanRouter::clear() {
        while (!frames_.empty()) {
            drop_frame(frames_.back());
            frames_.pop_back();
        }
        for (auto& frame: exiting_) {
            drop_frame(frame);
        }
        exiting_.clear();
    }

    auto NanRouter::request_pop() -> bool {
        return post_command([this] { (void)pop(); });
    }

    auto NanRouter::request_pop_to(std::string route_key) -> bool {
        return post_command([this, route_key = std::move(route_key)] { (void)pop_to(route_key); });
    }

    auto NanRouter::request_clear() -> bool {
        return post_command([this] { clear(); });
    }

    void NanRouter::push_page(std::unique_ptr<NanPage> page) {
        if (!page) {
            throw std::runtime_error("NanRouter::push_page: page is null");
        }

        auto scope = std::make_unique<reactive::ReactiveScope>(*graph_);
        auto async_scope = dispatcher_ && background_executor_
            ? std::make_unique<AsyncScope>(*dispatcher_, *background_executor_)
            : nullptr;
        PageContext context {
            *this,
            *graph_,
            *scope,
            theme(),
            store_,
            store_key_,
            resources_,
            font_loader_,
            font_families_,
            async_scope.get(),
            theme_manager_,
            dispatcher_
        };
        auto root = page->build(context);
        if (!root) {
            throw std::runtime_error("NanRouter::push_page: page build returned null root");
        }

        std::shared_ptr<PageFrame> frame;
        if (transition_enabled_) {
            // 转场时由包装帧控制可见性；页面根保持可见，避免根被标记不可见而失去焦点/命中。
            frame = std::make_shared<PageFrame>();
            frame->add_child(root);
            frame->set_visible(false);
            attach_root(frame);
        }
        else {
            root->set_visible(false);
            attach_root(root);
        }
        frames_.push_back(
            Frame {
                .page = std::move(page),
                .root = std::move(root),
                .frame = std::move(frame),
                .scope = std::move(scope),
                .async_scope = std::move(async_scope),
                .key = {},
                .active = false,
            }
        );
        frames_.back().key = std::string(frames_.back().page->route_key());
        sync_visibility();
        if (transition_enabled_ && frames_.back().frame) {
            fade_frame(frames_.back(), 1.0F);
        }
    }

    void NanRouter::sync_visibility() {
        // Find the previously active frame (if any) and the new top frame.
        Frame* deactivating = nullptr;
        Frame* activating = nullptr;

        for (auto& frame : frames_) {
            if (frame.active) {
                deactivating = &frame;
                break;
            }
        }

        // Set all frames invisible, mark top as visible.
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            frame_node(frames_[i])->set_visible(i + 1 == frames_.size());
        }

        if (!frames_.empty()) {
            activating = &frames_.back();
        }

        // Deactivate the previously active page before activating the new one.
        if (deactivating != nullptr && deactivating != activating) {
            deactivating->active = false;
            auto ctx = make_context_for(*deactivating);
            deactivating->page->on_deactivate(ctx);
        }

        // Activate the new top page.
        if (activating != nullptr && !activating->active) {
            activating->active = true;
            auto ctx = make_context_for(*activating);
            activating->page->on_activate(ctx);
        }
    }

    void NanRouter::attach_root(const std::shared_ptr<scene::NanNode2D>& root) {
        host_->add_child(root);
    }

    void NanRouter::detach_root(const std::shared_ptr<scene::NanNode2D>& root) {
        if (!root) {
            return;
        }
        host_->remove_child(*root);
    }

    void NanRouter::drop_frame(Frame& frame) {
        if (frame.active) {
            frame.active = false;
            auto ctx = make_context_for(frame);
            frame.page->on_deactivate(ctx);
        }
        detach_root(frame_node(frame));
        if (frame.async_scope != nullptr) {
            frame.async_scope->clear();
        }
        if (frame.scope != nullptr) {
            frame.scope->clear();
        }
    }

    auto NanRouter::post_command(std::move_only_function<void()> command) -> bool {
        if (dispatcher_ == nullptr) {
            return false;
        }
        auto lifetime = std::weak_ptr<void>(command_lifetime_);
        return dispatcher_->post([lifetime = std::move(lifetime),
                                  command = std::move(command)]() mutable {
            if (const auto alive = lifetime.lock()) {
                command();
            }
        });
    }

    auto NanRouter::make_context_for(Frame& frame) -> PageContext {
        return PageContext {
            *this,
            *graph_,
            *frame.scope,
            theme(),
            store_,
            store_key_,
            resources_,
            font_loader_,
            font_families_,
            frame.async_scope.get(),
            theme_manager_,
            dispatcher_
        };
    }

    void NanRouter::set_transition_enabled(const bool enabled) {
        transition_enabled_ = enabled;
    }

    auto NanRouter::transition_enabled() const -> bool {
        return transition_enabled_;
    }

    void NanRouter::set_transition_duration(const float seconds) {
        if (!std::isfinite(seconds) || seconds < 0.0F) {
            throw std::invalid_argument("transition duration must be finite and non-negative");
        }
        transition_duration_ = seconds;
    }

    auto NanRouter::frame_node(const Frame& frame) const -> std::shared_ptr<scene::NanNode2D> {
        if (frame.frame) {
            return frame.frame;
        }
        return frame.root;
    }

    void NanRouter::fade_frame(Frame& frame, const float target) {
        if (!frame.frame) {
            return;
        }
        if (frame.frame->get_tree() == nullptr) {
            // 未挂载（如 build() 阶段的首次 push）：无 Host 推进，直接跳到目标。
            frame.frame->opacity.clear_behavior();
            frame.frame->opacity.set_target(target);
            return;
        }
        frame.frame->opacity.set_behavior(
            animation::Behavior<float>(transition_duration_, animation::Easing::ease_out)
        );
        frame.frame->get_tree()->animation_host().set_target(
            *frame.frame, frame.frame->opacity, target, scene::DirtyFlags::paint
        );
    }

    void NanRouter::remove_top() {
        if (frames_.empty()) {
            return;
        }
        const bool can_transition = transition_enabled_ && frames_.back().frame
            && host_ != nullptr && host_->get_tree() != nullptr;
        if (can_transition) {
            Frame& outgoing = frames_.back();
            if (outgoing.active) {
                outgoing.active = false;
                auto ctx = make_context_for(outgoing);
                outgoing.page->on_deactivate(ctx);
            }
            fade_frame(outgoing, 0.0F);
            exiting_.push_back(std::move(outgoing));
            frames_.pop_back();
        }
        else {
            drop_frame(frames_.back());
            frames_.pop_back();
        }
    }

    void NanRouter::drop_completed_exits() {
        auto* tree = host_ != nullptr ? host_->get_tree() : nullptr;
        for (auto it = exiting_.begin(); it != exiting_.end();) {
            Frame& frame = *it;
            if (frame.frame && frame.frame->opacity.is_animating()) {
                ++it;
                continue;
            }
            // 生命周期清理是安全的；树 detach 延迟到 tree_commit（on_process 处于 process 阶段）。
            if (frame.async_scope != nullptr) {
                frame.async_scope->clear();
            }
            if (frame.scope != nullptr) {
                frame.scope->clear();
            }
            if (auto node = frame_node(frame); tree != nullptr && node != nullptr) {
                tree->defer_tree_mutation([host = host_, node = std::move(node)]() {
                    host->remove_child(*node);
                });
            }
            it = exiting_.erase(it);
        }
    }

} // namespace nandina::app
