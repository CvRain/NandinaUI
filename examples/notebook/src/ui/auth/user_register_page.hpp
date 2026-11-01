#ifndef NANDINA_EXAMPLES_NOTEBOOK_UI_AUTH_USER_REGISTER_PAGE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_UI_AUTH_USER_REGISTER_PAGE_HPP

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

namespace nan_example::notebook::ui
{
    class UserRegisterPage final: public nandina::app::Page<> {
    public:
        [[nodiscard]] auto build(nandina::app::PageContext& context)
            -> nandina::widget::View override;
    };
} // namespace nan_example::notebook::ui

#endif // NANDINA_EXAMPLES_NOTEBOOK_UI_AUTH_USER_REGISTER_PAGE_HPP
