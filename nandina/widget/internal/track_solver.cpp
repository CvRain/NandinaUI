#include "track_solver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace nandina::widget::internal
{
    void validate_tracks(const std::span<const TrackSpec> tracks, const float gap) {
        if (tracks.empty()) {
            throw std::invalid_argument("Table requires at least one column");
        }
        if (!std::isfinite(gap) || gap < 0.0F) {
            throw std::invalid_argument("Table gap must be finite and non-negative");
        }
        double total = static_cast<double>(gap) * static_cast<double>(tracks.size() - 1);
        for (const auto& track: tracks) {
            if (!std::isfinite(track.flex) || track.flex <= 0.0F
                || (track.width && (!std::isfinite(*track.width) || *track.width < 0.0F)))
            {
                throw std::invalid_argument("Table widths must be non-negative and flex positive");
            }
            total += track.width.value_or(0.0F);
        }
        if (total > static_cast<double>(std::numeric_limits<float>::max())) {
            throw std::invalid_argument("Table track widths and gaps must have a finite sum");
        }
    }

    namespace
    {
        auto finite_extent(const double total) -> float {
            if (!std::isfinite(total) || total < 0.0
                || total > static_cast<double>(std::numeric_limits<float>::max()))
            {
                throw std::invalid_argument(
                    "Table content extent must fit finite layout coordinates"
                );
            }
            return static_cast<float>(total);
        }

        void validate_solve_inputs(
            const std::span<const TrackSpec> tracks,
            const std::optional<float> available_width,
            const std::span<const float> intrinsic_widths
        ) {
            if (available_width && (!std::isfinite(*available_width) || *available_width < 0.0F)) {
                throw std::invalid_argument(
                    "Table available width must be finite and non-negative"
                );
            }
            if (available_width) {
                return;
            }
            if (intrinsic_widths.size() != tracks.size()) {
                throw std::invalid_argument("Table intrinsic widths must match the track count");
            }
            for (std::size_t index = 0; index < tracks.size(); ++index) {
                if (tracks[index].width) {
                    continue; // fixed columns are not measured for intrinsic width
                }
                if (intrinsic_widths[index] == std::numeric_limits<float>::infinity()) {
                    // 保持旧实现的可观察行为：旧几何路径让非有限测量宽度进入
                    // layout_extent()，因此正无穷报同一条内容范围消息。NaN 与
                    // 负无穷在旧 std::max(0, measured) 累积中都落为 0。
                    throw std::invalid_argument(
                        "Table content extent must fit finite layout coordinates"
                    );
                }
            }
        }
    } // namespace

    auto solve_tracks(
        const std::span<const TrackSpec> tracks,
        const std::optional<float> available_width,
        const std::span<const float> intrinsic_widths,
        const float gap
    ) -> TrackSolution {
        validate_tracks(tracks, gap);
        validate_solve_inputs(tracks, available_width, intrinsic_widths);

        TrackSolution solution;
        solution.widths.resize(tracks.size(), 0.0F);

        double reserved = static_cast<double>(gap) * static_cast<double>(tracks.size() - 1);
        double weight = 0.0;
        for (std::size_t column = 0; column < tracks.size(); ++column) {
            if (tracks[column].width) {
                solution.widths[column] = *tracks[column].width;
                reserved += solution.widths[column];
            }
            else {
                weight += tracks[column].flex;
                if (!available_width) {
                    // Intrinsic widths are supplied by the caller; flex columns only.
                    // 旧实现用 max(0, measured) 初始化列宽：负值、负无穷与
                    // NaN 都静默夹取为 0。显式写出 NaN 分支，不把行为寄托给比较算法。
                    const float intrinsic = intrinsic_widths[column];
                    solution.widths[column] =
                        std::isnan(intrinsic) ? 0.0F : std::max(0.0F, intrinsic);
                }
            }
        }
        if (available_width && weight > 0.0) {
            const double free = std::max(0.0, static_cast<double>(*available_width) - reserved);
            for (std::size_t column = 0; column < tracks.size(); ++column) {
                if (!tracks[column].width) {
                    solution.widths[column] =
                        static_cast<float>(free * tracks[column].flex / weight);
                }
            }
        }

        double total_width = static_cast<double>(gap) * static_cast<double>(tracks.size() - 1);
        for (const auto width: solution.widths) {
            total_width += width;
        }
        solution.total_width = finite_extent(total_width);
        return solution;
    }
} // namespace nandina::widget::internal
