#include <catch2/catch_test_macros.hpp>

#include <nandina/resource/backends/builtin_backend.hpp>
#include <nandina/resource/backends/directory_backend.hpp>
#include <nandina/resource/backends/memory_backend.hpp>
#include <nandina/resource/resource_manager.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/text/font_family.hpp>
#include <nandina/text/font_pipeline.hpp>
#include <nandina/text/harfbuzz_text_backend.hpp>
#include <nandina/widget/primitives/text.hpp>

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
        void
        draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void
        draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        [[nodiscard]] auto supports_alpha_textures() const -> bool override {
            return true;
        }
        [[nodiscard]] auto create_alpha_texture(int, int, std::span<const std::uint8_t>)
            -> render::TextureHandle override {
            return {.value = next_texture++};
        }
        void destroy_texture(render::TextureHandle) override {
            ++destroyed_textures;
        }
        std::uint64_t next_texture = 1;
        std::size_t destroyed_textures = 0;
    };

    [[nodiscard]] auto read_font(const std::filesystem::path& path) -> std::vector<std::uint8_t> {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        REQUIRE(stream.good());
        const auto size = stream.tellg();
        stream.seekg(0);
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        REQUIRE(stream.read(reinterpret_cast<char*>(bytes.data()), size).good());
        return bytes;
    }

    [[nodiscard]] auto read_font() -> std::vector<std::uint8_t> {
        return read_font(std::filesystem::path(NANDINA_TEST_FONT_PATH));
    }

    [[nodiscard]] auto id(std::string_view value) -> resource::ResourceId {
        return *resource::ResourceId::parse(value);
    }

    struct TempDirectory {
        std::filesystem::path path = std::filesystem::temp_directory_path()
            / ("nandina-font-cache-" + resource::ResourceId::random().to_string());
        TempDirectory() {
            std::filesystem::create_directories(path);
        }
        ~TempDirectory() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    };
} // namespace

