#include <catch2/catch_test_macros.hpp>

#include "resource/backends/memory_backend.hpp"
#include "resource/backends/builtin_backend.hpp"
#include "resource/resource_manager.hpp"
#include "text/font_family.hpp"
#include "text/font_pipeline.hpp"
#include "text/harfbuzz_text_backend.hpp"
#include "scene/scene_tree.hpp"
#include "widget/primitives/text.hpp"

#include <filesystem>
#include <fstream>

using namespace nandina;

namespace
{
    class TextureDevice final: public render::IRenderDevice {
    public:
        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {}
        void draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&) override {}
        void draw_line(const foundation::NanPoint&, const foundation::NanPoint&, float, const foundation::NanColor&) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {}
        void draw_text(std::string_view, const foundation::NanPoint&, float, const foundation::NanColor&) override {}
        [[nodiscard]] auto supports_alpha_textures() const -> bool override { return true; }
        [[nodiscard]] auto create_alpha_texture(int, int, std::span<const std::uint8_t>)
            -> render::TextureHandle override {
            return {.value = next_texture++};
        }
        void destroy_texture(render::TextureHandle) override { ++destroyed_textures; }
        std::uint64_t next_texture = 1;
        std::size_t destroyed_textures = 0;
    };

    [[nodiscard]] auto read_font() -> std::vector<std::uint8_t> {
        const std::filesystem::path path(NANDINA_TEST_FONT_PATH);
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        REQUIRE(stream.good());
        const auto size = stream.tellg();
        stream.seekg(0);
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        REQUIRE(stream.read(reinterpret_cast<char*>(bytes.data()), size).good());
        return bytes;
    }

    [[nodiscard]] auto id(std::string_view value) -> resource::ResourceId {
        return *resource::ResourceId::parse(value);
    }
}

TEST_CASE("resource-backed FreeType faces retain font bytes", "[resource][font]") {
    resource::ResourceManager resources;
    auto memory = std::make_shared<resource::MemoryBackend>();
    const auto font_id = id("00112233-4455-4677-8899-aabbccddeeff");
    REQUIRE(memory->insert(
        font_id, resource::ResourceKey("fonts/test/regular"), "font/ttf", read_font()
    ).has_value());
    REQUIRE(memory->add_alias(resource::ResourceKey("fonts/default"), font_id).has_value());
    const auto mount = resources.mount(memory);
    text::FontLoader loader(resources);

    auto canonical = loader.load(resource::ResourceKey("fonts/test/regular"));
    auto alias = loader.load(resource::ResourceKey("fonts/default"));
    REQUIRE(canonical.has_value());
    REQUIRE(alias.has_value());
    REQUIRE(canonical->get() == alias->get());
    REQUIRE(resources.unmount(mount));
    memory.reset();
    REQUIRE_FALSE((*canonical)->family_name().empty());
    REQUIRE((*canonical)->metrics(20.0F).line_height > 0.0F);
    REQUIRE((*canonical)->glyph_metrics_by_index(1, 20.0F).glyph_index == 1);
}

TEST_CASE("FontLoader reports invalid resource bytes and supports invalidation", "[resource][font]") {
    resource::ResourceManager resources;
    auto memory = std::make_shared<resource::MemoryBackend>();
    const auto font_id = id("00112233-4455-4677-8899-aabbccddeeff");
    REQUIRE(memory->insert(
        font_id, resource::ResourceKey("fonts/test/regular"), "font/ttf", {1, 2, 3}
    ).has_value());
    (void)resources.mount(memory);
    text::FontLoader loader(resources);
    auto invalid = loader.load(font_id);
    REQUIRE_FALSE(invalid.has_value());
    REQUIRE(invalid.error().code == text::FontErrorCode::invalid_font);

    REQUIRE(memory->insert(
        font_id, resource::ResourceKey("fonts/test/regular"), "font/ttf", read_font(),
        resource::InsertMode::replace
    ).has_value());
    loader.invalidate(font_id);
    REQUIRE(loader.load(font_id).has_value());
}

