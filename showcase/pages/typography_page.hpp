//
// showcase/pages/typography_page.hpp — step 2b typescale/leading/role demonstration.
//

#ifndef NANDINAUI_TYPOGRAPHY_PAGE_HPP
#define NANDINAUI_TYPOGRAPHY_PAGE_HPP

#include <nandina/app/nan_page.hpp>

namespace nandina::showcase
{
    class TypographyPage: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::showcase

#endif // NANDINAUI_TYPOGRAPHY_PAGE_HPP
