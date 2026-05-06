module;

#include <memory>
#include <thorvg-1/thorvg.h>

export module nandina.showcase.footer_section;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.runtime.nan_widget;

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

class FooterSection final : public runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<FooterSection>;

    static auto create() -> Ptr {
        return Ptr{new FooterSection()};
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_content_root) {
            m_content_root->set_bounds(x, y, w, h);
        }

        return *this;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 26.0f};
    }

protected:
    void on_draw(tvg::SwCanvas& canvas) override {
        const auto rect = bounds();
        draw_line(canvas, rect.x(), rect.y(), rect.x() + rect.width(), rect.y(), 55, 57, 75, 100);
    }

private:
    FooterSection() {
        using namespace nandina::app;

        auto content = padding(
            label("Built with NandinaUI · ThorVG rendering · C++26 modules")
                .font_size(7.0f)
                .color(color4_to_nancolor(110, 112, 130)))
            .padding(0.0f, 12.0f, 0.0f, 0.0f);

        m_content_root = add_child(mount(std::move(content)));
    }

    nandina::runtime::NanWidget* m_content_root{nullptr};
};

} // namespace nandina::showcase