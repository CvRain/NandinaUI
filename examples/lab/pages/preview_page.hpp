#ifndef NANDINAUI_LAB_PREVIEW_PAGE_HPP
#define NANDINAUI_LAB_PREVIEW_PAGE_HPP

#include <nandina/app/nan_page.hpp>

namespace nandina::lab
{
    class PreviewPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::lab

#endif // NANDINAUI_LAB_PREVIEW_PAGE_HPP
