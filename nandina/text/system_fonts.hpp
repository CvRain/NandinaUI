//
// text/system_fonts - cross-platform system font discovery + CJK fallback.
//
// 发现系统字体目录中的 CJK 字体（思源黑体 / Noto Sans CJK / Sarasa / 苹方 /
// 微软雅黑 / 文泉驿等），按已知文件名优先级挑选，挂载为资源并注册成默认
// 回退字体。这样内置的拉丁默认字体遇到 CJK 码位时自动落到系统 CJK 字体，
// 避免「豆腐块」。找不到时静默降级（返回 false）。
//

#ifndef NANDINA_EXPERIMENT_TEXT_SYSTEM_FONTS_HPP
#define NANDINA_EXPERIMENT_TEXT_SYSTEM_FONTS_HPP

#include "font_family.hpp"

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace nandina::text
{
    /// 按平台返回系统字体目录（供发现扫描）。
    [[nodiscard]] auto system_font_directories() -> std::vector<std::filesystem::path>;

    /// 在给定目录中按已知 CJK 字体文件名优先级查找，返回首个匹配的字体文件。
    [[nodiscard]] auto find_cjk_font_in(const std::vector<std::filesystem::path>& directories)
        -> std::optional<std::filesystem::path>;

    /// 在给定目录中按文件名子串（大小写不敏感）查找首个字体文件。
    [[nodiscard]] auto find_system_font_in(
        const std::vector<std::filesystem::path>& directories,
        std::string_view file_hint
    ) -> std::optional<std::filesystem::path>;

    /// 便捷入口：在系统字体目录中查找 CJK 字体。
    [[nodiscard]] auto find_system_cjk_font() -> std::optional<std::filesystem::path>;

    /// 便捷入口：在系统字体目录中按文件名子串查找首个字体文件。
    [[nodiscard]] auto find_system_font(std::string_view file_hint)
        -> std::optional<std::filesystem::path>;

    /// 把系统 CJK 字体挂载为资源并注册为默认回退。无 CJK 字体时返回 false。
    [[nodiscard]] auto
    register_system_cjk_fallback(resource::ResourceManager& resources, FontFamilyRegistry& registry)
        -> FontResult<bool>;
} // namespace nandina::text

#endif // NANDINA_EXPERIMENT_TEXT_SYSTEM_FONTS_HPP
