//
// app/shell_context — services for a window's persistent shell.
//

#ifndef NANDINA_EXPERIMENT_APP_SHELL_CONTEXT_HPP
#define NANDINA_EXPERIMENT_APP_SHELL_CONTEXT_HPP

#include "nan_router.hpp"
#include "router_outlet.hpp"
#include "../widget/build_context.hpp"

namespace nandina::resource
{
    class ResourceManager;
}

namespace nandina::text
{
    class FontLoader;
    class FontFamilyRegistry;
}

namespace nandina::app
{
    /// Build-time services for content that remains mounted while pages change.
    class ShellContext {
    public:
        ShellContext(
            reactive::Graph& graph,
            reactive::ReactiveScope& scope,
            theme::ThemeManager& theme_manager,
            resource::ResourceManager* resources,
            scene::OverlayHost* overlay_host,
            widget::DragController* drag_controller,
            Navigation navigation,
            std::shared_ptr<RouterOutlet> outlet,
            const Routes& routes,
            reactive::Signal<NanTypeKey>& current_page
        ) noexcept:
            graph_(&graph),
            scope_(&scope),
            theme_manager_(&theme_manager),
            resources_(resources),
            overlay_host_(overlay_host),
            drag_controller_(drag_controller),
            navigation_(std::move(navigation)),
            outlet_(std::move(outlet)),
            routes_(&routes),
            current_page_(&current_page) {}

        [[nodiscard]] auto ui() const -> widget::BuildContext {
            return widget::BuildContext(
                *graph_,
                *scope_,
                *theme_manager_,
                resources_,
                overlay_host_,
                drag_controller_
            );
        }

        [[nodiscard]] auto navigation() const -> Navigation { return navigation_; }

        [[nodiscard]] auto outlet() const -> std::shared_ptr<RouterOutlet> { return outlet_; }

        /// 路由表（只读）：外壳拿它当**数据**，据此生成导航项。
        [[nodiscard]] auto routes() const noexcept -> const Routes& { return *routes_; }

        /// 当前页面类型键的响应式来源：导航高亮的事实来源，换页后自动更新。
        /// 外壳在 `set_shell()` 时就会绑定它 —— 那时还没有任何页面，首屏由
        /// 随后的 `start()` 发布出来。
        [[nodiscard]] auto current_page() const noexcept -> reactive::Signal<NanTypeKey>& {
            return *current_page_;
        }

        [[nodiscard]] auto has_overlay_host() const noexcept -> bool {
            return overlay_host_ != nullptr;
        }

        [[nodiscard]] auto overlay_host() const -> scene::OverlayHost& {
            if (overlay_host_ == nullptr) {
                throw std::runtime_error("ShellContext::overlay_host: service is unavailable");
            }
            return *overlay_host_;
        }

    private:
        reactive::Graph* graph_;
        reactive::ReactiveScope* scope_;
        theme::ThemeManager* theme_manager_;
        resource::ResourceManager* resources_;
        scene::OverlayHost* overlay_host_;
        widget::DragController* drag_controller_;
        Navigation navigation_;
        std::shared_ptr<RouterOutlet> outlet_;
        const Routes* routes_ = nullptr;
        reactive::Signal<NanTypeKey>* current_page_ = nullptr;
    };
} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_SHELL_CONTEXT_HPP
