//
// Manual text-performance benchmark (not a CI test).
//
// The unit tests only assert structural invariants. Wall-clock budgets are not
// reliable under ASan/UBSan or on shared CI runners: on 2026-11-16 a hard
// `elapsed < 500.0` assertion in font_tests measured 556.86 ms on a CI runner
// with no behavior change. The three complexity guards that used to live in
// font_tests.cpp, foundation_tests.cpp and text_layout_header_tests.cpp are
// therefore measured here with warmup, repeated samples and a p95 threshold.
//
// Usage:
//   meson compile -C buildDir text_performance_benchmark
//   ./buildDir/tests/text_performance_benchmark
//   ./buildDir/tests/text_performance_benchmark --samples 15 --max-p95-ms 3000
//
// The default threshold is deliberately wide. This is a hand-run release
// sanity check for accidental O(n^2) regressions, not a stable CI
// performance gate; use --max-p95-ms 0 to disable the threshold.
//

#include <nandina/foundation/utf8.hpp>
#include <nandina/text/font_face.hpp>
#include <nandina/text/harfbuzz_text_backend.hpp>
#include <nandina/text/text_layout_backend.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;

    struct Options {
        int warmup = 2;
        int samples = 9;
        double max_p95_ms = 2000.0;
    };

    struct Result {
        std::string name;
        std::vector<double> samples_ms;
        double p50_ms = 0.0;
        double p95_ms = 0.0;
        double max_ms = 0.0;
    };

    [[nodiscard]] auto usage(const char* program) -> const char* {
        return "usage: %s [--samples N] [--warmup N] [--max-p95-ms MS]\n";
    }

    [[nodiscard]] auto parse_args(const int argc, char** argv) -> Options {
        Options options;
        for (int index = 1; index < argc; ++index) {
            const std::string_view arg = argv[index];
            const auto need_value = [argc, argv, index, arg]() -> std::string_view {
                if (index + 1 >= argc) {
                    std::fprintf(
                        stderr,
                        "text_performance_benchmark: missing value for %s\n",
                        std::string(arg).c_str()
                    );
                    std::exit(2);
                }
                return argv[index + 1];
            };
            if (arg == "--help" || arg == "-h") {
                std::printf(usage(argv[0]), argv[0]);
                std::exit(0);
            }
            if (arg == "--samples") {
                options.samples = std::atoi(std::string(need_value()).c_str());
                if (options.samples < 1) {
                    std::fprintf(stderr, "text_performance_benchmark: --samples must be >= 1\n");
                    std::exit(2);
                }
                ++index;
                continue;
            }
            if (arg == "--warmup") {
                options.warmup = std::atoi(std::string(need_value()).c_str());
                if (options.warmup < 0) {
                    std::fprintf(stderr, "text_performance_benchmark: --warmup must be >= 0\n");
                    std::exit(2);
                }
                ++index;
                continue;
            }
            if (arg == "--max-p95-ms") {
                options.max_p95_ms = std::atof(std::string(need_value()).c_str());
                if (options.max_p95_ms < 0.0) {
                    std::fprintf(stderr, "text_performance_benchmark: --max-p95-ms must be >= 0\n");
                    std::exit(2);
                }
                ++index;
                continue;
            }
            std::fprintf(
                stderr,
                "text_performance_benchmark: unknown argument %s\n",
                std::string(arg).c_str()
            );
            std::fprintf(stderr, usage(argv[0]), argv[0]);
            std::exit(2);
        }
        return options;
    }

    [[nodiscard]] auto percentile(const std::vector<double>& sorted, const double fraction)
        -> double {
        if (sorted.empty()) {
            return 0.0;
        }
        const auto last = sorted.size() - 1;
        auto index = static_cast<std::size_t>(std::ceil(fraction * sorted.size()));
        if (index > 0) {
            --index;
        }
        index = std::min(index, last);
        return sorted[index];
    }

    template<typename Body>
    [[nodiscard]] auto measure(std::string name, const Options& options, Body&& body) -> Result {
        for (int warmup = 0; warmup < options.warmup; ++warmup) {
            body();
        }
        Result result {.name = std::move(name)};
        result.samples_ms.reserve(static_cast<std::size_t>(options.samples));
        for (int sample = 0; sample < options.samples; ++sample) {
            const auto started = Clock::now();
            body();
            const auto elapsed_ms =
                std::chrono::duration<double, std::milli>(Clock::now() - started).count();
            result.samples_ms.push_back(elapsed_ms);
        }
        auto sorted = result.samples_ms;
        std::ranges::sort(sorted);
        result.p50_ms = percentile(sorted, 0.50);
        result.p95_ms = percentile(sorted, 0.95);
        result.max_ms = sorted.back();
        return result;
    }

    void require_or_exit(const bool passed, const char* what) {
        if (!passed) {
            std::fprintf(stderr, "text_performance_benchmark: validation failed: %s\n", what);
            std::exit(1);
        }
    }

    [[nodiscard]] auto caret_source() -> std::string {
        std::string source;
        source.reserve(3 * 8000);
        for (int index = 0; index < 8000; ++index) {
            source += "abc";
        }
        return source;
    }

    [[nodiscard]] auto huge_cjk_run(const int clusters) -> std::string {
        std::string run;
        run.reserve(static_cast<std::size_t>(clusters) * 3);
        for (int index = 0; index < clusters; ++index) {
            run += "汉";
        }
        return run;
    }
} // namespace

