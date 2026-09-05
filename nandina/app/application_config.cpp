#include "application_config.hpp"

#include <array>
#include <cstdlib>
#include <stdexcept>

#if defined(__linux__)
    #include <unistd.h>
#endif

namespace nandina::app
{
    auto NanApplicationConfig::for_process(std::string application_id) -> NanApplicationConfig {
        NanApplicationConfig result {.application_id = std::move(application_id)};
#if defined(__linux__)
        std::array<char, 4096> path {};
        const auto count = readlink("/proc/self/exe", path.data(), path.size() - 1);
        if (count <= 0 || static_cast<std::size_t>(count) >= path.size()) {
            throw std::runtime_error("NanApplicationConfig: cannot resolve executable path");
        }
        result.executable_path = std::string(path.data(), static_cast<std::size_t>(count));
#else
        // todo 需适配windows使用（主要问题在nanres上，nanres还是作为扩展选项使用更好）
        // std::format 在windows上可能出现报错情况，需要准备fmt库作为备用选项
        // Clangd: In template: call to consteval function 'std::basic_format_arg<std::basic_format_context<std::__format::_Sink_iter<char>, char>>::_S_to_enum<const char *>' is not a constant expression
        throw std::runtime_error(
            "NanApplicationConfig: process discovery is unsupported on this platform"
        );
#endif
        constexpr std::array names {
            "HOME",
            "XDG_CONFIG_HOME",
            "XDG_DATA_HOME",
            "XDG_DATA_DIRS",
            "XDG_STATE_HOME",
            "XDG_CACHE_HOME",
        };
        for (const auto* name: names) {
            if (const auto* value = std::getenv(name); value != nullptr && *value != '\0') {
                result.environment.emplace(name, value);
            }
        }
        return result;
    }
} // namespace nandina::app
