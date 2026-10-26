#include "pomodoro_page.hpp"

#include "lab_style.hpp"

#include <nandina/widget/controls.hpp>

#include <format>
#include <string>

namespace nandina::lab
{
    namespace
    {
        inline constexpr int FOCUS_SECONDS = 25 * 60;
        inline constexpr int BREAK_SECONDS = 5 * 60;

        enum class Phase { focus, break_time };

        [[nodiscard]] auto format_clock(const int seconds) -> std::string {
            return std::format("{:02}:{:02}", seconds / 60, seconds % 60);
        }

        /// The timer owns the frame-driven state. It is deliberately a control rather
        /// than a page-local wall-clock thread: every write stays on the UI thread and
        /// pause/resume are deterministic with respect to the frame loop.
        class PomodoroTimer final: public widget::Column {
        public:
            explicit PomodoroTimer(widget::BuildContext ui):
                remaining_(ui.graph(), FOCUS_SECONDS),
                running_(ui.graph(), false),
                phase_(ui.graph(), Phase::focus),
                completed_(ui.graph(), 0) {
                set_gap(12.0F);
                set_cross_alignment(widget::LayoutAlignment::stretch);
                set_main_alignment(widget::LayoutAlignment::center);

                auto& clock = ui.computed([this] { return format_clock(remaining_.get()); });
                auto& phase_text = ui.computed([this] {
                    return phase_.get() == Phase::focus ? std::string {"Focus session"}
                                                        : std::string {"Break"};
                });
                auto& start_text = ui.computed([this] {
                    return running_.get() ? std::string {"Pause"} : std::string {"Start"};
                });
                auto& completed_text = ui.computed([this] {
                    return std::format("{} focus session(s) completed", completed_.get());
                });

                add(ui.make<widget::Label>(phase_text)
                        .color_token(theme::ColorToken::muted_foreground)
                        .font_size(14.0F)
                        .width(widget::authoring::fill)
                        .configure([](widget::Label& label) {
                            label.set_align(theme::TextAlign::center);
                        })
                        .build());
                add(ui.make<widget::Label>(clock)
                        .color_token(theme::ColorToken::foreground)
                        .font_size(64.0F)
                        .width(widget::authoring::fill)
                        .configure([](widget::Label& label) {
                            label.set_align(theme::TextAlign::center);
                        })
                        .build());
                add(ui.make<widget::Label>(completed_text)
                        .color_token(theme::ColorToken::muted_foreground)
                        .font_size(12.0F)
                        .width(widget::authoring::fill)
                        .configure([](widget::Label& label) {
                            label.set_align(theme::TextAlign::center);
                        })
                        .build());

                auto controls = ui.row()
                                    .gap(8.0F)
                                    .cross_alignment(widget::LayoutAlignment::center)
                                    .configure([](widget::Row& row) {
                                        row.set_main_alignment(widget::LayoutAlignment::center);
                                    });
                controls.children(ui.make<widget::Button>(start_text)
                                      .treatment(theme::ButtonTreatment::filled)
                                      .on_click([this] { running_.set(!running_.peek()); }));
                controls.children(ui.make<widget::Button>("Reset")
                                      .treatment(theme::ButtonTreatment::outlined)
                                      .on_click([this] {
                                          phase_.set(Phase::focus);
                                          remaining_.set(FOCUS_SECONDS);
                                          running_.set(false);
                                          accumulator_ = 0.0F;
                                      }));
                controls.children(ui.make<widget::Button>("Skip")
                                      .treatment(theme::ButtonTreatment::ghost)
                                      .on_click([this] {
                                          advance_phase();
                                          running_.set(false);
                                          accumulator_ = 0.0F;
                                      }));
                add(controls.build());
            }

            void on_process(const float dt) override {
                if (!running_.peek()) {
                    return;
                }
                accumulator_ += dt;
                const int elapsed = static_cast<int>(accumulator_);
                if (elapsed <= 0) {
                    return;
                }
                accumulator_ -= static_cast<float>(elapsed);
                const int next = remaining_.peek() - elapsed;
                if (next > 0) {
                    remaining_.set(next);
                    return;
                }
                if (phase_.peek() == Phase::focus) {
                    completed_.update([](int& value) { ++value; });
                }
                advance_phase();
                running_.set(false);
                accumulator_ = 0.0F;
            }

        private:
            void advance_phase() {
                const bool focus = phase_.peek() == Phase::focus;
                phase_.set(focus ? Phase::break_time : Phase::focus);
                remaining_.set(focus ? BREAK_SECONDS : FOCUS_SECONDS);
            }

            reactive::Signal<int> remaining_;
            reactive::Signal<bool> running_;
            reactive::Signal<Phase> phase_;
            reactive::Signal<int> completed_;
            float accumulator_ = 0.0F;
        };
    } // namespace

    auto PomodoroPage::build(app::PageContext& context) -> widget::View {
        auto ui = context.ui();
        auto content = page_column(ui)
                           .children(page_title(ui, "Focus"))
                           .children(body_text(
                               ui,
                               "A 25/5 pomodoro timer driven by the frame loop. Start, pause and "
                               "reset are ordinary reactive writes; the page is destroyed when "
                               "you leave the route, so the timer does not keep running in the "
                               "background.",
                               5
                           ))
                           .children(card(ui, ui.make<PomodoroTimer>()))
                           .children(caption(
                               ui,
                               "The clock uses the application frame delta, not a background "
                               "thread. Leaving the route stops the timer."
                           ));

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(PAGE_PADDING)).child(content))
            .build();
    }
} // namespace nandina::lab
