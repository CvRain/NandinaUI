//
// resource/build_location — read the nanres dev build-tree package metadata.
//

#include "build_location.hpp"

#include "../foundation/json.hpp"

#include <fstream>
#include <sstream>

namespace nandina::resource
{
    auto read_build_location_metadata(const std::filesystem::path& json_path)
        -> std::expected<BuildLocationMetadata, std::string> {
        std::ifstream file(json_path, std::ios::binary);
        if (!file) {
            return std::unexpected("cannot open build location metadata: " + json_path.string());
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();

        auto document = foundation::parse_json(buffer.str());
        if (!document) {
            return std::unexpected("invalid build location JSON: " + document.error());
        }
        try {
            const auto package_id = document->at("package_id").get<std::string>();
            const auto package_root =
                std::filesystem::path(document->at("package_root").get<std::string>());
            const auto database = document->value("database", std::string("resources.db"));
            if (package_id.empty() || package_root.empty()) {
                return std::unexpected(
                    "build location metadata is missing package_id or package_root"
                );
            }
            return BuildLocationMetadata {
                .package_id = package_id,
                .package_root = package_root,
                .database = database,
            };
        }
        catch (const nlohmann::json::exception& error) {
            return std::unexpected(std::string("invalid build location JSON: ") + error.what());
        }
    }
} // namespace nandina::resource
