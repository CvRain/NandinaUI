//
// Created by cvrain on 2026/10/8.
//

#include "label_page.hpp"

namespace nandina::showcase
{

    auto LabelPage::build(app::PageContext& context) -> widget::View {
        return context.ui().column().build();
    }
} // namespace nandina::showcase