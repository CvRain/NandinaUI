//
// Created by cvrain on 2026/4/28.
//

module;

#include <memory>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.surface;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_insets;
import nandina.foundation.color;
import nandina.reactive.state;
import nandina.reactive.prop;

/**
 * nandina.widgets.surface
 *
 * Surface — 容器组件。
 *
 * 职责：
 * - 背景色 / 圆角 / 描边 / 阴影
 * - Padding 内边距（通过 NanInsets 控制子节点收缩）
 * - 至多一个子节点
 *
 * 设计：
 * - Surface 自身是一个 NanWidget，owner 唯一子节点
 * - set_bounds 时根据 padding 收缩子节点空间
 * - 颜色使用 NanColor + reactive Prop<T> 实现响应式
 */
export namespace nandina::widgets {

    /**
     * Surface — 基础容器组件
     *
     * 用法：
     *   auto surface = Surface::create()
     *       .set_bg_color(NanColor::from(NanRgb{30, 30, 46}))
     *       .set_corner_radius(8.0f)
     *       .set_padding(geometry::NanInsets{12.0f});
     *   surface->add_child(std::move(label));
     */
    class Surface : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Surface>;

        ~Surface() override = default;

        // ── 诊断计数器（每帧由 flush_root_layout 读取并重置） ──
        inline static thread_local int s_measure_count{0};

        // ── 建造者模式 ──────────────────────────────────────
        static auto create() -> Ptr {
            return Ptr{new Surface()};
        }

        // ── 属性设置（返回引用支持链式）────────────────────
        virtual auto set_bg_color(const nandina::NanColor& color) -> Surface& {
            m_bg_color.set(color);
            mark_dirty();
            return *this;
        }

        virtual auto set_corner_radius(float radius) -> Surface& {
            m_corner_radius.set(radius);
            mark_dirty();
            return *this;
        }

        virtual auto set_padding(const geometry::NanInsets& insets) -> Surface& {
            m_padding.set(insets);
            mark_layout_dirty();
            return *this;
        }

        virtual auto set_border_color(const nandina::NanColor& color) -> Surface& {
            m_border_color = color;
            mark_dirty();
            return *this;
        }

        virtual auto set_border_width(float width) -> Surface& {
            m_border_width = width;
            mark_dirty();
            return *this;
        }

        // ── 属性访问 ────────────────────────────────────────
        [[nodiscard]] virtual auto bg_color() const noexcept -> const nandina::NanColor& {
            return m_bg_color.get();
        }

        [[nodiscard]] virtual auto corner_radius() const noexcept -> float {
            return m_corner_radius.get();
        }

        [[nodiscard]] virtual auto padding() const noexcept -> const geometry::NanInsets& {
            return m_padding.get();
        }

        [[nodiscard]] auto border_color() const noexcept -> const nandina::NanColor& {
            return m_border_color;
        }

        [[nodiscard]] auto border_width() const noexcept -> float {
            return m_border_width;
        }

        // ── 布局覆盖 ────────────────────────────────────────
        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const auto& pad = m_padding.get();
            const geometry::NanConstraints child_constraints{
                std::max(0.0f, constraints.min_width() - pad.left() - pad.right()),
                constraints.max_width() == geometry::NanConstraints::k_infinity
                    ? geometry::NanConstraints::k_infinity
                    : std::max(0.0f, constraints.max_width() - pad.left() - pad.right()),
                std::max(0.0f, constraints.min_height() - pad.top() - pad.bottom()),
                constraints.max_height() == geometry::NanConstraints::k_infinity
                    ? geometry::NanConstraints::k_infinity
                    : std::max(0.0f, constraints.max_height() - pad.top() - pad.bottom()),
            };

            for_each_child([&](runtime::NanWidget& child) {
                child.measure(child_constraints.is_tight()
                    ? child_constraints
                    : child_constraints.loosen());
            });

            // 计算 preferred_size 并缓存，避免后续 derive_child_max_size 等路径重复遍历子树
            m_cached_preferred_size = compute_preferred_size();
            m_pref_size_valid = true;
            set_measured_layout_state(constraints, m_cached_preferred_size);

