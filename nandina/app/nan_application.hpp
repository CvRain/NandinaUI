//
// Created by cvrain on 2026/7/4.
//
// app/nan_application — 应用主入口。
//
// NanApplication 是进程级的编排者:
//   - 初始化进程级日志服务;
//   - 持有响应式 Graph —— 它必须比任何 window / widget 活得久, 因为 widget 卸载时
//     会回访 graph (dispose effect / detach source)。App 持有它 = 最长生命周期;
//   - run(window): 进入阻塞主循环, 每帧驱动 window tick, 直到窗口请求关闭。
//
// 用法:
//   NanApplication app;
//   MyWindow window{app, {.title = "Demo"}};
//   return app.run(window);
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_APPLICATION_HPP
#define NANDINA_EXPERIMENT_APP_NAN_APPLICATION_HPP

#include "../reactive/graph.hpp"
#include "../resource/platform_resource_locator.hpp"
#include "../resource/resource_manager.hpp"
#include "../text/font_family.hpp"
#include "../text/font_loader.hpp"
#include "../theme/style_document.hpp"
#include "../theme/theme_manager.hpp"
#include "application_config.hpp"
#include "nan_page.hpp"
#include "nan_router.hpp"
#include "nan_store.hpp"
#include "root_view.hpp"
#include "ui_dispatcher.hpp"
#include "window_config.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace nandina::app
{

    class NanWindow;

    class NanApplication {
    public:
        NanApplication();
        explicit NanApplication(NanApplicationConfig config);
        ~NanApplication();

        NanApplication(const NanApplication&) = delete;
        auto operator=(const NanApplication&) -> NanApplication& = delete;
        NanApplication(NanApplication&&) = delete;
        auto operator=(NanApplication&&) -> NanApplication& = delete;

        /// 响应式调度图。所有 signal / effect 依附其上; 生命周期覆盖整个 app。
        [[nodiscard]] auto graph() -> reactive::Graph&;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme() const -> const theme::NanTheme&;
        [[nodiscard]] auto theme_manager() -> theme::ThemeManager&;
        [[nodiscard]] auto theme_manager() const -> const theme::ThemeManager&;
        [[nodiscard]] auto apply_styles(const theme::StyleDocument& document)
            -> std::expected<void, std::string>;
        [[nodiscard]] auto load_styles(const std::filesystem::path& path)
            -> std::expected<void, std::string>;
        [[nodiscard]] auto resources() -> resource::ResourceManager&;
        [[nodiscard]] auto resources() const -> const resource::ResourceManager&;
        [[nodiscard]] auto font_loader() -> text::FontLoader&;
        [[nodiscard]] auto font_families() -> text::FontFamilyRegistry&;
        [[nodiscard]] auto application_id() const noexcept -> std::string_view;
        [[nodiscard]] auto platform_paths() const -> const resource::PlatformResourceLocator&;
        [[nodiscard]] auto dispatcher() -> UiDispatcher&;
        [[nodiscard]] auto background_executor() -> BackgroundExecutor&;

        template<typename StoreT, typename... Args>
            requires std::derived_from<StoreT, NanStore>
        auto use_store(Args&&... args) -> StoreT& {
            store_ = std::make_unique<StoreT>(graph_, std::forward<Args>(args)...);
            store_key_ = nan_type_key<StoreT>();
            return static_cast<StoreT&>(*store_);
        }

        template<typename StoreT>
            requires std::derived_from<StoreT, NanStore>
        [[nodiscard]] auto store() -> StoreT& {
            if (store_ == nullptr || store_key_ != nan_type_key<StoreT>()) {
                throw std::runtime_error(
                    "NanApplication::store: requested store type is not installed"
                );
            }
            return static_cast<StoreT&>(*store_);
        }

        [[nodiscard]] auto store_base() -> NanStore*;
        [[nodiscard]] auto store_type_key() const -> NanTypeKey;

        /// 进入阻塞主循环: 打开窗口, 每帧 tick, 直到窗口关闭。返回进程退出码。
        auto run(NanWindow& window) -> int;

        /// 用指定 Page 创建普通路由窗口并进入主循环，无需为一次性 setup 继承 NanWindow。
        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        auto run_page(WindowConfig config) -> int {
            return run_configured(std::move(config), [](NanRouter& router) {
                router.template push<PageT>();
            });
        }

        /// 带强类型参数启动首页。高级窗口钩子仍通过显式 NanWindow 子类提供。
        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::constructible_from<PageT, typename PageT::Params>
        auto run_page(WindowConfig config, typename PageT::Params params) -> int {
            return run_configured(
                std::move(config),
                [params = std::move(params)](NanRouter& router) mutable {
                    router.template push<PageT>(std::move(params));
                }
            );
        }

        /// Run an ordinary single-window application from a root view factory.
        /// PageContext factories can use routing and stores; BuildContext factories
        /// are the compact path for a self-contained view.
        template<typename Factory>
            requires RootViewFactory<std::decay_t<Factory>>
        auto run(WindowConfig config, Factory&& factory) -> int {
            return run_page<detail::RootViewPage>(
                std::move(config),
                detail::make_root_view_params(std::forward<Factory>(factory))
            );
        }

    private:
        using WindowSetup = std::move_only_function<void(NanRouter&)>;

        auto run_configured(WindowConfig config, WindowSetup setup) -> int;

        UiDispatcher dispatcher_;
        BackgroundExecutor background_executor_;
        reactive::Graph graph_;
        theme::ThemeManager theme_manager_;
        std::string application_id_;
        std::optional<resource::PlatformResourceLocator> platform_paths_;
        resource::ResourceManager resources_;
        std::unique_ptr<text::FontLoader> font_loader_;
        text::FontFamilyRegistry font_families_;
        std::vector<std::shared_ptr<resource::IResourceBackend>> resource_backends_;
        std::unique_ptr<NanStore> store_;
        NanTypeKey store_key_ = nullptr;
    };

    struct RunConfig {
        std::string id;
        WindowConfig window;
    };

    /// Minimal process entry point. Explicit NanApplication remains available
    /// when startup needs stores, resource registration, or theme configuration.
    template<typename Factory>
        requires RootViewFactory<std::decay_t<Factory>>
    auto run(RunConfig config, Factory&& factory) -> int {
        NanApplication application(NanApplicationConfig::for_process(std::move(config.id)));
        return application.run(std::move(config.window), std::forward<Factory>(factory));
    }

    /// Recommended application entry: the first page is also the future router root.
    template<typename PageT>
        requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
        && std::default_initializable<PageT>
    auto run(RunConfig config) -> int {
        NanApplication application(NanApplicationConfig::for_process(std::move(config.id)));
        return application.template run_page<PageT>(std::move(config.window));
    }

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_APPLICATION_HPP
