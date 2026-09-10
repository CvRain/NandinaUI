#include "overlay_host.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nandina::widget::internal
{
    namespace
    {
        class OverlaySurface final: public scene::NanControl {
        public:
            [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
                return false;
            }

            [[nodiscard]] auto accepts_child(const scene::NanNode& child) const -> bool override {
                return child.as_control() != nullptr;
            }

        protected:
            [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                return constraints.constrain(size());
            }

            void on_layout() override {
                const scene::LayoutConstraints child_constraints {
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
        id_ = 0;
        host_.reset();
    }

    auto OverlayHost::create() -> std::shared_ptr<OverlayHost> {
        auto result = std::shared_ptr<OverlayHost>(new OverlayHost());
        result->initialize();
        return result;
    }

    void OverlayHost::initialize() {
        content_layer_ = scene::CanvasLayer::create(scene::CanvasSpace::screen, 0);
        overlay_layer_ = scene::CanvasLayer::create(scene::CanvasSpace::screen, 1000);
        overlay_surface_ = std::make_shared<OverlaySurface>();
        overlay_layer_->set_layout_root(overlay_surface_);
        add_layer(content_layer_);
        add_layer(overlay_layer_);
    }

    auto OverlayHost::set_content(std::shared_ptr<scene::NanControl> content)
        -> scene::NanControl& {
        if (!content) {
            throw std::invalid_argument("OverlayHost::set_content: content is null");
        }
        return content_layer_->set_layout_root(std::move(content));
    }

    auto OverlayHost::content() const -> scene::NanControl* {
        return content_layer_->layout_root();
    }

    auto OverlayHost::present(
        std::shared_ptr<scene::NanControl> overlay,
        const OverlayOptions options
    ) -> OverlayHandle {
        if (!overlay) {
            throw std::invalid_argument("OverlayHost::present: overlay is null");
        }
        if (overlay->parent() != nullptr) {
            throw std::logic_error("OverlayHost::present: overlay must be detached");
        }

        overlay->set_z_index(options.order);
        const auto id = next_id_++;
        entries_.push_back(Entry {.id = id, .control = overlay});
        overlay_surface_->add_child(std::move(overlay));
        return OverlayHandle {std::static_pointer_cast<OverlayHost>(shared_from_this()), id};
    }

    auto OverlayHost::overlay_count() const -> std::size_t {
        return entries_.size();
    }

    auto OverlayHost::contains(const std::uint64_t id) const -> bool {
        return std::ranges::any_of(entries_, [id](const Entry& entry) {
            return entry.id == id && !entry.control.expired();
        });
    }

    auto OverlayHost::close(const std::uint64_t id) -> bool {
        const auto found = std::ranges::find(entries_, id, &Entry::id);
        if (found == entries_.end()) {
            return false;
        }
        auto overlay = found->control.lock();
        entries_.erase(found);
        if (overlay != nullptr && overlay->parent() == overlay_surface_.get()) {
            overlay_surface_->remove_and_delete(*overlay);
        }
        return true;
    }
}
