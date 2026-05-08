module;

#include <array>
#include <memory>
#include <tuple>

export module nandina.showcase.stack_demo_card;

import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.runtime.nan_widget;
import nandina.widgets.label;
import nandina.widgets.surface;

namespace {
    static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
        const uint8_t a = 255) -> nandina::NanColor {
        return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
    }
}

export namespace nandina::showcase {

    class StackDemoLayers final : public nandina::runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<StackDemoLayers>;

        static auto create() -> Ptr {
            return Ptr{new StackDemoLayers()};
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            if (m_stack) {
                m_stack->set_bounds(x, y, w, h);
            }
            return *this;
        }

    private:
        StackDemoLayers() {
            using namespace nandina::widgets;
            using namespace nandina::layout;

            auto stack = Stack::Create();
            stack->align_items(LayoutAlignment::center)
                .justify_content(LayoutAlignment::center);

            constexpr auto stack_box_colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>>({
                {99, 102, 241, 180},
                {147, 150, 255, 200},
                {200, 200, 255, 220},
            });
            constexpr auto box_sizes = std::to_array<geometry::NanSize>({
                geometry::NanSize{140.0f, 60.0f},
                geometry::NanSize{90.0f, 40.0f},
                geometry::NanSize{50.0f, 20.0f},
            });

            for (size_t i = 0; i < box_sizes.size(); ++i) {
                const auto& [r, g, b, a] = stack_box_colors[i];
                auto box                 = Surface::create();
                box->set_bg_color(color4_to_nancolor(r, g, b, a))
                    .set_corner_radius(8.0f);

                auto sized = SizedBox::Create();
                sized->size(box_sizes[i])
                    .child(std::move(box));
                stack->add(std::move(sized));
            }

            m_stack = stack.get();
            add_child(std::move(stack));
        }

        nandina::layout::Stack* m_stack{nullptr};
    };

    class StackDemoCard final : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<StackDemoCard>;

        static auto create() -> Ptr {
            return Ptr{new StackDemoCard()};
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);

            if (m_container) {
                m_container->set_bounds(x, y, w, h);
            }
            if (m_title) {
                m_title->set_bounds(x + 14.0f, y + 14.0f, 200.0f, 16.0f);
            }
            if (m_footer) {
                m_footer->set_bounds(x + 14.0f, y + h - 14.0f, w - 28.0f, 12.0f);
            }

            if (m_layers) {
                m_layers->set_bounds(x + 20.0f, y + 30.0f, w - 40.0f, h - 46.0f);
            }

            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return {0.0f, 110.0f};
        }

    private:
        StackDemoCard() {
            using namespace nandina::widgets;

            auto container = Surface::create();
            container->set_bg_color(color4_to_nancolor(50, 52, 72))
                .set_corner_radius(8.0f);
            m_container = add_child(std::move(container));

            auto title = Label::create();
            title->set_text("Stack Layout Demo")
                .set_font_size(9.0f)
                .set_color(color4_to_nancolor(220, 220, 240));
            m_title = add_child(std::move(title));

            m_layers = add_child(StackDemoLayers::create());

            auto footer = Label::create();
            footer->set_text("Column · Row · Stack — all from BasicLayoutBackend")
                .set_font_size(6.0f)
                .set_color(color4_to_nancolor(110, 112, 130));
            m_footer = add_child(std::move(footer));
        }

        runtime::NanWidget* m_container{nullptr};
        runtime::NanWidget* m_title{nullptr};
        runtime::NanWidget* m_layers{nullptr};
        runtime::NanWidget* m_footer{nullptr};
    };

} // namespace nandina::showcase
