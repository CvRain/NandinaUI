//
// Image (texture) node tests — Stage 1 of the texture subsystem.
//

#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "render/render_device.hpp"
#include "render/texture_cache.hpp"
#include "resource/backends/memory_backend.hpp"
#include "resource/resource_manager.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/controls.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <deque>
#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    [[nodiscard]] auto resource_id(std::string_view value) -> resource::ResourceId {
        return *resource::ResourceId::parse(value);
    }

    /// 记录纹理调用，验证 load / size / draw_texture_region 的调用序与参数。
    class TextureRecordingDevice final: public render::IRenderDevice {
    public:
        struct Region {
            foundation::NanRect source;
            foundation::NanRect destination;
            foundation::NanColor tint;
        };

        std::vector<std::string> loaded;
        int memory_loads = 0;
        std::vector<std::uint8_t> loaded_bytes;
        std::string loaded_media_type;
        int draw_regions = 0;
        int drawn_rects = 0;
        int rgba_uploads = 0;
        std::vector<render::TextureHandle> destroyed;
        std::vector<Region> regions;
        bool fail_loads = false;
        foundation::NanSize size_to_return {64.0F, 32.0F};
        render::ImageLoadOptions last_options {};

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {
            ++drawn_rects;
        }
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

        [[nodiscard]] auto
        load_texture_from_file(std::string_view path, const render::ImageLoadOptions& options)
            -> render::TextureHandle override {
            if (fail_loads) {
                return {};
            }
            loaded.emplace_back(path);
            last_options = options;
            return render::TextureHandle {.value = loaded.size()};
        }

        [[nodiscard]] auto texture_size(render::TextureHandle) -> foundation::NanSize override {
            return size_to_return;
        }

        [[nodiscard]] auto load_texture_from_memory(
            std::span<const std::uint8_t> bytes,
            std::string_view media_type,
            const render::ImageLoadOptions& options
        ) -> render::TextureHandle override {
            if (fail_loads) {
                return {};
            }
            ++memory_loads;
            loaded_bytes.assign(bytes.begin(), bytes.end());
            loaded_media_type = media_type;
            last_options = options;
            return render::TextureHandle {.value = static_cast<std::uint64_t>(memory_loads + 100)};
        }

        void draw_texture_region(
            render::TextureHandle,
            const foundation::NanRect& source,
            const foundation::NanRect& destination,
            const foundation::NanColor& tint
        ) override {
            ++draw_regions;
            regions.push_back(Region {source, destination, tint});
        }

        void destroy_texture(const render::TextureHandle texture) override {
            destroyed.push_back(texture);
        }

        [[nodiscard]] auto create_rgba_texture(int, int, std::span<const std::uint8_t>)
            -> render::TextureHandle override {
            ++rgba_uploads;
            return render::TextureHandle {.value = static_cast<std::uint64_t>(200 + rgba_uploads)};
        }
    };

    class RecordingDecoder final: public render::IImageDecoder {
    public:
        int calls = 0;
        std::vector<std::uint8_t> decoded_bytes;

        [[nodiscard]] auto decode_memory(
            std::span<const std::uint8_t> bytes,
            std::string_view,
            const render::ImageLoadOptions&
        ) -> render::DecodedImage override {
            ++calls;
            if (!bytes.empty()) {
                decoded_bytes.push_back(bytes[0]);
            }
            return render::DecodedImage {
                .width = 2,
                .height = 1,
                .rgba = std::vector<std::uint8_t>(8, 255),
            };
        }
    };

    /// 前 N 次失败、之后成功的解码器，用于验证失败重试。
    class FlakyDecoder final: public render::IImageDecoder {
    public:
        explicit FlakyDecoder(int failures_before_success): failures_(failures_before_success) {}

        [[nodiscard]] auto decode_memory(
            std::span<const std::uint8_t>,
            std::string_view,
            const render::ImageLoadOptions&
        ) -> render::DecodedImage override {
            ++calls;
            if (calls <= failures_) {
                return render::DecodedImage {};
            }
            return render::DecodedImage {
                .width = 2,
                .height = 1,
                .rgba = std::vector<std::uint8_t>(8, 255),
            };
        }

        int calls = 0;

    private:
        int failures_ = 0;
    };

    class ManualTaskQueue {
    public:
        [[nodiscard]] auto submit(std::move_only_function<void()> task) -> bool {
            tasks.push_back(std::move(task));
            return true;
        }

        void drain() {
            while (!tasks.empty()) {
                auto task = std::move(tasks.front());
                tasks.pop_front();
                task();
            }
        }

        std::deque<std::move_only_function<void()>> tasks;
    };

    /// 包装 MemoryBackend 并记录按 key 的 find 次数，用于证明资源读取发生在后台。
    class RecordingBackend final: public resource::IResourceBackend {
    public:
        explicit RecordingBackend(std::string name):
            inner_(std::make_shared<resource::MemoryBackend>(std::move(name))) {}

        [[nodiscard]] auto name() const noexcept -> std::string_view override {
            return inner_->name();
        }
        [[nodiscard]] auto find(const resource::ResourceKey& key) const
            -> resource::ResourceLookup override {
            ++key_finds;
            return inner_->find(key);
        }
        [[nodiscard]] auto find(resource::ResourceId id) const
            -> resource::ResourceLookup override {
            return inner_->find(id);
        }
        [[nodiscard]] auto insert(
            resource::ResourceId id,
            resource::ResourceKey key,
            std::string media_type,
            std::vector<std::uint8_t> bytes
        ) {
            return inner_->insert(id, std::move(key), std::move(media_type), std::move(bytes));
        }

        mutable int key_finds = 0;

    private:
        std::shared_ptr<resource::MemoryBackend> inner_;
    };
} // namespace

