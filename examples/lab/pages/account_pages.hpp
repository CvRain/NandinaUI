#ifndef NANDINAUI_LAB_ACCOUNT_PAGES_HPP
#define NANDINAUI_LAB_ACCOUNT_PAGES_HPP

#include <nandina/app/nan_page.hpp>

namespace nandina::lab
{
    class LoginPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };

    class RegisterPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::lab

#endif // NANDINAUI_LAB_ACCOUNT_PAGES_HPP
