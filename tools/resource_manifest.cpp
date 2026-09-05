#include "resource/resource_manifest.hpp"

#include <openssl/evp.h>
#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

namespace nandina::resource
{
    namespace
    {
        [[nodiscard]] auto storage_from_string(std::string_view value)
            -> std::optional<ManifestStorage> {
            if (value == "auto") { return ManifestStorage::automatic; }
            if (value == "embedded") { return ManifestStorage::embedded; }
            if (value == "external") { return ManifestStorage::external; }
            return std::nullopt;
        }

        [[nodiscard]] auto storage_name(ManifestStorage storage) -> std::string_view {
            switch (storage) {
            case ManifestStorage::automatic: return "auto";
            case ManifestStorage::embedded: return "embedded";
            case ManifestStorage::external: return "external";
            }
            return "auto";
        }

        [[nodiscard]] auto glob_matches(std::string_view pattern, std::string_view value) -> bool {
            std::size_t pattern_index = 0;
            std::size_t value_index = 0;
            std::size_t star = std::string_view::npos;
            std::size_t retry = 0;
            while (value_index < value.size()) {
                if (pattern_index < pattern.size()
                    && (pattern[pattern_index] == '?' || pattern[pattern_index] == value[value_index]))
                {
                    ++pattern_index;
                    ++value_index;
                }
                else if (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
                    star = pattern_index++;
                    retry = value_index;
                }
                else if (star != std::string_view::npos) {
                    pattern_index = star + 1;
                    value_index = ++retry;
                }
                else { return false; }
            }
            while (pattern_index < pattern.size() && pattern[pattern_index] == '*') { ++pattern_index; }
            return pattern_index == pattern.size();
        }

        [[nodiscard]] auto toml_escape(std::string_view value) -> std::string {
            std::string result;
            result.reserve(value.size());
            for (const char character: value) {
                if (character == '\\' || character == '"') { result.push_back('\\'); }
                result.push_back(character);
            }
            return result;
        }

        [[nodiscard]] auto parse_root(const toml::table& table, const std::filesystem::path& base)
            -> std::expected<ResourceScanRoot, std::string> {
            const auto path = table["path"].value<std::string>();
            if (!path || path->empty()) { return std::unexpected("root.path is required"); }
            const auto prefix = table["key_prefix"].value_or(std::string {});
            if (!prefix.empty() && !ResourceKey::parse(prefix)) {
                return std::unexpected("root.key_prefix is not canonical");
            }
            return ResourceScanRoot {(base / *path).lexically_normal(), prefix};
        }

        [[nodiscard]] auto parse_locked_resource(const toml::table& table)
            -> std::expected<LockedResource, std::string> {
            const auto id_text = table["id"].value<std::string>();
            const auto key_text = table["key"].value<std::string>();
            const auto source = table["source"].value<std::string>();
            const auto media_type = table["media_type"].value<std::string>();
            const auto size = table["size"].value<std::int64_t>();
            const auto hash = table["sha256"].value<std::string>();
            const auto storage_text = table["storage"].value<std::string>();
            const auto streaming = table["streaming"].value<bool>();
            const auto revision = table["revision"].value<std::int64_t>();
            const auto id = id_text ? ResourceId::parse(*id_text) : std::nullopt;
            const auto key = key_text ? ResourceKey::parse(*key_text) : std::nullopt;
            const auto storage = storage_text ? storage_from_string(*storage_text) : std::nullopt;
            if (!id || !key || !source || source->empty() || !media_type || !size || *size < 0
                || !hash || hash->size() != 64 || !storage || !streaming || !revision
                || *revision < 1)
            {
                return std::unexpected("lock resource entry is malformed");
            }
            return LockedResource {
                .id = *id,
                .key = *key,
                .source = *source,
                .media_type = *media_type,
                .size = static_cast<std::uint64_t>(*size),
                .sha256 = *hash,
                .storage = *storage,
                .streaming = *streaming,
                .revision = static_cast<std::uint64_t>(*revision),
            };
        }
    }