TEST_CASE("image loads its source on first draw and draws the region", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    tree.set_root(image);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 100.0F)) >= 1);

    tree.draw(dev);

    REQUIRE(dev.loaded.size() == 1);
    REQUIRE(dev.loaded[0] == "hero.png");
    REQUIRE(dev.draw_regions == 1);
    // 源为自然尺寸，目标为节点世界矩形。
    REQUIRE(dev.regions[0].source.get_width() == Catch::Approx(64.0F));
    REQUIRE(dev.regions[0].source.get_height() == Catch::Approx(32.0F));
    REQUIRE(dev.regions[0].destination.get_width() == Catch::Approx(200.0F));
    REQUIRE(dev.regions[0].destination.get_height() == Catch::Approx(100.0F));
    REQUIRE(image->natural_size().get_width() == Catch::Approx(64.0F));
}

TEST_CASE("image loads its source exactly once across draws", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.loaded.size() == 1);
    REQUIRE(dev.draw_regions == 2);
}

TEST_CASE("image skips drawing when the texture fails to load", "[image]") {
    TextureRecordingDevice dev;
    dev.fail_loads = true;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("missing.png");
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.loaded.empty());
    REQUIRE(dev.draw_regions == 0);
}

TEST_CASE("image multiplies tint alpha by inherited opacity", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_local_opacity(0.5F);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.draw_regions == 1);
    REQUIRE(dev.regions[0].tint.alpha() == Catch::Approx(0.5F));
}

TEST_CASE("image honors source_rect crop", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_source_rect(foundation::NanRect::from_xywh(8.0F, 4.0F, 16.0F, 8.0F));
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.draw_regions == 1);
    REQUIRE(dev.regions[0].source.get_left() == Catch::Approx(8.0F));
    REQUIRE(dev.regions[0].source.get_top() == Catch::Approx(4.0F));
    REQUIRE(dev.regions[0].source.get_width() == Catch::Approx(16.0F));
    REQUIRE(dev.regions[0].source.get_height() == Catch::Approx(8.0F));
}

