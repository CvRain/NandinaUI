//
// render/backends/sdf_primitive_geometry — SDF 图元的覆盖几何。
//

#ifndef NANDINA_EXPERIMENT_RENDER_BACKENDS_SDF_PRIMITIVE_GEOMETRY_HPP
#define NANDINA_EXPERIMENT_RENDER_BACKENDS_SDF_PRIMITIVE_GEOMETRY_HPP

#include "../../foundation/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nandina::render::detail
{
    enum class SdfPrimitiveMode : std::uint8_t {
        fill = 0,
        outline = 1,
        segment = 2,
        clipped_circle = 3,
        shadow = 4,
    };

    inline constexpr float sdf_aa_padding = 1.0F;

    using SdfOutlineGeometry = struct SdfOutlineGeometry {
        foundation::NanRect centerline_bounds;
        float centerline_radius = 0.0F;
        float half_width = 0.0F;
    };

    /**
     * 将描边的四条外边界分别吸附到最近的逻辑像素边界。不能只对 origin 或 size
     * 取整：文本测量产生的小数宽度会让右边界继续落在像素内部，竖线仍会发虚。
     */
    [[nodiscard]] inline auto snap_outline_bounds(const foundation::NanRect& bounds)
        -> foundation::NanRect {
        return foundation::NanRect::from_ltrb(
            std::round(bounds.get_left()),
            std::round(bounds.get_top()),
            std::round(bounds.get_right()),
            std::round(bounds.get_bottom())
        );
    }

    using SdfOutlineInput = struct SdfOutlineInput {
        foundation::NanRect outer_bounds;
        float outer_radius;
        float thickness;
    };

    /**
     * RenderDevice 的 outline 矩形表示组件外边界。SDF 描边却以轮廓线为中心，
     * 因此中心线需内移半个线宽；否则 1px 边框有一半落在 bounds 外并被 AA
     * 分摊成两条半透明像素。圆角半径也同步减去半线宽，保持外轮廓半径不变。
     * todo: 使用SdfOutlineInput代替函数传参，修复sdf_inner_outline_geometry()的参数顺序不一致问题
     */
    [[nodiscard]] inline auto sdf_inner_outline_geometry(
        const foundation::NanRect& outer_bounds,
        const float outer_radius,
        const float thickness
    ) -> SdfOutlineGeometry {
        const auto snapped_bounds = snap_outline_bounds(outer_bounds);
        const float max_half_width =
            std::min(snapped_bounds.get_width(), snapped_bounds.get_height()) * 0.5F;
        const float half_width = std::clamp(thickness * 0.5F, 0.0F, max_half_width);
        return {
            .centerline_bounds = foundation::NanRect::from_ltrb(
                snapped_bounds.get_left() + half_width,
                snapped_bounds.get_top() + half_width,
                snapped_bounds.get_right() - half_width,
                snapped_bounds.get_bottom() - half_width
            ),
            .centerline_radius = std::max(0.0F, outer_radius - half_width),
            .half_width = half_width,
        };
    }

    /**
     * SDF 的覆盖 quad 必须包含图元边界外侧的抗锯齿过渡区。
     * 描边以传入轮廓为中心，还需额外容纳外侧的半线宽。组件边框先通过
     * sdf_inner_outline_geometry() 把中心线移入组件，再调用本函数。
     */
    [[nodiscard]] inline auto sdf_quad_bounds(
        const foundation::NanRect& shape_bounds,
        const SdfPrimitiveMode mode,
        const float half_width = 0.0F
    ) -> foundation::NanRect {
        // outline 需容纳外侧半线宽；shadow 的软边衰减（spread）落在图元外侧。
        const float soft_padding = mode == SdfPrimitiveMode::outline
            ? std::max(0.0F, half_width)
            : mode == SdfPrimitiveMode::shadow ? std::max(0.0F, half_width) : 0.0F;
        return shape_bounds.expanded(soft_padding + sdf_aa_padding);
    }

    /** 包含线段圆头与抗锯齿过渡区的最小覆盖 quad。 */
    [[nodiscard]] inline auto sdf_segment_bounds(
        const foundation::NanPoint& start,
        const foundation::NanPoint& end,
        const float half_width
    ) -> foundation::NanRect {
        const float padding = std::max(0.0F, half_width) + sdf_aa_padding;
        return foundation::NanRect::from_ltrb(
            std::min(start.get_x(), end.get_x()) - padding,
            std::min(start.get_y(), end.get_y()) - padding,
            std::max(start.get_x(), end.get_x()) + padding,
            std::max(start.get_y(), end.get_y()) + padding
        );
    }
} // namespace nandina::render::detail

#endif // NANDINA_EXPERIMENT_RENDER_BACKENDS_SDF_PRIMITIVE_GEOMETRY_HPP
