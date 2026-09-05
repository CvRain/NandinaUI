#include "widget/build_context.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace nandina;

namespace
{
    class ScopedProbe final: public scene::NanControl {
    public:
        explicit ScopedProbe(widget::BuildContext ui): graph_(&ui.graph()) {}

        [[nodiscard]] auto graph() const noexcept -> reactive::Graph& {
            return *graph_;
        }

    private:
        reactive::Graph* graph_;
    };
}

TEST_CASE(
    "BuildContext supports custom components without the controls umbrella",
    "[authoring][headers]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    const widget::BuildContext ui {graph, scope, themes};

    const auto probe = ui.make<ScopedProbe>().build();

    REQUIRE(&probe->graph() == &graph);
}