TEST_CASE("image contain letterboxes and centers by default", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_scale_mode(widget::ImageScale::contain);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.draw_regions == 1);
    // 64:32 的自然尺寸放入 100x100：等比缩到宽 100、高 50，垂直居中。
    REQUIRE(dev.regions[0].destination.get_width() == Catch::Approx(100.0F));
    REQUIRE(dev.regions[0].destination.get_height() == Catch::Approx(50.0F));
    REQUIRE(dev.regions[0].destination.get_top() == Catch::Approx(25.0F));
    REQUIRE(dev.regions[0].source.get_width() == Catch::Approx(64.0F));
}

TEST_CASE("image contain alignment start and end anchor the box", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_scale_mode(widget::ImageScale::contain);
    image->set_alignment(widget::ImageAlignment::start);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);
    image->set_alignment(widget::ImageAlignment::end);
    tree.draw(dev);

    REQUIRE(dev.regions[0].destination.get_top() == Catch::Approx(0.0F));
    REQUIRE(dev.regions[0].destination.get_left() == Catch::Approx(0.0F));
    REQUIRE(dev.regions[1].destination.get_bottom() == Catch::Approx(100.0F));
    REQUIRE(dev.regions[1].destination.get_right() == Catch::Approx(100.0F));
}

TEST_CASE("image cover crops the source to the destination aspect", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_scale_mode(widget::ImageScale::cover);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.draw_regions == 1);
    // 64:32 → 100x100 的 cover：裁宽度到 32，水平居中（left=16）。
    REQUIRE(dev.regions[0].source.get_width() == Catch::Approx(32.0F));
    REQUIRE(dev.regions[0].source.get_height() == Catch::Approx(32.0F));
    REQUIRE(dev.regions[0].source.get_left() == Catch::Approx(16.0F));
    REQUIRE(dev.regions[0].destination.get_width() == Catch::Approx(100.0F));
    REQUIRE(dev.regions[0].destination.get_height() == Catch::Approx(100.0F));
}

TEST_CASE("image passes load options to the device", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_load_options(
        render::ImageLoadOptions {
            .resize = foundation::NanSize(32.0F, 32.0F),
            .tint = foundation::NanColor::from_hex(0xFF0000),
        }
    );
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.last_options.resize.has_value());
    REQUIRE(dev.last_options.resize->get_width() == Catch::Approx(32.0F));
    REQUIRE(dev.last_options.resize->get_height() == Catch::Approx(32.0F));
    REQUIRE(dev.last_options.tint.has_value());
}

TEST_CASE(
    "image loads res URI bytes through the resource manager exactly once",
    "[image][resource]"
) {
    auto backend = std::make_shared<resource::MemoryBackend>("images");
    const std::vector<std::uint8_t> png_bytes {0x89, 0x50, 0x4E, 0x47};
    REQUIRE(backend
                ->insert(
                    resource::ResourceId::random(),
                    resource::ResourceKey("assets/hero.png"),
                    "image/png",
                    png_bytes
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);

    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("res://assets/hero.png");
    image->set_resource_manager(&resources);
    image->set_load_options(
        render::ImageLoadOptions {
            .resize = foundation::NanSize(24.0F, 12.0F),
        }
    );
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 1);
    REQUIRE(dev.loaded.empty());
    REQUIRE(dev.loaded_bytes == png_bytes);
    REQUIRE(dev.loaded_media_type == "image/png");
    REQUIRE(dev.last_options.resize.has_value());
    REQUIRE(dev.draw_regions == 2);
}

TEST_CASE("image res URI failures are safe and are not retried", "[image][resource]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("res://missing.png");
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 0);
    REQUIRE(dev.loaded.empty());
    REQUIRE(dev.draw_regions == 0);

    auto backend = std::make_shared<resource::MemoryBackend>("documents");
    REQUIRE(backend
                ->insert(
                    resource::ResourceId::random(),
                    resource::ResourceKey("readme.txt"),
                    "text/plain",
                    std::vector<std::uint8_t> {'n', 'o'}
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);
    image->set_resource_manager(&resources);
    image->set_source("res://still-missing.png");

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 0);
    REQUIRE(dev.draw_regions == 0);

    image->set_source("res://readme.txt");

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 0);
    REQUIRE(dev.draw_regions == 0);
}

