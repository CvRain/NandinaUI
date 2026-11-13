/// showcase/pages/collapsible_page.hpp — Collapsible 的保留内容、状态同步与嵌套展示。
#pragma once

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

namespace nandina::showcase
{
    /// 演示 P4.2c Collapsible：内容收起后仍由组件保留、程序化 set_open 静默、用户切换发出
    /// Signal、键盘可切换，以及「Collapsible 内再嵌套 Collapsible」的组合。
    class CollapsiblePage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::showcase
