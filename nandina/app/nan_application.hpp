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

namespace nandina::render
{
    class IImageDecoder;
} // namespace nandina::render
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
    class ISystemPreferencesSource;

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

        /// 挂载应用自带的资源后端（例如 MemoryBackend/DirectoryBackend/自定义后端）。
        /// 必须在窗口创建与第一次字体/图片/主题资源消费之前调用；后端由应用保活。
        /// 已配置 resource_package 的自动发现与本入口互不排斥，priority 高者优先。
        void mount_resource_backend(
            std::shared_ptr<resource::IResourceBackend> backend,
            int priority = 0
        );
        [[nodiscard]] auto font_loader() -> text::FontLoader&;
        [[nodiscard]] auto font_families() -> text::FontFamilyRegistry&;
        [[nodiscard]] auto application_id() const noexcept -> std::string_view;
        [[nodiscard]] auto platform_paths() const -> const resource::PlatformResourceLocator&;
        /// 图片解码服务。默认是后端无关的 CPU 解码器（stb），应用可替换成自己的实现。
        [[nodiscard]] auto image_decoder() -> std::shared_ptr<render::IImageDecoder>;
        void set_image_decoder(std::shared_ptr<render::IImageDecoder> decoder);
        [[nodiscard]] auto dispatcher() -> UiDispatcher&;
        [[nodiscard]] auto background_executor() -> BackgroundExecutor&;

        template<typename StoreT, typename... Args>
            requires std::derived_from<StoreT, NanStore>
        auto use_store(Args&&... args) -> StoreT& {
            store_ = std::make_unique<StoreT>(graph_, std::forward<Args>(args)...);
            store_key_ = store_key<StoreT>();
            return static_cast<StoreT&>(*store_);
        }

        template<typename StoreT>
            requires std::derived_from<StoreT, NanStore>
        [[nodiscard]] auto store() -> StoreT& {
            if (store_ == nullptr || store_key_ != store_key<StoreT>()) {
                throw std::runtime_error(
                    "NanApplication::store: requested store type is not installed"
                );
            }
            return static_cast<StoreT&>(*store_);
        }

        [[nodiscard]] auto store_base() -> NanStore*;
        [[nodiscard]] auto store_type_key() const -> StoreKey;

        // ---- OS 偏好（无障碍文字缩放，P4.6 Step 3） ----

        /// 注入 OS 偏好来源。仅在尚未注入时生效（应用注入优先于窗口打开时自动接入的平台
        /// 来源）；传 nullptr 是空操作，解除用 `detach_system_preferences()`。不接管所有权。
        void attach_system_preferences(ISystemPreferencesSource* source);
        /// 解除 OS 偏好来源并回到 1.0（保留显式 override）。
        void detach_system_preferences() noexcept;
        [[nodiscard]] auto system_preferences_source() const noexcept -> ISystemPreferencesSource*;
        /// 立即读取来源快照并应用（窗口打开、首帧构建前调用）。返回有效缩放是否改变。
        auto sync_system_preferences() -> bool;
        /// 节流轮询来源变化并应用（宿主在帧首调用；未变化时无 theme revision）。
        auto poll_system_preferences() -> bool;
        /// 运行时/应用级文字缩放覆盖：nullopt = 跟随 OS 来源（无来源回退 1.0）；非法值按
        /// 1.0 处理。`max_text_px` 是主题字号 cap（逻辑 px，0 = 关闭），与 OS 无关。
        void set_text_scale_override(std::optional<float> scale, float max_text_px = 0.0F);
        [[nodiscard]] auto text_scale_override() const noexcept -> std::optional<float>;
        [[nodiscard]] auto effective_text_scale() const noexcept -> float;

        /// 进入阻塞主循环: 打开窗口, 每帧 tick, 直到窗口关闭。返回进程退出码。
        auto run(NanWindow& window) -> int;

        /// 用指定 Page 创建普通单页窗口并进入主循环，无需为一次性 setup 继承 NanWindow。
        template<typename PageT>
            requires std::derived_from<PageT, Page<typename PageT::Params>>
            && std::default_initializable<PageT>
        auto run_page(WindowConfig config) -> int {
            return run_configured(std::move(config), [](NanRouter& router) {
                const auto configured = router.configure(
                    Routes {
                        route<PageT>({.address = "main", .title = "main page"}),
                    }
                );
                if (!configured) {
                    throw std::logic_error(
                        "NanApplication::run_page: cannot configure route: "
                        + describe(configured.error())
                    );
                }
                if (!router.template start<PageT>()) {
                    throw std::runtime_error("NanApplication::run_page: failed to start page");
                }
            });
        }

        /// 带强类型参数启动首页。高级窗口钩子仍通过显式 NanWindow 子类提供。
        template<typename PageT>
            requires std::derived_from<PageT, Page<typename PageT::Params>>
            && std::constructible_from<PageT, typename PageT::Params>
        auto run_page(WindowConfig config, typename PageT::Params params) -> int {
            return run_configured(
                std::move(config),
                [params = std::move(params)](NanRouter& router) mutable {
                    const auto configured = router.configure(
                        Routes {
                            route<PageT>({.address = "main", .title = "main page"}),
                        }
                    );
                    if (!configured) {
                        throw std::logic_error(
                            "NanApplication::run_page: cannot configure route: "
                            + describe(configured.error())
                        );
                    }
                    if (!router.template start<PageT>(std::move(params))) {
                        throw std::runtime_error("NanApplication::run_page: failed to start page");
                    }
                }
            );
        }

        /// Run an ordinary single-window application from a root view factory.
        /// The root factory receives only BuildContext; applications that need
        /// routing or stores should use a NanWindow with typed routes.
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
        /// override > OS > 1.0 的实际应用点：只在有效值/cap 变化时发布 theme revision。
        auto apply_effective_text_scale() -> bool;

        UiDispatcher dispatcher_;
        BackgroundExecutor background_executor_;
        reactive::Graph graph_;
        theme::ThemeManager theme_manager_;
        std::string application_id_;
        std::shared_ptr<render::IImageDecoder> image_decoder_;
        std::optional<resource::PlatformResourceLocator> platform_paths_;
        resource::ResourceManager resources_;
        std::unique_ptr<text::FontLoader> font_loader_;
        text::FontFamilyRegistry font_families_;
        std::vector<std::shared_ptr<resource::IResourceBackend>> resource_backends_;
        std::unique_ptr<NanStore> store_;
        StoreKey store_key_ = {};

        ISystemPreferencesSource* system_preferences_ = nullptr;
        std::optional<float> os_text_scale_;
        std::optional<float> text_scale_override_;
        float text_scale_max_px_ = 0.0F;
        float effective_text_scale_ = 1.0F;
        std::optional<float> applied_text_scale_;
        float applied_max_text_px_ = 0.0F;
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
        requires std::derived_from<PageT, Page<typename PageT::Params>>
        && std::default_initializable<PageT>
    auto run(RunConfig config) -> int {
        NanApplication application(NanApplicationConfig::for_process(std::move(config.id)));
        return application.template run_page<PageT>(std::move(config.window));
    }

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_APPLICATION_HPP