TEST_CASE("image switches between packaged resources and file paths", "[image][resource]") {
    auto backend = std::make_shared<resource::MemoryBackend>("images");
    REQUIRE(backend
                ->insert(
                    resource::ResourceId::random(),
                    resource::ResourceKey("logo.png"),
                    "image/png",
                    std::vector<std::uint8_t> {1, 2, 3}
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);

    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("res://logo.png");
    image->set_resource_manager(&resources);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);
    image->set_source("logo-from-disk.png");
    tree.draw(dev);
    image->set_source("res://logo.png");
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 2);
    REQUIRE(dev.loaded == std::vector<std::string> {"logo-from-disk.png"});
    REQUIRE(dev.draw_regions == 3);
    REQUIRE(dev.destroyed.size() == 2);
}

TEST_CASE("window texture cache reuses a packaged image across node lifetimes", "[image][cache]") {
    auto backend = std::make_shared<resource::MemoryBackend>("images");
    REQUIRE(backend
                ->insert(
                    resource_id("00112233-4455-4677-8899-aabbccddeeff"),
                    resource::ResourceKey("logo.png"),
                    "image/png",
                    std::vector<std::uint8_t> {1, 2, 3}
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);

    TextureRecordingDevice dev;
    render::TextureCache cache(dev);
    scene::NanSceneTree tree;
    tree.set_texture_cache(cache);

    auto first = widget::Image::create("res://logo.png");
    first->set_resource_manager(&resources);
    tree.set_root(first);
    tree.draw(dev);
    tree.set_root(nullptr);
    first.reset();

    auto second = widget::Image::create("res://logo.png");
    second->set_resource_manager(&resources);
    tree.set_root(second);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 1);
    REQUIRE(dev.draw_regions == 2);
    REQUIRE(dev.destroyed.empty());
    REQUIRE(cache.retained_entries() == 1);
}

TEST_CASE(
    "async packaged image shows a placeholder before UI-thread upload",
    "[image][cache][async]"
) {
    auto backend = std::make_shared<resource::MemoryBackend>("images");
    REQUIRE(backend
                ->insert(
                    resource_id("10213243-5465-4768-899a-abbccddeeff0"),
                    resource::ResourceKey("hero.png"),
                    "image/png",
                    std::vector<std::uint8_t> {1, 2, 3}
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);

    TextureRecordingDevice device;
    auto decoder = std::make_shared<RecordingDecoder>();
    ManualTaskQueue background;
    ManualTaskQueue ui;
    render::TextureCache cache(
        device,
        {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [&background](
                                     std::move_only_function<void()> task
                                 ) { return background.submit(std::move(task)); },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
        }
    );
    scene::NanSceneTree tree;
    tree.set_texture_cache(cache);
    auto image = widget::Image::create("res://hero.png");
    image->set_resource_manager(&resources);
    image->set_size(foundation::NanSize(100.0F, 50.0F));
    image->set_load_mode(widget::ImageLoadMode::asynchronous);
    image->set_placeholder_color(foundation::NanColor::from_hex(0x808080));
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 50.0F));

    tree.draw(device);
    REQUIRE(image->load_state() == widget::ImageLoadState::loading);
    REQUIRE(device.drawn_rects == 1);
    REQUIRE(device.draw_regions == 0);
    REQUIRE(device.rgba_uploads == 0);
    REQUIRE(decoder->calls == 0);

    background.drain();
    REQUIRE(decoder->calls == 1);
    REQUIRE(device.rgba_uploads == 0);
    REQUIRE(ui.tasks.size() == 1);

    ui.drain();
    REQUIRE(image->load_state() == widget::ImageLoadState::ready);
    REQUIRE(device.rgba_uploads == 1);
    REQUIRE(image->natural_size() == foundation::NanSize(2.0F, 1.0F));

    tree.draw(device);
    REQUIRE(device.drawn_rects == 1);
    REQUIRE(device.draw_regions == 1);
}

