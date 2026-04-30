//
// Created by cvrain on 2026/4/29.
//

module;

#include <memory>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.spacer;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_size;

/**
 * nandina.widgets.spacer
 *
 * Spacer — 弹性空白组件。
 *
 * 职责：
 * - 在 Row/Column 等 Flex 布局中占据剩余空间
 * - 不进行任何绘制（完全透明）
 * - 通过 flex_factor 控制弹性比例
 *
 * 设计：
 * - 继承 NanWidget，不进行任何绘制（on_draw 为空）
 * - 覆盖 flex_factor() 返回指定 flex 值（默认 1）
 * - 支持链式 set_flex(factor) 设置弹性因子
 *
 * 用法：
 *   // 左侧按钮，中间空白，右侧按钮
 *   auto row = Row::create();
 *   row->add_child(Button::create().set_text("Left"));
 *   row->add_child(Spacer::create());           // 弹性空白，占用剩余空间
 *   row->add_child(Button::create().set_text("Right"));
 *
 *   // 自定义弹性比例
 *   row->add_child(Spacer::create().set_flex(2));  // 占 2 份弹性空间
 */
export namespace nandina::widgets {

    /**
     * Spacer — 弹性空白组件。
     *
     * 在 Flex 布局中占据剩余空间。不进行绘制。
     * 支持通过 set_flex(factor) 控制弹性比例。
     *
     * 用法：
     *   auto spacer = Spacer::create().set_flex(1);
     *   parent->add_child(std::move(spacer));
     */
    class Spacer : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Spacer>;

        ~Spacer() override = default;

        static auto create() -> Ptr {
            return Ptr{new Spacer()};
        }

        /**
         * @brief 设置弹性因子
         * @param factor 弹性因子（0 = 固定，> 0 = 弹性扩展）
         * @return Spacer& 支持链式调用
         */
        auto set_flex(int factor) -> Spacer& {
            m_flex = factor;
            return *this;
        }

        /**
         * @brief 返回弹性因子
         */
        [[nodiscard]] auto flex_factor() const noexcept -> int override {
            return m_flex;
        }

        /**
         * @brief Spacer 首选尺寸为 0
         *
         * Spacer 不贡献 preferred_size，完全由 flex 布局分配空间。
         */
        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return geometry::NanSize{0.0f, 0.0f};
        }

    protected:
        // Spacer 不进行任何绘制
        void on_draw(tvg::SwCanvas& /*canvas*/) override {
            // 无操作 — Spacer 是完全透明的空白
        }

    private:
        Spacer() = default;

        int m_flex{1};
    };

} // namespace nandina::widgets