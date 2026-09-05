#include "font_family.hpp"

#include <algorithm>
#include <cmath>
#include <mutex>

namespace nandina::text
{
    namespace
    {
        [[nodiscard]] auto font_error(FontErrorCode code, std::string message) -> FontError {
            return {.code = code, .operation = "font.family", .message = std::move(message)};
        }
    } // namespace

    auto FontFamilyRegistry::register_family(
        resource::ResourceKey family,
        std::vector<FontFaceSpec> faces
    ) -> FontResult<void> {
        if (faces.empty()
            || std::ranges::any_of(
                faces,
                [](const auto& face) { return face.weight < 1 || face.weight > 1000; }
            ))
        {
            return std::unexpected(
                font_error(FontErrorCode::no_matching_face, "family requires valid faces")
            );
        }
        std::unique_lock lock(mutex_);
        if (families_.contains(family) || aliases_.contains(family)) {
            return std::unexpected(
                font_error(FontErrorCode::duplicate_family, "font family already exists")
            );
        }
        families_.emplace(std::move(family), Family {.faces = std::move(faces)});
        return {};
    }

    auto FontFamilyRegistry::register_face(
        resource::ResourceKey family,
        std::shared_ptr<FreeTypeFontFace> face,
        const int weight,
        const FontSlant slant
    ) -> FontResult<void> {
        if (!face) {
            return std::unexpected(font_error(FontErrorCode::no_matching_face, "null face"));
        }
        if (weight < 1 || weight > 1000) {
            return std::unexpected(
                font_error(FontErrorCode::no_matching_face, "invalid face weight")
            );
        }
        FontFaceSpec spec {
            .resource = resource::ResourceKey(std::string("font/direct")),
            .face_index = 0,
            .weight = weight,
            .slant = slant,
            .direct_face = std::move(face),
        };
        std::unique_lock lock(mutex_);
        if (aliases_.contains(family)) {
            return std::unexpected(
                font_error(FontErrorCode::duplicate_family, "face key clashes with a family alias")
            );
        }
        if (const auto existing = families_.find(family); existing != families_.end()) {
            // 追加一个 face 变体：同族可含 regular/bold/italic 多个 face，按 weight/slant 匹配。
            existing->second.faces.push_back(std::move(spec));
        }
        else {
            families_.emplace(std::move(family), Family {.faces = {std::move(spec)}});
        }
        return {};
    }

    auto FontFamilyRegistry::add_alias(resource::ResourceKey alias, resource::ResourceKey family)
        -> FontResult<void> {
        std::unique_lock lock(mutex_);
        if (!families_.contains(family)) {
            return std::unexpected(
                font_error(FontErrorCode::unknown_family, "alias target family does not exist")
            );
        }
        if (families_.contains(alias) || aliases_.contains(alias)) {
            return std::unexpected(
                font_error(FontErrorCode::duplicate_alias, "font family alias already exists")
            );
        }
        aliases_.emplace(std::move(alias), std::move(family));
        return {};
    }

    auto FontFamilyRegistry::set_fallbacks(
        resource::ResourceKey family,
        std::vector<resource::ResourceKey> fallbacks
    ) -> FontResult<void> {
        std::unique_lock lock(mutex_);
        auto found = families_.find(family);
        if (found == families_.end()) {
            return std::unexpected(
                font_error(FontErrorCode::unknown_family, "font family does not exist")
            );
        }
        for (const auto& fallback: fallbacks) {
            if (!families_.contains(fallback) || fallback == family) {
                return std::unexpected(
                    font_error(FontErrorCode::invalid_fallback, "invalid font fallback")
                );
            }
        }
        found->second.fallbacks = std::move(fallbacks);
        return {};
    }

    auto FontFamilyRegistry::set_default_family(resource::ResourceKey family) -> FontResult<void> {
        std::unique_lock lock(mutex_);
        if (!families_.contains(family)) {
            return std::unexpected(
                font_error(FontErrorCode::unknown_family, "default font family does not exist")
            );
        }
        default_family_ = std::move(family);
        return {};
    }

    auto FontFamilyRegistry::set_default_fallbacks(std::vector<resource::ResourceKey> families)
        -> FontResult<void> {
        std::unique_lock lock(mutex_);
        for (const auto& family: families) {
            if (!families_.contains(family)) {
                return std::unexpected(font_error(
                    FontErrorCode::invalid_fallback,
                    "default fallback family does not exist"
                ));
            }
        }
        default_fallbacks_ = std::move(families);
        return {};
    }