TEST_CASE("resource-backed FreeType faces retain font bytes", "[resource][font]") {
    resource::ResourceManager resources;
    auto memory = std::make_shared<resource::MemoryBackend>();
    const auto font_id = id("00112233-4455-4677-8899-aabbccddeeff");
    REQUIRE(
        memory
            ->insert(font_id, resource::ResourceKey("fonts/test/regular"), "font/ttf", read_font())
            .has_value()
    );
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

TEST_CASE(
    "FontLoader reports invalid resource bytes and supports invalidation",
    "[resource][font]"
) {
    resource::ResourceManager resources;
    auto memory = std::make_shared<resource::MemoryBackend>();
    const auto font_id = id("00112233-4455-4677-8899-aabbccddeeff");
    REQUIRE(
        memory->insert(font_id, resource::ResourceKey("fonts/test/regular"), "font/ttf", {1, 2, 3})
            .has_value()
    );
    (void)resources.mount(memory);
    text::FontLoader loader(resources);
    auto invalid = loader.load(font_id);
    REQUIRE_FALSE(invalid.has_value());
    REQUIRE(invalid.error().code == text::FontErrorCode::invalid_font);

    REQUIRE(memory
                ->insert(
                    font_id,
                    resource::ResourceKey("fonts/test/regular"),
                    "font/ttf",
                    read_font(),
                    resource::InsertMode::replace
                )
                .has_value());
    loader.invalidate_wide(font_id);
    REQUIRE(loader.load(font_id).has_value());
}

TEST_CASE(
    "FontLoader cache identity includes mount and publish generation",
    "[resource][font][cache]"
) {
    const auto font_id = id("00112233-4455-4677-8899-aabbccddeeff");
    const auto key = resource::ResourceKey("fonts/shared");
    const auto regular_bytes = read_font();
    const auto bold_bytes = read_font(std::filesystem::path(NANDINA_BOLD_TEST_FONT_PATH));

    resource::ResourceManager resources;
    text::FontLoader loader(resources);
    auto first_backend = std::make_shared<resource::MemoryBackend>("first");
    REQUIRE(first_backend->insert(font_id, key, "font/ttf", regular_bytes).has_value());
    const auto first_mount = resources.mount(first_backend);

    const auto first = loader.load(key);
    REQUIRE(first.has_value());
    const auto first_family = (*first)->family_name();

    // 同一挂载、同一 ID 的新发布必须得到新 face，而不是旧代次的缓存。
    REQUIRE(first_backend
                ->insert(font_id, key, "font/ttf", bold_bytes, resource::InsertMode::replace)
                .has_value());
    const auto replacement = loader.load(key);
    REQUIRE(replacement.has_value());
    REQUIRE(replacement->get() != first->get());
    REQUIRE((*replacement)->family_name() != first_family);

    // 跨挂载复用同一 ID 也必须重新解析，而不是按裸 ID 命中旧 face。
    REQUIRE(resources.unmount(first_mount));
    auto second_backend = std::make_shared<resource::MemoryBackend>("second");
    REQUIRE(second_backend->insert(font_id, key, "font/ttf", regular_bytes).has_value());
    (void)resources.mount(second_backend);
    const auto second = loader.load(key);
    REQUIRE(second.has_value());
    REQUIRE(second->get() != replacement->get());
    REQUIRE((*second)->family_name() == first_family);
}

TEST_CASE("FontLoader does not share non-snapshot directory content", "[resource][font][cache]") {
    TempDirectory temp;
    const auto file = temp.path / "font.ttf";
    std::filesystem::copy_file(NANDINA_TEST_FONT_PATH, file);
    const auto font_id = id("01112233-4455-4677-8899-aabbccddeeff");
    const auto key = resource::ResourceKey("fonts/directory");
    auto backend = resource::DirectoryBackend::open({
        .root = temp.path,
        .resources = {{
            .id = font_id,
            .key = key,
            .relative_path = "font.ttf",
            .media_type = "font/ttf",
        }},
    });
    REQUIRE(backend.has_value());

    resource::ResourceManager resources;
    (void)resources.mount(*backend);
    text::FontLoader loader(resources);
    const auto first = loader.load(key);
    REQUIRE(first.has_value());
    const auto first_family = (*first)->family_name();

    // 非快照来源没有发布入口；文件被外部改写后不得复用按作用域键缓存的旧 face。
    std::filesystem::copy_file(
        NANDINA_BOLD_TEST_FONT_PATH,
        file,
        std::filesystem::copy_options::overwrite_existing
    );
    const auto second = loader.load(key);
    REQUIRE(second.has_value());
    REQUIRE((*second)->family_name() != first_family);
}

TEST_CASE("FontLoader direct handles bypass the shared face cache", "[resource][font][cache]") {
    const auto font_id = id("02112233-4455-4677-8899-aabbccddeeff");
    const auto key = resource::ResourceKey("fonts/direct");
    const auto handle = std::make_shared<const resource::Resource>(
        font_id,
        key,
        "font/ttf",
        resource::ResourceStorage::memory,
        read_font()
    );
    resource::ResourceManager resources;
    text::FontLoader loader(resources);

    const auto first = loader.load(handle);
    const auto second = loader.load(handle);
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    REQUIRE(first->get() != second->get());
}

TEST_CASE("FontLoader scoped invalidation only drops its own scope", "[resource][font][cache]") {
    const auto key_a = resource::ResourceKey("fonts/a");
    const auto key_b = resource::ResourceKey("fonts/b");
    const auto bytes = read_font();
    auto backend_a = std::make_shared<resource::MemoryBackend>("a");
    auto backend_b = std::make_shared<resource::MemoryBackend>("b");
    REQUIRE(backend_a->insert(id("03112233-4455-4677-8899-aabbccddeeff"), key_a, "font/ttf", bytes)
                .has_value());
    REQUIRE(backend_b->insert(id("04112233-4455-4677-8899-aabbccddeeff"), key_b, "font/ttf", bytes)
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend_a);
    (void)resources.mount(backend_b);
    text::FontLoader loader(resources);

    const auto reference_a = resources.require(key_a);
    const auto reference_b = resources.require(key_b);
    REQUIRE(reference_a.has_value());
    REQUIRE(reference_b.has_value());
    const auto face_a = loader.load(key_a);
    const auto face_b = loader.load(key_b);
    REQUIRE(face_a.has_value());
    REQUIRE(face_b.has_value());
    REQUIRE(face_a->get() == loader.load(key_a).value().get());

    loader.invalidate(*reference_a);
    const auto reloaded_a = loader.load(key_a);
    const auto reloaded_b = loader.load(key_b);
    REQUIRE(reloaded_a.has_value());
    REQUIRE(reloaded_b.has_value());
    REQUIRE(reloaded_a->get() != face_a->get());
    REQUIRE(reloaded_b->get() == face_b->get());
}

TEST_CASE(
    "FontLoader bounds retained faces and evicts least recently used",
    "[resource][font][cache][reclaim]"
) {
    const auto key_a = resource::ResourceKey("fonts/reclaim/a");
    const auto key_b = resource::ResourceKey("fonts/reclaim/b");
    auto backend = std::make_shared<resource::MemoryBackend>("reclaim");
    REQUIRE(backend
                ->insert(id("05112233-4455-4677-8899-aabbccddeeff"), key_a, "font/ttf", read_font())
                .has_value());
    REQUIRE(backend
                ->insert(id("06112233-4455-4677-8899-aabbccddeeff"), key_b, "font/ttf", read_font())
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);
    text::FontLoader loader(resources, text::FontLoaderLimits {.max_entries = 1});

    const auto first = loader.load(key_a);
    REQUIRE(first.has_value());
    REQUIRE(loader.cached_entries() == 1);
    const auto second = loader.load(key_b);
    REQUIRE(second.has_value());
    REQUIRE(loader.cached_entries() == 1);

    // A 是最久未使用项，已被 B 挤出关联；旧 face 仍由调用方持有。
    const auto first_again = loader.load(key_a);
    REQUIRE(first_again.has_value());
    REQUIRE(first_again->get() != first->get());
    REQUIRE(loader.cached_entries() == 1);
}

TEST_CASE(
    "FontLoader eviction follows recent use, not insertion order",
    "[resource][font][cache][reclaim]"
) {
    const auto key_a = resource::ResourceKey("fonts/lru/a");
    const auto key_b = resource::ResourceKey("fonts/lru/b");
    const auto key_c = resource::ResourceKey("fonts/lru/c");
    auto backend = std::make_shared<resource::MemoryBackend>("lru");
    REQUIRE(backend
                ->insert(id("09112233-4455-4677-8899-aabbccddeeff"), key_a, "font/ttf", read_font())
                .has_value());
    REQUIRE(backend
                ->insert(id("0a112233-4455-4677-8899-aabbccddeeff"), key_b, "font/ttf", read_font())
                .has_value());
    REQUIRE(backend
                ->insert(id("0b112233-4455-4677-8899-aabbccddeeff"), key_c, "font/ttf", read_font())
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);
    text::FontLoader loader(resources, text::FontLoaderLimits {.max_entries = 2});

    const auto a = loader.load(key_a);
    const auto b = loader.load(key_b);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    // 访问 A 刷新其最近使用时间；随后插入 C 应淘汰 B，而不是 A。
    REQUIRE(loader.load(key_a).value().get() == a->get());
    const auto c = loader.load(key_c);
    REQUIRE(c.has_value());
    REQUIRE(loader.cached_entries() == 2);

    // A 被命中刷新过，因此 C 插入时淘汰的是 B；A 与 C 仍可命中同一次创建的 face。
    REQUIRE(loader.load(key_a).value().get() == a->get());
    REQUIRE(loader.load(key_c).value().get() == c->get());

    const auto b_again = loader.load(key_b);
    REQUIRE(b_again.has_value());
    REQUIRE(b_again->get() != b->get());
    REQUIRE(loader.load(key_c).value().get() == c->get());
    // 重新载入 B 后最久未使用的是 A，它应被挤出关联。
    REQUIRE(loader.load(key_a).value().get() != a->get());
}

TEST_CASE("FontLoader sweeps expired face associations", "[resource][font][cache][reclaim]") {
    const auto key_a = resource::ResourceKey("fonts/sweep/a");
    const auto key_b = resource::ResourceKey("fonts/sweep/b");
    auto backend = std::make_shared<resource::MemoryBackend>("sweep");
    REQUIRE(backend
                ->insert(id("07112233-4455-4677-8899-aabbccddeeff"), key_a, "font/ttf", read_font())
                .has_value());
    REQUIRE(backend
                ->insert(id("08112233-4455-4677-8899-aabbccddeeff"), key_b, "font/ttf", read_font())
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);
    text::FontLoader loader(resources, text::FontLoaderLimits {.max_entries = 4});

    {
        const auto transient = loader.load(key_a);
        REQUIRE(transient.has_value());
    }
    REQUIRE(loader.cached_entries() == 1);
    // 下一次访问先清扫已释放的 weak face，再插入新关联。
    const auto retained = loader.load(key_b);
    REQUIRE(retained.has_value());
    REQUIRE(loader.cached_entries() == 1);
}

TEST_CASE("FontFamily resolves weight aliases and ordered fallbacks", "[resource][font][family]") {
    resource::ResourceManager resources;
    auto memory = std::make_shared<resource::MemoryBackend>();
    const auto primary_id = id("00112233-4455-4677-8899-aabbccddeeff");
    const auto fallback_id = id("10112233-4455-4677-8899-aabbccddeeff");
    const auto bytes = read_font();
    REQUIRE(memory->insert(primary_id, resource::ResourceKey("fonts/ui/regular"), "font/ttf", bytes)
                .has_value());
    REQUIRE(memory
                ->insert(
                    fallback_id,
                    resource::ResourceKey("fonts/fallback/regular"),
                    "font/ttf",
                    bytes
                )
                .has_value());
    (void)resources.mount(memory);
    text::FontLoader loader(resources);
    text::FontFamilyRegistry registry;
    REQUIRE(registry
                .register_family(
                    resource::ResourceKey("families/ui"),
                    {
                        {.resource = resource::ResourceKey("fonts/ui/regular"), .weight = 400},
                        {.resource = resource::ResourceKey("fonts/ui/regular"), .weight = 700},
                    }
                )
                .has_value());
    REQUIRE(registry
                .register_family(
                    resource::ResourceKey("families/fallback"),
                    {
                        {.resource = resource::ResourceKey("fonts/fallback/regular")},
                    }
                )
                .has_value());
    REQUIRE(registry
                .add_alias(
                    resource::ResourceKey("families/default"),
                    resource::ResourceKey("families/ui")
                )
                .has_value());
    REQUIRE(registry
                .set_fallbacks(
                    resource::ResourceKey("families/ui"),
                    {resource::ResourceKey("families/fallback")}
                )
                .has_value());
    REQUIRE(registry.set_default_family(resource::ResourceKey("families/ui")).has_value());

    auto resolved = registry.resolve(
        {
            .family = resource::ResourceKey("families/default"),
            .weight = 650,
        },
        loader
    );
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->faces.size() == 2);
    REQUIRE(resolved->specs.front().weight == 700);

    text::HarfBuzzTextLayoutBackend backend(
        resolved->faces.front(),
        std::vector<std::shared_ptr<text::FreeTypeFontFace>>(
            resolved->faces.begin() + 1,
            resolved->faces.end()
        )
    );
    REQUIRE(backend.font_count() == 2);
}

