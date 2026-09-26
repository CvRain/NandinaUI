//
// Created by cvrain on 2026/9/23.
//

#include "main_window.hpp"
#include "nandina/foundation/nan_logger.hpp"

#include <nandina/widget/controls.hpp>

namespace nandina::showcase
{

    namespace
    {
        class DetailsPage;

        class HomePage final: public nandina::app::Page<> {
        public:
            [[nodiscard]] auto build(nandina::app::PageContext& context)
                -> nandina::widget::View override;
        };

        class DetailsPage final: public nandina::app::Page<> {
        public:
            [[nodiscard]] auto build(nandina::app::PageContext& context)
                -> nandina::widget::View override;
        };

        auto HomePage::build(nandina::app::PageContext& context) -> nandina::widget::View {
            auto ui = context.ui();
            const auto navigation = context.navigation();
            return ui.center()
                .child(ui.make<nandina::widget::Button>("Open details").on_click([navigation] {
                    (void)navigation.navigate<DetailsPage>();
                }))
                .build();
        }

        auto DetailsPage::build(nandina::app::PageContext& context) -> nandina::widget::View {
            auto ui = context.ui();
            const auto navigation = context.navigation();
            return ui.center()
                .child(ui.make<nandina::widget::Button>("Back to home").on_click([navigation] {
                    (void)navigation.navigate<HomePage>();
                }))
                .build();
        }
    } // namespace

    MainWindow::MainWindow(
        nandina::app::NanApplication& application,
        const nandina::app::WindowConfig& config
    ):
        NanWindow(application, config) {}

    void MainWindow::on_setup() {
        NanWindow::on_setup();
        const auto routes = nandina::app::Routes {
            nandina::app::route<HomePage>({.key = "home", .title = "Home"}),
            nandina::app::route<DetailsPage>({.key = "details", .title = "Details"}),
        };
        auto& router = use_router(routes);
        set_shell([](nandina::app::ShellContext& context) -> nandina::widget::View {
            auto ui = context.ui();
            const auto navigation = context.navigation();
            return ui.row()
                .configure([](nandina::widget::Row& shell) {
                    shell.set_width(nandina::widget::authoring::fill)
                        .set_height(nandina::widget::authoring::fill);
                })
                .children(
                    ui.make<nandina::widget::Button>("Home").on_click([navigation] {
                        nandina::log::info("Navigating to home page");
                        (void)navigation.navigate<HomePage>();
                    }),
                    nandina::widget::authoring::make<nandina::widget::Expanded>().child(
                        context.outlet()
                    )
                )
                .build();
        });
        (void)router.start<HomePage>();
    }
} // namespace nandina::showcase