    auto load_resource_policy(const std::filesystem::path& path)
        -> std::expected<ResourcePolicy, std::string> {
        try {
            const auto document = toml::parse_file(path.string());
            const auto legacy_package_id = document["package_id"].value<std::string>();
            const auto simple_package_id = document["package"].value<std::string>();
            if (legacy_package_id && simple_package_id
                && *legacy_package_id != *simple_package_id) {
                return std::unexpected("package and package_id disagree");
            }
            const auto package_id = legacy_package_id.or_else([&] { return simple_package_id; });
            if (!package_id || !ResourceKey::parse(*package_id) || package_id->contains('/')) {
                return std::unexpected("package_id must be a canonical single segment");
            }
            ResourcePolicy policy {
                .package_id = *package_id,
                .base_directory = std::filesystem::absolute(path).parent_path(),
                .include_hidden = document["include_hidden"].value_or(false),
                .allow_symlinks = document["allow_symlinks"].value_or(false),
                .package_directory = document["package_directory"].value_or(std::string("package")),
                .embed_threshold = static_cast<std::uint64_t>(
                    document["embed_threshold"].value_or<std::int64_t>(1024 * 1024)
                ),
            };
            if (policy.package_directory.empty() || policy.package_directory.is_absolute()
                || policy.embed_threshold == 0)
            {
                return std::unexpected("package_directory or embed_threshold is invalid");
            }
            for (const auto& part: policy.package_directory) {
                if (part == "." || part == "..") {
                    return std::unexpected("package_directory contains traversal");
                }
            }
            const auto* roots = document["roots"].as_array();
            if (roots && !roots->empty()) {
                for (const auto& node: *roots) {
                    const auto* table = node.as_table();
                    if (!table) { return std::unexpected("roots entries must be tables"); }
                    auto root = parse_root(*table, policy.base_directory);
                    if (!root) { return std::unexpected(root.error()); }
                    policy.roots.push_back(std::move(*root));
                }
            }
            else {
                const auto source = document["source"].value_or(std::string("assets"));
                if (source.empty() || std::filesystem::path(source).is_absolute()) {
                    return std::unexpected("source must be a relative non-empty path");
                }
                for (const auto& part: std::filesystem::path(source)) {
                    if (part == "." || part == "..") {
                        return std::unexpected("source contains traversal");
                    }
                }
                policy.roots.push_back({
                    .path = (policy.base_directory / source).lexically_normal(),
                    .key_prefix = {},
                });
            }
            if (const auto* excludes = document["excludes"].as_array()) {
                for (const auto& node: *excludes) {
                    const auto value = node.value<std::string>();
                    if (!value) { return std::unexpected("excludes entries must be strings"); }
                    policy.excludes.push_back(*value);
                }
            }
            if (const auto* rules = document["rules"].as_array()) {
                for (const auto& node: *rules) {
                    const auto* table = node.as_table();
                    if (!table) { return std::unexpected("rules entries must be tables"); }
                    const auto glob = (*table)["glob"].value<std::string>();
                    if (!glob || glob->empty()) { return std::unexpected("rule.glob is required"); }
                    ResourcePolicyRule rule {.glob = *glob};
                    rule.media_type = (*table)["media_type"].value<std::string>();
                    if (const auto storage = (*table)["storage"].value<std::string>()) {
                        rule.storage = storage_from_string(*storage);
                        if (!rule.storage) { return std::unexpected("rule.storage is invalid"); }
                    }
                    rule.streaming = (*table)["streaming"].value<bool>();
                    policy.rules.push_back(std::move(rule));
                }
            }
            if (const auto* overrides = document["resources"].as_array()) {
                for (const auto& node: *overrides) {
                    const auto* table = node.as_table();
                    if (!table) { return std::unexpected("resources entries must be tables"); }
                    const auto key_text = (*table)["key"].value<std::string>();
                    const auto key = key_text ? ResourceKey::parse(*key_text) : std::nullopt;
                    if (!key_text || key_text->empty() || !key) {
                        return std::unexpected("resources[].key must be a canonical key");
                    }
                    ResourceOverride override {};
                    override.media_type = (*table)["media_type"].value<std::string>();
                    if (const auto storage = (*table)["storage"].value<std::string>()) {
                        override.storage = storage_from_string(*storage);
                        if (!override.storage) { return std::unexpected("resources[].storage is invalid"); }
                    }
                    override.streaming = (*table)["streaming"].value<bool>();
                    if (policy.overrides.contains(*key)) {
                        return std::unexpected("duplicate resources override for key: " + key_text.value());
                    }
                    policy.overrides.emplace(std::move(*key), std::move(override));
                }
            }
            if (const auto* aliases = document["aliases"].as_table()) {
                for (const auto& [alias, target_node]: *aliases) {
                    const auto target = target_node.value<std::string>();
                    const auto alias_key = ResourceKey::parse(alias.str());
                    const auto target_key = target ? ResourceKey::parse(*target) : std::nullopt;
                    if (!alias_key || !target_key) { return std::unexpected("alias keys must be canonical"); }
                    policy.aliases.emplace(*alias_key, *target_key);
                }
            }
            return policy;
        }
        catch (const toml::parse_error& error) {
            return std::unexpected(std::string("resources.toml parse error: ") + error.description().data());
        }
    }

