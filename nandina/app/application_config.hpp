#ifndef NANDINA_EXPERIMENT_APP_APPLICATION_CONFIG_HPP
#define NANDINA_EXPERIMENT_APP_APPLICATION_CONFIG_HPP

#include <filesystem>
#include <map>
#include <string>

namespace nandina::app
{
    struct NanApplicationConfig {
        std::string application_id;
        std::filesystem::path executable_path;
        std::map<std::string, std::string, std::less<>> environment;
        /// 资源包文件名（相对于定位到的资源根）。**默认为空：启动不自动发现/打开包**；
        /// 需要自动开包时显式设为 "resources.db"，或使用 NanApplication::mount_resource_backend()
        /// 注入自定义后端。空值不是"回退到默认 resources.db"。
        std::string resource_package;

        [[nodiscard]] static auto for_process(std::string application_id) -> NanApplicationConfig;
    };
} // namespace nandina::app

#endif
