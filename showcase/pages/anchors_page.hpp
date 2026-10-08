/// showcase/pages/anchors_page.hpp — anchors 的作者案例：侧边栏停靠边切换。
#pragma once

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

namespace nandina::showcase
{
    /// 页面根就是锚定画布：header / 侧边栏 / 编辑区是同层兄弟，用锚线表达空间关系。
    /// 切换停靠边只替换锚点描述，组件树、子项顺序与节点身份都不变（树 ≠ 布局）。
    class AnchorsPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::showcase
