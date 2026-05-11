module;

#include <cstdio>
#include <memory>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.showcase;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.foundation.nan_constraints;
import nandina.foundation.nan_insets;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.runtime.nan_widget;
import nandina.widgets;

namespace {

using nandina::NanColor;
using nandina::NanRgb;
using nandina::layout::LayoutAlignment;

auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255) -> NanColor {
    return NanColor::from(NanRgb{r, g, b, a});
}

auto draw_rect(tvg::SwCanvas& canvas,
    const float x, const float y, const float w, const float h,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a,
    const float corner = 0.0f) -> void {
    auto* shape = tvg::Shape::gen();
    shape->appendRect(x, y, w, h, corner, corner);
    shape->fill(r, g, b, a);
    canvas.add(shape);
}

auto make_label(std::string_view text, const float font_size, const NanColor& color) -> nandina::widgets::Label::Ptr {
    auto label = nandina::widgets::Label::create();
    label->set_text(text)
        .set_font_size(font_size)
        .set_color(color);
    return label;
}

auto make_icon_slot(const nandina::widgets::IconType type, const float icon_size, const float slot_size,
    const NanColor& color) -> nandina::runtime::NanWidget::Ptr {
    auto icon = nandina::widgets::Icon::create();
    icon->set_type(type)
        .set_size(icon_size)
        .set_color(color);

    auto center = nandina::layout::Center::Create();
    center->child(std::move(icon));

    auto slot = nandina::layout::SizedBox::Create();
    slot->width(slot_size)
        .height(slot_size)
        .child(std::move(center));
    return slot;
}

auto make_pill(std::string_view text, const NanColor& bg, const NanColor& fg) -> nandina::runtime::NanWidget::Ptr {
    auto pill = nandina::widgets::Surface::create();
    pill->set_bg_color(bg)
        .set_corner_radius(999.0f)
        .set_padding(nandina::geometry::NanInsets{10.0f, 4.0f, 10.0f, 4.0f});
    pill->add_child(make_label(text, 7.0f, fg));
    return pill;
}

auto make_bullet_row(const nandina::widgets::IconType icon_type, const NanColor& icon_color,
    std::string_view title, std::string_view detail) -> nandina::runtime::NanWidget::Ptr {
    auto row = nandina::layout::Row::Create();
    row->align_items(LayoutAlignment::center)
        .gap(10.0f);

    row->add(make_icon_slot(icon_type, 8.0f, 12.0f, icon_color));

    auto text_column = nandina::layout::Column::Create();
    text_column->gap(2.0f)
        .align_items(LayoutAlignment::stretch);
    text_column->add(make_label(title, 8.0f, color4_to_nancolor(228, 231, 244)));
    text_column->add(make_label(detail, 6.0f, color4_to_nancolor(132, 138, 160)));

    auto expanded = nandina::layout::Expanded::Create();
    expanded->child(std::move(text_column));
    row->add(std::move(expanded));
    return row;
}

auto make_progress_row(std::string_view label_text, const float progress, const NanColor& bar_color)
    -> nandina::runtime::NanWidget::Ptr {
    auto row = nandina::layout::Row::Create();
    row->align_items(LayoutAlignment::center)
        .gap(12.0f);

    auto label_slot = nandina::layout::SizedBox::Create();
    label_slot->width(132.0f)
        .height(14.0f)
        .child(make_label(label_text, 7.0f, color4_to_nancolor(160, 162, 180)));
    row->add(std::move(label_slot));

    auto bar = nandina::widgets::ProgressBar::create();
    bar->set_progress(progress)
        .set_bar_color(bar_color)
        .set_track_color(color4_to_nancolor(42, 44, 62))
        .set_bar_height(6.0f)
        .set_corner_radius(3.0f);

    auto bar_expanded = nandina::layout::Expanded::Create();
    bar_expanded->child(std::move(bar));
    row->add(std::move(bar_expanded));

    char pct_text[8];
    const int pct_len = std::snprintf(pct_text, sizeof(pct_text), "%d%%", static_cast<int>(progress * 100.0f));
    auto pct_slot = nandina::layout::SizedBox::Create();
    pct_slot->width(38.0f)
        .height(14.0f)
        .child(make_label(std::string_view{pct_text, static_cast<std::size_t>(pct_len)}, 7.0f,
            color4_to_nancolor(110, 112, 130)));
    row->add(std::move(pct_slot));

    return row;
}

auto make_sidebar_button(std::string_view label, const nandina::widgets::IconType icon, const bool active = false)
    -> nandina::widgets::SidebarMenuButton::Ptr {
    auto button = nandina::widgets::SidebarMenuButton::create();
    button->set_label(label)
        .set_icon_type(icon)
        .set_active(active);
    return button;
}