TEST_CASE(
    "async resource load reads and decodes on the background worker",
    "[image][cache][async]"
) {
    auto backend = std::make_shared<RecordingBackend>("images");
    REQUIRE(backend
                ->insert(
                    resource_id("10213243-5465-4768-899a-abbccddeeff0"),
                    resource::ResourceKey("hero.png"),
                    "image/png",
                    std::vector<std::uint8_t> {1, 2, 3}
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);

    TextureRecordingDevice device;
    auto decoder = std::make_shared<RecordingDecoder>();
    ManualTaskQueue background;
    ManualTaskQueue ui;
    render::TextureCache cache(
        device,
        {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [&background](
                                     std::move_only_function<void()> task
                                 ) { return background.submit(std::move(task)); },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
        }
    );
    scene::NanSceneTree tree;
    tree.set_texture_cache(cache);
    auto image = widget::Image::create("res://hero.png");
    image->set_resource_manager(&resources);
    image->set_size(foundation::NanSize(100.0F, 50.0F));
    image->set_load_mode(widget::ImageLoadMode::asynchronous);
    image->set_placeholder_color(foundation::NanColor::from_hex(0x808080));
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 50.0F));

    tree.draw(device);
    // C5.5：UI 线程只显示占位，既不读资源也不解码。
    REQUIRE(image->load_state() == widget::ImageLoadState::loading);
    REQUIRE(backend->key_finds == 0);
    REQUIRE(decoder->calls == 0);
    REQUIRE(device.rgba_uploads == 0);

    background.drain();
    // 后台任务内完成 res:// 读取 + 解码，仍未上传 GPU。
    REQUIRE(backend->key_finds == 1);
    REQUIRE(decoder->calls == 1);
    REQUIRE(device.rgba_uploads == 0);
    REQUIRE(ui.tasks.size() == 1);

    ui.drain();
    REQUIRE(image->load_state() == widget::ImageLoadState::ready);
    REQUIRE(device.rgba_uploads == 1);
}

TEST_CASE("async texture cache merges matching pending decodes", "[image][cache][async]") {
    TextureRecordingDevice device;
    auto decoder = std::make_shared<RecordingDecoder>();
    ManualTaskQueue background;
    ManualTaskQueue ui;
    render::TextureCache cache(
        device,
        {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [&background](
                                     std::move_only_function<void()> task
                                 ) { return background.submit(std::move(task)); },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
        }
    );
    auto bytes =
        std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t> {1, 2, 3});
    std::shared_ptr<render::CachedTexture> first;
    std::shared_ptr<render::CachedTexture> second;

    REQUIRE(cache.load_memory_async(
        "same-resource",
        std::shared_ptr<const void>(bytes),
        *bytes,
        "image/png",
        {},
        [&first](auto texture) { first = std::move(texture); }
    ));
    REQUIRE(cache.load_memory_async(
        "same-resource",
        std::shared_ptr<const void>(bytes),
        *bytes,
        "image/png",
        {},
        [&second](auto texture) { second = std::move(texture); }
    ));
    REQUIRE(background.tasks.size() == 1);

    background.drain();
    ui.drain();

    REQUIRE(decoder->calls == 1);
    REQUIRE(device.rgba_uploads == 1);
    REQUIRE(first != nullptr);
    REQUIRE(second == first);
}

TEST_CASE(
    "rejected async submission completes without leaving a stuck request",
    "[image][cache][async]"
) {
    TextureRecordingDevice device;
    auto decoder = std::make_shared<RecordingDecoder>();
    ManualTaskQueue ui;
    render::TextureCache cache(
        device,
        {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [](std::move_only_function<void()>) { return false; },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
        }
    );
    auto bytes =
        std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t> {1, 2, 3});
    bool completed = false;

    REQUIRE(cache.load_memory_async(
        "rejected",
        std::shared_ptr<const void>(bytes),
        *bytes,
        "image/png",
        {},
        [&completed](auto texture) {
            REQUIRE(texture == nullptr);
            completed = true;
        }
    ));

    REQUIRE(completed);
    REQUIRE(decoder->calls == 0);
    cache.begin_frame();
    REQUIRE(decoder->calls == 0);
}