    auto FontFamilyRegistry::resolve(const FontRequest& request, FontLoader& loader) const
        -> FontResult<ResolvedFontFamily> {
        std::map<resource::ResourceKey, Family> families;
        std::map<resource::ResourceKey, resource::ResourceKey> aliases;
        std::optional<resource::ResourceKey> default_family;
        std::vector<resource::ResourceKey> default_fallbacks;
        {
            std::shared_lock lock(mutex_);
            families = families_;
            aliases = aliases_;
            default_family = default_family_;
            default_fallbacks = default_fallbacks_;
        }
        std::optional<resource::ResourceKey> family = request.family;
        if (family) {
            if (const auto alias = aliases.find(*family); alias != aliases.end()) {
                family = alias->second;
            }
            if (!families.contains(*family)) {
                family = default_family;
            }
        }
        else {
            family = default_family;
        }
        if (!family || !families.contains(*family)) {
            return std::unexpected(font_error(
                FontErrorCode::unknown_family,
                "font request cannot resolve a registered or default family"
            ));
        }

        std::vector<resource::ResourceKey> order;
        std::set<resource::ResourceKey> visited;
        const auto append = [&](const auto& self, const resource::ResourceKey& current) -> bool {
            if (!visited.insert(current).second) {
                return false;
            }
            order.push_back(current);
            for (const auto& fallback: families.at(current).fallbacks) {
                if (!self(self, fallback)) {
                    return false;
                }
            }
            return true;
        };
        if (!append(append, *family)) {
            return std::unexpected(
                font_error(FontErrorCode::invalid_fallback, "font fallback cycle detected")
            );
        }
        if (default_family && !visited.contains(*default_family)
            && !append(append, *default_family))
        {
            return std::unexpected(
                font_error(FontErrorCode::invalid_fallback, "font fallback cycle detected")
            );
        }
        for (const auto& fallback: default_fallbacks) {
            if (!visited.contains(fallback) && !append(append, fallback)) {
                return std::unexpected(
                    font_error(FontErrorCode::invalid_fallback, "font fallback cycle detected")
                );
            }
        }

        ResolvedFontFamily result;
        std::set<std::pair<std::string, std::uint32_t>> loaded;
        std::optional<FontError> load_error;
        for (const auto& current: order) {
            const auto& candidates = families.at(current).faces;
            const FontFaceSpec* best = nullptr;
            int best_slant = 3;
            int best_weight = 1001;
            for (const auto& candidate: candidates) {
                const int slant_score = candidate.slant == request.slant
                    ? 0
                    : ((candidate.slant != FontSlant::normal && request.slant != FontSlant::normal)
                           ? 1
                           : 2);
                const int weight_score = std::abs(candidate.weight - request.weight);
                if (!best
                    || std::pair(slant_score, weight_score) < std::pair(best_slant, best_weight))
                {
                    best = &candidate;
                    best_slant = slant_score;
                    best_weight = weight_score;
                }
            }
            if (!best) {
                continue;
            }
            if (best->direct_face) {
                result.specs.push_back(*best);
                result.faces.push_back(best->direct_face);
                continue;
            }
            const auto identity = std::pair(std::string(best->resource.value()), best->face_index);
            if (!loaded.insert(identity).second) {
                continue;
            }
            auto face = loader.load(best->resource, best->face_index);
            if (!face) {
                if (!load_error) {
                    load_error = face.error();
                }
                continue;
            }
            result.specs.push_back(*best);
            result.faces.push_back(*face);
        }
        if (result.faces.empty()) {
            if (load_error) {
                return std::unexpected(std::move(*load_error));
            }
            return std::unexpected(
                font_error(FontErrorCode::no_matching_face, "font family resolved no faces")
            );
        }
        return result;
    }

    auto register_builtin_default_font_family(FontFamilyRegistry& registry) -> FontResult<void> {
        const auto family = resource::ResourceKey(std::string(builtin_default_font_family_key));
        auto registered = registry.register_family(
            family,
            {{
                .resource = resource::ResourceKey(std::string(resource::builtin_default_font_key)),
            }}
        );
        if (!registered) {
            return registered;
        }
        return registry.set_default_family(family);
    }

    auto register_optional_font_fallback(
        FontFamilyRegistry& registry,
        resource::ResourceManager& resources,
        resource::ResourceKey family,
        resource::ResourceKey font
    ) -> FontResult<bool> {
        auto found = resources.find(font);
        if (!found) {
            return std::unexpected(
                FontError {
                    .code = FontErrorCode::resource_failure,
                    .operation = "font.family.optional",
                    .message = found.error().message,
                    .cause = found.error(),
                }
            );
        }
        if (!*found) {
            return false;
        }
        auto registered = registry.register_family(family, {{.resource = std::move(font)}});
        if (!registered) {
            return std::unexpected(registered.error());
        }
        auto fallbacks = registry.set_default_fallbacks({std::move(family)});
        if (!fallbacks) {
            return std::unexpected(fallbacks.error());
        }
        return true;
    }
} // namespace nandina::text
