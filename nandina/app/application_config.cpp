#include "application_config.hpp"

#include <array>
#include <cstdlib>
#include <stdexcept>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

namespace nandina::app
{
    auto NanApplicationConfig::for_process(std::string application_id) -> NanApplicationConfig {
        NanApplicationConfig result {.application_id = std::move(application_id)};
#if defined(_WIN32)
        // 宽字符路径：避免非 ASCII 用户名时的 ANSI 转换丢失。
        std::wstring path(32768, L'\0');
        const auto count =
            ::GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (count == 0 || count >= path.size()) {
            throw std::runtime_error("NanApplicationConfig: cannot resolve executable path");
        }
        path.resize(count);
        result.executable_path = std::filesystem::path(path);
#elif defined(__linux__)
        std::array<char, 4096> path {};
        const auto count = readlink("/proc/self/exe", path.data(), path.size() - 1);
        if (count <= 0 || static_cast<std::size_t>(count) >= path.size()) {
            throw std::runtime_error("NanApplicationConfig: cannot resolve executable path");
        }
        result.executable_path = std::string(path.data(), static_cast<std::size_t>(count));
#else
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
#if defined(_WIN32)
        // Windows 实验性适配：沿用 XDG 命名的资源定位器，用 Windows 已知目录补齐
        // （HOME→USERPROFILE，config→APPDATA，其余→LOCALAPPDATA）。这不是官方目录约定，
        // 只为让应用能启动；非 ASCII 路径受 ANSI getenv 限制，尚未验收。
        const auto environment_or = [&result](const char* name) -> std::string {
            const auto found = result.environment.find(name);
            return found == result.environment.end() ? std::string {} : found->second;
        };
        if (!environment_or("HOME").empty()) {
            // 保留显式 HOME（MSYS2/Git Bash 等环境会设置）。
        }
        else if (
            const auto* profile = std::getenv("USERPROFILE"); profile != nullptr && *profile != '\0'
        )
        {
            result.environment.emplace("HOME", profile);
        }
        else {
            throw std::runtime_error("NanApplicationConfig: neither HOME nor USERPROFILE is set");
        }
        const auto provide = [&result](const char* name, const char* fallback) {
            if (result.environment.contains(name)) {
                return;
            }
            if (const auto* value = std::getenv(fallback); value != nullptr && *value != '\0') {
                result.environment.emplace(name, value);
            }
        };
        provide("XDG_CONFIG_HOME", "APPDATA");
        provide("XDG_DATA_HOME", "LOCALAPPDATA");
        provide("XDG_STATE_HOME", "LOCALAPPDATA");
        provide("XDG_CACHE_HOME", "LOCALAPPDATA");
#endif
        return result;
    }
} // namespace nandina::app