    auto load_resource_lock(const std::filesystem::path& path)
        -> std::expected<ResourceLockManifest, std::string> {
        try {
            const auto document = toml::parse_file(path.string());
            const auto format = document["format"].value<std::int64_t>();
            const auto package_id = document["package_id"].value<std::string>();
            if (!format || *format != 1 || !package_id) {
                return std::unexpected("lock manifest header is malformed");
            }
            ResourceLockManifest result {.package_id = *package_id};
            if (const auto* resources = document["resources"].as_array()) {
                std::set<ResourceId> ids;
                std::set<ResourceKey> keys;
                for (const auto& node: *resources) {
                    const auto* table = node.as_table();
                    if (!table) { return std::unexpected("lock resources entries must be tables"); }
                    auto resource = parse_locked_resource(*table);
                    if (!resource) { return std::unexpected(resource.error()); }
                    if (!ids.insert(resource->id).second || !keys.insert(resource->key).second) {
                        return std::unexpected("lock manifest contains duplicate identities");
                    }
                    result.resources.push_back(std::move(*resource));
                }
            }
            return result;
        }
        catch (const toml::parse_error& error) {
            return std::unexpected(std::string("resources.lock.toml parse error: ") + error.description().data());
        }
    }

    auto sha256_file(const std::filesystem::path& path)
        -> std::expected<std::string, std::string> {
        std::ifstream input(path, std::ios::binary);
        if (!input) { return std::unexpected("cannot open resource for hashing: " + path.string()); }
        EVP_MD_CTX* raw_context = EVP_MD_CTX_new();
        if (!raw_context) { return std::unexpected("cannot allocate SHA-256 context"); }
        const auto context = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>(raw_context, EVP_MD_CTX_free);
        if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1) {
            return std::unexpected("cannot initialize SHA-256");
        }
        std::array<char, 64 * 1024> buffer {};
        while (input) {
            input.read(buffer.data(), buffer.size());
            const auto count = input.gcount();
            if (count > 0 && EVP_DigestUpdate(context.get(), buffer.data(), count) != 1) {
                return std::unexpected("cannot update SHA-256");
            }
        }
        if (!input.eof()) { return std::unexpected("resource read failed while hashing"); }
        std::array<unsigned char, EVP_MAX_MD_SIZE> digest {};
        unsigned int size = 0;
        if (EVP_DigestFinal_ex(context.get(), digest.data(), &size) != 1 || size != 32) {
            return std::unexpected("cannot finalize SHA-256");
        }
        std::ostringstream output;
        output << std::hex << std::setfill('0');
        for (unsigned int index = 0; index < size; ++index) {
            output << std::setw(2) << static_cast<unsigned int>(digest[index]);
        }
        return output.str();
    }

    auto resource_scan_options(const ResourcePolicy& policy) -> ResourceScanOptions {
        ResourceScanOptions options {
            .roots = policy.roots,
            .excludes = policy.excludes,
            .output_paths = {
                policy.base_directory / "resources.lock.toml",
                policy.base_directory / policy.package_directory,
            },
            .include_hidden = policy.include_hidden,
            .allow_symlinks = policy.allow_symlinks,
        };
        for (const auto& rule: policy.rules) {
            if (rule.media_type) { options.type_rules.push_back({rule.glob, *rule.media_type}); }
        }
        return options;
    }

    auto build_resource_lock(
        const ResourcePolicy& policy,
        const ResourceScanResult& scan,
        const std::optional<ResourceLockManifest>& previous
    ) -> std::expected<ResourceLockManifest, std::string> {
        if (!scan.valid()) { return std::unexpected("cannot build lock manifest from an invalid scan"); }
        if (previous && previous->package_id != policy.package_id) {
            return std::unexpected("lock package_id does not match resources.toml");
        }
        std::map<std::string, const LockedResource*> by_source;
        std::map<std::string, std::vector<const LockedResource*>> by_hash;
        if (previous) {
            for (const auto& item: previous->resources) {
                by_source.emplace(item.source.generic_string(), &item);
                by_hash[item.sha256].push_back(&item);
            }
        }
        ResourceLockManifest result {.package_id = policy.package_id};
        std::set<ResourceId> claimed;
        for (const auto& scanned: scan.resources) {
            auto hash = sha256_file(scanned.source_path);
            if (!hash) { return std::unexpected(hash.error()); }
            const auto relative = std::filesystem::relative(scanned.source_path, policy.base_directory)
                                      .lexically_normal();
            ManifestStorage storage = ManifestStorage::automatic;
            bool streaming = false;
            std::string media_type = scanned.media_type;
            for (const auto& rule: policy.rules) {
                if (!glob_matches(rule.glob, scanned.key.value())) { continue; }
                if (rule.storage) { storage = *rule.storage; }
                if (rule.streaming) { streaming = *rule.streaming; }
            }
            // per-key overrides win over glob rules（`[[resources]]`）。
            if (const auto found = policy.overrides.find(scanned.key); found != policy.overrides.end()) {
                if (found->second.storage) { storage = *found->second.storage; }
                if (found->second.streaming) { streaming = *found->second.streaming; }
                if (found->second.media_type) { media_type = *found->second.media_type; }
            }
            const LockedResource* identity = nullptr;
            if (const auto found = by_source.find(relative.generic_string()); found != by_source.end()) {
                identity = found->second;
            }
            else if (const auto found = by_hash.find(*hash); found != by_hash.end()) {
                std::vector<const LockedResource*> available;
                std::ranges::copy_if(found->second, std::back_inserter(available), [&](const auto* item) {
                    return !claimed.contains(item->id);
                });
                if (available.size() > 1) {
                    return std::unexpected("ambiguous content-hash move for " + relative.generic_string());
                }
                if (available.size() == 1) { identity = available.front(); }
            }
            const auto resource_id = identity ? identity->id : ResourceId::random();
            if (!claimed.insert(resource_id).second) {
                return std::unexpected("one previous UUID matched multiple scanned resources");
            }
            const bool content_changed = identity && identity->sha256 != *hash;
            result.resources.push_back({
                .id = resource_id,
                .key = scanned.key,
                .source = relative,
                .media_type = std::move(media_type),
                .size = scanned.size,
                .sha256 = *hash,
                .storage = storage,
                .streaming = streaming,
                .revision = identity ? identity->revision + (content_changed ? 1U : 0U) : 1U,
            });
        }
        const std::set<ResourceKey> generated_keys = [&] {
            std::set<ResourceKey> keys;
            for (const auto& item: result.resources) { keys.insert(item.key); }
            return keys;
        }();
        for (const auto& [alias, target]: policy.aliases) {
            if (generated_keys.contains(alias)) {
                return std::unexpected("alias collides with a canonical resource key: " + std::string(alias.value()));
            }
            if (!generated_keys.contains(target)) {
                return std::unexpected("alias target does not exist: " + std::string(target.value()));
            }
        }
        return result;
    }

    auto write_resource_lock_atomic(
        const ResourceLockManifest& manifest,
        const std::filesystem::path& path
    ) -> std::expected<void, std::string> {
        const auto temporary = path.string() + ".tmp";
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) { return std::unexpected("cannot create lock manifest temporary file"); }
            output << "format = 1\npackage_id = \"" << toml_escape(manifest.package_id) << "\"\n";
            for (const auto& item: manifest.resources) {
                output << "\n[[resources]]\n"
                       << "id = \"" << item.id.to_string() << "\"\n"
                       << "key = \"" << toml_escape(item.key.value()) << "\"\n"
                       << "source = \"" << toml_escape(item.source.generic_string()) << "\"\n"
                       << "media_type = \"" << toml_escape(item.media_type) << "\"\n"
                       << "size = " << item.size << "\n"
                       << "sha256 = \"" << item.sha256 << "\"\n"
                       << "storage = \"" << storage_name(item.storage) << "\"\n"
                       << "streaming = " << (item.streaming ? "true" : "false") << "\n"
                       << "revision = " << item.revision << "\n";
            }
            output.flush();
            if (!output) { std::filesystem::remove(temporary); return std::unexpected("cannot write lock manifest"); }
        }
        std::error_code error;
        std::filesystem::rename(temporary, path, error);
        if (error) {
            std::filesystem::remove(temporary);
            return std::unexpected("cannot replace lock manifest: " + error.message());
        }
        return {};
    }
} // namespace nandina::resource
