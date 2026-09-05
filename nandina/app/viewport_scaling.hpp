//
// app/viewport_scaling — fixed-design viewport scaling policy and coordinate mapping.
//

#ifndef NANDINA_EXPERIMENT_APP_VIEWPORT_SCALING_HPP
#define NANDINA_EXPERIMENT_APP_VIEWPORT_SCALING_HPP

#include "../foundation/geometry.hpp"
#include "../foundation/transform2d.hpp"

namespace nandina::app
{
    /** 固定设计视口放入窗口时的等比例适配方式。 */
    enum class ViewportFit {
        contain, ///< 完整显示设计视口，剩余区域留白（letterbox）。
        cover,   ///< 填满窗口，超出窗口的设计区域被裁切。
    };

    /** contain/cover 在某一轴出现剩余量时的对齐位置。 */
    enum class ViewportAnchor {
        start,
        center,
        end,
    };

    /**
     * 整体界面等比例缩放策略。
     *
     * `design_size` 是布局使用的固定逻辑尺寸，不是物理像素尺寸。DPI scale 与用户
     * accessibility scale 属于后续独立乘数，不能写入这里。
     */
    struct ViewportScalePolicy {
        foundation::NanSize design_size;
        ViewportFit fit = ViewportFit::contain;
        ViewportAnchor horizontal_anchor = ViewportAnchor::center;
        ViewportAnchor vertical_anchor = ViewportAnchor::center;
    };

    /** 固定逻辑视口到当前 screen-space viewport 的可逆映射。 */
    struct ViewportMapping {
        foundation::NanSize screen_size;
        foundation::NanSize logical_size;
        float scale = 1.0F;
        foundation::NanPoint offset;

        /** logical → screen；uniform scale 后平移到 anchor 位置。 */
        [[nodiscard]] auto transform() const -> foundation::NanTransform2D;
        [[nodiscard]] auto logical_to_screen(foundation::NanPoint point) const
            -> foundation::NanPoint;
        [[nodiscard]] auto screen_to_logical(foundation::NanPoint point) const
            -> foundation::NanPoint;
        [[nodiscard]] auto content_bounds() const -> foundation::NanRect;
    };

    /**
     * 计算固定设计视口映射。尺寸必须有限且严格为正，否则抛出 invalid_argument。
     * 该函数不访问窗口或渲染后端，可独立用于布局、输入和测试。
     */
    [[nodiscard]] auto make_viewport_mapping(
        foundation::NanSize screen_size,
        const ViewportScalePolicy& policy
    ) -> ViewportMapping;

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_VIEWPORT_SCALING_HPP