TEST_CASE("FontPipelineCache owns ordered render resources", "[resource][font][pipeline]") {
    resource::ResourceManager resources;
    auto memory = std::make_shared<resource::MemoryBackend>();
    REQUIRE(memory
                ->insert(
                    id("00112233-4455-4677-8899-aabbccddeeff"),
                    resource::ResourceKey("fonts/ui/regular"),
                    "font/ttf",
                    read_font()
                )
                .has_value());
    (void)resources.mount(memory);
    text::FontLoader loader(resources);
    text::FontFamilyRegistry families;
    REQUIRE(families
                .register_family(
                    resource::ResourceKey("families/ui"),
                    {{
                        .resource = resource::ResourceKey("fonts/ui/regular"),
                    }}
                )
                .has_value());
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
    const text::TextPipeline explicit_pipeline {
        .backend = &text::deterministic_text_layout_backend(),
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
    REQUIRE(memory
                ->insert(
                    id("00112233-4455-4677-8899-aabbccddeeff"),
                    resource::ResourceKey("fonts/ui/regular"),
                    "font/ttf",
                    read_font()
                )
                .has_value());
    (void)resources.mount(memory);
    text::FontLoader loader(resources);
    text::FontFamilyRegistry families;
    REQUIRE(families
                .register_family(
                    resource::ResourceKey("families/ui"),
                    {{
                        .resource = resource::ResourceKey("fonts/ui/regular"),
                    }}
                )
                .has_value());
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
        auto pipeline = cache.get(theme::FontRequest {.weight = 400});
        REQUIRE(pipeline.has_value());
        regular = *pipeline;
    }
    REQUIRE_FALSE(regular.expired());
    REQUIRE(cache.retained_pipeline_count() == 1);

    {
        auto pipeline = cache.get(theme::FontRequest {.weight = 700});
        REQUIRE(pipeline.has_value());
    }
    REQUIRE(regular.expired());
    REQUIRE(cache.retained_pipeline_count() == 1);
    REQUIRE(device.destroyed_textures == 1);

    auto recreated = cache.get(theme::FontRequest {.weight = 400});
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

    const theme::FontRequest missing_request {
        .family = resource::ResourceKey("families/not-installed"),
        .weight = 700,
        .slant = theme::FontSlant::italic,
    };
    auto resolved = families.resolve(missing_request, loader);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->faces.size() == 1);
    REQUIRE(resolved->specs.front().resource.value() == resource::builtin_default_font_key);

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
    REQUIRE(families
                .register_family(
                    resource::ResourceKey("families/broken"),
                    {{.resource = resource::ResourceKey("fonts/not-installed")}}
                )
                .has_value());

    auto resolved = families.resolve({.family = resource::ResourceKey("families/broken")}, loader);

    REQUIRE(resolved.has_value());
    REQUIRE(resolved->faces.size() == 1);
    REQUIRE(resolved->specs.front().resource.value() == resource::builtin_default_font_key);
}

TEST_CASE("font resolution still fails without any default family", "[resource][font][fallback]") {
    resource::ResourceManager resources;
    text::FontLoader loader(resources);
    text::FontFamilyRegistry families;

    auto resolved =
        families.resolve({.family = resource::ResourceKey("families/not-installed")}, loader);

    REQUIRE_FALSE(resolved.has_value());
    REQUIRE(resolved.error().code == text::FontErrorCode::unknown_family);
}