auto build_sidebar() -> nandina::widgets::Sidebar::Ptr {
    auto sidebar = nandina::widgets::Sidebar::create();
    sidebar->set_header_title("NandinaUI")
        .set_user_name("main")
        .set_user_role("temporary showcase");

    sidebar->add_menu_item(make_sidebar_button("Overview", nandina::widgets::IconType::Circle, true));
    sidebar->add_menu_item(make_sidebar_button("Layout", nandina::widgets::IconType::Square));
    sidebar->add_menu_item(make_sidebar_button("Widgets", nandina::widgets::IconType::Triangle));
    sidebar->add_menu_item(make_sidebar_button("Authoring", nandina::widgets::IconType::Dots));

    sidebar->add_project_item(make_sidebar_button("Issue 086", nandina::widgets::IconType::Dot));
    sidebar->add_project_item(make_sidebar_button("Showcase Rewrite", nandina::widgets::IconType::ArrowUp));
    return sidebar;
}

auto build_hero_surface() -> nandina::widgets::Surface::Ptr {
    auto hero = nandina::widgets::Surface::create();
    hero->set_bg_color(color4_to_nancolor(44, 60, 86))
        .set_corner_radius(12.0f)
        .set_padding(nandina::geometry::NanInsets{18.0f, 16.0f, 18.0f, 16.0f});

    auto content = nandina::layout::Column::Create();
    content->gap(10.0f)
        .align_items(LayoutAlignment::stretch);

    content->add(make_label("Temporary Capability Showcase", 14.0f, color4_to_nancolor(240, 242, 250)));
    content->add(make_label(
        "这版展示只保留当前已经稳定的 runtime、layout、widgets 和 authoring 能力，用来代替旧的伪产品 dashboard。",
        7.0f,
        color4_to_nancolor(192, 198, 220)));

    auto pills = nandina::layout::Row::Create();
    pills->align_items(LayoutAlignment::center)
        .gap(10.0f);
    pills->add(make_pill("layout protocol active", color4_to_nancolor(64, 86, 124), color4_to_nancolor(228, 231, 244)));
    pills->add(make_pill("widgets in cleanup", color4_to_nancolor(66, 74, 98), color4_to_nancolor(228, 231, 244)));
    pills->add(make_pill("showcase reset", color4_to_nancolor(58, 92, 82), color4_to_nancolor(220, 240, 230)));
    content->add(std::move(pills));

    hero->add_child(std::move(content));
    return hero;
}

auto build_scope_panel() -> nandina::widgets::Panel::Ptr {
    auto panel = nandina::widgets::Panel::create();
    panel->set_title("Current Scope")
        .set_header_height(30.0f)
        .set_header_color(color4_to_nancolor(38, 40, 56))
        .set_bg_color(color4_to_nancolor(50, 52, 72))
        .set_corner_radius(10.0f)
        .set_padding(nandina::geometry::NanInsets{14.0f, 14.0f, 14.0f, 14.0f});

    auto column = nandina::layout::Column::Create();
    column->gap(10.0f)
        .align_items(LayoutAlignment::stretch);
    column->add(make_bullet_row(nandina::widgets::IconType::Dot, color4_to_nancolor(99, 102, 241),
        "Layout protocol", "measure/layout/measured-size 已成为主线约束。"));
    column->add(make_bullet_row(nandina::widgets::IconType::Dot, color4_to_nancolor(95, 200, 130),
        "Widget cleanup", "Sidebar、Card、Button 等控件正在持续去手工 frame。"));
    column->add(make_bullet_row(nandina::widgets::IconType::Dot, color4_to_nancolor(245, 158, 60),
        "Authoring API", "mount、row、column、padding、label/button/card/panel 可直接验证。"));
    column->add(make_bullet_row(nandina::widgets::IconType::Dot, color4_to_nancolor(236, 110, 130),
        "Temporary showcase", "当前展示改成能力看板，而不是继续维护旧 dashboard 假数据。"));
    panel->add_child(std::move(column));
    return panel;
}

