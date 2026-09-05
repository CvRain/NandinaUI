//
// widget/image - textured image node (image/texture subsystem).
//
// 一张按文件路径加载的图片：首次绘制时懒加载 RGBA 纹理，把自然尺寸作为默认尺寸，
// 支持 tint 着色、source_rect 裁剪、stretch/contain/cover 缩放与 contain 对齐，
// per-node opacity 自动合成。节点持有共享 RAII 纹理；窗口缓存负责跨页面复用与有界驻留。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_IMAGE_HPP
#define NANDINA_EXPERIMENT_WIDGET_IMAGE_HPP

#include "../foundation/geometry.hpp"
#include "../foundation/nandina_color.hpp"
#include "../render/render_device.hpp"
#include "../scene/control.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::resource
{
    class ResourceManager;
}

namespace nandina::render
{
    class CachedTexture;
    class TextureCache;
} // namespace nandina::render

namespace nandina::widget
{
    enum class ImageLoadMode { automatic, synchronous, asynchronous };
    enum class ImageLoadState { idle, loading, ready, failed };

    /// 图片如何适配其布局尺寸。
    enum class ImageScale {
        stretch, // 拉伸填满（可能变形）
        contain, // 等比缩放完整放入（留白）
        cover, // 等比缩放铺满（裁剪溢出）
    };

    /// contain 空余区域的放置位置（对 stretch/cover 无影响）。
    enum class ImageAlignment {
        center,
        start, // 左上
        end, // 右下
    };

    class Image: public scene::NanControl {
    public:
        explicit Image(std::string source = {});

        [[nodiscard]] static auto create(std::string source = {}) -> std::shared_ptr<Image>;

        void set_source(std::string path);
        [[nodiscard]] auto source() const -> std::string_view;

        /// 设置用于解析 res:// 资源的非 owning 服务；切换服务后下次绘制重新加载。
        void set_resource_manager(resource::ResourceManager* resources);
        [[nodiscard]] auto resource_manager() const noexcept -> resource::ResourceManager*;

        void set_texture_cache(render::TextureCache* cache);
        [[nodiscard]] auto texture_cache() const noexcept -> render::TextureCache*;

        void set_tint(foundation::NanColor tint);
        [[nodiscard]] auto tint() const -> foundation::NanColor;

        /// 裁剪纹理的源区域（纹理像素坐标）；未设置时用整张纹理。
        void set_source_rect(foundation::NanRect rect);
        void clear_source_rect();
        [[nodiscard]] auto source_rect() const -> const std::optional<foundation::NanRect>&;

        void set_scale_mode(ImageScale mode);
        [[nodiscard]] auto scale_mode() const -> ImageScale;

        void set_alignment(ImageAlignment alignment);
        [[nodiscard]] auto alignment() const -> ImageAlignment;

        /// 已加载纹理的自然像素尺寸；未加载时 {0,0}。
        [[nodiscard]] auto natural_size() const -> foundation::NanSize;

        /// 加载时的预处理（crop/resize/tint）；修改后下次绘制会按新选项重新加载。
        void set_load_options(render::ImageLoadOptions options);
        [[nodiscard]] auto load_options() const -> const render::ImageLoadOptions&;

        void set_load_mode(ImageLoadMode mode);
        [[nodiscard]] auto load_mode() const noexcept -> ImageLoadMode;
        [[nodiscard]] auto load_state() const noexcept -> ImageLoadState;

        void set_placeholder_color(foundation::NanColor color);
        void clear_placeholder_color();
        [[nodiscard]] auto placeholder_color() const -> const std::optional<foundation::NanColor>&;

        /// C5.6b：视口预取距离（>0 时，仅在图片世界矩形与当前 clip 视口（外扩该距离）
        /// 相交时才触发加载，否则保持 idle 只画占位）。
        void set_prefetch_distance(float distance);
        [[nodiscard]] auto prefetch_distance() const noexcept -> float;

        auto on_draw(render::DrawContext& ctx) -> void override;
        void apply_texture_cache(render::TextureCache& cache) override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;

    private:
        struct DrawRects {
            foundation::NanRect source;
            foundation::NanRect destination;
        };

        void ensure_loaded(render::IRenderDevice& device, int priority);
        void reset_load();
        void complete_async(
            std::uint64_t generation,
            std::string source,
            std::shared_ptr<render::CachedTexture> texture
        );
        [[nodiscard]] auto compute_rects(const foundation::NanRect& world) const -> DrawRects;

        std::string source_;
        resource::ResourceManager* resources_ = nullptr;
        render::TextureCache* texture_cache_ = nullptr;
        std::shared_ptr<render::CachedTexture> texture_;
        foundation::NanSize natural_size_ {};
        foundation::NanColor tint_ = foundation::NanColor::from_hex(0xFFFFFF);
        render::ImageLoadOptions load_options_ {};
        std::optional<foundation::NanRect> source_rect_;
        std::optional<foundation::NanColor> placeholder_color_;
        ImageScale scale_mode_ = ImageScale::stretch;
        ImageAlignment alignment_ = ImageAlignment::center;
        ImageLoadMode load_mode_ = ImageLoadMode::automatic;
        ImageLoadState load_state_ = ImageLoadState::idle;
        float prefetch_distance_ = 0.0F;
        std::uint64_t load_generation_ = 0;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_IMAGE_HPP
