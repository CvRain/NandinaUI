/// showcase/pages/surface_page.hpp — Surface 盒样式与 Card 内容边距的视觉展示。
#pragma once

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

namespace nandina::showcase
{
    /// 演示 P4.1b 的视觉表面作者能力：`ui.surface()` 的盒样式（填充/描边/圆角/内边距）、
    /// Card 的四方内容边距（配方默认 / 显式 8 / 显式 24）与嵌套边界的如实呈现。
    class SurfacePage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::showcase
