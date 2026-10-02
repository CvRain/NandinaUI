#pragma once

#include "../component_catalog.hpp"

#include <nandina/app/nan_page.hpp>

#include <utility>

namespace nandina::showcase
{
    struct ComponentPageParams {
        ComponentId component;

        ComponentPageParams() = delete;
        explicit ComponentPageParams(const ComponentId component): component(component) {}
    };

    class ComponentPage final: public app::Page<ComponentPageParams> {
    public:
        explicit ComponentPage(ComponentPageParams params): Page(std::move(params)) {}

        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::showcase
