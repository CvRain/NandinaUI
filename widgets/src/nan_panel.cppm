//
// Created by cvrain on 2026/4/29.
//

module;

#include <memory>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.panel;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.reactive.prop;

/**
 * nandina.widgets.panel
 *
 * Panel — 带标题栏的容器组件。
 *
 * 职责：
 * - 继承 Surface 的全部能力（背景色、圆角、内边距、描边）
 * - 顶部标题栏（header）：可自定义颜色和高度
 * - 标题文字显示
 * - 标题栏下方的内容区用于放置子节点
 *
 * 设计：
 * - Panel 继承自 Surface，获得背景/圆角/padding/描边能力
 * - 标题栏渲染在顶部区域，使用独立的 header_color
 * - set_bounds 将子节点定位到标题栏下方 + padding 区域内
 * - 标题使用点阵模拟绘制（与 Label 一致，M5 后由真实字体渲染替换）
 */
export namespace nandina::widgets {

    /**
     * Panel — 带标题栏的容器组件
     *
     * 用法：
     *   auto panel = Panel::create()
     *       .set_title("Project Settings")
     *       .set_header_color(NanColor::from(NanRgb{38, 40, 56}))
     *       .set_bg_color(NanColor::from(NanRgb{42, 44, 62}))
     *       .set_corner_radius(8.0f)
     *       .set_padding(geometry::NanInsets{12.0f});
     *   panel->add_child(std::move(content));
     */
    class Panel : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Panel>;

        ~Panel() override = default;

        static auto create() -> Ptr {
            return Ptr{new Panel()};
        }

