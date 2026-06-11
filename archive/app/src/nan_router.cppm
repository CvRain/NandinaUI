module;

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

export module nandina.app.router;

import nandina.app.application;
import nandina.app.page;

export namespace nandina::app {

    /**
     * NanRouter — 最小路由器（Issue 064/065）
     *
     * 职责：
     *   - 持有已注册页面的所有权
     *   - 维护当前激活的 route_key
     *   - 接受导航请求并广播变更给订阅者
     *   - 按需构造当前页面的 NanComponent
     *
     * 典型使用：
     *   auto router = NanRouter::create();
     *   router->register_page(std::make_unique<OverviewPage>());
     *   router->on_navigate([&](std::string_view key) { ... });
     *   router->navigate_to("overview");
     *   auto component = router->build_current();
     */
    class NanRouter {
    public:
        using Ptr = std::shared_ptr<NanRouter>;

        static auto create() -> Ptr {
            return std::make_shared<NanRouter>();
        }

        // ── 页面注册 ──────────────────────────────────────────

        /// 注册一个页面；第一个注册的页面将成为初始路由
        auto register_page(std::unique_ptr<NanPage> page) -> void {
            if (!page) return;
            if (m_current_key.empty()) {
                m_current_key = std::string{page->route_key()};
            }
            m_pages.push_back(std::move(page));
        }

        // ── 导航 ──────────────────────────────────────────────

        /**
         * 导航到指定 route_key。
         * @return true  — 找到页面并成功切换
         *         false — 未找到对应 key，路由保持不变
         */
        auto navigate_to(std::string_view key) -> bool {
            for (auto& page : m_pages) {
                if (page->route_key() == key) {
                    if (m_current_key == key) return true; // 已在当前页
                    m_current_key = std::string{key};
                    fire_callbacks(m_current_key);
                    return true;
                }
            }
            return false;
        }

        // ── 查询 ──────────────────────────────────────────────

        [[nodiscard]] auto current_key() const noexcept -> std::string_view {
            return m_current_key;
        }

        [[nodiscard]] auto pages() const noexcept -> const std::vector<std::unique_ptr<NanPage>>& {
            return m_pages;
        }

        // ── 订阅导航事件 ──────────────────────────────────────

        /// 每次导航成功后回调，参数为新的 route_key
        auto on_navigate(std::function<void(std::string_view)> cb) -> void {
            if (cb) {
                m_callbacks.push_back(std::move(cb));
            }
        }

        // ── 构造当前页面组件 ──────────────────────────────────

        /// 调用当前页面的 build()，返回新创建的 NanComponent
        [[nodiscard]] auto build_current() -> NanComponent::Ptr {
            for (auto& page : m_pages) {
                if (page->route_key() == m_current_key) {
                    return page->build();
                }
            }
            return nullptr;
        }

    private:
        auto fire_callbacks(std::string_view key) -> void {
            for (auto& cb : m_callbacks) {
                cb(key);
            }
        }

        std::vector<std::unique_ptr<NanPage>> m_pages;
        std::string m_current_key;
        std::vector<std::function<void(std::string_view)>> m_callbacks;
    };

} // namespace nandina::app
