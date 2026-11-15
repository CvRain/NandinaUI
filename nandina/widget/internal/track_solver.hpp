//
// widget/internal/track_solver — pure column-track math shared by static Table (and later
// DataTable).
//
// 静态 Table 的列轨道解算原先内嵌在 table.cpp::geometry() 里：固定列保留宽度、flex 列按权重
// 分剩余空间、无界宽度下取内容 intrinsic 最大值、gap 参与总宽并做有限性校验。DataTable 也
// 需要同一套数学，因此这里提取为**无状态纯函数**：不测量任何节点，intrinsic 由调用方测量后
// 传入。规则与错误消息保持与 Table 既有行为一致（见 docs/references/
// data_components_and_virtualization.md §8.2、§9.9）。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_TRACK_SOLVER_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_TRACK_SOLVER_HPP

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace nandina::widget::internal
{
    /// 单条列轨道的纯输入。固定宽度优先；否则按 flex 权重分配。
    struct TrackSpec {
        std::optional<float> width;
        float flex = 1.0F;
    };

    struct TrackSolution {
        std::vector<float> widths;
        float total_width = 0.0F;
    };

    /// 校验轨道定义与 gap：至少一列、flex 有限正数、固定宽有限非负、gap 有限非负、
    /// 固定宽与 gap 总和可表示为有限 float。失败消息与静态 Table 保持一致。
    void validate_tracks(std::span<const TrackSpec> tracks, float gap);

    /// 求解列宽。
    ///
    /// - `available_width` 有界时按固定列 + 剩余空间 flex 分配；`intrinsic_widths` 不使用。
    /// - `available_width` 无界时 flex 列取 `intrinsic_widths` 中的值；此时 `intrinsic_widths`
    ///   数量必须等于轨道数，flex 列的值必须有限且非负（固定列的值被忽略）。
    /// - 自身执行 `validate_tracks()` 与上述输入校验，不依赖调用者先验证。
    /// - 结果总宽不可表示为有限 float 时抛 `std::invalid_argument`（消息与 Table 的内容
    ///   范围检查一致）。
    [[nodiscard]] auto solve_tracks(
        std::span<const TrackSpec> tracks,
        std::optional<float> available_width,
        std::span<const float> intrinsic_widths,
        float gap
    ) -> TrackSolution;
} // namespace nandina::widget::internal

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_TRACK_SOLVER_HPP
