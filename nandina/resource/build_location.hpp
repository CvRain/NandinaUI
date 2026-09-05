#ifndef NANDINA_EXPERIMENT_RESOURCE_BUILD_LOCATION_HPP
#define NANDINA_EXPERIMENT_RESOURCE_BUILD_LOCATION_HPP

#include <expected>
#include <filesystem>
#include <string>

namespace nandina::resource
{
    /// Dev-only build-tree package metadata recorded by the nanres build helper
    /// (`resource-location.json`, written beside `resources.db`).
    struct BuildLocationMetadata {
        std::string package_id;
        std::filesystem::path package_root;
        std::string database;
    };

    /// 读取 nanres build helper 生成的 resource-location.json（build-tree 开发元数据）。
    /// 返回包 id / 构建树包根 / 数据库文件名。文件缺失或格式非法时返回 unexpected。
    /// 这是开发期查找构建树包的辅助；release/portable staging 仍直查 resources.db。
    [[nodiscard]] auto read_build_location_metadata(const std::filesystem::path& json_path)
        -> std::expected<BuildLocationMetadata, std::string>;
} // namespace nandina::resource

#endif
