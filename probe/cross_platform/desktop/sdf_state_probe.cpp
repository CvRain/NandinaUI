#include "sdf_state_spirv.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    constexpr SDL_Color BACKGROUND {32, 48, 64, 255};
    constexpr int TARGET_WIDTH = 540;
    constexpr int TARGET_HEIGHT = 360;
    constexpr int DRAWS_PER_FRAME = 256;

    void require(bool condition, const std::string& operation) {
        if (!condition) {
            throw std::runtime_error(operation + ": " + SDL_GetError());
        }
    }

    struct alignas(16) Parameters {
        std::array<float, 4> bounds;
        std::array<float, 4> style;
        std::array<float, 4> multiplier {1, 1, 1, 1};
        std::array<float, 4> sentinel {1, 1, 1, 1};
    };

    static_assert(sizeof(Parameters) == 64);

    struct Shape {
        SDL_FRect bounds;
        float radius;
        float half_stroke;
        SDL_FColor color;
    };

    constexpr Shape FIRST {{20, 20, 80, 64}, 14, 0, {0.8F, 0.2F, 0.1F, 0.5F}};
    constexpr Shape SECOND {{120, 30, 100, 50}, 8, 2, {0.1F, 0.8F, 0.2F, 0.8F}};
    constexpr Shape THIRD {{245, 24, 70, 70}, 24, 0, {0.2F, 0.3F, 0.9F, 0.7F}};
    constexpr Shape OVERLAP {{50, 45, 30, 24}, 4, 0, {0.9F, 0.8F, 0.1F, 0.4F}};
    constexpr Shape THIN {{245, 110, 64, 30}, 0.75F, 0.5F, {0.7F, 0.9F, 0.2F, 0.6F}};

    auto parameters_for(const Shape& shape, float scale) -> Parameters {
        return {
            {(shape.bounds.x + shape.bounds.w / 2) * scale,
             (shape.bounds.y + shape.bounds.h / 2) * scale,
             shape.bounds.w * scale / 2,
             shape.bounds.h * scale / 2},
            {shape.radius * scale,
             shape.half_stroke * scale,
             shape.half_stroke > 0 ? 1.0F : 0.0F,
             3},
        };
    }

    struct Context {
        SDL_GPUDevice* gpu = nullptr;
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        SDL_GPUShader* shader = nullptr;
        SDL_GPUShader* budget_shader = nullptr;
        SDL_GPURenderState* state = nullptr;
        SDL_GPURenderState* budget_state = nullptr;
        SDL_Texture* white = nullptr;
        SDL_Texture* image = nullptr;
        SDL_Texture* target = nullptr;
        int state_sets = 0;
        int uniform_updates = 0;

        ~Context() {
            if (renderer) {
                SDL_SetGPURenderState(renderer, nullptr);
                SDL_FlushRenderer(renderer);
            }
            SDL_DestroyGPURenderState(budget_state);
            SDL_DestroyGPURenderState(state);
            SDL_DestroyTexture(target);
            SDL_DestroyTexture(image);
            SDL_DestroyTexture(white);
            if (budget_shader) {
                SDL_ReleaseGPUShader(gpu, budget_shader);
            }
            if (shader) {
                SDL_ReleaseGPUShader(gpu, shader);
            }
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_DestroyGPUDevice(gpu);
            SDL_Quit();
        }

        void initialize() {
            require(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
            gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
            require(gpu != nullptr, "SDL_CreateGPUDevice(vulkan)");
            window =
                SDL_CreateWindow("Nandina SDF state experiment", TARGET_WIDTH, TARGET_HEIGHT, 0);
            require(window != nullptr, "SDL_CreateWindow");
            renderer = SDL_CreateGPURenderer(gpu, window);
            require(renderer != nullptr, "SDL_CreateGPURenderer");
            require(SDL_SetRenderVSync(renderer, 0), "disable vsync");
            require(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND), "geometry blend");
            shader = make_shader(nandina::probe::SHAPE_SPIRV);
            budget_shader = make_shader(nandina::probe::BUDGET_SPIRV);
            SDL_GPURenderStateCreateInfo info {};
            info.fragment_shader = shader;
            state = SDL_CreateGPURenderState(renderer, &info);
            require(state != nullptr, "shape state");
            info.fragment_shader = budget_shader;
            budget_state = SDL_CreateGPURenderState(renderer, &info);
            require(budget_state != nullptr, "budget state");
            white = make_texture(1, 1, SDL_TEXTUREACCESS_STATIC);
            const std::array<Uint8, 4> white_pixel {255, 255, 255, 255};
            require(SDL_UpdateTexture(white, nullptr, white_pixel.data(), 4), "upload white");
            image = make_texture(2, 2, SDL_TEXTUREACCESS_STATIC);
            const std::array<Uint8, 16> pixels {
                200,
                40,
                80,
                128,
                20,
                220,
                60,
                255,
                40,
                80,
                230,
                255,
                220,
                180,
                40,
                255,
            };
            require(SDL_UpdateTexture(image, nullptr, pixels.data(), 8), "upload RGBA image");
            target = make_texture(TARGET_WIDTH, TARGET_HEIGHT, SDL_TEXTUREACCESS_TARGET);
            const auto properties = SDL_GetGPUDeviceProperties(gpu);
            std::cout
                << "driver=" << SDL_GetGPUDeviceDriver(gpu) << " device="
                << SDL_GetStringProperty(properties, SDL_PROP_GPU_DEVICE_NAME_STRING, "unknown")
                << " SDL=" << SDL_GetVersion() << " uniform_bytes=" << sizeof(Parameters) << '\n';
        }

        auto make_shader(std::span<const std::uint32_t> words) -> SDL_GPUShader* {
            SDL_GPUShaderCreateInfo info {};
            info.code = reinterpret_cast<const Uint8*>(words.data());
            info.code_size = words.size_bytes();
            info.entrypoint = "main";
            info.format = SDL_GPU_SHADERFORMAT_SPIRV;
            info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
            info.num_samplers = 1;
            info.num_uniform_buffers = 1;
            auto* result = SDL_CreateGPUShader(gpu, &info);
            require(result != nullptr, "create embedded shader");
            return result;
        }

        auto make_texture(int width, int height, SDL_TextureAccess access) -> SDL_Texture* {
            auto* result =
                SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, access, width, height);
            require(result != nullptr, "create texture");
            require(SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND), "texture blend");
            require(SDL_SetTextureScaleMode(result, SDL_SCALEMODE_NEAREST), "nearest sampling");
            return result;
        }

        void set_state(SDL_GPURenderState* next) {
            require(SDL_SetGPURenderState(renderer, next), "set render state");
            ++state_sets;
        }

        void upload(const Parameters& parameters) {
            require(
                SDL_SetGPURenderStateFragmentUniforms(state, 0, &parameters, sizeof(parameters)),
                "update uniform"
            );
            ++uniform_updates;
        }

        void clear() {
            require(
                SDL_SetRenderDrawColor(renderer, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255),
                "background"
            );
            require(SDL_RenderClear(renderer), "clear");
        }

        void quad(const Shape& shape, float scale) {
            const float padding = shape.half_stroke + 1.0F / scale;
            const float left = shape.bounds.x - padding;
            const float top = shape.bounds.y - padding;
            const float right = shape.bounds.x + shape.bounds.w + padding;
            const float bottom = shape.bounds.y + shape.bounds.h + padding;
            const std::array<SDL_Vertex, 4> vertices {{
                {{left, top}, shape.color, {0, 0}},
                {{right, top}, shape.color, {1, 0}},
                {{right, bottom}, shape.color, {1, 1}},
                {{left, bottom}, shape.color, {0, 1}},
            }};
            const std::array<int, 6> indices {0, 1, 2, 0, 2, 3};
            require(
                SDL_RenderGeometry(
                    renderer,
                    white,
                    vertices.data(),
                    vertices.size(),
                    indices.data(),
                    indices.size()
                ),
                "draw quad"
            );
        }

        void draw(const Shape& shape, float scale, bool update = true) {
            if (update) {
                upload(parameters_for(shape, scale));
            }
            set_state(state);
            quad(shape, scale);
        }
    };

    using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;

    auto pixel(SDL_Surface* surface, int horizontal, int vertical) -> SDL_Color {
        SDL_Color result {};
        require(
            SDL_ReadSurfacePixel(
                surface,
                horizontal,
                vertical,
                &result.r,
                &result.g,
                &result.b,
                &result.a
            ),
            "read pixel"
        );
        return result;
    }

    auto over(SDL_Color background, SDL_FColor foreground) -> SDL_Color {
        const auto channel = [alpha = foreground.a](Uint8 base, float value) {
            return static_cast<Uint8>(std::lround(base * (1 - alpha) + value * 255 * alpha));
        };
        return {
            channel(background.r, foreground.r),
            channel(background.g, foreground.g),
            channel(background.b, foreground.b),
            255
        };
    }

    void expect(
        SDL_Surface* surface,
        int horizontal,
        int vertical,
        SDL_Color expected,
        const std::string& label
    ) {
        const auto actual = pixel(surface, horizontal, vertical);
        if (std::abs(actual.r - expected.r) > 2 || std::abs(actual.g - expected.g) > 2
            || std::abs(actual.b - expected.b) > 2)
        {
            throw std::runtime_error(
                "pixel mismatch: " + label + " at " + std::to_string(horizontal) + ","
                + std::to_string(vertical) + " actual=" + std::to_string(actual.r) + ","
                + std::to_string(actual.g) + "," + std::to_string(actual.b)
                + " expected=" + std::to_string(expected.r) + "," + std::to_string(expected.g) + ","
                + std::to_string(expected.b)
            );
        }
    }

    void verify_scene(Context& context, float scale, std::string_view fault) {
        require(SDL_SetRenderTarget(context.renderer, context.target), "set target");
        require(SDL_SetRenderScale(context.renderer, scale, scale), "set scale");
        context.clear();
        context.draw(FIRST, scale);
        if (fault != "restore") {
            context.set_state(nullptr);
        }
        const SDL_FRect destination {120, 120, 32, 32};
        require(
            SDL_RenderTexture(context.renderer, context.image, nullptr, &destination),
            "draw uploaded image"
        );
        require(SDL_SetRenderDrawColor(context.renderer, 240, 230, 210, 255), "text color");
        require(SDL_RenderDebugText(context.renderer, 24, 130, "SDF"), "interleaved glyph texture");
        context.draw(SECOND, scale, fault != "uniform");
        context.set_state(nullptr);
        context.draw(THIRD, scale);
        context.set_state(nullptr);
        context.draw(OVERLAP, scale);
        context.set_state(nullptr);
        context.draw(THIN, scale);
        context.set_state(nullptr);
        require(SDL_RenderDebugText(context.renderer, 24, 190, "SDF"), "reference glyph texture");
        Surface surface(SDL_RenderReadPixels(context.renderer, nullptr), SDL_DestroySurface);
        require(surface != nullptr, "read before present");
        const auto sample =
            [&](float horizontal, float vertical, SDL_Color expected, const std::string& label) {
                expect(
                    surface.get(),
                    static_cast<int>(horizontal * scale),
                    static_cast<int>(vertical * scale),
                    expected,
                    label
                );
            };
        sample(
            40,
            50,
            over(BACKGROUND, FIRST.color),
            "first fill retained after uniform mutations"
        );
        sample(21, 21, BACKGROUND, "rounded corner");
        sample(18, 50, BACKGROUND, "outside fill");
        sample(170, 30, over(BACKGROUND, SECOND.color), "second outline uniform");
        sample(170, 55, BACKGROUND, "outline hollow center");
        sample(280, 55, over(BACKGROUND, THIRD.color), "third size/radius/color");
        sample(246, 25, BACKGROUND, "third rounded corner");
        sample(65, 55, over(over(BACKGROUND, FIRST.color), OVERLAP.color), "transparent overlap");
        sample(277, 125, BACKGROUND, "thin outline hollow center");
        for (const auto& shape: {FIRST, THIN}) {
            const int center = static_cast<int>((shape.bounds.x + shape.bounds.w / 2) * scale);
            const int edge = static_cast<int>(shape.bounds.y * scale);
            for (int vertical = edge - 2; vertical <= edge + 3; ++vertical) {
                const float distance = shape.bounds.y * scale - (vertical + 0.5F);
                const float contour = shape.half_stroke > 0
                    ? std::abs(distance) - shape.half_stroke * scale
                    : distance;
                const float ramp = std::clamp((contour + 1) / 2, 0.0F, 1.0F);
                auto color = shape.color;
                color.a *= 1 - ramp * ramp * (3 - 2 * ramp);
                expect(
                    surface.get(),
                    center,
                    vertical,
                    over(BACKGROUND, color),
                    "analytic straight edge coverage"
                );
            }
        }
        sample(
            125,
            125,
            over(BACKGROUND, {200 / 255.0F, 40 / 255.0F, 80 / 255.0F, 128 / 255.0F}),
            "texture restore"
        );
        sample(145, 125, {20, 220, 60, 255}, "texture upper right");
        sample(125, 145, {40, 80, 230, 255}, "texture lower left");
        sample(145, 145, {220, 180, 40, 255}, "texture lower right");
        int foreground = 0;
        for (int vertical = 0; vertical < static_cast<int>(8 * scale); ++vertical) {
            for (int horizontal = 0; horizontal < static_cast<int>(24 * scale); ++horizontal) {
                const auto expected = pixel(
                    surface.get(),
                    static_cast<int>(24 * scale) + horizontal,
                    static_cast<int>(190 * scale) + vertical
                );
                foreground += expected.r != BACKGROUND.r;
                expect(
                    surface.get(),
                    static_cast<int>(24 * scale) + horizontal,
                    static_cast<int>(130 * scale) + vertical,
                    expected,
                    "glyph restore"
                );
            }
        }
        require(foreground > 0, "reference text must contain glyphs");
        std::cout << "scene scale=" << scale << " passed\n";
    }

    void verify_budget(Context& context) {
        require(SDL_SetRenderScale(context.renderer, 1, 1), "budget scale");
        context.clear();
        std::array<std::array<float, 4>, 256> payload {};
        const auto parameters = parameters_for(FIRST, 1);
        payload[0] = parameters.bounds;
        payload[1] = parameters.style;
        payload[1][3] = 255;
        payload[2] = {1, 1, 1, 1};
        payload.back() = {0.5F, 1, 1, 1};
        require(
            SDL_SetGPURenderStateFragmentUniforms(
                context.budget_state,
                0,
                payload.data(),
                sizeof(payload)
            ),
            "4 KiB uniform upload"
        );
        context.set_state(context.budget_state);
        context.quad(FIRST, 1);
        context.set_state(nullptr);
        Surface surface(SDL_RenderReadPixels(context.renderer, nullptr), SDL_DestroySurface);
        require(surface != nullptr, "budget readback");
        auto expected_color = FIRST.color;
        expected_color.r *= 0.5F;
        expect(
            surface.get(),
            40,
            50,
            over(BACKGROUND, expected_color),
            "4 KiB last vector sentinel"
        );
        std::cout << "budget verified_bytes=" << sizeof(payload)
                  << " slots_used=1 last_vector=255 passed\n";
    }

    void verify_reused_state(Context& context) {
        context.clear();
        context.draw(FIRST, 1);
        context.set_state(nullptr);
        const SDL_FRect destination {120, 120, 32, 32};
        require(
            SDL_RenderTexture(context.renderer, context.image, nullptr, &destination),
            "native interlude"
        );
        context.set_state(context.state);
        context.quad(FIRST, 1);
        context.set_state(nullptr);
        Surface surface(SDL_RenderReadPixels(context.renderer, nullptr), SDL_DestroySurface);
        require(surface != nullptr, "reused state readback");
        expect(
            surface.get(),
            40,
            50,
            over(over(BACKGROUND, FIRST.color), FIRST.color),
            "reused uniform after native draw"
        );
        std::cout << "reused state after native texture passed\n";
    }

    // 不可变参数 state：同一帧内交错使用两个不同参数块，像素必须各自正确。
    // 这条检查是"参数提交策略"的像素侧证据：缓存/绑定不能改变图元的外观。
    void verify_cached_states(Context& context) {
        require(SDL_SetRenderTarget(context.renderer, context.target), "cached target");
        require(SDL_SetRenderScale(context.renderer, 1, 1), "cached scale");
        context.clear();
        const auto create = [&](const Shape& shape) {
            SDL_GPURenderStateCreateInfo info {};
            info.fragment_shader = context.shader;
            auto* created = SDL_CreateGPURenderState(context.renderer, &info);
            require(created != nullptr, "create cached state");
            const auto parameters = parameters_for(shape, 1);
            require(
                SDL_SetGPURenderStateFragmentUniforms(created, 0, &parameters, sizeof(parameters)),
                "seed cached state"
            );
            return created;
        };
        auto* first = create(FIRST);
        auto* second = create(SECOND);
        context.set_state(first);
        context.quad(FIRST, 1);
        context.set_state(second);
        context.quad(SECOND, 1);
        context.set_state(nullptr);
        Surface surface(SDL_RenderReadPixels(context.renderer, nullptr), SDL_DestroySurface);
        require(surface != nullptr, "cached state readback");
        expect(surface.get(), 40, 50, over(BACKGROUND, FIRST.color), "cached state A fill");
        expect(surface.get(), 170, 30, over(BACKGROUND, SECOND.color), "cached state B outline");
        expect(surface.get(), 170, 55, BACKGROUND, "cached state B hollow center");
        SDL_DestroyGPURenderState(first);
        SDL_DestroyGPURenderState(second);
        std::cout << "immutable parameter states interleaved correctly\n";
    }

    auto milliseconds(Uint64 start, Uint64 end) -> double {
        return static_cast<double>(end - start) * 1e-6;
    }

    auto percentile(std::vector<double> samples, double fraction) -> double {
        std::sort(samples.begin(), samples.end());
        return samples[static_cast<std::size_t>(std::ceil(samples.size() * fraction)) - 1];
    }

    // 生产页面的常见形态不是"每个图元都不同参数"，而是成组的相同参数（列表行、按钮组）
    // 与少量交错。runs 模式用 8 个参数块、每块 64 次几何提交来模拟这种形态。
    constexpr int RUNS = 8;
    constexpr int RUN_ITERATIONS = DRAWS_PER_FRAME / RUNS;

    auto block_for(int slot) -> Parameters {
        auto shape = FIRST;
        shape.color.a = 1;
        shape.radius = 8.0F + static_cast<float>(slot % 4) * 2.0F;
        shape.bounds.w = 60.0F + static_cast<float>(slot) * 4.0F;
        return parameters_for(shape, 1);
    }

    auto same(const Parameters& left, const Parameters& right) -> bool {
        return std::memcmp(&left, &right, sizeof(Parameters)) == 0;
    }

    void benchmark(Context& context) {
        require(SDL_SetRenderTarget(context.renderer, nullptr), "benchmark window target");
        require(SDL_SetRenderScale(context.renderer, 1, 1), "benchmark scale");
        constexpr std::array<std::string_view, 8> MODES {
            "native",
            "fixed",
            "update",
            "switch",
            "interleaved",
            "dedup-runs",
            "cached-runs",
            "cached-alternate"
        };
        const auto create_parameter_state = [&](const Parameters& parameters) {
            SDL_GPURenderStateCreateInfo info {};
            info.fragment_shader = context.shader;
            auto* created = SDL_CreateGPURenderState(context.renderer, &info);
            require(created != nullptr, "create parameter state");
            require(
                SDL_SetGPURenderStateFragmentUniforms(created, 0, &parameters, sizeof(parameters)),
                "seed parameter state"
            );
            return created;
        };
        constexpr int CREATION_SAMPLES = 32;
        require(SDL_WaitForGPUIdle(context.gpu), "creation timing isolation");
        const auto creation_begin = SDL_GetTicksNS();
        std::array<SDL_GPURenderState*, CREATION_SAMPLES> created {};
        for (int index = 0; index < CREATION_SAMPLES; ++index) {
            created[static_cast<std::size_t>(index)] =
                create_parameter_state(block_for(index % RUNS));
        }
        const auto creation_end = SDL_GetTicksNS();
        for (auto* state: created) {
            SDL_DestroyGPURenderState(state);
        }
        std::cout << "state creation: samples=" << CREATION_SAMPLES << " create_and_seed_us_each="
                  << static_cast<double>(creation_end - creation_begin) / 1e3 / CREATION_SAMPLES
                  << '\n';
        std::array<SDL_GPURenderState*, RUNS> pool {};
        for (int slot = 0; slot < RUNS; ++slot) {
            pool[static_cast<std::size_t>(slot)] = create_parameter_state(block_for(slot));
        }
        for (int repeat = 0; repeat < 5; ++repeat) {
            for (std::size_t mode_offset = 0; mode_offset < MODES.size(); ++mode_offset) {
                const auto mode = MODES[(mode_offset + repeat) % MODES.size()];
                std::vector<double> submit;
                std::vector<double> present;
                std::vector<double> total;
                for (int frame = 0; frame < 1500; ++frame) {
                    SDL_PumpEvents();
                    require(SDL_WaitForGPUIdle(context.gpu), "benchmark frame isolation");
                    context.state_sets = 0;
                    context.uniform_updates = 0;
                    auto shape = FIRST;
                    shape.color.a = 1;
                    const auto begin = SDL_GetTicksNS();
                    context.clear();
                    if (mode == "fixed" || mode == "update" || mode == "switch"
                        || mode == "interleaved")
                    {
                        context.upload(parameters_for(shape, 1));
                        context.set_state(context.state);
                    }
                    if (mode == "dedup-runs") {
                        // 一个可变 state：只在参数块变化时上传，块内 64 次提交零上传零切换。
                        context.set_state(context.state);
                    }
                    bool seeded = false;
                    Parameters last {};
                    for (int draw = 0; draw < DRAWS_PER_FRAME; ++draw) {
                        if (mode == "update" || mode == "interleaved") {
                            shape.radius = draw % 2 == 0 ? 8.0F : 14.0F;
                            context.upload(parameters_for(shape, 1));
                        }
                        else if (mode == "dedup-runs") {
                            const auto block = block_for(draw / RUN_ITERATIONS);
                            if (!seeded || !same(block, last)) {
                                context.upload(block);
                                last = block;
                                seeded = true;
                            }
                        }
                        else if (mode == "cached-runs") {
                            context.set_state(
                                pool[static_cast<std::size_t>(draw / RUN_ITERATIONS)]
                            );
                        }
                        else if (mode == "cached-alternate") {
                            context.set_state(pool[static_cast<std::size_t>(draw % RUNS)]);
                        }
                        if (mode == "switch" || mode == "interleaved") {
                            context.set_state(context.state);
                        }
                        context.quad(shape, 1);
                        if (mode == "switch" || mode == "interleaved") {
                            context.set_state(nullptr);
                        }
                        context.quad(shape, 1);
                    }
                    context.set_state(nullptr);
                    require(SDL_FlushRenderer(context.renderer), "flush submission");
                    const auto submitted = SDL_GetTicksNS();
                    require(SDL_RenderPresent(context.renderer), "present");
                    const auto presented = SDL_GetTicksNS();
                    require(SDL_WaitForGPUIdle(context.gpu), "wait completion");
                    const auto completed = SDL_GetTicksNS();
                    if (frame >= 300) {
                        submit.push_back(milliseconds(begin, submitted));
                        present.push_back(milliseconds(submitted, presented));
                        total.push_back(milliseconds(begin, completed));
                    }
                }
                std::cout << "bench mode=" << mode << " repeat=" << repeat + 1
                          << " draws=" << DRAWS_PER_FRAME * 2
                          << " state_sets=" << context.state_sets
                          << " uniform_updates=" << context.uniform_updates
                          << " warmup=300 samples=1200 submit_p50_ms=" << percentile(submit, 0.50)
                          << " submit_p95_ms=" << percentile(submit, 0.95)
                          << " present_p50_ms=" << percentile(present, 0.50)
                          << " present_p95_ms=" << percentile(present, 0.95)
                          << " completed_p50_ms=" << percentile(total, 0.50)
                          << " completed_p95_ms=" << percentile(total, 0.95) << std::endl;
            }
        }
        for (auto* state: pool) {
            SDL_DestroyGPURenderState(state);
        }
    }
} // namespace

auto main(int argc, char** argv) -> int {
    bool require_gpu = false;
    bool run_benchmark = false;
    std::string_view fault;
    for (int argument = 1; argument < argc; ++argument) {
        const std::string_view value(argv[argument]);
        if (value == "--require-gpu") {
            require_gpu = true;
        }
        else if (value == "--benchmark") {
            run_benchmark = true;
        }
        else if (value == "--inject-uniform") {
            fault = "uniform";
        }
        else if (value == "--inject-restore") {
            fault = "restore";
        }
        else {
            std::cerr << "unknown argument: " << value << '\n';
            return 1;
        }
    }
    if (!require_gpu && std::getenv("DISPLAY") == nullptr
        && std::getenv("WAYLAND_DISPLAY") == nullptr)
    {
        std::cout << "SKIP: no desktop display; --require-gpu makes this an error\n";
        return 77;
    }
    try {
        Context context;
        context.initialize();
        for (const float scale: {1.0F, 1.25F, 1.5F}) {
            verify_scene(context, scale, fault);
        }
        verify_budget(context);
        verify_reused_state(context);
        verify_cached_states(context);
        if (run_benchmark) {
            benchmark(context);
        }
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