TEST_CASE("FontFamily resolves weight aliases and ordered fallbacks", "[resource][font][family]") {
    resource::ResourceManager resources;
    auto memory = std::make_shared<resource::MemoryBackend>();
    const auto primary_id = id("00112233-4455-4677-8899-aabbccddeeff");
    const auto fallback_id = id("10112233-4455-4677-8899-aabbccddeeff");
    const auto bytes = read_font();
    REQUIRE(memory->insert(
        primary_id, resource::ResourceKey("fonts/ui/regular"), "font/ttf", bytes
    ).has_value());
    REQUIRE(memory->insert(
        fallback_id, resource::ResourceKey("fonts/fallback/regular"), "font/ttf", bytes
    ).has_value());
    (void)resources.mount(memory);
    text::FontLoader loader(resources);
    text::FontFamilyRegistry registry;
    REQUIRE(registry.register_family(resource::ResourceKey("families/ui"), {
        {.resource = resource::ResourceKey("fonts/ui/regular"), .weight = 400},
        {.resource = resource::ResourceKey("fonts/ui/regular"), .weight = 700},
    }).has_value());
    REQUIRE(registry.register_family(resource::ResourceKey("families/fallback"), {
        {.resource = resource::ResourceKey("fonts/fallback/regular")},
    }).has_value());
    REQUIRE(registry.add_alias(
        resource::ResourceKey("families/default"), resource::ResourceKey("families/ui")
    ).has_value());
    REQUIRE(registry.set_fallbacks(
        resource::ResourceKey("families/ui"), {resource::ResourceKey("families/fallback")}
    ).has_value());
    REQUIRE(registry.set_default_family(resource::ResourceKey("families/ui")).has_value());

    auto resolved = registry.resolve({
        .family = resource::ResourceKey("families/default"),
        .weight = 650,
    }, loader);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->faces.size() == 2);
    REQUIRE(resolved->specs.front().weight == 700);

    text::HarfBuzzTextLayoutBackend backend(
        resolved->faces.front(),
        std::vector<std::shared_ptr<text::FreeTypeFontFace>>(
            resolved->faces.begin() + 1, resolved->faces.end()
        )
    );
    REQUIRE(backend.font_count() == 2);
}

TEST_CASE("FontPipelineCache owns ordered render resources", "[resource][font][pipeline]") {
    resource::ResourceManager resources;
    auto memory = std::make_shared<resource::MemoryBackend>();
    REQUIRE(memory->insert(
        id("00112233-4455-4677-8899-aabbccddeeff"),
        resource::ResourceKey("fonts/ui/regular"), "font/ttf", read_font()
    ).has_value());
    (void)resources.mount(memory);
    text::FontLoader loader(resources);
    text::FontFamilyRegistry families;
    REQUIRE(families.register_family(resource::ResourceKey("families/ui"), {{
        .resource = resource::ResourceKey("fonts/ui/regular"),
    }}).has_value());
    REQUIRE(families.set_default_family(resource::ResourceKey("families/ui")).has_value());
    TextureDevice device;
    text::FontPipelineCache cache(device, loader, families);

    auto first = cache.get({});
    auto second = cache.get({});
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    REQUIRE(first->get() == second->get());
    REQUIRE((*first)->font_count() == 1);
    REQUIRE((*first)->pipeline().backend != nullptr);
    REQUIRE((*first)->pipeline().renderer != nullptr);
    REQUIRE(device.next_texture == 2);

    auto text_node = std::make_shared<widget::primitives::Text>("context font");
    text_node->set_font_family(resource::ResourceKey("families/ui"));
    scene::NanSceneTree tree;
    tree.set_font_context(cache);
    tree.set_root(text_node);
    REQUIRE(text_node->font().family == resource::ResourceKey("families/ui"));
    auto requested = cache.get(text_node->font());
    REQUIRE(requested.has_value());
    REQUIRE(text_node->text_pipeline().backend == (*requested)->pipeline().backend);

    auto explicit_text = std::make_shared<widget::primitives::Text>("explicit pipeline");
    const widget::primitives::TextPipeline explicit_pipeline {
        .backend = &widget::primitives::deterministic_text_layout_backend(),
    };
    explicit_text->set_text_pipeline(explicit_pipeline);
    tree.set_root(explicit_text);
    REQUIRE(explicit_text->text_pipeline().backend == explicit_pipeline.backend);
}

