module;

#include <memory>
#include <string>
#include <thorvg-1/thorvg.h>

export module nandina.showcase.header_bar;

import nandina.foundation.color;
import nandina.foundation.nan_insets;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.runtime.nan_widget;
import nandina.widgets.button;
import nandina.widgets.label;
import nandina.widgets.surface;

namespace {
static void draw_line(tvg::SwCanvas& canvas,
    const float x1, const float y1, const float x2, const float y2,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a,
    const float width = 1.0f) {
    auto* shape = tvg::Shape::gen();
    shape->moveTo(x1, y1);
    shape->lineTo(x2, y2);
    shape->strokeWidth(width);
    shape->strokeFill(r, g, b, a);
    canvas.add(shape);
}

static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
    const uint8_t a = 255) -> nandina::NanColor {
    return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
}
}

export namespace nandina::showcase {

class HeaderActionBlock final : public widgets::Surface {
public:
    using Ptr = std::unique_ptr<HeaderActionBlock>;

    static auto create(const nandina::NanColor& color) -> Ptr {
        return Ptr{new HeaderActionBlock(color)};
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {28.0f, 24.0f};
    }

private:
    explicit HeaderActionBlock(const nandina::NanColor& color) {
        set_bg_color(color)
            .set_corner_radius(4.0f);
    }
};

class HeaderBar final : public widgets::Surface {
public:
    using Ptr = std::unique_ptr<HeaderBar>;

    static auto create() -> Ptr {
        return Ptr{new HeaderBar()};
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 44.0f};
    }

protected:
    void on_draw(tvg::SwCanvas& canvas) override {
        Surface::on_draw(canvas);
        const auto rect = bounds();
        draw_line(canvas, rect.x(), rect.y() + rect.height(), rect.x() + rect.width(), rect.y() + rect.height(), 50, 52, 70, 120);
    }

private:
    HeaderBar() {
        set_bg_color(color4_to_nancolor(35, 37, 54, 240));

        auto row = nandina::layout::Row::Create();
        row->padding(20.0f, 0.0f, 6.0f, 0.0f)
            .align_items(nandina::layout::LayoutAlignment::center)
            .gap(10.0f);

        auto title = nandina::widgets::Label::create();
        title->set_text("Dashboard / Overview")
            .set_font_size(10.0f)
            .set_color(color4_to_nancolor(160, 162, 180));
        m_title_label = static_cast<nandina::widgets::Label*>(title.get());
        row->add(std::move(title));

        row->add(nandina::layout::Spacer::Create());
        row->add(HeaderActionBlock::create(color4_to_nancolor(55, 57, 75, 200)));
        row->add(HeaderActionBlock::create(color4_to_nancolor(55, 57, 75, 200)));

        nandina::widgets::ButtonColors btn_colors;
        btn_colors.bg            = color4_to_nancolor(99, 102, 241);
        btn_colors.bg_hover      = color4_to_nancolor(147, 150, 255);
        btn_colors.bg_pressed    = color4_to_nancolor(80, 82, 200);
        btn_colors.corner_radius = 6.0f;
        btn_colors.padding       = nandina::geometry::NanInsets{10.0f, 6.0f, 10.0f, 6.0f};

        auto button = nandina::widgets::Button::create();
        button->set_text("+")
            .set_colors(btn_colors);
        m_add_button = button.get();
        m_add_button->on_click([this]() {
            ++m_click_count;
            m_add_button->set_text("+" + std::to_string(m_click_count));
        });

        auto button_box = nandina::layout::SizedBox::Create();
        button_box->width(32.0f)
            .height(32.0f)
            .child(std::move(button));
        row->add(std::move(button_box));

        add_child(std::move(row));
    }

    nandina::widgets::Label* m_title_label{nullptr};
    nandina::widgets::Button* m_add_button{nullptr};
    int m_click_count{0};
};

} // namespace nandina::showcase