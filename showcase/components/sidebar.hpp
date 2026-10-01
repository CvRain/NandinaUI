#pragma once

#include <nandina/app/shell_context.hpp>
#include <nandina/widget/authoring.hpp>
#include <nandina/widget/button.hpp>

namespace nandina::showcase
{
    class Sidebar {
    public:
        explicit Sidebar(const app::ShellContext& context);

        auto build_shell() -> widget::View;
    private:
        const app::ShellContext& context;
    };
} // namespace nandina::showcase