TEST_CASE(
    "FontPipelineCache retains recent pipelines within its bounds",
    "[resource][font][cache]"
) {
    resource::ResourceManager resources;
    auto memory = std::make_shared<resource::MemoryBackend>();
    REQUIRE(memory->insert(
        id("00112233-4455-4677-8899-aabbccddeeff"),
        resource::ResourceKey("fonts/ui/regular"), "font/ttf", read_font()
    ).has_value());
    (void)resources.mount(memory);
    text::FontLoader loader(resources);
    text::FontFamilyRegistry families;
    REQUIRE(families.register_family(resource::ResourceKey("families/ui"), {{
        .resource = resource::ResourceKey("fonts/ui/regular"),
    }}).has_value());
    REQUIRE(families.set_default_family(resource::ResourceKey("families/ui")).has_value());
    TextureDevice device;
    text::FontPipelineCache cache(
        device,
        loader,
        families,
        text::FontPipelineCacheLimits {
            .max_retained_pipelines = 1,
            .max_retained_bytes = 16U * 1024U * 1024U,
        }
    );

    std::weak_ptr<text::FontPipeline> regular;
    {
        auto pipeline = cache.get(text::FontRequest {.weight = 400});
        REQUIRE(pipeline.has_value());
        regular = *pipeline;
    }
    REQUIRE_FALSE(regular.expired());
    REQUIRE(cache.retained_pipeline_count() == 1);

    {
        auto pipeline = cache.get(text::FontRequest {.weight = 700});
        REQUIRE(pipeline.has_value());
    }
    REQUIRE(regular.expired());
    REQUIRE(cache.retained_pipeline_count() == 1);
    REQUIRE(device.destroyed_textures == 1);

    auto recreated = cache.get(text::FontRequest {.weight = 400});
    REQUIRE(recreated.has_value());
    REQUIRE(device.next_texture == 4);
}

TEST_CASE("builtin default family creates a portable text pipeline", "[resource][font][builtin]") {
    resource::ResourceManager resources;
    (void)resources.mount(resource::BuiltinBackend::create(), -1000);
    text::FontLoader loader(resources);
    text::FontFamilyRegistry families;
    REQUIRE(text::register_builtin_default_font_family(families).has_value());

    auto resolved = families.resolve({}, loader);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->faces.size() == 1);
    REQUIRE(resolved->specs.front().resource.value() == "fonts/default");
    REQUIRE_FALSE(resolved->faces.front()->family_name().empty());

    TextureDevice device;
    text::FontPipelineCache cache(device, loader, families);
    auto pipeline = cache.get({
        .family = resource::ResourceKey("families/default-ui"),
    });
    REQUIRE(pipeline.has_value());
    REQUIRE((*pipeline)->pipeline().backend != nullptr);
    REQUIRE((*pipeline)->pipeline().renderer != nullptr);
}

TEST_CASE(
    "unknown requested family falls back to the portable default pipeline",
    "[resource][font][fallback]"
) {
    resource::ResourceManager resources;
    (void)resources.mount(resource::BuiltinBackend::create(), -1000);
    text::FontLoader loader(resources);
    text::FontFamilyRegistry families;
    REQUIRE(text::register_builtin_default_font_family(families).has_value());
    TextureDevice device;
    text::FontPipelineCache cache(device, loader, families);

    const text::FontRequest missing_request {
        .family = resource::ResourceKey("families/not-installed"),
        .weight = 700,
        .slant = text::FontSlant::italic,
    };
    auto resolved = families.resolve(missing_request, loader);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->faces.size() == 1);
    REQUIRE(
        resolved->specs.front().resource.value()
        == resource::builtin_default_font_key
    );

    auto text_node = std::make_shared<widget::primitives::Text>("missing family fallback");
    text_node->set_font(missing_request);
    scene::NanSceneTree tree;
    tree.set_font_context(cache);
    REQUIRE_NOTHROW(tree.set_root(text_node));
    REQUIRE(text_node->font() == missing_request);
    REQUIRE(text_node->text_pipeline().backend != nullptr);
    REQUIRE(text_node->text_pipeline().renderer != nullptr);
}

TEST_CASE(
    "unloadable requested family continues through the default chain",
    "[resource][font][fallback]"
) {
    resource::ResourceManager resources;
    (void)resources.mount(resource::BuiltinBackend::create(), -1000);
    text::FontLoader loader(resources);
    text::FontFamilyRegistry families;
    REQUIRE(text::register_builtin_default_font_family(families).has_value());
    REQUIRE(
        families.register_family(
            resource::ResourceKey("families/broken"),
            {{.resource = resource::ResourceKey("fonts/not-installed")}}
        ).has_value()
    );

    auto resolved = families.resolve(
        {.family = resource::ResourceKey("families/broken")},
        loader
    );

    REQUIRE(resolved.has_value());
    REQUIRE(resolved->faces.size() == 1);
    REQUIRE(
        resolved->specs.front().resource.value()
        == resource::builtin_default_font_key
    );
}

TEST_CASE("font resolution still fails without any default family", "[resource][font][fallback]") {
    resource::ResourceManager resources;
    text::FontLoader loader(resources);
    text::FontFamilyRegistry families;

    auto resolved = families.resolve(
        {.family = resource::ResourceKey("families/not-installed")},
        loader
    );

    REQUIRE_FALSE(resolved.has_value());
    REQUIRE(resolved.error().code == text::FontErrorCode::unknown_family);
}
