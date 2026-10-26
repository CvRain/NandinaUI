#include "lab_window.hpp"

#include "pages/account_pages.hpp"
#include "pages/notes_page.hpp"
#include "pages/pomodoro_page.hpp"
#include "pages/preview_page.hpp"

#include <nandina/foundation/nan_logger.hpp>
#include <nandina/widget/controls.hpp>

#include <cstdlib>
#include <string>
#include <string_view>

namespace nandina::lab
{
    namespace
    {
        [[nodiscard]] auto nav_label(const app::RouteEntry& entry) -> std::string {
            if (!entry.options.title.empty()) {
                return entry.options.title;
            }
            return entry.options.address;
        }

        auto build_sidebar(const app::ShellContext& context) -> std::shared_ptr<scene::NanControl> {
            const auto ui = context.ui();
            auto& current = context.current_page();

            auto title = ui.make<widget::Label>("Nandina Lab")
                             .color_token(theme::ColorToken::foreground)
                             .font_size(22.0F)
                             .width(widget::authoring::fill);
            auto subtitle = ui.make<widget::Label>("routing · forms · lists · timers")
                                .color_token(theme::ColorToken::muted_foreground)
                                .font_size(12.0F)
                                .width(widget::authoring::fill)
                                .configure([](widget::Label& label) {
                                    label.set_overflow(text::TextOverflow::wrap);
                                    label.set_max_lines(2);
                                });
            auto navigation = ui.column().width(widget::authoring::fill).gap(12.0F);
            for (const auto& section: context.routes().nav_sections()) {
                auto items = ui.column().width(widget::authoring::fill).gap(4.0F);
                items.children(ui.make<widget::Label>(std::string(section.type))
                                   .color_token(theme::ColorToken::muted_foreground)
                                   .font_size(11.0F)
                                   .width(widget::authoring::fill));
                for (const auto* entry: section.entries) {
                    const auto page_key = entry->page_key;
                    auto button = ui.make<widget::Button>(nav_label(*entry))
                                      .width(widget::authoring::fill)
                                      .height(36.0F)
                                      .treatment(theme::ButtonTreatment::outlined)
                                      .label_align(theme::TextAlign::start)
                                      .on_click([navigation = context.navigation(), page_key] {
                                          if (const auto entered = navigation.navigate_to(page_key);
                                              !entered) {
                                              log::error(
                                                  "lab sidebar: cannot enter route: {}",
                                                  describe(entered.error())
                                              );
                                          }
                                      });
                    const auto built = button.build();
                    auto& active =
                        ui.computed([&current, page_key] { return current.get() == page_key; });
                    ui.bind(
                        built,
                        [](widget::Button& target, const bool selected) {
                            target.set_treatment(
                                selected ? theme::ButtonTreatment::filled
                                         : theme::ButtonTreatment::outlined
                            );
                        },
                        active
                    );
                    items.children(built);
                }
                navigation.children(items);
            }

            auto content = ui.column()
                               .width(widget::authoring::fill)
                               .height(widget::authoring::fill)
                               .gap(12.0F)
                               .cross_alignment(widget::LayoutAlignment::stretch)
                               .children(title)
                               .children(subtitle)
                               .children(ui.make<widget::Divider>())
                               .children(ui.expanded().child(
                                   ui.scroll_view().width(widget::authoring::fill).child(navigation)
                               ));

            return ui.make<widget::Card>()
                .width(232.0F)
                .height(widget::authoring::fill)
                .child(content)
                .build();
        }

        auto build_shell(const app::ShellContext& context) -> widget::View {
            const auto ui = context.ui();
            return ui.row()
                .width(widget::authoring::fill)
                .height(widget::authoring::fill)
                .gap(8.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(
                    ui.padding(foundation::NanInsets::all(8.0F)).child(build_sidebar(context))
                )
                .children(ui.expanded().child(context.outlet()))
                .build();
        }
    } // namespace

    LabWindow::LabWindow(app::NanApplication& application, const app::WindowConfig& config):
        NanWindow(application, config) {
        if (const char* smoke = std::getenv("NANDINA_LAB_SMOKE_SECONDS");
            smoke != nullptr && *smoke != '\0')
        {
            smoke_seconds_ = std::strtof(smoke, nullptr);
        }
    }

    void LabWindow::on_frame(const float dt) {
        NanWindow::on_frame(dt);
        if (smoke_seconds_ <= 0.0F) {
            return;
        }
        smoke_elapsed_ += dt;
        if (smoke_elapsed_ >= smoke_seconds_) {
            request_close();
        }
    }

    void LabWindow::on_setup() {
        NanWindow::on_setup();
        const auto routes = app::Routes {
            app::route<LoginPage>(app::RouteOptions {
                .address = "account/login",
                .title = "Sign in",
                .type = "Account"
            }),
            app::route<RegisterPage>(app::RouteOptions {
                .address = "account/register",
                .title = "Register",
                .type = "Account"
            }),
            app::route<NotesPage>(
                app::RouteOptions {.address = "notes", .title = "Notes", .type = "Workspace"}
            ),
            app::route<PreviewPage>(app::RouteOptions {
                .address = "notes/preview",
                .title = "Preview",
                .type = "Workspace",
                .show_in_nav = false
            }),
            app::route<PomodoroPage>(
                app::RouteOptions {.address = "focus", .title = "Focus", .type = "Workspace"}
            )
        };
        auto& router = use_router(routes);
        set_shell([](const app::ShellContext& context) { return build_shell(context); });
        const std::string_view start = std::getenv("NANDINA_LAB_START_ROUTE") != nullptr
            ? std::string_view {std::getenv("NANDINA_LAB_START_ROUTE")}
            : std::string_view {"notes"};
        if (start == "preview") {
            (void)router.start<PreviewPage>();
        }
        else if (start == "focus") {
            (void)router.start<PomodoroPage>();
        }
        else if (start == "login") {
            (void)router.start<LoginPage>();
        }
        else {
            (void)router.start<NotesPage>();
        }
    }
} // namespace nandina::lab
