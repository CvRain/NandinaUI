#ifndef NANDINAUI_LAB_POMODORO_PAGE_HPP
#define NANDINAUI_LAB_POMODORO_PAGE_HPP

#include <nandina/app/nan_page.hpp>

namespace nandina::lab
{
    class PomodoroPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::lab

#endif // NANDINAUI_LAB_POMODORO_PAGE_HPP
