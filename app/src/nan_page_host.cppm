module;

#include <memory>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.app.page_host;

import nandina.app.application;
import nandina.app.router;
import nandina.foundation.nan_constraints;
import nandina.foundation.nan_size;
import nandina.runtime.nan_widget;

export namespace nandina::app {

    /**
     * NanPageHost — 页面宿主容器（Issue 066）
     *
     * 职责：
     *   - 持有 NanRouter 引用，订阅导航事件
     *   - 在导航发生时延迟替换当前页面组件（在下次 draw 前执行，避免在事件回调中修改树）
     *   - 透明地将自身 bounds 传递给当前页面组件
     *   - 作为 NanComponent 参与父级布局
     *
     * 典型用法：
     *   auto host = std::make_unique<NanPageHost>(router);
     *   parent->add_child(std::move(host));
     */
    class NanPageHost final : public NanComponent {
    public:
        explicit NanPageHost(NanRouter::Ptr router)
            : m_router(std::move(router)) {
            if (!m_router) return;

            // 初始页面立即构造
            load_initial_page();

            // 订阅后续导航事件 —— 仅设置 pending flag，实际替换在 draw 时执行
            m_router->on_navigate([this](std::string_view /*key*/) {
                m_pending_swap = true;
                mark_layout_dirty();
                mark_dirty();
            });
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            if (m_current) {
                m_current->runtime::NanWidget::set_bounds(x, y, w, h);
                flush_layout();
            }
            return *this;
        }

        void draw(tvg::SwCanvas& canvas) override {
            // 延迟页面替换：在 draw 入口完成，避免在事件回调中修改子树
            if (m_pending_swap) {
                swap_to_current_page();
                m_pending_swap = false;
            }
            if (m_current && m_current->is_layout_dirty()) {
                flush_layout();
            }
            NanComponent::draw(canvas);
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return m_current ? m_current->preferred_size() : geometry::NanSize{};
        }

    private:
        auto load_initial_page() -> void {
            if (!m_router) return;
            auto component = m_router->build_current();
            if (component) {
                m_current = static_cast<NanComponent*>(add_child(std::move(component)));
            }
        }

        auto swap_to_current_page() -> void {
            if (!m_router) return;

            // 清除旧页面（clear_children 会重置 parent 指针并 mark_layout_dirty）
            clear_children();
            m_current = nullptr;

            auto component = m_router->build_current();
            if (component) {
                m_current = static_cast<NanComponent*>(add_child(std::move(component)));
                // 立即分配当前 bounds 给新页面
                if (m_current) {
                    m_current->runtime::NanWidget::set_bounds(x(), y(), width(), height());
                    flush_layout();
                }
            }
        }

        auto flush_layout() -> void {
            if (!m_current) return;
            m_current->measure(geometry::NanConstraints::tight(width(), height()));
            m_current->layout();
        }

        NanRouter::Ptr  m_router;
        NanComponent*   m_current{nullptr};
        bool            m_pending_swap{false};
    };

} // namespace nandina::app
