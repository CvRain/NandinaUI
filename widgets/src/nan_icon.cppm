//
// Created by cvrain on 2026/4/29.
//

module;

#include <cmath>
#include <memory>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.icon;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.reactive.prop;

/**
 * nandina.widgets.icon
 *
 * Icon — 简单几何图标组件。
 *
 * 职责：
 * - 渲染常见小图标形状：Square（方块）、Circle（圆）、Triangle（三角）、
 *   Check（对勾）、ArrowUp（上箭头）、ArrowDown（下箭头）、Dots（小圆点）
 * - 图标颜色、大小、描边宽度可配置
 * - 首选尺寸固定为 size x size（正方形）
 *
 * 用法：
 *   auto icon = Icon::create()
 *       .set_type(IconType::Square)
 *       .set_size(24.0f)
 *       .set_color(NanColor::from(NanRgb{99, 102, 241}));
 *   parent->add_child(std::move(icon));
 */
export namespace nandina::widgets {

    /// 图标形状枚举
    enum class IconType : uint8_t {
        Square,     // 小方块（导航图标用）
        Circle,     // 圆形（Logo 用）
        Triangle,   // 三角形（播放/装饰用）
        Check,      // 对勾
        ArrowUp,    // 上箭头
        ArrowDown,  // 下箭头
        Dots,       // 小圆点（活动列表脉冲用）
        Dot,        // 单圆点（项目列表装饰用）
    };

    class Icon : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Icon>;

        ~Icon() override = default;

        static auto create() -> Ptr {
            return Ptr{new Icon()};
        }

        // ── 属性设置 ──────────────────────────────────────
        auto set_type(IconType type) -> Icon& {
            m_type = type;
            mark_dirty();
            return *this;
        }

        /// 图标绘制区域大小（永远是正方形）
        auto set_size(float size) -> Icon& {
            m_size.set(size);
            mark_dirty();
            return *this;
        }

        /// 描边宽度（仅对 Check/Arrow 等线框图标有效）
        auto set_stroke_width(float width) -> Icon& {
            m_stroke_width = width;
            mark_dirty();
            return *this;
        }

        auto set_color(const nandina::NanColor& color) -> Icon& {
            m_color.set(color);
            mark_dirty();
            return *this;
        }

        // ── 属性访问 ──────────────────────────────────────
        [[nodiscard]] auto type() const noexcept -> IconType {
            return m_type;
        }

        [[nodiscard]] auto size() const noexcept -> float {
            return m_size.get();
        }

        [[nodiscard]] auto color() const noexcept -> const nandina::NanColor& {
            return m_color.get();
        }

        // ── 首选尺寸 ──────────────────────────────────────
        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const float s = m_size.get();
            return {s, s};
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto bnds = bounds();
            const float cx = bnds.center().x();
            const float cy = bnds.center().y();
            const float s = m_size.get();
            const auto& clr = m_color.get();
            const auto rgb = clr.to<nandina::NanRgb>();

            switch (m_type) {
            case IconType::Square:
                draw_square(canvas, cx, cy, s, rgb);
                break;
            case IconType::Circle:
                draw_circle(canvas, cx, cy, s * 0.5f, rgb);
                break;
            case IconType::Triangle:
                draw_triangle(canvas, cx, cy, s, rgb);
                break;
            case IconType::Check:
                draw_check(canvas, cx, cy, s, rgb);
                break;
            case IconType::ArrowUp:
                draw_arrow(canvas, cx, cy, s, rgb, true);
                break;
            case IconType::ArrowDown:
                draw_arrow(canvas, cx, cy, s, rgb, false);
                break;
            case IconType::Dots:
                draw_dots(canvas, cx, cy, s, rgb);
                break;
            case IconType::Dot:
                draw_dot(canvas, cx, cy, s, rgb);
                break;
            }
        }

    private:
        Icon() = default;

        static void draw_square(tvg::SwCanvas& canvas,
            float cx, float cy, float size,
            const nandina::NanRgb& rgb) {
            const float half = size * 0.4f;
            auto* shape = tvg::Shape::gen();
            shape->appendRect(cx - half, cy - half, half * 2.0f, half * 2.0f, 2, 2);
            shape->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            canvas.add(shape);
        }

        static void draw_circle(tvg::SwCanvas& canvas,
            float cx, float cy, float radius,
            const nandina::NanRgb& rgb) {
            auto* shape = tvg::Shape::gen();
            shape->appendCircle(cx, cy, radius, radius);
            shape->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            canvas.add(shape);
        }

        static void draw_triangle(tvg::SwCanvas& canvas,
            float cx, float cy, float size,
            const nandina::NanRgb& rgb) {
            const float s = size * 0.4f;
            auto* shape = tvg::Shape::gen();
            shape->moveTo(cx, cy - s);
            shape->lineTo(cx + s * 0.866f, cy + s * 0.5f);
            shape->lineTo(cx - s * 0.866f, cy + s * 0.5f);
            shape->close();
            shape->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            canvas.add(shape);
        }

        static void draw_check(tvg::SwCanvas& canvas,
            float cx, float cy, float size,
            const nandina::NanRgb& rgb,
            float stroke_width = 2.0f) {
            const float s = size * 0.3f;
            auto* shape = tvg::Shape::gen();
            shape->moveTo(cx - s * 0.7f, cy);
            shape->lineTo(cx - s * 0.2f, cy + s * 0.6f);
            shape->lineTo(cx + s * 0.8f, cy - s * 0.5f);
            shape->strokeWidth(stroke_width);
            shape->strokeFill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            canvas.add(shape);
        }

        static void draw_arrow(tvg::SwCanvas& canvas,
            float cx, float cy, float size,
            const nandina::NanRgb& rgb,
            bool up,
            float stroke_width = 2.5f) {
            const float s = size * 0.3f;
            const float dir = up ? -1.0f : 1.0f;
            auto* shape = tvg::Shape::gen();
            // 竖线
            shape->moveTo(cx, cy + dir * s * 1.0f);
            shape->lineTo(cx, cy - dir * s * 1.0f);
            // 箭头（斜线）
            shape->moveTo(cx - s * 0.6f, cy - dir * s * 0.3f);
            shape->lineTo(cx, cy - dir * s * 1.0f);
            shape->lineTo(cx + s * 0.6f, cy - dir * s * 0.3f);
            shape->strokeWidth(stroke_width);
            shape->strokeFill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            canvas.add(shape);
        }

        static void draw_dots(tvg::SwCanvas& canvas,
            float cx, float cy, float size,
            const nandina::NanRgb& rgb) {
            const float dot_r = size * 0.1f;
            const float spacing = size * 0.25f;
            for (int i = -1; i <= 1; ++i) {
                auto* shape = tvg::Shape::gen();
                shape->appendCircle(cx + static_cast<float>(i) * spacing, cy, dot_r, dot_r);
                shape->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
                canvas.add(shape);
            }
        }

        static void draw_dot(tvg::SwCanvas& canvas,
            float cx, float cy, float size,
            const nandina::NanRgb& rgb) {
            const float dot_r = size * 0.15f;
            auto* shape = tvg::Shape::gen();
            shape->appendCircle(cx, cy, dot_r, dot_r);
            shape->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            canvas.add(shape);
        }

        IconType m_type{IconType::Square};
        reactive::Prop<float> m_size{16.0f};
        reactive::Prop<nandina::NanColor> m_color{
            nandina::NanColor::from(nandina::NanRgb{220, 220, 240})};
        float m_stroke_width{2.0f};
    };

} // namespace nandina::widgets
