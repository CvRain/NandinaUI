//
// Created by cvrain on 2026/4/29.
//

module;

#include <memory>

export module nandina.widgets.panel;

import nandina.foundation.color;
import nandina.foundation.nan_insets;
import nandina.widgets.surface;

/**
 * nandina.widgets.panel
 *
 * Panel — 中性内容面板容器。
 *
 * 职责：
 * - 继承 Surface 的全部能力（背景色、圆角、内边距、描边）
 * - 作为一个中性的内容面板容器，供页面作者放置任意子组件
 * - 不承载 header / title / footer 这类高层语义
 *
 * 设计：
 * - Panel 继承自 Surface，获得背景/圆角/padding/描边能力
 * - 布局与内容边界完全复用 Surface
 * - 标题、描述、accent、footer 等结构语义应交由 Card / Dialog 等高层组件承担
 */
export namespace nandina::widgets {
    /**
     * Panel — 中性内容面板容器
     *
     * 用法：
     *   auto panel = Panel::create()
     *       .set_bg_color(NanColor::from(NanRgb{42, 44, 62}))
     *       .set_corner_radius(8.0f)
     *       .set_padding(geometry::NanInsets{12.0f});
     *   panel->add_child(std::move(content));
     */
    class Panel : public Surface {
    public:
        using Ptr = std::unique_ptr<Panel>;

        ~Panel() override = default;

        static auto create() -> Ptr {
            return Ptr{new Panel()};
        }

        // ── 背景色 ────────────────────────────────────────
        auto set_bg_color(const nandina::NanColor &color) -> Panel& override {
            Surface::set_bg_color(color);
            return *this;
        }

        [[nodiscard]] auto bg_color() const noexcept -> const nandina::NanColor& override {
            return Surface::bg_color();
        }

        // ── 圆角 ──────────────────────────────────────────
        auto set_corner_radius(const float radius) -> Panel& override {
            Surface::set_corner_radius(radius);
            return *this;
        }

        [[nodiscard]] auto corner_radius() const noexcept -> float override {
            return Surface::corner_radius();
        }

        // ── 内边距 ────────────────────────────────────────
        auto set_padding(const geometry::NanInsets &insets) -> Panel& override {
            Surface::set_padding(insets);
            return *this;
        }

        [[nodiscard]] auto padding() const noexcept -> const geometry::NanInsets& override {
            return Surface::padding();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return Surface::preferred_size();
        }

        // ── 描边 ──────────────────────────────────────────
        auto set_border_color(const nandina::NanColor &color) -> Panel& override {
            Surface::set_border_color(color);
            return *this;
        }

        auto set_border_width(const float width) -> Panel& override {
            Surface::set_border_width(width);
            return *this;
        }

    private:
        Panel() {
            Panel::set_bg_color(nandina::NanColor::from(nandina::NanRgb{30, 30, 46}));
            Panel::set_corner_radius(8.0f);
        }
    };
} // namespace nandina::widgets