TEST_CASE("destroyed texture cache drops background decode completion", "[image][cache][async]") {
    TextureRecordingDevice device;
    auto decoder = std::make_shared<RecordingDecoder>();
    ManualTaskQueue background;
    ManualTaskQueue ui;
    auto cache = std::make_unique<render::TextureCache>(
        device,
        render::TextureCacheLimits {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [&background](
                                     std::move_only_function<void()> task
                                 ) { return background.submit(std::move(task)); },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
        }
    );
    auto bytes =
        std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t> {1, 2, 3});
    REQUIRE(cache->load_memory_async(
        "closing-resource",
        std::shared_ptr<const void>(bytes),
        *bytes,
        "image/png",
        {},
        [](auto) {}
    ));

    cache.reset();
    background.drain();

    // C5.5b：合作取消在解码前检查缓存存活，已销毁的缓存跳过解码、不上传、不回 UI。
    REQUIRE(decoder->calls == 0);
    REQUIRE(ui.tasks.empty());
    REQUIRE(device.rgba_uploads == 0);
}

TEST_CASE(
    "async texture cache respects the concurrent decode budget",
    "[image][cache][async][budget]"
) {
    TextureRecordingDevice device;
    auto decoder = std::make_shared<RecordingDecoder>();
    ManualTaskQueue background;
    ManualTaskQueue ui;
    render::TextureCache cache(
        device,
        {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [&background](
                                     std::move_only_function<void()> task
                                 ) { return background.submit(std::move(task)); },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
            .max_concurrent_decodes = 1,
        }
    );
    auto bytes =
        std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t> {1, 2, 3});
    REQUIRE(cache.load_memory_async(
        "first",
        std::shared_ptr<const void>(bytes),
        *bytes,
        "image/png",
        {},
        [](auto) {}
    ));
    REQUIRE(cache.load_memory_async(
        "second",
        std::shared_ptr<const void>(bytes),
        *bytes,
        "image/png",
        {},
        [](auto) {}
    ));

    // 预算 1：只提交第一个，第二个排队。
    REQUIRE(background.tasks.size() == 1);
    background.drain();
    REQUIRE(decoder->calls == 1);
    REQUIRE(ui.tasks.size() == 1);

    ui.drain();
    // 第一个完成后第二个才被提交。
    REQUIRE(background.tasks.size() == 1);
    background.drain();
    ui.drain();
    REQUIRE(decoder->calls == 2);
    REQUIRE(device.rgba_uploads == 2);
}

TEST_CASE(
    "async decode retries a bounded number of times on failure",
    "[image][cache][async][retry]"
) {
    TextureRecordingDevice device;
    auto decoder = std::make_shared<FlakyDecoder>(/*failures_before_success=*/1);
    ManualTaskQueue background;
    ManualTaskQueue ui;
    render::TextureCache cache(
        device,
        {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [&background](
                                     std::move_only_function<void()> task
                                 ) { return background.submit(std::move(task)); },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
            .max_load_attempts = 2,
        }
    );
    auto bytes =
        std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t> {1, 2, 3});
    std::shared_ptr<render::CachedTexture> result;
    REQUIRE(cache.load_memory_async(
        "retry",
        std::shared_ptr<const void>(bytes),
        *bytes,
        "image/png",
        {},
        [&result](auto texture) { result = std::move(texture); }
    ));

    background.drain();
    ui.drain();

    REQUIRE(decoder->calls == 2);
    REQUIRE(result != nullptr);
    REQUIRE(device.rgba_uploads == 1);
}

