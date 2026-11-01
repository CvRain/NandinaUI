// showcase/pages/input_focus_page.hpp — D1 第一阶段的人工验收页。
#pragma once

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

namespace nandina::showcase
{
    /// 输入与焦点：模态提示、可编辑例外、用户偏好、两种 roving 模型与焦点恢复。
    class InputFocusPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::showcase
