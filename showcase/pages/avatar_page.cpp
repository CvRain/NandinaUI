//
// Created by cvrain on 2026/9/27.
//

#include "avatar_page.hpp"

namespace nandina::showcase
{
    auto AvatarPage::build(app::PageContext& context) -> widget::View {
        const auto& ui = context.ui();

        return ui.column().build();
    }
} // namespace nandina::showcase