TEST_CASE("async uploads respect the per-frame budget", "[image][cache][async][budget]") {
    TextureRecordingDevice device;
    auto decoder = std::make_shared<RecordingDecoder>();
    ManualTaskQueue background;
    ManualTaskQueue ui;
    render::TextureCache cache(
        device,
        {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [&background](
                                     std::move_only_function<void()> task
                                 ) { return background.submit(std::move(task)); },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
            .max_concurrent_decodes = 2,
            .max_uploads_per_frame = 1,
        }
    );
    auto bytes =
        std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t> {1, 2, 3});
    REQUIRE(cache.load_memory_async(
        "one",
        std::shared_ptr<const void>(bytes),
        *bytes,
        "image/png",
        {},
        [](auto) {}
    ));
    REQUIRE(cache.load_memory_async(
        "two",
        std::shared_ptr<const void>(bytes),
        *bytes,
        "image/png",
        {},
        [](auto) {}
    ));

    cache.begin_frame();
    background.drain(); // 两个解码同帧完成 → 两个 finish 进入 tasks 队列。
    ui.drain(); // 预算 1：只上传第一个，第二个排队。
    REQUIRE(device.rgba_uploads == 1);

    cache.begin_frame(); // 下一帧补做第二个。
    REQUIRE(device.rgba_uploads == 2);
}

TEST_CASE(
    "ready upload bytes apply backpressure to pending decodes",
    "[image][cache][async][budget]"
) {
    TextureRecordingDevice device;
    auto decoder = std::make_shared<RecordingDecoder>();
    ManualTaskQueue background;
    ManualTaskQueue ui;
    render::TextureCache cache(
        device,
        {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [&background](
                                     std::move_only_function<void()> task
                                 ) { return background.submit(std::move(task)); },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
            .max_concurrent_decodes = 2,
            .max_ready_upload_bytes = 8,
            .max_uploads_per_frame = 1,
        }
    );
    std::vector<std::shared_ptr<std::vector<std::uint8_t>>> owners;
    for (std::uint8_t value = 1; value <= 4; ++value) {
        auto bytes = std::make_shared<std::vector<std::uint8_t>>(
            std::initializer_list<std::uint8_t> {value}
        );
        REQUIRE(cache.load_memory_async(
            std::to_string(value),
            std::shared_ptr<const void>(bytes),
            *bytes,
            "image/png",
            {},
            [](auto) {}
        ));
        owners.push_back(std::move(bytes));
    }

    cache.begin_frame();
    REQUIRE(background.tasks.size() == 2);
    background.drain();
    ui.drain();
    REQUIRE(background.tasks.size() == 1);

    background.drain();
    ui.drain();
    REQUIRE(decoder->calls == 3);
    REQUIRE(background.tasks.empty());

    cache.begin_frame();
    REQUIRE(background.tasks.empty());
    cache.begin_frame();
    REQUIRE(background.tasks.size() == 1);
}

TEST_CASE("texture cache keys include image preprocessing options", "[image][cache]") {
    TextureRecordingDevice dev;
    render::TextureCache cache(dev);

    const auto original = cache.load_file("hero.png");
    const auto resized = cache.load_file(
        "hero.png",
        render::ImageLoadOptions {.resize = foundation::NanSize(32.0F, 32.0F)}
    );
    const auto original_again = cache.load_file("hero.png");

    REQUIRE(original != nullptr);
    REQUIRE(resized != nullptr);
    REQUIRE(original_again == original);
    REQUIRE(dev.loaded.size() == 2);
    REQUIRE(cache.retained_entries() == 2);
}

