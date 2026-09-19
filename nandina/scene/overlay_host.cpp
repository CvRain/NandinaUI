#include "overlay_host.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nandina::scene
{
    namespace
    {
        class OverlaySurface final: public NanControl {
        public:
            [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
                return false;
            }

            [[nodiscard]] auto accepts_child(const NanNode& child) const -> bool override {
                return child.as_control() != nullptr;
            }

        protected:
            [[nodiscard]] auto on_measure(LayoutConstraints constraints)
                -> foundation::NanSize override {
                return constraints.constrain(size());
            }

            void on_layout() override {
                const LayoutConstraints child_constraints {
                    .min_width = 0.0F,
                    .max_width = width(),
                    .min_height = 0.0F,
                    .max_height = height(),
                };
                for (std::size_t index = 0; index < child_count(); ++index) {
                    auto* child = get_child(index) != nullptr
                        ? get_child(index)->as_control()
                        : nullptr;
                    if (child == nullptr || !child->visible()) {
                        continue;
                    }
                    const auto measured = child->measure_layout(child_constraints);
                    child->layout_to(foundation::NanRect::from_origin_size(
                        child->position(), measured
                    ));
                }
            }
        };
    }

    OverlayHandle::OverlayHandle(std::weak_ptr<OverlayHost> host, const std::uint64_t id):
        host_(std::move(host)), id_(id) {}

    OverlayHandle::~OverlayHandle() {
        close();
    }

    OverlayHandle::OverlayHandle(OverlayHandle&& other) noexcept:
        host_(std::move(other.host_)), id_(std::exchange(other.id_, 0)) {}

    auto OverlayHandle::operator=(OverlayHandle&& other) noexcept -> OverlayHandle& {
        if (this == &other) {
            return *this;
        }
        close();
        host_ = std::move(other.host_);
        id_ = std::exchange(other.id_, 0);
        return *this;
    }

    auto OverlayHandle::mounted() const -> bool {
        auto host = host_.lock();
        return id_ != 0 && host != nullptr && host->contains(id_);
    }

    void OverlayHandle::close() {
        if (id_ == 0) {
            return;
        }
        if (auto host = host_.lock(); host != nullptr) {
            (void)host->close(id_);
        }
        // 刻意保留 id_ 与 host_：mounted() 依据条目状态判断，关闭后仍要能查询
        // close_reason()（组件据此区分是用户关的还是随父层关的）。id 永不复用，
        // 因此保留不会指向别的浮层。
    }

    auto OverlayHost::create() -> std::shared_ptr<OverlayHost> {
        auto result = std::shared_ptr<OverlayHost>(new OverlayHost());
        result->self_ = result;
        result->initialize();
        return result;
    }

    void OverlayHost::initialize() {
        content_layer_ = CanvasLayer::create(CanvasSpace::screen, 0);
        overlay_layer_ = CanvasLayer::create(CanvasSpace::screen, 1000);
        overlay_surface_ = std::make_shared<OverlaySurface>();
        overlay_layer_->set_layout_root(overlay_surface_);
        add_layer(content_layer_);
        add_layer(overlay_layer_);
    }

    auto OverlayHost::set_content(std::shared_ptr<NanControl> content)
        -> NanControl& {
        if (!content) {
            throw std::invalid_argument("OverlayHost::set_content: content is null");
        }
        return content_layer_->set_layout_root(std::move(content));
    }

    auto OverlayHost::content() const -> NanControl* {
        return content_layer_->layout_root();
    }

    void OverlayHost::clear_content() {
        if (content_layer_ == nullptr) {
            return;
        }
        content_layer_->clear_layout_root();
    }

    void OverlayHost::clear_overlays() {
        // close() also removes the child from overlay_surface_ and updates the input
        // mode. Remove from the back so every id remains valid while entries_ shrinks.
        // 整棵树都标 host_teardown：窗口销毁时组件应当按"宿主关闭"处理，
        // 而不是被当成"父浮层收起了"。
        while (overlay_count() > 0) {
            const auto live = std::ranges::find_if(entries_, [](const Entry& entry) {
                return entry.close_reason == OverlayCloseReason::none;
            });
            if (live == entries_.end()) {
                break;
            }
            if (live->parent != 0 && contains(live->parent)) {
                // 由父层递归关闭，避免把祖先留到最后单独处理。
                const auto parent = live->parent;
                (void)close_descendants(parent, OverlayCloseReason::host_teardown);
                (void)close(parent, OverlayCloseReason::host_teardown);
            }
            else {
                (void)close(live->id, OverlayCloseReason::host_teardown);
            }
        }
    }

    auto OverlayHost::present(
        std::shared_ptr<NanControl> overlay,
        const OverlayOptions options
    ) -> OverlayHandle {
        if (!overlay) {
            throw std::invalid_argument("OverlayHost::present: overlay is null");
        }
        if (overlay->parent() != nullptr) {
            throw std::logic_error("OverlayHost::present: overlay must be detached");
        }

        // 父浮层必须仍存在：父已被关闭时不允许再挂子层，否则会立刻变成孤儿。
        if (options.parent != 0 && !contains(options.parent)) {
            throw std::invalid_argument(
                "OverlayHost::present: parent overlay does not exist"
            );
        }

        overlay->set_z_index(static_cast<int>(options.level));
        const auto id = next_id_++;
        entries_.push_back(Entry {
            .id = id,
            .control = overlay,
            .block_below = options.block_below,
            .parent = options.parent,
        });
        overlay_surface_->add_child(std::move(overlay));
        update_input_mode();

        OverlayHandle handle {
            std::static_pointer_cast<OverlayHost>(shared_from_this()),
            id
        };
        handle.parent_id_ = options.parent;
        return handle;
    }

    auto OverlayHost::overlay_count() const -> std::size_t {
        // 已关闭的条目会作为"墓碑"保留，统计时不计入。
        return static_cast<std::size_t>(std::ranges::count_if(
            entries_,
            [](const Entry& entry) { return entry.close_reason == OverlayCloseReason::none; }
        ));
    }

    auto OverlayHost::contains(const std::uint64_t id) const -> bool {
        return std::ranges::any_of(entries_, [id](const Entry& entry) {
            return entry.id == id && entry.close_reason == OverlayCloseReason::none;
        });
    }

    auto OverlayHost::overlay_containing(const NanNode& node) const -> std::uint64_t {
        // 从节点向上走：第一个自底向上的祖先若属于某个在挂浮层，那个浮层就是它的宿主。
        std::uint64_t innermost = 0;
        for (const auto* current = node.parent(); current != nullptr; current = current->parent()) {
            const auto found = std::ranges::find_if(entries_, [current](const Entry& entry) {
                if (entry.close_reason != OverlayCloseReason::none) {
                    return false;
                }
                const auto control = entry.control.lock();
                return control != nullptr && control.get() == current;
            });
            if (found != entries_.end()) {
                // 继续向上找更外层的浮层，最终取最内层（第一次命中即最内层）。
                innermost = found->id;
                break;
            }
        }
        return innermost;
    }

    auto OverlayHost::overlay_close_reason(const std::uint64_t id) const
        -> OverlayCloseReason {
        const auto found = std::ranges::find(entries_, id, &Entry::id);
        return found == entries_.end() ? OverlayCloseReason::none : found->close_reason;
    }

    auto OverlayHost::overlay_parent(const std::uint64_t id) const -> std::uint64_t {
        const auto found = std::ranges::find(entries_, id, &Entry::id);
        return found == entries_.end() ? 0 : found->parent;
    }

    auto OverlayHost::overlay_child_count(const std::uint64_t id) const -> std::size_t {
        return static_cast<std::size_t>(std::ranges::count_if(
            entries_,
            [id](const Entry& entry) {
                return entry.parent == id && entry.close_reason == OverlayCloseReason::none;
            }
        ));
    }

    auto OverlayHost::hosts_node(const NanNode& node) const -> bool {
        for (const auto* current = node.parent(); current != nullptr; current = current->parent()) {
            if (current == overlay_layer_.get()) {
                return true;
            }
        }
        return false;
    }

    auto OverlayHost::viewport_size() const -> foundation::NanSize {
        return overlay_surface_ != nullptr ? overlay_surface_->size() : foundation::NanSize {};
    }

    auto OverlayHost::weak_self() const noexcept -> std::weak_ptr<OverlayHost> {
        return self_;
    }

    auto OverlayHost::close(const std::uint64_t id, const OverlayCloseReason reason) -> bool {
        const auto found = std::ranges::find(entries_, id, &Entry::id);
        if (found == entries_.end() || found->close_reason != OverlayCloseReason::none) {
            return false;
        }
        // 先关掉自己的后代：内层浮层不能比外层活得久，否则会留在屏幕上且指向已消失的
        // 锚点。后代各自带上 OverlayCloseReason::parent。
        (void)close_descendants(id, reason == OverlayCloseReason::owner
                ? OverlayCloseReason::parent
                : reason);

        // found 可能因为在上面关闭后代时被 erase 而失效，重新查找一次。
        const auto current = std::ranges::find(entries_, id, &Entry::id);
        if (current == entries_.end()) {
            return false;
        }
        auto overlay = current->control.lock();
        // 条目保留（只标记原因）：句柄据此查询关闭原因，且 id 永不复用。
        current->close_reason = reason;
        current->control.reset();
        if (overlay != nullptr && overlay->parent() == overlay_surface_.get()) {
            overlay_surface_->remove_and_delete(*overlay);
        }
        update_input_mode();
        return true;
    }

    auto OverlayHost::close_descendants(
        const std::uint64_t id,
        const OverlayCloseReason reason
    ) -> std::size_t {
        // 直接子层里可能有已经失效的条目，逐个通过 close() 递归下去。
        std::vector<std::uint64_t> children;
        for (const auto& entry: entries_) {
            if (entry.parent == id && entry.close_reason == OverlayCloseReason::none) {
                children.push_back(entry.id);
            }
        }
        std::size_t closed = 0;
        for (const auto child: children) {
            if (close(child, reason)) {
                ++closed;
            }
        }
        return closed;
    }

    void OverlayHost::update_input_mode() {
        const bool blocks = std::ranges::any_of(entries_, [](const Entry& entry) {
            return entry.block_below && entry.close_reason == OverlayCloseReason::none;
        });
        overlay_layer_->set_input_mode(
            blocks ? LayerInputMode::block_below : LayerInputMode::pass
        );
    }

    auto OverlayHandle::close_reason() const noexcept -> OverlayCloseReason {
        if (auto host = host_.lock(); host != nullptr) {
            return host->overlay_close_reason(id_);
        }
        return OverlayCloseReason::none;
    }
} // namespace nandina::scene