        // ── 标题 ──────────────────────────────────────────
        auto set_title(std::string_view title) -> Panel& {
            m_title.set(std::string{title});
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto title() const noexcept -> const std::string& {
            return m_title.get();
        }

        // ── 标题栏颜色 ────────────────────────────────────
        auto set_header_color(const nandina::NanColor& color) -> Panel& {
            m_header_color.set(color);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto header_color() const noexcept -> const nandina::NanColor& {
            return m_header_color.get();
        }

        // ── 标题栏高度 ────────────────────────────────────
        auto set_header_height(float height) -> Panel& {
            m_header_height.set(height);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto header_height() const noexcept -> float {
            return m_header_height.get();
        }

        // ── 背景色 ────────────────────────────────────────
        auto set_bg_color(const nandina::NanColor& color) -> Panel& {
            m_bg_color.set(color);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto bg_color() const noexcept -> const nandina::NanColor& {
            return m_bg_color.get();
        }

        // ── 圆角 ──────────────────────────────────────────
        auto set_corner_radius(float radius) -> Panel& {
            m_corner_radius.set(radius);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto corner_radius() const noexcept -> float {
            return m_corner_radius.get();
        }

        // ── 内边距 ────────────────────────────────────────
        auto set_padding(const geometry::NanInsets& insets) -> Panel& {
            m_padding.set(insets);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto padding() const noexcept -> const geometry::NanInsets& {
            return m_padding.get();
        }

        // ── 描边 ──────────────────────────────────────────
        auto set_border_color(const nandina::NanColor& color) -> Panel& {
            m_border_color = color;
            mark_dirty();
            return *this;
        }

        auto set_border_width(float width) -> Panel& {
            m_border_width = width;
            mark_dirty();
            return *this;
        }

        // ── 布局 ──────────────────────────────────────────
        auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);

            // 定位子节点至标题栏下方 + padding 区域内
            const auto& pad = m_padding.get();
            const float header_h = m_header_height.get();
            const float child_x = x + pad.left();
            const float child_y = y + header_h + pad.top();
            const float child_w = w - pad.left() - pad.right();
            const float child_h = h - header_h - pad.top() - pad.bottom();

            for_each_child([&](runtime::NanWidget& child) {
                child.set_bounds(child_x, child_y, child_w, child_h);
            });

            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            geometry::NanSize child_pref{0.0f, 0.0f};

            for_each_child([&](const runtime::NanWidget& child) {
                const auto cp = child.preferred_size();
                child_pref = geometry::NanSize{
                    std::max(child_pref.width(), cp.width()),
                    std::max(child_pref.height(), cp.height())
                };
            });

            const auto& pad = m_padding.get();
            const float header_h = m_header_height.get();
            return geometry::NanSize{
                child_pref.width() + pad.left() + pad.right(),
                header_h + child_pref.height() + pad.top() + pad.bottom()
            };
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto rect = bounds();
            const float radius = m_corner_radius.get();
            const float header_h = m_header_height.get();

            // ── 1. 背景填充 ──────────────────────────────
            {
                const auto& bg = m_bg_color.get();
                const auto bg_rgb = bg.to<nandina::NanRgb>();
                auto* shape = tvg::Shape::gen();
                shape->appendRect(rect.x(), rect.y(), rect.width(), rect.height(), radius, radius);
                shape->fill(bg_rgb.red(), bg_rgb.green(), bg_rgb.blue(), bg_rgb.alpha());
                canvas.add(shape);
            }

            // ── 2. 标题栏背景 ────────────────────────────
            {
                const auto& hdr = m_header_color.get();
                const auto hdr_rgb = hdr.to<nandina::NanRgb>();

                // 主体（无圆角矩形）
                auto* header = tvg::Shape::gen();
                header->appendRect(rect.x(), rect.y(), rect.width(), header_h, 0.0f, 0.0f);
                header->fill(hdr_rgb.red(), hdr_rgb.green(), hdr_rgb.blue(), hdr_rgb.alpha());
                canvas.add(header);

                // 覆盖顶部圆角区域（使 header 顶部圆角与 Panel 一致）
                if (radius > 0.0f && header_h > radius) {
                    auto* top_round = tvg::Shape::gen();
                    top_round->appendRect(rect.x(), rect.y(), rect.width(), radius, radius, radius);
                    top_round->fill(hdr_rgb.red(), hdr_rgb.green(), hdr_rgb.blue(), hdr_rgb.alpha());
                    canvas.add(top_round);
                }

                // 标题栏底部分隔线
                auto* divider = tvg::Shape::gen();
                divider->moveTo(rect.x(), rect.y() + header_h);
                divider->lineTo(rect.x() + rect.width(), rect.y() + header_h);
                divider->strokeWidth(1.0f);
                divider->strokeFill(hdr_rgb.red(), hdr_rgb.green(), hdr_rgb.blue(),
                                    static_cast<uint8_t>(hdr_rgb.alpha() * 0.8f));
                canvas.add(divider);
            }

            // ── 3. 标题文字 ──────────────────────────────
            {
                const auto& title = m_title.get();
                if (!title.empty()) {
                    const float fs = m_title_font_size.get();
                    const float spacing = fs * 0.8f;
                    const float dot_r = fs * 0.25f;
                    const float text_width = static_cast<float>(title.size()) * spacing;

                    // 水平居中（在 header 区域内）
                    const float title_x = rect.x() + (rect.width() - text_width) * 0.5f + dot_r;
                    const float title_y = rect.y() + header_h * 0.5f + dot_r;

                    const auto& text_clr = m_title_color.get();
                    const auto text_rgb = text_clr.to<nandina::NanRgb>();

                    for (size_t i = 0; i < title.size(); ++i) {
                        if (title[i] == ' ') continue;
                        auto* dot = tvg::Shape::gen();
                        const float cx = title_x + static_cast<float>(i) * spacing;
                        const float cy = title_y;
                        dot->appendCircle(cx, cy, dot_r, dot_r);
                        dot->fill(text_rgb.red(), text_rgb.green(), text_rgb.blue(), text_rgb.alpha());
                        canvas.add(dot);
                    }
                }
            }

            // ── 4. 描边（如果有） ──────────────────────────
            if (m_border_width > 0.0f) {
                const auto bc_rgb = m_border_color.to<nandina::NanRgb>();
                auto* border = tvg::Shape::gen();
                border->appendRect(rect.x(), rect.y(), rect.width(), rect.height(), radius, radius);
                border->strokeWidth(m_border_width);
                border->strokeFill(bc_rgb.red(), bc_rgb.green(), bc_rgb.blue(), bc_rgb.alpha());
                canvas.add(border);
            }
        }

    private:
        Panel() = default;

        reactive::Prop<std::string> m_title{""};
        reactive::Prop<nandina::NanColor> m_bg_color{nandina::NanColor::from(nandina::NanRgb{30, 30, 46})};
        reactive::Prop<nandina::NanColor> m_header_color{nandina::NanColor::from(nandina::NanRgb{38, 40, 56})};
        reactive::Prop<nandina::NanColor> m_title_color{nandina::NanColor::from(nandina::NanRgb{220, 220, 240})};
        reactive::Prop<float> m_corner_radius{8.0f};
        reactive::Prop<float> m_header_height{28.0f};
        reactive::Prop<float> m_title_font_size{13.0f};
        reactive::Prop<geometry::NanInsets> m_padding{geometry::NanInsets{}};

        nandina::NanColor m_border_color{nandina::NanColor::from(nandina::NanRgb{0, 0, 0})};
        float m_border_width{0.0f};
    };

} // namespace nandina::widgets