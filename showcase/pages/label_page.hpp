//
// Created by cvrain on 2026/10/8.
//

#ifndef NANDINAUI_LABEL_PAGE_HPP
#define NANDINAUI_LABEL_PAGE_HPP

#include <nandina/app/nan_page.hpp>

namespace nandina::showcase
{
    class LabelPage: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::showcase

#endif // NANDINAUI_LABEL_PAGE_HPP