auto build_layers_panel() -> nandina::widgets::Panel::Ptr {
    auto panel = nandina::widgets::Panel::create();
    panel->set_title("Layer Snapshot")
        .set_header_height(30.0f)
        .set_header_color(color4_to_nancolor(38, 40, 56))
        .set_bg_color(color4_to_nancolor(50, 52, 72))
        .set_corner_radius(10.0f)
        .set_padding(nandina::geometry::NanInsets{14.0f, 14.0f, 14.0f, 14.0f});

    auto column = nandina::layout::Column::Create();
    column->gap(10.0f)
        .align_items(LayoutAlignment::stretch);
    column->add(make_bullet_row(nandina::widgets::IconType::Circle, color4_to_nancolor(147, 150, 255),
        "foundation", "geometry / constraints / color / logging 等底层数据结构已稳定。"));
    column->add(make_bullet_row(nandina::widgets::IconType::Circle, color4_to_nancolor(99, 102, 241),
        "runtime", "widget tree、dirty、hit-test、window bridge 已经可跑。"));
    column->add(make_bullet_row(nandina::widgets::IconType::Circle, color4_to_nancolor(95, 200, 130),
        "layout", "row/column/stack/padding/expanded/sized_box 是当前主拼装原语。"));
    column->add(make_bullet_row(nandina::widgets::IconType::Circle, color4_to_nancolor(245, 158, 60),
        "widgets", "surface / button / panel / card / sidebar / split-row 已接到主链。"));
    column->add(make_bullet_row(nandina::widgets::IconType::Circle, color4_to_nancolor(80, 200, 220),
        "app", "NanAppWindow + authoring root 已能稳定承载展示应用。"));
    panel->add_child(std::move(column));
    return panel;
}

auto build_widget_coverage_card() -> nandina::widgets::Card::Ptr {
    auto card = nandina::widgets::Card::create();
    card->set_title("Widget Coverage")
        .set_show_accent(true)
        .set_bg_color(color4_to_nancolor(50, 52, 72))
        .set_corner_radius(10.0f)
        .set_padding(nandina::geometry::NanInsets{14.0f, 14.0f, 14.0f, 14.0f});

    auto column = nandina::layout::Column::Create();
    column->gap(12.0f)
        .align_items(LayoutAlignment::stretch);
    column->add(make_progress_row("Surface / Panel / Card", 0.82f, color4_to_nancolor(99, 102, 241)));
    column->add(make_progress_row("Sidebar family", 0.76f, color4_to_nancolor(95, 200, 130)));
    column->add(make_progress_row("SplitRow + helpers", 0.72f, color4_to_nancolor(245, 158, 60)));
    column->add(make_progress_row("Showcase rewrite", 0.48f, color4_to_nancolor(236, 110, 130)));
    card->add_child(std::move(column));
    return card;
}

auto build_actions_card() -> nandina::widgets::Card::Ptr {
    auto card = nandina::widgets::Card::create();
    card->set_title("Interactive Samples")
        .set_bg_color(color4_to_nancolor(50, 52, 72))
        .set_corner_radius(10.0f)
        .set_padding(nandina::geometry::NanInsets{14.0f, 14.0f, 14.0f, 14.0f});

    auto column = nandina::layout::Column::Create();
    column->gap(12.0f)
        .align_items(LayoutAlignment::stretch);
    column->add(make_label("用现有 widgets 直接做一套临时展示，优先验证布局链和组合模式。", 7.0f,
        color4_to_nancolor(160, 162, 180)));

    auto row = nandina::layout::Row::Create();
    row->align_items(LayoutAlignment::center)
        .gap(12.0f);

    nandina::widgets::ButtonColors accent_colors;
    accent_colors.bg = color4_to_nancolor(99, 102, 241);
    accent_colors.bg_hover = color4_to_nancolor(120, 123, 255);
    accent_colors.bg_pressed = color4_to_nancolor(80, 82, 200);

    auto layout_btn = nandina::widgets::Button::create();
    layout_btn->set_text("Layout").set_colors(accent_colors);
    row->add(std::move(layout_btn));

    nandina::widgets::ButtonColors widget_colors;
    widget_colors.bg = color4_to_nancolor(95, 200, 130);
    widget_colors.bg_hover = color4_to_nancolor(110, 214, 145);
    widget_colors.bg_pressed = color4_to_nancolor(70, 170, 108);

    auto widgets_btn = nandina::widgets::Button::create();
    widgets_btn->set_text("Widgets").set_colors(widget_colors);
    row->add(std::move(widgets_btn));

    auto disabled_btn = nandina::widgets::Button::create();
    disabled_btn->set_text("Preview")
        .set_disabled(true);
    row->add(std::move(disabled_btn));

    column->add(std::move(row));
    column->add(make_progress_row("stability", 0.66f, color4_to_nancolor(80, 200, 220)));
    card->add_child(std::move(column));
    return card;
}

auto build_notes_card() -> nandina::widgets::Card::Ptr {
    auto card = nandina::widgets::Card::create();
    card->set_title("Temporary Notes")
        .set_bg_color(color4_to_nancolor(50, 52, 72))
        .set_corner_radius(10.0f)
        .set_padding(nandina::geometry::NanInsets{14.0f, 14.0f, 14.0f, 14.0f});

    auto column = nandina::layout::Column::Create();
    column->gap(10.0f)
        .align_items(LayoutAlignment::stretch);
    column->add(make_bullet_row(nandina::widgets::IconType::Dot, color4_to_nancolor(99, 102, 241),
        "Less fake product UI", "展示改为能力板，直接反映当前工程真实状态。"));
    column->add(make_bullet_row(nandina::widgets::IconType::Dot, color4_to_nancolor(95, 200, 130),
        "More direct coverage", "新的页面尽量直接复用现有 widgets 和 layout primitives。"));
    column->add(make_bullet_row(nandina::widgets::IconType::Dot, color4_to_nancolor(245, 158, 60),
        "Rewrite-friendly", "后面继续迭代时，只需要扩展这一页和对应测试。"));
    card->add_child(std::move(column));
    return card;
}