auto main(int argc, char** argv) -> int {
    const Options options = parse_args(argc, argv);
    std::printf(
        "text performance benchmark (manual; samples=%d, warmup=%d)\n",
        options.samples,
        options.warmup
    );

    // 1. HarfBuzz caret-stop dedupe: O(n^2) used to take seconds for one line
    // with 8000 clusters. The structural contract is covered by font_tests.
    auto face = std::make_shared<nandina::text::FreeTypeFontFace>(
        std::filesystem::path(NANDINA_TEST_FONT_PATH)
    );
    nandina::text::HarfBuzzTextLayoutBackend backend(face);
    const auto caret_text = caret_source();
    const auto caret_input = nandina::text::TextLayoutInput {
        .text = caret_text,
        .style =
            nandina::text::TextStyle {
                .font_size = 16.0F,
                .overflow = nandina::text::TextOverflow::clip,
                .max_lines = 1,
            },
        .constraints = nandina::foundation::NanLayoutConstraints::loose(),
    };
    {
        const auto validation = backend.layout(caret_input);
        require_or_exit(validation.lines.size() == 1, "caret layout has one line");
        require_or_exit(
            validation.lines.front().caret_stops.size() == caret_text.size() + 1,
            "caret stops dedupe to one per cluster plus the trailing stop"
        );
    }
    auto caret = measure("caret-stop dedupe (8000 clusters)", options, [&] {
        static_cast<void>(backend.layout(caret_input));
    });

    // 2. Grapheme iteration: O(n^2) used to re-segment the remaining text per
    // cluster. The structural contract is covered by foundation_tests.
    const auto grapheme_run = huge_cjk_run(10000);
    const auto grapheme_advance = [&] {
        std::size_t clusters = 0;
        for (auto cursor = std::size_t {0}; cursor < grapheme_run.size();) {
            cursor = nandina::foundation::utf8::next_grapheme_boundary(grapheme_run, cursor);
            ++clusters;
        }
        return clusters;
    };
    require_or_exit(grapheme_advance() == 10000, "grapheme iteration yields 10000 clusters");
    auto grapheme = measure("grapheme advance (10000 clusters)", options, [&] {
        static_cast<void>(grapheme_advance());
    });

    // 3. Deterministic wrap: the fallback backend used to re-segment the whole
    // remaining text per grapheme. The structural contract is covered by
    // text_layout_header_tests.
    const auto wrap_text = huge_cjk_run(5000);
    const auto wrap_input = nandina::text::TextLayoutInput {
        .text = wrap_text,
        .style =
            nandina::text::TextStyle {
                .font_size = 16.0F,
                .overflow = nandina::text::TextOverflow::wrap,
                .max_lines = 100000,
            },
        .constraints = nandina::foundation::NanLayoutConstraints {
            .min_width = 1173.0F,
            .max_width = 1173.0F,
        },
    };
    const auto& wrap_backend = nandina::text::deterministic_text_layout_backend();
    {
        const auto validation = wrap_backend.layout(wrap_input);
        require_or_exit(!validation.lines.empty(), "wrap layout has lines");
        require_or_exit(validation.lines.size() > 1, "long wrap run splits into multiple lines");
        require_or_exit(validation.size.get_width() <= 1173.0F + 0.001F, "wrap width is bounded");
    }
    auto wrap = measure("deterministic wrap (5000 chars)", options, [&] {
        static_cast<void>(wrap_backend.layout(wrap_input));
    });

    const auto results = std::vector<Result> {caret, grapheme, wrap};
    std::printf("\n%-38s %10s %10s %10s\n", "workload", "p50 (ms)", "p95 (ms)", "max (ms)");
    bool failed = false;
    for (const auto& result: results) {
        std::printf(
            "%-38s %10.2f %10.2f %10.2f\n",
            result.name.c_str(),
            result.p50_ms,
            result.p95_ms,
            result.max_ms
        );
        if (options.max_p95_ms > 0.0 && result.p95_ms > options.max_p95_ms) {
            failed = true;
        }
    }
    if (options.max_p95_ms > 0.0) {
        std::printf(
            "\np95 threshold: %.2f ms -> %s\n",
            options.max_p95_ms,
            failed ? "FAIL" : "PASS"
        );
    }
    else {
        std::printf("\np95 threshold: disabled\n");
    }
    return failed ? 1 : 0;
}
