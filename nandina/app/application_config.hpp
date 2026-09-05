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
        std::string resource_package = "resources.db";

        [[nodiscard]] static auto for_process(std::string application_id) -> NanApplicationConfig;
    };
} // namespace nandina::app

#endif
