//
// Created by cvrain on 2026/10/29.
//

#pragma once

#include <nandina/app/nan_page.hpp>

namespace nan_example::notebook::ui
{
    class UserLoginPage: public nandina::app::Page<> {
    public:
        [[nodiscard]] auto build(nandina::app::PageContext& context) -> nandina::widget::View override;
    };
} // namespace nandina::examples::notebook::ui