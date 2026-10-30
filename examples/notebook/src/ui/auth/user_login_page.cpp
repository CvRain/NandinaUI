//
// Created by cvrain on 2026/10/29.
//

#include "user_login_page.hpp"

#include <nandina/widget/controls.hpp>

namespace nan_example::notebook::ui
{
    auto UserLoginPage::build(nandina::app::PageContext& context) -> nandina::widget::View {
        const auto& ui = context.ui();

        return ui.center().child(ui.make<nandina::widget::Label>("User login")).build();
    }
} // namespace nan_example::notebook::ui