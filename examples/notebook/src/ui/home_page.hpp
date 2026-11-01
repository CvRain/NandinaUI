#ifndef NANDINA_EXAMPLES_NOTEBOOK_UI_HOME_PAGE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_UI_HOME_PAGE_HPP

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

namespace nan_example::notebook::ui
{
    /// Placeholder for the signed-in notebook workspace.
    ///
    /// It exists so the authentication flow has a real destination and a logout path;
    /// the notes UI will replace its body in a later batch.
    class HomePage final: public nandina::app::Page<> {
    public:
        [[nodiscard]] auto build(nandina::app::PageContext& context)
            -> nandina::widget::View override;
    };
} // namespace nan_example::notebook::ui

#endif // NANDINA_EXAMPLES_NOTEBOOK_UI_HOME_PAGE_HPP
