module;

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <thorvg-1/thorvg.h>

export module nandina.widgets.tabs;

import nandina.foundation.color;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_size;
import nandina.reactive.event_signal;
import nandina.runtime.nan_widget;
import nandina.theme.nan_style;
import nandina.widgets.button;

export namespace nandina::widgets {

    class Tabs final : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Tabs>;

        ~Tabs() override = default;

        static auto create() -> Ptr {
            return Ptr{new Tabs()};
        }

        auto add_tab(std::string label, runtime::NanWidget::Ptr content) -> Tabs& {
            auto trigger = Button::create();
            auto* trigger_ptr = trigger.get();
            trigger_ptr->text(label);
            trigger_ptr->on_click([this, index = tabs_.size()] {
                active_index(index);
            });
            add_child(std::move(trigger));

            auto* content_ptr = content.get();
            content_ptr->set_visible(false);
            content_ptr->set_hit_test_visible(false);
            add_child(std::move(content));

            tabs_.push_back(TabEntry{
                .label = std::move(label),
                .trigger = trigger_ptr,
                .content = content_ptr,
            });
            sync_active_tab();
            mark_layout_dirty();
            return *this;
        }

        auto active_index(const std::size_t index) -> Tabs& {
            if (tabs_.empty()) {
                active_index_ = 0;
                return *this;
            }

            const auto next = std::min(index, tabs_.size() - 1);
            if (active_index_ == next) {
                return *this;
            }

            active_index_ = next;
            sync_active_tab();
            changed_signal_.emit(active_index_);
            mark_layout_dirty();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto active_index() const noexcept -> std::size_t {
            return active_index_;
        }

        [[nodiscard]] auto tab_count() const noexcept -> std::size_t {
            return tabs_.size();
        }

        [[nodiscard]] auto active_content() const noexcept -> runtime::NanWidget* {
            if (tabs_.empty()) {
                return nullptr;
            }
            return tabs_[active_index_].content;
        }

        auto on_changed(std::function<void(std::size_t)> callback) -> Tabs& {
            changed_signal_.connect(std::move(callback));
            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto& style = theme::NanStylePrimitives::current().tabs;
            float trigger_width = 0.0f;
            float max_content_width = 0.0f;
            float max_content_height = 0.0f;

            for (const auto& tab : tabs_) {
                if (tab.trigger) {
                    const auto pref = tab.trigger->preferred_size();
                    trigger_width += std::max(style.trigger_min_width, pref.width());
                }
                if (tab.content) {
                    const auto pref = tab.content->preferred_size();
                    max_content_width = std::max(max_content_width, pref.width());
                    max_content_height = std::max(max_content_height, pref.height());
                }
            }

            if (!tabs_.empty()) {
                trigger_width += style.trigger_gap * static_cast<float>(tabs_.size() - 1);
            }
            trigger_width += style.trigger_padding_h * 2.0f;

            const auto& pad = style.content_padding;
            const float content_width = max_content_width + pad.left() + pad.right();
            const float content_height = max_content_height + pad.top() + pad.bottom();

            return {
                std::max(trigger_width, content_width),
                style.header_height + style.content_gap + content_height,
            };
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const auto& style = theme::NanStylePrimitives::current().tabs;
            const auto& pad = style.content_padding;
            const float content_width = constraints.max_width() == geometry::NanConstraints::k_infinity
                ? geometry::NanConstraints::k_infinity
                : std::max(0.0f, constraints.max_width() - pad.left() - pad.right());
            const float content_height = constraints.max_height() == geometry::NanConstraints::k_infinity
                ? geometry::NanConstraints::k_infinity
                : std::max(0.0f, constraints.max_height() - style.header_height - style.content_gap - pad.top() - pad.bottom());

            for (const auto& tab : tabs_) {
                if (tab.trigger) {
                    tab.trigger->measure(geometry::NanConstraints{0.0f, geometry::NanConstraints::k_infinity, 0.0f, style.header_height});
                }
                if (tab.content) {
                    tab.content->measure(geometry::NanConstraints{0.0f, content_width, 0.0f, content_height});
                }
            }

            set_measured_layout_state(constraints, constraints.constrain(preferred_size()));
        }

        auto layout() -> void override {
            const auto& style = theme::NanStylePrimitives::current().tabs;
            float cursor_x = x() + style.trigger_padding_h;
            const float trigger_y = y() + style.trigger_padding_v;
            const float trigger_height = std::max(0.0f, style.header_height - style.trigger_padding_v * 2.0f);

            for (const auto& tab : tabs_) {
                if (!tab.trigger) {
                    continue;
                }

                const float trigger_width = std::max(style.trigger_min_width, tab.trigger->preferred_size().width());
                tab.trigger->set_bounds(cursor_x, trigger_y, trigger_width, trigger_height);
                tab.trigger->layout();
                cursor_x += trigger_width + style.trigger_gap;
            }

            const auto& pad = style.content_padding;
            const float content_y = y() + style.header_height + style.content_gap + pad.top();
            const float content_w = std::max(0.0f, width() - pad.left() - pad.right());
            const float content_h = std::max(0.0f, height() - style.header_height - style.content_gap - pad.top() - pad.bottom());

            for (const auto& tab : tabs_) {
                if (!tab.content) {
                    continue;
                }
                tab.content->set_bounds(x() + pad.left(), content_y, content_w, content_h);
                if (tab.content->visible()) {
                    tab.content->layout();
                }
            }

            clear_layout_dirty();
        }

        [[nodiscard]] auto is_interactive() const noexcept -> bool override {
            return true;
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto& style = theme::NanStylePrimitives::current().tabs;
            const auto header_rgb = style.header_bg.to<NanRgb>();
            const auto panel_rgb = style.panel_bg.to<NanRgb>();
            const auto border_rgb = style.border.to<NanRgb>();

            auto* header = tvg::Shape::gen();
            header->appendRect(x(), y(), width(), style.header_height, style.corner_radius, style.corner_radius);
            header->fill(header_rgb.red(), header_rgb.green(), header_rgb.blue(), header_rgb.alpha());
            canvas.add(header);

            auto* panel = tvg::Shape::gen();
            panel->appendRect(x(), y() + style.header_height, width(), std::max(0.0f, height() - style.header_height), style.corner_radius, style.corner_radius);
            panel->fill(panel_rgb.red(), panel_rgb.green(), panel_rgb.blue(), panel_rgb.alpha());
            canvas.add(panel);

            if (style.border_width > 0.0f) {
                auto* border = tvg::Shape::gen();
                border->appendRect(x(), y(), width(), height(), style.corner_radius, style.corner_radius);
                border->strokeWidth(style.border_width);
                border->strokeFill(border_rgb.red(), border_rgb.green(), border_rgb.blue(), border_rgb.alpha());
                canvas.add(border);
            }
        }

    private:
        struct TabEntry {
            std::string label;
            Button* trigger{nullptr};
            runtime::NanWidget* content{nullptr};
        };

        Tabs() = default;

        auto sync_active_tab() -> void {
            const auto& style = theme::NanStylePrimitives::current().tabs;
            for (std::size_t i = 0; i < tabs_.size(); ++i) {
                auto& tab = tabs_[i];
                const bool active = i == active_index_;
                if (tab.trigger) {
                    tab.trigger->size(style.trigger_size)
                        .variant(active ? style.trigger_active_variant : style.trigger_variant)
                        .color_variant(active ? style.trigger_active_color_variant : style.trigger_color_variant);
                }
                if (tab.content) {
                    tab.content->set_visible(active);
                    tab.content->set_hit_test_visible(active);
                }
            }
        }

        std::vector<TabEntry> tabs_{};
        std::size_t active_index_{0};
        reactive::EventSignal<std::size_t> changed_signal_;
    };
}
