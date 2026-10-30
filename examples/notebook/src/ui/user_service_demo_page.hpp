#ifndef NANDINA_EXAMPLES_NOTEBOOK_UI_USER_SERVICE_DEMO_PAGE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_UI_USER_SERVICE_DEMO_PAGE_HPP

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

namespace nan_example::notebook::ui
{
    /// Minimal page showing the composition-root wiring:
    /// PageContext -> NotebookStore -> UserService -> UserRepository -> Database.
    class UserServiceDemoPage final: public nandina::app::Page<> {
    public:
        [[nodiscard]] auto build(nandina::app::PageContext& context) -> nandina::widget::View override;
    };
} // namespace nandina::examples::notebook::ui

#endif // NANDINA_EXAMPLES_NOTEBOOK_UI_USER_SERVICE_DEMO_PAGE_HPP