            s_measure_count += 1;
        }

        auto layout() -> void override {
            layout_content_children();
            clear_layout_dirty();
        }

        auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            return *this;
        }

        // ── 绘制 ────────────────────────────────────────────
    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            draw_background(canvas);
        }

        [[nodiscard]] auto content_bounds(const float top_offset = 0.0f) const noexcept -> geometry::NanRect {
            const auto rect = bounds();
            const auto& pad = m_padding.get();
            return geometry::NanRect{
                geometry::NanPoint{
                    rect.x() + pad.left(),
                    rect.y() + pad.top() + top_offset,
                },
                geometry::NanSize{
                    std::max(0.0f, rect.width() - pad.left() - pad.right()),
                    std::max(0.0f, rect.height() - pad.top() - pad.bottom() - top_offset),
                }
            };
        }

        auto layout_content_children(const float top_offset = 0.0f, const runtime::NanWidget* skip = nullptr) -> void {
            const auto content = content_bounds(top_offset);
            for_each_child([&](runtime::NanWidget& child) {
                if (&child == skip) {
                    return;
                }
                child.set_bounds(
                    content.x(),
                    content.y(),
                    content.width(),
                    content.height());
                child.layout();
            });
        }

        [[nodiscard]] auto measure_content_preferred_size(
            const runtime::NanWidget* skip = nullptr) const noexcept -> geometry::NanSize {
            geometry::NanSize child_pref{0.0f, 0.0f};

            for_each_child([&](const runtime::NanWidget& child) {
                if (&child == skip) {
                    return;
                }

                const auto cp = child.preferred_size();
                child_pref    = geometry::NanSize{
                    std::max(child_pref.width(), cp.width()),
                    std::max(child_pref.height(), cp.height())
                };
            });

            return child_pref;
        }

        /** 绘制背景与描边 — 子类可扩展 */
        virtual void draw_background(tvg::SwCanvas& canvas) {
            const auto rect    = bounds();
            const auto& bg     = m_bg_color.get();
            const auto bg_rgb  = bg.to<nandina::NanRgb>();
            const float radius = m_corner_radius.get();

            // 背景填充
            auto* shape = tvg::Shape::gen();
            shape->appendRect(rect.x(), rect.y(), rect.width(), rect.height(), radius, radius);
            shape->fill(bg_rgb.red(), bg_rgb.green(), bg_rgb.blue(), bg_rgb.alpha());
            canvas.add(shape);

            // 描边（如果有）
            if (m_border_width > 0.0f) {
                const auto& bc    = m_border_color;
                const auto bc_rgb = bc.to<nandina::NanRgb>();
                auto* border      = tvg::Shape::gen();
                border->appendRect(rect.x(), rect.y(), rect.width(), rect.height(), radius, radius);
                border->strokeWidth(m_border_width);
                border->strokeFill(bc_rgb.red(), bc_rgb.green(), bc_rgb.blue(), bc_rgb.alpha());
                canvas.add(border);
            }

            // 子节点由基类 draw() 遍历绘制
        }

        // ── 首选尺寸（考虑 padding + 子节点） ──────────────
        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            // layout_dirty 时缓存失效（内容可能已变化），需要重算
            if (m_pref_size_valid && !is_layout_dirty()) {
                return m_cached_preferred_size;
            }
            return compute_preferred_size();
        }

    protected:
        Surface() = default;

        reactive::Prop<nandina::NanColor> m_bg_color{nandina::NanColor::from(nandina::NanRgb{255, 255, 255})};
        reactive::Prop<float> m_corner_radius{0.0f};
        reactive::Prop<geometry::NanInsets> m_padding{geometry::NanInsets{}};

        nandina::NanColor m_border_color{nandina::NanColor::from(nandina::NanRgb{0, 0, 0})};
        float m_border_width{0.0f};

        // ── preferred_size 缓存（resize 时避免重复遍历子树） ──
        mutable geometry::NanSize m_cached_preferred_size{};
        mutable bool              m_pref_size_valid{false};

    private:
        [[nodiscard]] auto compute_preferred_size() const noexcept -> geometry::NanSize {
            const auto child_pref = measure_content_preferred_size();
            const auto& pad = m_padding.get();
            return geometry::NanSize{
                child_pref.width() + pad.left() + pad.right(),
                child_pref.height() + pad.top() + pad.bottom()
            };
        }
    };

} // namespace nandina::widgets