TEST_CASE("texture cache evicts inactive least-recently-used textures", "[image][cache]") {
    TextureRecordingDevice dev;
    render::TextureCache cache(
        dev,
        render::TextureCacheLimits {
            .max_retained_entries = 1,
            .max_retained_bytes = 1024U * 1024U,
        }
    );

    std::weak_ptr<render::CachedTexture> first_weak;
    {
        auto first = cache.load_file("first.png");
        REQUIRE(first != nullptr);
        first_weak = first;
    }
    REQUIRE_FALSE(first_weak.expired());

    {
        auto second = cache.load_file("second.png");
        REQUIRE(second != nullptr);
    }

    REQUIRE(first_weak.expired());
    REQUIRE(dev.destroyed.size() == 1);
    REQUIRE(cache.retained_entries() == 1);
}

TEST_CASE(
    "BuildContext injects resource services into authored images",
    "[image][resource][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager themes;
    resource::ResourceManager resources;
    widget::BuildContext ui(graph, scope, themes, &resources);

    auto image = ui.make<widget::Image>("res://logo.png").build();

    REQUIRE(image->resource_manager() == &resources);
}

TEST_CASE("image defers loading outside its clip viewport", "[image][viewport]") {
    TextureRecordingDevice device;
    scene::NanSceneTree tree;

    auto clip = std::make_shared<scene::NanControl>();
    clip->set_overflow(scene::ControlOverflow::clip);
    clip->set_size(foundation::NanSize(100.0F, 100.0F));

    auto image = widget::Image::create("hero.png");
    image->set_size(foundation::NanSize(20.0F, 20.0F));
    image->set_position(foundation::NanPoint(1000.0F, 0.0F)); // 视口外
    image->set_prefetch_distance(50.0F);
    image->set_placeholder_color(foundation::NanColor::from_hex(0x808080));
    clip->add_child(image);

    tree.set_root(clip);
    tree.draw(device);

    // C5.6b：图片在 clip 视口（外扩预取）之外，不加载，只画占位。
    REQUIRE(image->load_state() == widget::ImageLoadState::idle);
    REQUIRE(device.loaded.empty());
    REQUIRE(device.drawn_rects == 1);

    // 移入视口后触发加载。
    image->set_position(foundation::NanPoint(80.0F, 0.0F));
    tree.draw(device);
    REQUIRE(device.loaded.size() == 1);
    REQUIRE(device.loaded.front() == "hero.png");
    REQUIRE(image->load_state() == widget::ImageLoadState::ready);
}

TEST_CASE("async decode priority orders the pending queue", "[image][cache][async][priority]") {
    TextureRecordingDevice device;
    auto decoder = std::make_shared<RecordingDecoder>();
    ManualTaskQueue background;
    ManualTaskQueue ui;
    render::TextureCache cache(
        device,
        {},
        render::TextureCacheAsyncServices {
            .decoder = decoder,
            .submit_background = [&background](
                                     std::move_only_function<void()> task
                                 ) { return background.submit(std::move(task)); },
            .post_ui =
                [&ui](std::move_only_function<void()> task) { return ui.submit(std::move(task)); },
            .max_concurrent_decodes = 1,
        }
    );
    auto a =
        std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t> {1, 2, 3});
    auto b =
        std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t> {2, 3, 4});
    auto c =
        std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t> {3, 4, 5});
    REQUIRE(cache.load_memory_async(
        "a",
        std::shared_ptr<const void>(a),
        *a,
        "image/png",
        {},
        [](auto) {},
        /*priority=*/50
    ));
    REQUIRE(cache.load_memory_async(
        "b",
        std::shared_ptr<const void>(b),
        *b,
        "image/png",
        {},
        [](auto) {},
        /*priority=*/100
    ));
    REQUIRE(cache.load_memory_async(
        "c",
        std::shared_ptr<const void>(c),
        *c,
        "image/png",
        {},
        [](auto) {},
        /*priority=*/0
    ));

    // 预算 1：先提交 a（首个），b/c 排队。a 完成后按优先级先提交 c（0）再提交 b（100）。
    background.drain();
    ui.drain();
    background.drain();
    ui.drain();
    background.drain();
    ui.drain();

    REQUIRE(decoder->calls == 3);
    REQUIRE(decoder->decoded_bytes == std::vector<std::uint8_t> {1, 3, 2});
}