auto build_bottom_row() -> nandina::layout::Row::Ptr {
    auto row = nandina::layout::Row::Create();
    row->align_items(LayoutAlignment::stretch)
        .gap(20.0f);

    auto first = nandina::layout::Expanded::Create();
    first->child(build_widget_coverage_card());
    row->add(std::move(first));

    auto second = nandina::layout::Expanded::Create();
    second->child(build_actions_card());
    row->add(std::move(second));

    auto third = nandina::layout::Expanded::Create();
    third->child(build_notes_card());
    row->add(std::move(third));
    return row;
}

} // namespace

export class MainComponent final : public nandina::app::NanComponent {
public:
    MainComponent() {
        build_widget_tree();
    }

protected:
    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);
        m_layout_valid = false;
        return *this;
    }

    auto on_draw(tvg::SwCanvas& canvas) -> void override {
        if (!m_layout_valid) {
            flush_layout();
        }

        draw_rect(canvas, 0.0f, 0.0f, width(), height(), 21, 24, 32, 255);
    }

private:
    auto build_widget_tree() -> void {
        auto root_row = nandina::layout::Row::Create();
        root_row->align_items(LayoutAlignment::stretch);
        m_root_row = root_row.get();
        add_child(std::move(root_row));

        auto sidebar_slot = nandina::layout::SizedBox::Create();
        sidebar_slot->width(260.0f)
            .child(build_sidebar());
        m_sidebar_slot = sidebar_slot.get();
        m_root_row->add(std::move(sidebar_slot));

        auto main_expanded = nandina::layout::Expanded::Create();
        m_main_expanded = main_expanded.get();
        m_root_row->add(std::move(main_expanded));

        auto main_padding = nandina::layout::Padding::Create();
        main_padding->padding(20.0f);
        m_main_padding = main_padding.get();
        m_main_expanded->child(std::move(main_padding));

        auto main_column = nandina::layout::Column::Create();
        main_column->align_items(LayoutAlignment::stretch)
            .gap(20.0f);
        m_main_column = main_column.get();
        m_main_padding->child(std::move(main_column));

        auto hero_slot = nandina::layout::SizedBox::Create();
        hero_slot->height(136.0f)
            .child(build_hero_surface());
        m_hero_slot = hero_slot.get();
        m_main_column->add(std::move(hero_slot));

        auto split = nandina::widgets::SplitRow::create();
        split->set_gap(20.0f)
            .set_split_ratio(0.46f)
            .set_preferred_height(280.0f);
        split->set_leading(build_scope_panel());
        split->set_trailing(build_layers_panel());
        m_split_row = split.get();
        m_main_column->add(std::move(split));

        auto bottom_slot = nandina::layout::SizedBox::Create();
        bottom_slot->height(224.0f)
            .child(build_bottom_row());
        m_bottom_slot = bottom_slot.get();
        m_main_column->add(std::move(bottom_slot));
    }

    auto flush_layout() -> void {
        if (!m_root_row) {
            return;
        }

        m_root_row->measure(nandina::geometry::NanConstraints::tight(width(), height()));
        m_root_row->set_bounds(0.0f, 0.0f, width(), height());
        m_layout_valid = true;
    }

private:
    nandina::layout::Row* m_root_row{nullptr};
    nandina::layout::SizedBox* m_sidebar_slot{nullptr};
    nandina::layout::Expanded* m_main_expanded{nullptr};
    nandina::layout::Padding* m_main_padding{nullptr};
    nandina::layout::Column* m_main_column{nullptr};
    nandina::layout::SizedBox* m_hero_slot{nullptr};
    nandina::widgets::SplitRow* m_split_row{nullptr};
    nandina::layout::SizedBox* m_bottom_slot{nullptr};
    bool m_layout_valid{false};
};

export class MainWindow final : public nandina::app::NanAppWindow {
public:
    MainWindow()
        : nandina::app::NanAppWindow({
              .title = "NandinaUI — Temporary Capability Showcase",
              .width = 1280,
              .height = 720,
              .resizable = true,
              .high_dpi = true,
          }) {
        set_root(nandina::app::adopt(std::make_unique<MainComponent>()));
    }
};
