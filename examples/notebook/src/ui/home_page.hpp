#ifndef NANDINA_EXAMPLES_NOTEBOOK_UI_HOME_PAGE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_UI_HOME_PAGE_HPP

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

namespace nan_example::notebook::ui
{
    /// Signed-in notebook workspace (B3 minimal vertical slice).
    ///
    /// Three columns over `NotebookStore`: group filter + creation, note list, and a
    /// title/body editor with new/save/delete actions. The page only reads and writes
    /// store signals/repositories; SQL stays in the data layer.
    class HomePage final: public nandina::app::Page<> {
    public:
        [[nodiscard]] auto build(nandina::app::PageContext& context)
            -> nandina::widget::View override;
    };
} // namespace nan_example::notebook::ui

#endif // NANDINA_EXAMPLES_NOTEBOOK_UI_HOME_PAGE_HPP
