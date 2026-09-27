//
// Created by cvrain on 2026/9/23.
//

#include "main_window.hpp"

#include <nandina/widget/controls.hpp>

#include "pages/avatar_page.hpp"
#include "pages/home_page.hpp"

namespace nandina::showcase
{
    MainWindow::MainWindow(
        nandina::app::NanApplication& application,
        const nandina::app::WindowConfig& config
    ):
        NanWindow(application, config) {}

    void MainWindow::on_setup() {
        NanWindow::on_setup();
        const auto routes = nandina::app::Routes {
            nandina::app::route<ShowcaseHomePage>({.key = "home", .title = "home page"}),
            nandina::app::route<AvatarPage>({.key = "avatar", .title = "avatar page"}),
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
                        (void)navigation.navigate<ShowcaseHomePage>();
                    }),
                    ui.make<nandina::widget::Button>("Avatar").on_click([navigation] {
                        (void)navigation.navigate<AvatarPage>();
                    }),
                    nandina::widget::authoring::make<nandina::widget::Expanded>().child(
                        context.outlet()
                    )
                )
                .build();
        });
        (void)router.start<ShowcaseHomePage>();
    }
} // namespace nandina::showcase
