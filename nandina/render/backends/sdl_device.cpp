//
// render/backends/sdl_device — SdlRenderDevice 实现。
//

#include "sdl_device.hpp"

#include "../../foundation/nandina_color_space.hpp"
#include "../detail/parameter_state_cache.hpp"
#include "sdf_primitive_geometry.hpp"
#include "sdf_rounded_spirv.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace nandina::render
{
    namespace
    {
        constexpr float kPi = 3.14159265358979323846F;

        void check_sdl(const bool ok, std::string_view operation) {
            if (!ok) {
                throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
            }
        }

        /// 圆角/圆的分段数：按半径取，兼顾 CPU 顶点量与视觉圆滑度。
        [[nodiscard]] auto segments_for(const float radius) -> int {
            return std::clamp(static_cast<int>(std::ceil(radius * 0.75F)), 6, 48);
        }

        [[nodiscard]] auto to_sdl_color(const foundation::NanColor& color) -> SDL_Color {
            const auto rgb = color.to<foundation::NanHexRgb>();
            return SDL_Color {
                .r = rgb.red,
                .g = rgb.green,
                .b = rgb.blue,
                .a = rgb.alpha,
            };
        }

        [[nodiscard]] auto to_vertex_color(const foundation::NanColor& color) -> SDL_FColor {
            const auto converted = to_sdl_color(color);
            return SDL_FColor {
                .r = static_cast<float>(converted.r) / 255.0F,
                .g = static_cast<float>(converted.g) / 255.0F,
                .b = static_cast<float>(converted.b) / 255.0F,
                .a = static_cast<float>(converted.a) / 255.0F,
            };
        }

        void set_color(SDL_Renderer& renderer, const foundation::NanColor& color) {
            const auto converted = to_sdl_color(color);
            check_sdl(
                SDL_SetRenderDrawColor(
                    &renderer,
                    converted.r,
                    converted.g,
                    converted.b,
                    converted.a
                ),
                "SDL_SetRenderDrawColor"
            );
        }

        /// A8 → RGBA8（白色 + alpha）。与生产的 raylib 后端 alpha_to_rgba 同构：
        /// 两边都因为「2D API 只吃 RGBA」而付出 4 倍纹理字节，这个代价本身就是结论之一。
        [[nodiscard]] auto alpha_to_rgba(const std::span<const std::uint8_t> alpha)
            -> std::vector<std::uint8_t> {
            std::vector<std::uint8_t> rgba(alpha.size() * 4);
            for (std::size_t index = 0; index < alpha.size(); ++index) {
                rgba[index * 4 + 0] = 255;
                rgba[index * 4 + 1] = 255;
                rgba[index * 4 + 2] = 255;
                rgba[index * 4 + 3] = alpha[index];
            }
            return rgba;
        }
    } // namespace

    // SDF 路径（不可变参数 state + 有界 LRU，策略见
    // docs/references/render_device_path_contract.md §9）。
    struct SdlRenderDevice::SdfPath {
        using Cache = detail::ParameterStateCache<SDL_GPURenderState*, 8>;
        using Key = Cache::Key;
        static constexpr std::size_t kCacheCapacity = 256;

        SDL_Renderer* renderer = nullptr;
        SDL_GPUShader* shader = nullptr;
        SDL_GPURenderState* bound = nullptr;
        SDL_Texture* white = nullptr;
        std::uint64_t binds = 0;
        Cache cache;

        SdfPath(SDL_Renderer* target, SDL_GPUShader* fragment, SDL_Texture* white_texture):
            renderer(target),
            shader(fragment),
            white(white_texture),
            cache(
                kCacheCapacity,
                [this](const Key& key) { return create_state(key); },
                [this](SDL_GPURenderState* state) {
                    if (state != nullptr) {
                        if (bound == state) {
                            unbind();
                        }
                        SDL_DestroyGPURenderState(state);
                    }
                }
            ) {}

        ~SdfPath() {
            (void)SDL_SetGPURenderState(renderer, nullptr);
            bound = nullptr;
            cache.clear();
            if (white != nullptr) {
                SDL_DestroyTexture(white);
            }
            if (shader != nullptr) {
                // 释放 shader 需要 GPU device：从 renderer 属性取，而不是让调用方再传一份所有权。
                const auto properties = SDL_GetRendererProperties(renderer);
                auto* gpu = static_cast<SDL_GPUDevice*>(SDL_GetPointerProperty(
                    properties,
                    SDL_PROP_RENDERER_GPU_DEVICE_POINTER,
                    nullptr
                ));
                if (gpu != nullptr) {
                    SDL_ReleaseGPUShader(gpu, shader);
                }
            }
        }

        SdfPath(const SdfPath&) = delete;
        SdfPath& operator=(const SdfPath&) = delete;

        void bind(SDL_GPURenderState* state) {
            if (bound == state) {
                return;
            }
            check_sdl(SDL_SetGPURenderState(renderer, state), "SDL_SetGPURenderState");
            bound = state;
            if (state != nullptr) {
                ++binds;
            }
        }

        void unbind() {
            bind(nullptr);
        }

        auto create_state(const Key& key) -> SDL_GPURenderState* {
            SDL_GPURenderStateCreateInfo info {};
            info.fragment_shader = shader;
            auto* state = SDL_CreateGPURenderState(renderer, &info);
            check_sdl(state != nullptr, "SDL_CreateGPURenderState");
            // 参数块就是缓存键本身：创建即播种，之后不再上传，从结构上排除"忘记更新"。
            if (!SDL_SetGPURenderStateFragmentUniforms(state, 0, key.data(), sizeof(key))) {
                const auto error =
                    std::string("SDL_SetGPURenderStateFragmentUniforms: ") + SDL_GetError();
                SDL_DestroyGPURenderState(state);
                throw std::runtime_error(error);
            }
            return state;
        }
    };

    SdlRenderDevice::SdlRenderDevice(SDL_Renderer& renderer): renderer_(&renderer) {
        // 几何图元的混合必须在这里打开：`SDL_RenderGeometry` / `FillRect` / 线段都沿用 renderer 的
        // draw blend mode，而纹理另有自己的 `SDL_SetTextureBlendMode`。不设置时默认不混合，
        // 所有半透明几何都会被画成不透明 —— 表现是按钮状态层白底白字、涟漪变成实心硬边盘、
        // 对话框遮罩把整页压黑（D7 的缺陷，见 reference_draft.md）。
        //
        // 放在构造（设备初始化边界）而不是逐帧设置：renderer 由平台创建、只交给本设备绘制，
        // 帧内没有任何路径会改这个状态；逐帧重设反而会掩盖"某处偷改状态"的问题。
        check_sdl(
            SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND),
            "SDL_SetRenderDrawBlendMode(BLEND)"
        );

        // SDF 路径是可选增强：普通 renderer（软件/OpenGL）上 SDL_CreateGPURenderState 会失败，
        // 此时如实记录原因并继续走 CPU 几何，而不是让整个页面不可用。
        SDL_GPUShaderCreateInfo shader_info {};
        const auto shader_words = std::span(detail::ROUNDED_SPIRV);
        shader_info.code = reinterpret_cast<const Uint8*>(shader_words.data());
        shader_info.code_size = shader_words.size_bytes();
        shader_info.entrypoint = "main";
        shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        shader_info.num_samplers = 1;
        shader_info.num_uniform_buffers = 1;
        auto* gpu = static_cast<SDL_GPUDevice*>(SDL_GetPointerProperty(
            SDL_GetRendererProperties(renderer_),
            SDL_PROP_RENDERER_GPU_DEVICE_POINTER,
            nullptr
        ));
        if (gpu == nullptr) {
            sdf_reason_ = "renderer has no GPU device (SDF requires the GPU renderer)";
        }
        else {
            auto* shader = SDL_CreateGPUShader(gpu, &shader_info);
            if (shader == nullptr) {
                sdf_reason_ = std::string("SDL_CreateGPUShader: ") + SDL_GetError();
            }
            else {
                auto* white = SDL_CreateTexture(
                    renderer_,
                    SDL_PIXELFORMAT_RGBA32,
                    SDL_TEXTUREACCESS_STATIC,
                    1,
                    1
                );
                if (white == nullptr) {
                    SDL_ReleaseGPUShader(gpu, shader);
                    sdf_reason_ = std::string("SDL_CreateTexture(white): ") + SDL_GetError();
                }
                else {
                    const std::array<Uint8, 4> pixel {255, 255, 255, 255};
                    if (!SDL_UpdateTexture(white, nullptr, pixel.data(), 4)) {
                        sdf_reason_ = std::string("SDL_UpdateTexture(white): ") + SDL_GetError();
                        SDL_DestroyTexture(white);
                        SDL_ReleaseGPUShader(gpu, shader);
                    }
                    else {
                        auto candidate = std::make_unique<SdfPath>(renderer_, shader, white);
                        // 用一个真实 state 探测可用性：非 GPU renderer 上会失败。
                        SDL_GPURenderStateCreateInfo probe_info {};
                        probe_info.fragment_shader = shader;
                        auto* probe_state = SDL_CreateGPURenderState(renderer_, &probe_info);
                        if (probe_state == nullptr) {
                            sdf_reason_ =
                                std::string("SDL_CreateGPURenderState: ") + SDL_GetError();
                            candidate.reset();
                        }
                        else {
                            SDL_DestroyGPURenderState(probe_state);
                            sdf_ = std::move(candidate);
                        }
                    }
                }
            }
        }
    }

    SdlRenderDevice::~SdlRenderDevice() = default;

    auto SdlRenderDevice::stats() const -> const SdlDeviceStats& {
        return stats_;
    }

    auto SdlRenderDevice::analytic_antialiasing() const -> bool {
        float scale = 0.0F;
        return sdf_scale(scale);
    }

    auto SdlRenderDevice::sdf_diagnostics() const -> SdfDiagnostics {
        if (!sdf_) {
            return {};
        }
        return {
            .binds = sdf_->binds,
            .hits = sdf_->cache.hits(),
            .misses = sdf_->cache.misses(),
            .cached = sdf_->cache.size(),
            .capacity = sdf_->cache.capacity(),
        };
    }

    auto SdlRenderDevice::sdf_unavailable_reason() const -> std::string_view {
        return sdf_reason_;
    }

    auto SdlRenderDevice::sdf_scale(float& scale) const -> bool {
        if (!sdf_) {
            return false;
        }
        float horizontal = 1.0F;
        float vertical = 1.0F;
        int width = 0;
        int height = 0;
        SDL_RendererLogicalPresentation presentation {};
        SDL_FRect destination {};
        if (!SDL_GetRenderLogicalPresentation(renderer_, &width, &height, &presentation)
            || !SDL_GetRenderLogicalPresentationRect(renderer_, &destination))
        {
            return false;
        }
        // SDF 的距离场假设各向同性；非等比缩放时无法用同一个半径表达，交回 CPU 几何。
        if (!SDL_GetRenderScale(renderer_, &horizontal, &vertical)) {
            return false;
        }
        if (presentation != SDL_LOGICAL_PRESENTATION_DISABLED && width > 0 && height > 0) {
            horizontal *= destination.w / static_cast<float>(width);
            vertical *= destination.h / static_cast<float>(height);
        }
        if (!std::isfinite(horizontal) || !std::isfinite(vertical) || horizontal <= 0.0F
            || vertical <= 0.0F || horizontal != vertical)
        {
            return false;
        }
        scale = horizontal;
        return true;
    }

    void SdlRenderDevice::unbind_sdf() {
        if (sdf_) {
            sdf_->unbind();
        }
    }

    auto SdlRenderDevice::draw_sdf_shape(
        const foundation::NanRect& bounds,
        const float radius,
        const float half_width,
        const int mode,
        const foundation::NanColor& color
    ) -> bool {
        float scale = 0.0F;
        if (!sdf_scale(scale)) {
            unbind_sdf();
            return false;
        }
        // 覆盖 quad 要包住 AA 过渡区：设备空间 1px（逻辑坐标 1/scale）。
        const auto cover = bounds.expanded(std::max(half_width, 0.0F) + 1.0F / scale);
        const auto vertex_color = to_vertex_color(color);
        const std::array<SDL_Vertex, 4> vertices {{
            {{cover.get_left(), cover.get_top()}, vertex_color, {0.0F, 0.0F}},
            {{cover.get_right(), cover.get_top()}, vertex_color, {1.0F, 0.0F}},
            {{cover.get_right(), cover.get_bottom()}, vertex_color, {1.0F, 1.0F}},
            {{cover.get_left(), cover.get_bottom()}, vertex_color, {0.0F, 1.0F}},
        }};
        const std::array<int, 6> indices {0, 1, 2, 0, 2, 3};

        // 参数块（目标像素）：中心、半尺寸、圆角半径、半线宽、模式。
        const auto center = bounds.get_center();
        SDL_Rect viewport {};
        SDL_FRect destination {};
        check_sdl(SDL_GetRenderViewport(renderer_, &viewport), "SDL_GetRenderViewport");
        check_sdl(
            SDL_GetRenderLogicalPresentationRect(renderer_, &destination),
            "SDL_GetRenderLogicalPresentationRect"
        );
        // Match SDL's integer pixel viewport origin, including letterboxing.
        const auto origin_x = std::floor(viewport.x * scale + destination.x);
        const auto origin_y = std::floor(viewport.y * scale + destination.y);
        const std::array<float, 8> values {
            center.get_x() * scale + origin_x,
            center.get_y() * scale + origin_y,
            bounds.get_width() * 0.5F * scale,
            bounds.get_height() * 0.5F * scale,
            radius * scale,
            half_width * scale,
            static_cast<float>(mode),
            0.0F,
        };
        SdfPath::Key key {};
        static_assert(sizeof(values) == sizeof(key));
        std::memcpy(key.data(), values.data(), sizeof(values));

        try {
            const auto acquired = sdf_->cache.acquire(key);
            sdf_->bind(acquired.handle);
        }
        catch (const std::runtime_error& error) {
            sdf_reason_ = error.what();
            sdf_.reset();
            return false;
        }
        check_sdl(
            SDL_RenderGeometry(renderer_, sdf_->white, vertices.data(), 4, indices.data(), 6),
            "SDL_RenderGeometry(SDF)"
        );
        return true;
    }

    void SdlRenderDevice::begin_frame() {
        ++stats_.frames;
        unbind_sdf();
    }

    void SdlRenderDevice::end_frame() {
        unbind_sdf();
        // present 由宿主负责：设备只负责把图元提交给 renderer（与 raylib 后端的边界一致）。
    }

    void SdlRenderDevice::clear(const foundation::NanColor& color) {
        set_color(*renderer_, color);
        check_sdl(SDL_RenderClear(renderer_), "SDL_RenderClear");
    }

    void SdlRenderDevice::set_clip(const foundation::NanRect& screen_rect) {
        ++stats_.clips;
        const SDL_Rect clip {
            .x = static_cast<int>(std::lround(screen_rect.get_left())),
            .y = static_cast<int>(std::lround(screen_rect.get_top())),
            .w = static_cast<int>(std::lround(screen_rect.get_width())),
            .h = static_cast<int>(std::lround(screen_rect.get_height())),
        };
        check_sdl(SDL_SetRenderClipRect(renderer_, &clip), "SDL_SetRenderClipRect");
    }

    void SdlRenderDevice::clear_clip() {
        check_sdl(SDL_SetRenderClipRect(renderer_, nullptr), "SDL_SetRenderClipRect(clear)");
    }

    void
    SdlRenderDevice::draw_rect(const foundation::NanRect& rect, const foundation::NanColor& color) {
        ++stats_.rects;
        if (draw_sdf_shape(rect, 0.0F, 0.0F, 0, color)) {
            return;
        }
        set_color(*renderer_, color);
        const SDL_FRect target {
            .x = rect.get_left(),
            .y = rect.get_top(),
            .w = rect.get_width(),
            .h = rect.get_height(),
        };
        check_sdl(SDL_RenderFillRect(renderer_, &target), "SDL_RenderFillRect");
    }

    void SdlRenderDevice::draw_rect_outline(
        const foundation::NanRect& rect,
        const float thickness,
        const foundation::NanColor& color
    ) {
        ++stats_.rect_outlines;
        const auto edge = std::max(thickness, 0.0F);
        if (edge <= 0.0F) {
            return;
        }
        {
            const auto outline = detail::sdf_inner_outline_geometry(rect, 0.0F, edge);
            if (draw_sdf_shape(
                    outline.centerline_bounds,
                    outline.centerline_radius,
                    outline.half_width,
                    1,
                    color
                ))
            {
                return;
            }
        }
        // 四条边分别填充：SDL 的 SDL_RenderRect 只有 1px，厚度要自己拼。
        draw_rect(
            foundation::NanRect(
                rect.get_left(),
                rect.get_top(),
                rect.get_right(),
                rect.get_top() + edge
            ),
            color
        );
        draw_rect(
            foundation::NanRect(
                rect.get_left(),
                rect.get_bottom() - edge,
                rect.get_right(),
                rect.get_bottom()
            ),
            color
        );
        draw_rect(
            foundation::NanRect(
                rect.get_left(),
                rect.get_top() + edge,
                rect.get_left() + edge,
                rect.get_bottom() - edge
            ),
            color
        );
        draw_rect(
            foundation::NanRect(
                rect.get_right() - edge,
                rect.get_top() + edge,
                rect.get_right(),
                rect.get_bottom() - edge
            ),
            color
        );
    }

    auto SdlRenderDevice::draw_fan(
        const foundation::NanPoint& center,
        const float radius,
        const float start_radians,
        const float end_radians,
        const foundation::NanColor& color
    ) -> int {
        const auto segments = segments_for(radius);
        std::vector<SDL_Vertex> vertices;
        vertices.reserve(static_cast<std::size_t>(segments) + 2);
        const auto converted = to_sdl_color(color);
        const auto vertex = [&](const float x, const float y) {
            return SDL_Vertex {
                .position = SDL_FPoint {.x = x, .y = y},
                .color =
                    SDL_FColor {
                        .r = static_cast<float>(converted.r) / 255.0F,
                        .g = static_cast<float>(converted.g) / 255.0F,
                        .b = static_cast<float>(converted.b) / 255.0F,
                        .a = static_cast<float>(converted.a) / 255.0F,
                    },
                .tex_coord = SDL_FPoint {.x = 0.0F, .y = 0.0F},
            };
        };
        vertices.push_back(vertex(center.get_x(), center.get_y()));
        for (int index = 0; index <= segments; ++index) {
            const auto angle = start_radians
                + (end_radians - start_radians) * static_cast<float>(index)
                    / static_cast<float>(segments);
            vertices.push_back(vertex(
                center.get_x() + radius * std::cos(angle),
                center.get_y() + radius * std::sin(angle)
            ));
        }
        std::vector<int> indices;
        indices.reserve(static_cast<std::size_t>(segments) * 3);
        for (int index = 1; index <= segments; ++index) {
            indices.push_back(0);
            indices.push_back(index);
            indices.push_back(index + 1);
        }
        check_sdl(
            SDL_RenderGeometry(
                renderer_,
                nullptr,
                vertices.data(),
                static_cast<int>(vertices.size()),
                indices.data(),
                static_cast<int>(indices.size())
            ),
            "SDL_RenderGeometry(fan)"
        );
        stats_.generated_vertices += vertices.size();
        return static_cast<int>(vertices.size());
    }

    void SdlRenderDevice::draw_rounded_rect(
        const foundation::NanRect& rect,
        const float radius,
        const foundation::NanColor& color
    ) {
        ++stats_.rounded_rects;
        const auto clamped =
            std::clamp(radius, 0.0F, std::min(rect.get_width(), rect.get_height()) * 0.5F);
        if (draw_sdf_shape(rect, clamped, 0.0F, 0, color)) {
            return;
        }
        if (clamped <= 0.5F) {
            draw_rect(rect, color);
            return;
        }

        // 圆角矩形 = 中心矩形 + 左右边条 + 四个 90° 圆角扇。
        //
        // 为什么不做「整圈轮廓一次扇形三角化」：那样要保证轮廓点严格凸且顺序一致，
        // 一旦角度方向写反就会退化成自交多边形（实测渲染出 V 形）。拆成中间矩形 +
        // 四个**各自凸**的扇形则没有这个前提，也不依赖任何隐式约定。
        const auto left = rect.get_left();
        const auto top = rect.get_top();
        const auto right = rect.get_right();
        const auto bottom = rect.get_bottom();

        draw_rect(foundation::NanRect(left + clamped, top, right - clamped, bottom), color);
        draw_rect(
            foundation::NanRect(left, top + clamped, left + clamped, bottom - clamped),
            color
        );
        draw_rect(
            foundation::NanRect(right - clamped, top + clamped, right, bottom - clamped),
            color
        );

        // 角度以 +X 为 0、向 +Y（屏幕向下）增大。
        (void)draw_fan(
            foundation::NanPoint(left + clamped, top + clamped),
            clamped,
            kPi,
            kPi * 1.5F,
            color
        );
        (void)draw_fan(
            foundation::NanPoint(right - clamped, top + clamped),
            clamped,
            kPi * 1.5F,
            kPi * 2.0F,
            color
        );
        (void)draw_fan(
            foundation::NanPoint(right - clamped, bottom - clamped),
            clamped,
            0.0F,
            kPi * 0.5F,
            color
        );
        (void)draw_fan(
            foundation::NanPoint(left + clamped, bottom - clamped),
            clamped,
            kPi * 0.5F,
            kPi,
            color
        );
    }

    void SdlRenderDevice::draw_arc(
        const foundation::NanPoint& center,
        const float inner_radius,
        const float outer_radius,
        const float start_radians,
        const float sweep_radians,
        const foundation::NanColor& color
    ) {
        ++stats_.arcs;
        unbind_sdf();
        if (!std::isfinite(center.get_x()) || !std::isfinite(center.get_y())
            || !std::isfinite(inner_radius) || !std::isfinite(outer_radius)
            || !std::isfinite(start_radians) || !std::isfinite(sweep_radians)
            || outer_radius <= 0.0F || inner_radius < 0.0F || inner_radius >= outer_radius
            || std::abs(sweep_radians) < 1.0e-4F)
        {
            return;
        }
        // 与圆角描边同一套几何：同心环扇（外半径 / 内半径、同一圆心）。接口默认实现是
        // 弦逼近的线段序列（≤64 段），在细环上会露出明显折线 —— Spinner/ProgressBar 走这条。
        const auto sweep = std::abs(sweep_radians);
        const auto start = sweep_radians < 0.0F ? start_radians + sweep_radians : start_radians;
        const auto segments = std::clamp(static_cast<int>(std::ceil(sweep * 12.0F)), 6, 96);
        const auto converted = to_sdl_color(color);
        const auto vertex = [&](const float x, const float y) {
            return SDL_Vertex {
                .position = SDL_FPoint {.x = x, .y = y},
                .color =
                    SDL_FColor {
                        .r = static_cast<float>(converted.r) / 255.0F,
                        .g = static_cast<float>(converted.g) / 255.0F,
                        .b = static_cast<float>(converted.b) / 255.0F,
                        .a = static_cast<float>(converted.a) / 255.0F,
                    },
                .tex_coord = SDL_FPoint {.x = 0.0F, .y = 0.0F},
            };
        };
        std::vector<SDL_Vertex> vertices;
        vertices.reserve(static_cast<std::size_t>(segments + 1) * 2);
        for (int index = 0; index <= segments; ++index) {
            const auto angle =
                start + sweep * static_cast<float>(index) / static_cast<float>(segments);
            const auto cos = std::cos(angle);
            const auto sin = std::sin(angle);
            vertices.push_back(
                vertex(center.get_x() + outer_radius * cos, center.get_y() + outer_radius * sin)
            );
            vertices.push_back(
                vertex(center.get_x() + inner_radius * cos, center.get_y() + inner_radius * sin)
            );
        }
        std::vector<int> indices;
        indices.reserve(static_cast<std::size_t>(segments) * 6);
        for (int index = 0; index < segments; ++index) {
            const auto outer0 = index * 2;
            const auto inner0 = index * 2 + 1;
            const auto outer1 = index * 2 + 2;
            const auto inner1 = index * 2 + 3;
            indices.insert(indices.end(), {outer0, outer1, inner1, outer0, inner1, inner0});
        }
        check_sdl(
            SDL_RenderGeometry(
                renderer_,
                nullptr,
                vertices.data(),
                static_cast<int>(vertices.size()),
                indices.data(),
                static_cast<int>(indices.size())
            ),
            "SDL_RenderGeometry(arc)"
        );
        stats_.generated_vertices += vertices.size();
    }

    void SdlRenderDevice::draw_rounded_rect_outline(
        const foundation::NanRect& rect,
        const float radius,
        const float thickness,
        const foundation::NanColor& color
    ) {
        ++stats_.rounded_outlines;
        const auto edge = std::max(thickness, 0.0F);
        const auto clamped =
            std::clamp(radius, 0.0F, std::min(rect.get_width(), rect.get_height()) * 0.5F);
        if (edge <= 0.0F) {
            return;
        }
        {
            const auto outline = detail::sdf_inner_outline_geometry(rect, clamped, edge);
            if (draw_sdf_shape(
                    outline.centerline_bounds,
                    outline.centerline_radius,
                    outline.half_width,
                    1,
                    color
                ))
            {
                return;
            }
        }
        if (clamped <= 0.5F) {
            draw_rect_outline(rect, edge, color);
            return;
        }

        // 圆角描边 = 四条直边矩形 + 四个**同心环扇**（外半径 r、内半径 r-t，圆心相同）。
        // 内边界正是外边界向内偏移 t：半径减 t、四边各内缩 t，圆心不变。
        const auto left = rect.get_left();
        const auto top = rect.get_top();
        const auto right = rect.get_right();
        const auto bottom = rect.get_bottom();
        const auto inner_radius = std::max(0.0F, clamped - edge);

        draw_rect(foundation::NanRect(left + clamped, top, right - clamped, top + edge), color);
        draw_rect(
            foundation::NanRect(left + clamped, bottom - edge, right - clamped, bottom),
            color
        );
        draw_rect(foundation::NanRect(left, top + clamped, left + edge, bottom - clamped), color);
        draw_rect(foundation::NanRect(right - edge, top + clamped, right, bottom - clamped), color);

        const auto ring = [&](const foundation::NanPoint& center,
                              const float from,
                              const float to) {
            const auto segments = segments_for(clamped);
            const auto converted = to_sdl_color(color);
            const auto vertex = [&](const float x, const float y) {
                return SDL_Vertex {
                    .position = SDL_FPoint {.x = x, .y = y},
                    .color =
                        SDL_FColor {
                            .r = static_cast<float>(converted.r) / 255.0F,
                            .g = static_cast<float>(converted.g) / 255.0F,
                            .b = static_cast<float>(converted.b) / 255.0F,
                            .a = static_cast<float>(converted.a) / 255.0F,
                        },
                    .tex_coord = SDL_FPoint {.x = 0.0F, .y = 0.0F},
                };
            };
            std::vector<SDL_Vertex> vertices;
            vertices.reserve(static_cast<std::size_t>(segments + 1) * 2);
            for (int index = 0; index <= segments; ++index) {
                const auto angle =
                    from + (to - from) * static_cast<float>(index) / static_cast<float>(segments);
                const auto cos = std::cos(angle);
                const auto sin = std::sin(angle);
                vertices.push_back(
                    vertex(center.get_x() + clamped * cos, center.get_y() + clamped * sin)
                );
                vertices.push_back(
                    vertex(center.get_x() + inner_radius * cos, center.get_y() + inner_radius * sin)
                );
            }
            std::vector<int> indices;
            indices.reserve(static_cast<std::size_t>(segments) * 6);
            for (int index = 0; index < segments; ++index) {
                const auto outer0 = index * 2;
                const auto inner0 = index * 2 + 1;
                const auto outer1 = index * 2 + 2;
                const auto inner1 = index * 2 + 3;
                indices.insert(indices.end(), {outer0, outer1, inner1, outer0, inner1, inner0});
            }
            check_sdl(
                SDL_RenderGeometry(
                    renderer_,
                    nullptr,
                    vertices.data(),
                    static_cast<int>(vertices.size()),
                    indices.data(),
                    static_cast<int>(indices.size())
                ),
                "SDL_RenderGeometry(rounded outline)"
            );
            stats_.generated_vertices += vertices.size();
        };

        ring(foundation::NanPoint(right - clamped, top + clamped), -kPi * 0.5F, 0.0F);
        ring(foundation::NanPoint(right - clamped, bottom - clamped), 0.0F, kPi * 0.5F);
        ring(foundation::NanPoint(left + clamped, bottom - clamped), kPi * 0.5F, kPi);
        ring(foundation::NanPoint(left + clamped, top + clamped), kPi, kPi * 1.5F);
    }

    void SdlRenderDevice::draw_line(
        const foundation::NanPoint& a,
        const foundation::NanPoint& b,
        const float thickness,
        const foundation::NanColor& color
    ) {
        ++stats_.lines;
        unbind_sdf();
        const auto half = std::max(thickness, 0.0F) * 0.5F;
        const auto dx = b.get_x() - a.get_x();
        const auto dy = b.get_y() - a.get_y();
        const auto length = std::sqrt(dx * dx + dy * dy);
        if (length <= 1.0e-5F) {
            return;
        }
        // 法线方向偏移出四边形；比 SDL_RenderLine（1px）更符合 IRenderDevice 的厚度契约。
        const auto nx = -dy / length * half;
        const auto ny = dx / length * half;
        const auto converted = to_sdl_color(color);
        const auto vertex = [&](const float x, const float y) {
            return SDL_Vertex {
                .position = SDL_FPoint {.x = x, .y = y},
                .color =
                    SDL_FColor {
                        .r = static_cast<float>(converted.r) / 255.0F,
                        .g = static_cast<float>(converted.g) / 255.0F,
                        .b = static_cast<float>(converted.b) / 255.0F,
                        .a = static_cast<float>(converted.a) / 255.0F,
                    },
                .tex_coord = SDL_FPoint {.x = 0.0F, .y = 0.0F},
            };
        };
        const SDL_Vertex vertices[4] = {
            vertex(a.get_x() + nx, a.get_y() + ny),
            vertex(b.get_x() + nx, b.get_y() + ny),
            vertex(b.get_x() - nx, b.get_y() - ny),
            vertex(a.get_x() - nx, a.get_y() - ny),
        };
        const int indices[6] = {0, 1, 2, 0, 2, 3};
        check_sdl(
            SDL_RenderGeometry(renderer_, nullptr, vertices, 4, indices, 6),
            "SDL_RenderGeometry(line)"
        );
        stats_.generated_vertices += 4;
    }

    void SdlRenderDevice::draw_circle(
        const foundation::NanPoint& center,
        const float radius,
        const foundation::NanColor& color
    ) {
        ++stats_.circles;
        if (radius <= 0.0F) {
            return;
        }
        if (draw_sdf_shape(
                foundation::NanRect::from_ltrb(
                    center.get_x() - radius,
                    center.get_y() - radius,
                    center.get_x() + radius,
                    center.get_y() + radius
                ),
                radius,
                0.0F,
                0,
                color
            ))
        {
            return;
        }
        (void)draw_fan(center, radius, 0.0F, kPi * 2.0F, color);
    }

    void SdlRenderDevice::draw_circle_clipped_rounded_rect(
        const foundation::NanPoint& center,
        const float circle_radius,
        const foundation::NanRect& clip_rect,
        const float clip_radius,
        const foundation::NanColor& color
    ) {
        // 生产后端用 SDF 求解析交集；这里用「矩形裁剪 + 圆」的组合近似。差异是**能力差异**，
        // 不是等价实现：圆角处的裁剪会退化成方角。探测记录已把这条列为待比对项。
        set_clip(clip_rect);
        draw_circle(center, circle_radius, color);
        clear_clip();
        (void)clip_radius;
    }

    void SdlRenderDevice::draw_text(
        const std::string_view text,
        const foundation::NanPoint& pos,
        const float font_size,
        const foundation::NanColor& color
    ) {
        ++stats_.texts;
        unbind_sdf();
        set_color(*renderer_, color);
        // SDL 的内置 8x8 调试字体：能证明「文字能落到同一套坐标/裁剪里」，
        // 但**不是**真正的文本路径（真正的路径是 update_alpha_texture_region + draw_texture_region）。
        // SDL_RenderDebugText 用当前 draw color 渲染，字号靠 render scale 缩放。
        const auto scale = std::max(font_size / 8.0F, 0.5F);
        check_sdl(SDL_SetRenderScale(renderer_, scale, scale), "SDL_SetRenderScale");
        const std::string owned(text);
        check_sdl(
            SDL_RenderDebugText(renderer_, pos.get_x() / scale, pos.get_y() / scale, owned.c_str()),
            "SDL_RenderDebugText"
        );
        check_sdl(SDL_SetRenderScale(renderer_, 1.0F, 1.0F), "SDL_SetRenderScale(reset)");
    }

    auto SdlRenderDevice::supports_alpha_textures() const -> bool {
        return true;
    }

    auto SdlRenderDevice::create_alpha_texture(
        const int width,
        const int height,
        const std::span<const std::uint8_t> alpha
    ) -> render::TextureHandle {
        if (width <= 0 || height <= 0
            || alpha.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
        {
            return {};
        }
        ++stats_.alpha_uploads;
        if (std::getenv("NANDINA_PROBE_TRACE_TEXTURE") != nullptr) {
            std::fprintf(stderr, "[atlas] create %dx%d bytes=%zu\n", width, height, alpha.size());
        }
        // 用 `SDL_PIXELFORMAT_RGBA32`（平台首选 32 位 RGBA）而不是 `RGBA8888`：
        // 后者是**打包**格式（0xRRGGBBAA），小端机上的内存字节序是 A,B,G,R，与下面
        // alpha_to_rgba 产出的 R,G,B,A 缓冲不一致 —— alpha 会从错误的字节读出来，
        // 表现为「每个字形画成一个不透明方块」。实测就是这么错的。
        auto* texture = SDL_CreateTexture(
            renderer_,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STATIC,
            width,
            height
        );
        check_sdl(texture != nullptr, "SDL_CreateTexture(alpha atlas)");
        check_sdl(SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND), "SDL_SetTextureBlendMode");
        // 与生产后端一致：字形图集按 1:1 点采样，避免双线性过滤造成二次软化与邻区渗色
        // （raylib 后端同样设了 TEXTURE_FILTER_POINT）。
        check_sdl(
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST),
            "SDL_SetTextureScaleMode"
        );
        const auto rgba = alpha_to_rgba(
            alpha.first(static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
        );
        check_sdl(
            SDL_UpdateTexture(texture, nullptr, rgba.data(), width * 4),
            "SDL_UpdateTexture(alpha atlas)"
        );
        const auto handle = render::TextureHandle {next_texture_++};
        textures_[handle.value] = texture;
        return handle;
    }

    auto SdlRenderDevice::lookup(const render::TextureHandle handle) const -> SDL_Texture* {
        const auto found = textures_.find(handle.value);
        return found != textures_.end() ? found->second : nullptr;
    }

    void SdlRenderDevice::update_alpha_texture(
        const render::TextureHandle texture,
        const int width,
        const int height,
        const std::span<const std::uint8_t> alpha
    ) {
        auto* target = lookup(texture);
        if (target == nullptr || width <= 0 || height <= 0) {
            return;
        }
        ++stats_.alpha_uploads;
        const auto rgba = alpha_to_rgba(
            alpha.first(static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
        );
        check_sdl(SDL_UpdateTexture(target, nullptr, rgba.data(), width * 4), "SDL_UpdateTexture");
    }

    void SdlRenderDevice::update_alpha_texture_region(
        const render::TextureHandle texture,
        const int full_width,
        const int full_height,
        const foundation::NanRect& region,
        const std::span<const std::uint8_t> alpha
    ) {
        auto* target = lookup(texture);
        if (target == nullptr) {
            return;
        }
        // SDL_UpdateTexture 支持子矩形，但需要**整个图集**的紧凑缓冲；接口约定 alpha 就是
        // 完整缓冲，所以这里按行裁剪上传 —— 只传变化的行段，避免整张贴图重传。
        ++stats_.alpha_uploads;
        const auto left =
            std::clamp(static_cast<int>(region.get_left()), 0, std::max(full_width - 1, 0));
        const auto top =
            std::clamp(static_cast<int>(region.get_top()), 0, std::max(full_height - 1, 0));
        const auto width = std::clamp(static_cast<int>(region.get_width()), 1, full_width - left);
        const auto height = std::clamp(static_cast<int>(region.get_height()), 1, full_height - top);
        const SDL_Rect target_rect {.x = left, .y = top, .w = width, .h = height};
        std::vector<std::uint8_t> rgba(
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4
        );
        for (int row = 0; row < height; ++row) {
            const auto* source =
                alpha.data() + static_cast<std::size_t>(top + row) * full_width + left;
            for (int column = 0; column < width; ++column) {
                const auto offset = (static_cast<std::size_t>(row) * width + column) * 4;
                rgba[offset + 0] = 255;
                rgba[offset + 1] = 255;
                rgba[offset + 2] = 255;
                rgba[offset + 3] = source[column];
            }
        }
        check_sdl(
            SDL_UpdateTexture(target, &target_rect, rgba.data(), width * 4),
            "SDL_UpdateTexture(region)"
        );
    }

    void SdlRenderDevice::destroy_texture(const render::TextureHandle texture) {
        const auto found = textures_.find(texture.value);
        if (found == textures_.end()) {
            return;
        }
        SDL_DestroyTexture(found->second);
        textures_.erase(found);
    }

    void SdlRenderDevice::draw_texture_region(
        const render::TextureHandle texture,
        const foundation::NanRect& source,
        const foundation::NanRect& destination,
        const foundation::NanColor& tint
    ) {
        auto* target = lookup(texture);
        if (target == nullptr) {
            return;
        }
        ++stats_.texture_draws;
        unbind_sdf();
        const auto converted = to_sdl_color(tint);
        check_sdl(
            SDL_SetTextureColorMod(target, converted.r, converted.g, converted.b),
            "SDL_SetTextureColorMod"
        );
        check_sdl(SDL_SetTextureAlphaMod(target, converted.a), "SDL_SetTextureAlphaMod");
        const SDL_FRect source_rect {
            .x = source.get_left(),
            .y = source.get_top(),
            .w = source.get_width(),
            .h = source.get_height(),
        };
        const SDL_FRect destination_rect {
            .x = destination.get_left(),
            .y = destination.get_top(),
            .w = destination.get_width(),
            .h = destination.get_height(),
        };
        check_sdl(
            SDL_RenderTexture(renderer_, target, &source_rect, &destination_rect),
            "SDL_RenderTexture"
        );
    }

    auto SdlRenderDevice::texture_size(const render::TextureHandle texture) -> foundation::NanSize {
        auto* target = lookup(texture);
        if (target == nullptr) {
            return foundation::NanSize {};
        }
        float width = 0.0F;
        float height = 0.0F;
        check_sdl(SDL_GetTextureSize(target, &width, &height), "SDL_GetTextureSize");
        return foundation::NanSize(width, height);
    }

    auto SdlRenderDevice::supports_rounded_rect() const -> bool {
        return true;
    }

    auto SdlRenderDevice::supports_rounded_clip() const -> bool {
        // 圆角矩形是用三角形扇画出来的，绘制本身没问题；但**裁剪**只能交给
        // SDL_SetRenderClipRect，那是矩形 —— 圆角处的裁剪会退化成方角。所以这里如实
        // 返回 false，让 RipplePainter 走「自己 set_clip + 画圆」的降级路径。
        return false;
    }

    auto SdlRenderDevice::supports_image_loading() const -> bool {
        // 没有解码器：SDL 平台也不提供 `IImageDecoder`（见 sdl_platform_window 的
        // make_image_decoder）。如实返回 false，让 Image 走"明确不支持"的路径。
        return false;
    }

    auto SdlRenderDevice::supports_soft_shadows() const -> bool {
        // draw_rounded_rect_shadow 未实现（接口默认是 no-op），因此不能声称支持软阴影；
        // ShadowPainter 会退化成硬边圆角阴影。
        return false;
    }
} // namespace nandina::render
