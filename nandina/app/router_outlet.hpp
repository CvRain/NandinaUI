//
// app/router_outlet — the stable page mount owned by a window shell.
//

#ifndef NANDINA_EXPERIMENT_APP_ROUTER_OUTLET_HPP
#define NANDINA_EXPERIMENT_APP_ROUTER_OUTLET_HPP

#include "../scene/control.hpp"

#include <memory>

namespace nandina::app
{
    /// A shell-owned mount point for router content.
    ///
    /// 页面替换由 Outlet 自己负责：Shell 只把它放进布局，不直接增删页面节点。
    /// 换页是原子的——旧页面先摘、新页面后挂，任何时刻最多一个页面子节点，
    /// 因此不会出现"切换期两个子节点"的中间态。
    class RouterOutlet final: public scene::NanControl {
    public:
        /// 安装当前页面根节点。
        ///
        /// 在场景树遍历阶段（process / layout / post_layout / paint）调用时不能立即
        /// 改 `children_`，此时排入一次延迟换页；**同一阶段内多次调用只保留最后一次**，
        /// 于是连续换页在 flush 时合并，而不会排队成多个各自捕获了陈旧 current 的替换。
        /// 返回被请求的页面节点（延迟期间它尚未挂载，可用 `pending_page()` 区分）。
        auto set_page(std::shared_ptr<scene::NanNode2D> page) -> scene::NanNode2D&;

        /// 摘掉当前页面。同样在遍历阶段安全（延迟执行）。
        void clear_page();

        /// 当前**已挂载**的页面根节点，没有则为 nullptr。
        [[nodiscard]] auto page() const -> scene::NanNode2D*;

        /// 已请求但尚未挂载的页面（延迟换页排队期间非空）。
        [[nodiscard]] auto pending_page() const noexcept
            -> const std::shared_ptr<scene::NanNode2D>& {
            return pending_page_;
        }

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;

    private:
        /// 真正执行换页。当前子节点在**调用时刻**解析：延迟排队期间页面可能已经被
        /// 另一次换页替换过，用排队时捕获的 current 会摘错节点、留下僵尸页面。
        void apply_page_swap();

        std::shared_ptr<scene::NanNode2D> pending_page_;
        bool swap_queued_ = false;
    };
} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_ROUTER_OUTLET_HPP
