//
// Created by cvrain on 2026/7/4.
//

#include "nan_application.hpp"
#include "nan_window.hpp"

#include "../foundation/nan_logger.hpp"
#include "../resource/backends/builtin_backend.hpp"
#include "../resource/backends/sqlite_backend.hpp"
#include "../resource/build_location.hpp"
#include "../resource/platform_resource_locator.hpp"
#include "../text/system_fonts.hpp"

namespace nandina::app
{

    namespace
    {
        class ConfiguredWindow final: public NanWindow {
        public:
            ConfiguredWindow(
                NanApplication& application,
                WindowConfig config,
                std::move_only_function<void(NanRouter&)> setup
            ):
                NanWindow(application, std::move(config)),
                setup_(std::move(setup)) {}

        protected:
            void on_setup() override {
                auto setup = std::move(setup_);
                std::invoke(setup, use_router());
            }

        private:
            std::move_only_function<void(NanRouter&)> setup_;
        };

        void install_builtin_services(
            resource::ResourceManager& resources,
            std::vector<std::shared_ptr<resource::IResourceBackend>>& backends,
            text::FontFamilyRegistry& families
        ) {
            auto builtin = resource::BuiltinBackend::create();
            (void)resources.mount(builtin, -1000);
            backends.push_back(std::move(builtin));
            auto registered = text::register_builtin_default_font_family(families);
            if (!registered) {
                throw std::runtime_error(
                    "NanApplication: cannot register built-in default font family"
                );
            }
            // 系统字体回退：发现并挂载系统 CJK 字体作为默认回退；找不到时静默降级。
            (void)text::register_system_cjk_fallback(resources, families);
        }
    } // namespace

    NanApplication::NanApplication() {
        log::initialize();
        install_builtin_services(resources_, resource_backends_, font_families_);
        font_loader_ = std::make_unique<text::FontLoader>(resources_);
        log::get("app.application").info("NanApplication: initialized");
    }

    NanApplication::NanApplication(NanApplicationConfig config):
        application_id_(std::move(config.application_id)) {
        log::initialize({.name = application_id_.empty() ? "nandina" : application_id_});
        install_builtin_services(resources_, resource_backends_, font_families_);
        auto locator = resource::PlatformResourceLocator::create({
            .application_id = application_id_,
            .executable_path = std::move(config.executable_path),
            .environment = std::move(config.environment),
        });
        if (!locator) {
            throw std::invalid_argument("NanApplication: " + locator.error());
        }
        platform_paths_ = std::move(*locator);
        int priority = 1000;
        for (const auto& location: platform_paths_->resource_roots()) {
            // 开发期：优先消费 build-tree 元数据 resource-location.json，挂载其指向的
            // 构建树包；release/portable staging 无此文件时回落直查 `<root>/resources.db`。
            std::filesystem::path database = location.root / config.resource_package;
            std::filesystem::path external_root = location.root;
            const auto metadata_path = location.root / "resource-location.json";
            std::error_code metadata_error;
            if (std::filesystem::exists(metadata_path, metadata_error) && !metadata_error) {
                if (const auto metadata = resource::read_build_location_metadata(metadata_path)) {
                    database = metadata->package_root / metadata->database;
                    external_root = metadata->package_root;
                }
                else {
                    log::get("app.application")
                        .warn("NanApplication: {}: {}", metadata_path.string(), metadata.error());
                }
            }
            std::error_code error;
            if (!std::filesystem::exists(database, error)) {
                if (error) {
                    throw std::runtime_error(
                        "NanApplication: cannot inspect resource package path"
                    );
                }
                --priority;
                continue;
            }
            auto backend = resource::SQLiteBackend::open({
                .name = "package:" + database.string(),
                .database = database,
                .external_root = external_root,
            });
            if (!backend) {
                throw std::runtime_error(
                    "NanApplication: cannot mount resource package " + database.string() + ": "
                    + backend.error().message
                );
            }
            (void)resources_.mount(*backend, priority);
            resource_backends_.push_back(std::move(*backend));
            --priority;
        }
        font_loader_ = std::make_unique<text::FontLoader>(resources_);
        log::get("app.application").info("NanApplication: initialized {}", application_id_);
    }

    NanApplication::~NanApplication() {
        background_executor_.shutdown();
        dispatcher_.shutdown();
    }

    auto NanApplication::graph() -> reactive::Graph& {
        return graph_;
    }

    void NanApplication::set_theme(theme::NanTheme theme) {
        theme_manager_.set_theme(std::move(theme));
    }

    auto NanApplication::theme() const -> const theme::NanTheme& {
        return theme_manager_.theme();
    }

    auto NanApplication::theme_manager() -> theme::ThemeManager& {
        return theme_manager_;
    }

    auto NanApplication::theme_manager() const -> const theme::ThemeManager& {
        return theme_manager_;
    }

    auto NanApplication::apply_styles(const theme::StyleDocument& document)
        -> std::expected<void, std::string> {
        return document.apply(theme_manager_, &font_families_);
    }

    auto NanApplication::load_styles(const std::filesystem::path& path)
        -> std::expected<void, std::string> {
        auto document = theme::load_style_document(path);
        if (!document) {
            return std::unexpected(document.error());
        }
        return apply_styles(*document);
    }

    auto NanApplication::resources() -> resource::ResourceManager& {
        return resources_;
    }
    auto NanApplication::resources() const -> const resource::ResourceManager& {
        return resources_;
    }
    auto NanApplication::font_loader() -> text::FontLoader& {
        return *font_loader_;
    }
    auto NanApplication::font_families() -> text::FontFamilyRegistry& {
        return font_families_;
    }
    auto NanApplication::application_id() const noexcept -> std::string_view {
        return application_id_;
    }

    auto NanApplication::platform_paths() const -> const resource::PlatformResourceLocator& {
        if (!platform_paths_) {
            throw std::logic_error(
                "NanApplication::platform_paths: application was not created with process paths"
            );
        }
        return *platform_paths_;
    }

    auto NanApplication::dispatcher() -> UiDispatcher& {
        return dispatcher_;
    }

    auto NanApplication::background_executor() -> BackgroundExecutor& {
        return background_executor_;
    }

    auto NanApplication::store_base() -> NanStore* {
        return store_.get();
    }

    auto NanApplication::store_type_key() const -> NanTypeKey {
        return store_key_;
    }

    auto NanApplication::run(NanWindow& window) -> int {
        dispatcher_.require_ui_thread();
        window.open();
        while (!window.should_close()) {
            window.tick();
        }
        window.close();
        log::get("app.application").info("NanApplication: exited");
        return 0;
    }

    auto NanApplication::run_configured(WindowConfig config, WindowSetup setup) -> int {
        if (!setup) {
            throw std::invalid_argument("NanApplication::run_page: setup is empty");
        }
        ConfiguredWindow window {*this, std::move(config), std::move(setup)};
        return run(window);
    }

} // namespace nandina::app
