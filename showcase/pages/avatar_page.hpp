//
// Created by cvrain on 2026/9/27.
//

#pragma once

#include "nandina/app/nan_page.hpp"

namespace nandina::showcase
{
    class AvatarPage: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };

} // namespace nandina::showcase
