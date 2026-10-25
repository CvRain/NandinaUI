//
// Created by cvrain on 2026/10/24.
//

#ifndef NANDINAUI_IMAGE_PAGE_HPP
#define NANDINAUI_IMAGE_PAGE_HPP

#include <nandina/app/nan_page.hpp>

namespace nandina::showcase
{
    class ImagePage: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::showcase

#endif // NANDINAUI_IMAGE_PAGE_HPP
