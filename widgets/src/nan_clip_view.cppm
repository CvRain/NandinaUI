//
// nan_clip_view.cppm — 通用子树裁剪容器组件
//
// ClipView 是一个不可见的通用裁剪容器。
// 它不绘制任何内容，只负责在其 bounds 内自动裁剪所有子节点。
// 任何 widget 想启用子树裁剪，只需被 ClipView 包裹。
//
// 设计意图：
//   遵循 overflow-and-clip-contract.md 中"父容器声明 child 是否可见裁剪边界"
//   的设计方向，但以通用组件形式提供，避免每个容器重复实现 clip 逻辑。
//
// 用法：
//   auto clip = ClipView::Create();
//   clip->set_corner_radius(4.0f);        // 可选圆角裁剪
//   clip->set_clip_padding(NanInsets{4}); // 可选向内收缩裁剪区域
//   clip->add_child(std::move(child));
//

module;

#include <memory>
#include <optional>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.clip_view;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_constraints;

export namespace nandina::widgets
{

    /**
     * ClipView — 通用子树裁剪容器组件。
     *
     * ClipView 是一个不可见的裁剪容器。它自身不绘制任何内容，
     * 但会将其所有子节点的绘制限制在自身的裁剪边界内。
     *
     * 默认行为：
     *   - overflow_behavior = clip（自动启用子节点裁剪）
     *   - child_clip_rect  = 自身 bounds
     *   - child_clip_corner_radius = 0（不圆角）
     *   - 可实现与 padding 类似的 shrink 效果以裁剪到 content box
     *
     * 继承链：NanWidget → ClipView
     *
     *   auto clip = ClipView::Create();
     *   clip->set_corner_radius(8.0f);
     *   clip->add_child(std::move(someWidget));
     *   parent->add_child(std::move(clip));
     */
    class ClipView: public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<ClipView>;

        ~ClipView() override = default;

        static auto Create() -> Ptr {
            return Ptr(new ClipView());
        }

        // ── 裁剪区域向内收缩（类似 padding） ───────────
        auto set_clip_padding(const geometry::NanInsets& insets) -> ClipView& {
            m_clip_padding = insets;
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto clip_padding() const noexcept -> const geometry::NanInsets& {
            return m_clip_padding;
        }

        // ── 圆角裁剪 ──────────────────────────────────
        auto set_corner_radius(const float radius) -> ClipView& {
            m_corner_radius = radius;
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto corner_radius() const noexcept -> float {
            return m_corner_radius;
        }

        // ── 继承自 NanWidget 的 child_clip 协议 ────────
        // ClipView 始终启用裁剪，默认 clip 到自身 bounds

        [[nodiscard]] auto child_clip_rect() const noexcept
            -> std::optional<geometry::NanRect> override {
            const auto bnds = bounds();
            const float cx = bnds.x() + m_clip_padding.left();
            const float cy = bnds.y() + m_clip_padding.top();
            const float cw =
                std::max(0.0f, bnds.width() - m_clip_padding.left() - m_clip_padding.right());
            const float ch =
                std::max(0.0f, bnds.height() - m_clip_padding.top() - m_clip_padding.bottom());
            return geometry::NanRect {geometry::NanPoint {cx, cy}, geometry::NanSize {cw, ch}};
        }

        [[nodiscard]] auto child_clip_corner_radius() const noexcept -> float override {
            return m_corner_radius;
        }

        // ── 测量 ─────────────────────────────────────
        auto measure(const geometry::NanConstraints& constraints) -> void override {
            geometry::NanSize child_measured {0.0f, 0.0f};
            for_each_child([&](runtime::NanWidget& child) {
                child.measure(constraints);
                const auto ms = child.measured_size();
                const auto ps = child.preferred_size();
                child_measured = geometry::NanSize {
                    std::max(child_measured.width(), ms.width() > 0.0f ? ms.width() : ps.width()),
                    std::max(
                        child_measured.height(),
                        ms.height() > 0.0f ? ms.height() : ps.height()
                    ),
                };
            });
            set_measured_layout_state(constraints, constraints.constrain(child_measured));
        }

        // ── 布局 ─────────────────────────────────────
        auto layout() -> void override {
            const auto bnds = bounds();
            for_each_child([&](runtime::NanWidget& child) {
                child.set_bounds(bnds.x(), bnds.y(), bnds.width(), bnds.height());
                child.layout();
            });
            clear_layout_dirty();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            geometry::NanSize child_pref {0.0f, 0.0f};
            for_each_child([&](const runtime::NanWidget& child) {
                const auto cp = child.preferred_size();
                child_pref = geometry::NanSize {
                    std::max(child_pref.width(), cp.width()),
                    std::max(child_pref.height(), cp.height())
                };
            });
            return child_pref;
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept
            -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            return *this;
        }

    protected:
        // 不可见：不绘制任何内容
        void on_draw(tvg::SwCanvas& /*canvas*/) override {}

    private:
        ClipView() {
            // 默认启用子树裁剪
            set_overflow_behavior(nandina::runtime::OverflowBehavior::clip);
        }

        geometry::NanInsets m_clip_padding {};
        float m_corner_radius {0.0f};
    };

} // namespace nandina::widgets