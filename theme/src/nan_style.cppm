//
// nan_style.cppm — 组件样式原语（shadcn/ui 风格）
//
// 职责：
//   - 定义每个组件的默认样式（颜色、字体、间距、圆角等）
//   - 聚合为 NanStyle，开发者可覆盖任意属性
//   - 所有组件从此获取默认样式，确保视觉一致性
//
// 设计理念：
//   shadcn/ui 通过 CSS 变量（--primary, --radius 等）统一样式。
//   本模块提供等价的 C++ 结构体，开发者可：
//     auto style = NanStyle::default_style();
//     style.button.bg = my_accent;
//     style.button.corner_radius = 12.0f;
//     NanStyle::set_default(style);  // 全局生效
//
// 属于 theme 层，可被 widgets/app 消费。
//

module;

#include <memory>
#include <cstdint>

export module nandina.theme.nan_style;

export import nandina.foundation.color;
export import nandina.foundation.nan_insets;
export import nandina.theme.nan_primitive_tokens;
export import nandina.text.nan_font;

export namespace nandina::theme {

    // ═══════════════════════════════════════════════════════════════
    // § ButtonVariant / ButtonSize — theme 层定义，widgets 导入复用
    // ═══════════════════════════════════════════════════════════════

    enum class ColorVariant : std::uint8_t {
        inherit,
        primary,
        secondary,
        neutral,
        destructive,
    };

    enum class ButtonVariant : std::uint8_t {
        default_variant,
        secondary,
        outline,
        ghost,
        destructive,
        link,
    };

    enum class ButtonSize : std::uint8_t {
        xs,
        sm,
        md,
        lg,
        icon,
    };

    enum class TagSize : std::uint8_t {
        sm,
        md,
        lg,
    };

    enum class CheckboxSize : std::uint8_t {
        sm,
        md,
    };

    struct NanTextStyle {
        float               font_size{14.0f};
        text::NanFontWeight font_weight{text::NanFontWeight::regular};
        NanColor            font_color{NanColor::from(NanRgb{30, 30, 46})};
        text::TextOverflow  overflow{text::TextOverflow::wrap};
        bool                single_line{false};
        int                 max_lines{0};
    };

    // ═══════════════════════════════════════════════════════════════
    // § NanLabelStyle — Label 组件样式
    // ═══════════════════════════════════════════════════════════════

    struct NanLabelStyle {
        float           font_size{14.0f};
        text::NanFontWeight font_weight{text::NanFontWeight::regular};
        NanColor        font_color{NanColor::from(NanRgb{76, 79, 105})};
        NanColor        disabled_font_color{NanColor::from(NanRgb{154, 157, 180})};
        NanColor        error_font_color{NanColor::from(NanRgb{230, 69, 83})};
        NanColor        required_indicator_color{NanColor::from(NanRgb{230, 69, 83})};
        float           required_indicator_gap{4.0f};
        text::TextOverflow overflow{text::TextOverflow::wrap};
        bool            single_line{false};
        int             max_lines{0};
    };

    // ═══════════════════════════════════════════════════════════════
    // § NanButtonStyle — Button 组件样式
    // ═══════════════════════════════════════════════════════════════

    struct NanButtonStyle {
        struct PresetStyle {
            NanColor bg{NanColor::from(NanRgb{99, 102, 241})};
            NanColor bg_hover{NanColor::from(NanRgb{120, 123, 255})};
            NanColor bg_pressed{NanColor::from(NanRgb{80, 82, 200})};
            NanColor bg_disabled{NanColor::from(NanRgb{60, 62, 80})};
            NanColor text{NanColor::from(NanRgb{255, 255, 255})};
            NanColor text_disabled{NanColor::from(NanRgb{110, 112, 130})};
            NanColor border{NanColor::from(NanRgb{0, 0, 0, 0})};
            float border_width{0.0f};
        };

        struct SizeStyle {
            float height{40.0f};
            float font_size{14.0f};
            float padding_h{16.0f};
            float padding_v{8.0f};
            float gap{8.0f};
            float icon_size{18.0f};
            bool square{false};
        };

        struct ColorFamilyStyle {
            PresetStyle filled{};
            PresetStyle tonal{};
            PresetStyle outlined{};
            PresetStyle ghost{};
            PresetStyle destructive{};
            PresetStyle link{};
        };

        float           font_size{14.0f};
        text::NanFontWeight font_weight{text::NanFontWeight::medium};
        NanColor        font_color{NanColor::from(NanRgb{255, 255, 255})};
        text::TextOverflow overflow{text::TextOverflow::ellipsis};
        bool            single_line{true};
        float           corner_radius{6.0f};

        ColorVariant    color_variant{ColorVariant::inherit};
        ButtonVariant   variant{ButtonVariant::default_variant};
        ButtonSize      size{ButtonSize::md};

        ColorFamilyStyle primary_family{
            .filled = PresetStyle{},
            .tonal = PresetStyle{
                .bg = NanColor::from(NanRgb{230, 232, 250}),
                .bg_hover = NanColor::from(NanRgb{210, 213, 242}),
                .bg_pressed = NanColor::from(NanRgb{190, 193, 230}),
                .bg_disabled = NanColor::from(NanRgb{230, 232, 250}),
                .text = NanColor::from(NanRgb{69, 72, 200}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
            },
            .outlined = PresetStyle{
                .bg = NanColor::from(NanRgb{255, 255, 255}),
                .bg_hover = NanColor::from(NanRgb{230, 232, 250}),
                .bg_pressed = NanColor::from(NanRgb{210, 213, 242}),
                .bg_disabled = NanColor::from(NanRgb{255, 255, 255}),
                .text = NanColor::from(NanRgb{69, 72, 200}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
                .border = NanColor::from(NanRgb{99, 102, 241}),
                .border_width = 1.0f,
            },
            .ghost = PresetStyle{
                .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_hover = NanColor::from(NanRgb{230, 232, 250}),
                .bg_pressed = NanColor::from(NanRgb{210, 213, 242}),
                .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                .text = NanColor::from(NanRgb{69, 72, 200}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
            },
            .destructive = PresetStyle{},
            .link = PresetStyle{
                .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_hover = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_pressed = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                .text = NanColor::from(NanRgb{69, 72, 200}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
            },
        };

        PresetStyle filled{};
        PresetStyle tonal{
            .bg = NanColor::from(NanRgb{230, 232, 250}),
            .bg_hover = NanColor::from(NanRgb{210, 213, 242}),
            .bg_pressed = NanColor::from(NanRgb{190, 193, 230}),
            .bg_disabled = NanColor::from(NanRgb{230, 232, 250}),
            .text = NanColor::from(NanRgb{69, 72, 200}),
            .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
        };
        PresetStyle outlined{
            .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
            .bg_hover = NanColor::from(NanRgb{230, 232, 250}),
            .bg_pressed = NanColor::from(NanRgb{210, 213, 242}),
            .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
            .text = NanColor::from(NanRgb{69, 72, 200}),
            .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
            .border = NanColor::from(NanRgb{180, 183, 220}),
            .border_width = 1.0f,
        };
        PresetStyle ghost{
            .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
            .bg_hover = NanColor::from(NanRgb{230, 232, 250}),
            .bg_pressed = NanColor::from(NanRgb{210, 213, 242}),
            .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
            .text = NanColor::from(NanRgb{69, 72, 200}),
            .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
        };
        PresetStyle destructive{
            .bg = NanColor::from(NanRgb{230, 69, 83}),
            .bg_hover = NanColor::from(NanRgb{245, 90, 100}),
            .bg_pressed = NanColor::from(NanRgb{200, 50, 65}),
            .bg_disabled = NanColor::from(NanRgb{100, 60, 70}),
            .text = NanColor::from(NanRgb{255, 255, 255}),
            .text_disabled = NanColor::from(NanRgb{180, 160, 165}),
        };
        PresetStyle link{
            .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
            .bg_hover = NanColor::from(NanRgb{0, 0, 0, 0}),
            .bg_pressed = NanColor::from(NanRgb{0, 0, 0, 0}),
            .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
            .text = NanColor::from(NanRgb{69, 72, 200}),
            .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
        };

        ColorFamilyStyle secondary_family{
            .filled = PresetStyle{
                .bg = NanColor::from(NanRgb{114, 135, 253}),
                .bg_hover = NanColor::from(NanRgb{142, 161, 255}),
                .bg_pressed = NanColor::from(NanRgb{96, 114, 230}),
                .bg_disabled = NanColor::from(NanRgb{198, 206, 255}),
                .text = NanColor::from(NanRgb{255, 255, 255}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
            },
            .tonal = PresetStyle{
                .bg = NanColor::from(NanRgb{223, 229, 255}),
                .bg_hover = NanColor::from(NanRgb{204, 213, 255}),
                .bg_pressed = NanColor::from(NanRgb{184, 195, 248}),
                .bg_disabled = NanColor::from(NanRgb{223, 229, 255}),
                .text = NanColor::from(NanRgb{66, 84, 186}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
            },
            .outlined = PresetStyle{
                .bg = NanColor::from(NanRgb{255, 255, 255}),
                .bg_hover = NanColor::from(NanRgb{235, 239, 255}),
                .bg_pressed = NanColor::from(NanRgb{223, 229, 255}),
                .bg_disabled = NanColor::from(NanRgb{255, 255, 255}),
                .text = NanColor::from(NanRgb{66, 84, 186}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
                .border = NanColor::from(NanRgb{114, 135, 253}),
                .border_width = 1.0f,
            },
            .ghost = PresetStyle{
                .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_hover = NanColor::from(NanRgb{235, 239, 255}),
                .bg_pressed = NanColor::from(NanRgb{223, 229, 255}),
                .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                .text = NanColor::from(NanRgb{66, 84, 186}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
            },
            .destructive = PresetStyle{
                .bg = NanColor::from(NanRgb{114, 135, 253}),
                .bg_hover = NanColor::from(NanRgb{142, 161, 255}),
                .bg_pressed = NanColor::from(NanRgb{96, 114, 230}),
                .bg_disabled = NanColor::from(NanRgb{198, 206, 255}),
                .text = NanColor::from(NanRgb{255, 255, 255}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
            },
            .link = PresetStyle{
                .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_hover = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_pressed = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                .text = NanColor::from(NanRgb{66, 84, 186}),
                .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
            },
        };
        ColorFamilyStyle neutral_family{
            .filled = PresetStyle{
                .bg = NanColor::from(NanRgb{92, 95, 119}),
                .bg_hover = NanColor::from(NanRgb{108, 111, 133}),
                .bg_pressed = NanColor::from(NanRgb{76, 79, 105}),
                .bg_disabled = NanColor::from(NanRgb{204, 207, 220}),
                .text = NanColor::from(NanRgb{255, 255, 255}),
                .text_disabled = NanColor::from(NanRgb{154, 157, 180}),
            },
            .tonal = PresetStyle{
                .bg = NanColor::from(NanRgb{239, 241, 245}),
                .bg_hover = NanColor::from(NanRgb{230, 233, 239}),
                .bg_pressed = NanColor::from(NanRgb{220, 224, 232}),
                .bg_disabled = NanColor::from(NanRgb{239, 241, 245}),
                .text = NanColor::from(NanRgb{76, 79, 105}),
                .text_disabled = NanColor::from(NanRgb{154, 157, 180}),
            },
            .outlined = PresetStyle{
                .bg = NanColor::from(NanRgb{255, 255, 255}),
                .bg_hover = NanColor::from(NanRgb{239, 241, 245}),
                .bg_pressed = NanColor::from(NanRgb{230, 233, 239}),
                .bg_disabled = NanColor::from(NanRgb{255, 255, 255}),
                .text = NanColor::from(NanRgb{76, 79, 105}),
                .text_disabled = NanColor::from(NanRgb{154, 157, 180}),
                .border = NanColor::from(NanRgb{166, 173, 200}),
                .border_width = 1.0f,
            },
            .ghost = PresetStyle{
                .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_hover = NanColor::from(NanRgb{239, 241, 245}),
                .bg_pressed = NanColor::from(NanRgb{230, 233, 239}),
                .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                .text = NanColor::from(NanRgb{76, 79, 105}),
                .text_disabled = NanColor::from(NanRgb{154, 157, 180}),
            },
            .destructive = PresetStyle{
                .bg = NanColor::from(NanRgb{92, 95, 119}),
                .bg_hover = NanColor::from(NanRgb{108, 111, 133}),
                .bg_pressed = NanColor::from(NanRgb{76, 79, 105}),
                .bg_disabled = NanColor::from(NanRgb{204, 207, 220}),
                .text = NanColor::from(NanRgb{255, 255, 255}),
                .text_disabled = NanColor::from(NanRgb{154, 157, 180}),
            },
            .link = PresetStyle{
                .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_hover = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_pressed = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                .text = NanColor::from(NanRgb{76, 79, 105}),
                .text_disabled = NanColor::from(NanRgb{154, 157, 180}),
            },
        };
        ColorFamilyStyle destructive_family{
            .filled = PresetStyle{
                .bg = NanColor::from(NanRgb{230, 69, 83}),
                .bg_hover = NanColor::from(NanRgb{245, 90, 100}),
                .bg_pressed = NanColor::from(NanRgb{200, 50, 65}),
                .bg_disabled = NanColor::from(NanRgb{239, 184, 188}),
                .text = NanColor::from(NanRgb{255, 255, 255}),
                .text_disabled = NanColor::from(NanRgb{180, 160, 165}),
            },
            .tonal = PresetStyle{
                .bg = NanColor::from(NanRgb{251, 225, 228}),
                .bg_hover = NanColor::from(NanRgb{248, 205, 210}),
                .bg_pressed = NanColor::from(NanRgb{243, 183, 190}),
                .bg_disabled = NanColor::from(NanRgb{251, 225, 228}),
                .text = NanColor::from(NanRgb{178, 47, 61}),
                .text_disabled = NanColor::from(NanRgb{180, 160, 165}),
            },
            .outlined = PresetStyle{
                .bg = NanColor::from(NanRgb{255, 255, 255}),
                .bg_hover = NanColor::from(NanRgb{251, 225, 228}),
                .bg_pressed = NanColor::from(NanRgb{248, 205, 210}),
                .bg_disabled = NanColor::from(NanRgb{255, 255, 255}),
                .text = NanColor::from(NanRgb{230, 69, 83}),
                .text_disabled = NanColor::from(NanRgb{180, 160, 165}),
                .border = NanColor::from(NanRgb{230, 69, 83}),
                .border_width = 1.0f,
            },
            .ghost = PresetStyle{
                .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_hover = NanColor::from(NanRgb{251, 225, 228}),
                .bg_pressed = NanColor::from(NanRgb{248, 205, 210}),
                .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                .text = NanColor::from(NanRgb{230, 69, 83}),
                .text_disabled = NanColor::from(NanRgb{180, 160, 165}),
            },
            .destructive = PresetStyle{
                .bg = NanColor::from(NanRgb{230, 69, 83}),
                .bg_hover = NanColor::from(NanRgb{245, 90, 100}),
                .bg_pressed = NanColor::from(NanRgb{200, 50, 65}),
                .bg_disabled = NanColor::from(NanRgb{239, 184, 188}),
                .text = NanColor::from(NanRgb{255, 255, 255}),
                .text_disabled = NanColor::from(NanRgb{180, 160, 165}),
            },
            .link = PresetStyle{
                .bg = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_hover = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_pressed = NanColor::from(NanRgb{0, 0, 0, 0}),
                .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                .text = NanColor::from(NanRgb{230, 69, 83}),
                .text_disabled = NanColor::from(NanRgb{180, 160, 165}),
            },
        };

        SizeStyle xs{
            .height = 24.0f,
            .font_size = 11.0f,
            .padding_h = 8.0f,
            .padding_v = 2.0f,
            .gap = 4.0f,
            .icon_size = 14.0f,
        };
        SizeStyle sm{
            .height = 32.0f,
            .font_size = 12.0f,
            .padding_h = 12.0f,
            .padding_v = 4.0f,
            .gap = 6.0f,
            .icon_size = 16.0f,
        };
        SizeStyle md{};
        SizeStyle lg{
            .height = 48.0f,
            .font_size = 16.0f,
            .padding_h = 24.0f,
            .padding_v = 12.0f,
            .gap = 10.0f,
            .icon_size = 22.0f,
        };
        SizeStyle icon{
            .height = 40.0f,
            .font_size = 0.0f,
            .padding_h = 0.0f,
            .padding_v = 0.0f,
            .gap = 0.0f,
            .icon_size = 20.0f,
            .square = true,
        };
    };

    // ═══════════════════════════════════════════════════════════════
    // § NanCardStyle — Card 组件样式
    // ═══════════════════════════════════════════════════════════════

    struct NanCardStyle {
        NanColor bg{NanColor::from(NanRgb{255, 255, 255})};
        NanColor border{NanColor::from(NanRgb{226, 228, 240})};
        float corner_radius{8.0f};
        float border_width{1.0f};
        geometry::NanInsets padding{16.0f};

        float           title_font_size{16.0f};
        text::NanFontWeight title_font_weight{text::NanFontWeight::semiBold};
        NanColor        title_font_color{NanColor::from(NanRgb{30, 30, 46})};
    };

    struct NanPanelStyle {
        NanColor bg{NanColor::from(NanRgb{255, 255, 255})};
        NanColor border{NanColor::from(NanRgb{226, 228, 240})};
        float corner_radius{8.0f};
        float border_width{1.0f};
        geometry::NanInsets padding{12.0f};

        float           title_font_size{14.0f};
        text::NanFontWeight title_font_weight{text::NanFontWeight::semiBold};
        NanColor        title_font_color{NanColor::from(NanRgb{76, 79, 105})};
    };

    struct NanInputStyle {
        struct ColorFamilyStyle {
            NanColor font_color{NanColor::from(NanRgb{30, 30, 46})};
            NanColor placeholder_font_color{NanColor::from(NanRgb{154, 157, 180})};
            NanColor bg{NanColor::from(NanRgb{239, 241, 245})};
            NanColor border{NanColor::from(NanRgb{204, 207, 220})};
            NanColor border_focus{NanColor::from(NanRgb{99, 102, 241})};
        };

        float           font_size{14.0f};
        text::NanFontWeight font_weight{text::NanFontWeight::regular};
        NanColor        font_color{NanColor::from(NanRgb{30, 30, 46})};
        float           placeholder_font_size{14.0f};
        NanColor        placeholder_font_color{NanColor::from(NanRgb{154, 157, 180})};

        ColorVariant    color_variant{ColorVariant::inherit};
        NanColor bg{NanColor::from(NanRgb{239, 241, 245})};
        NanColor border{NanColor::from(NanRgb{204, 207, 220})};
        NanColor border_focus{NanColor::from(NanRgb{99, 102, 241})};
        float corner_radius{6.0f};
        float border_width{1.0f};
        geometry::NanInsets padding{12.0f, 10.0f, 12.0f, 10.0f};

        ColorFamilyStyle secondary_family{
            .font_color = NanColor::from(NanRgb{48, 52, 70}),
            .placeholder_font_color = NanColor::from(NanRgb{114, 135, 253}),
            .bg = NanColor::from(NanRgb{245, 247, 255}),
            .border = NanColor::from(NanRgb{186, 198, 255}),
            .border_focus = NanColor::from(NanRgb{114, 135, 253}),
        };
        ColorFamilyStyle neutral_family{
            .font_color = NanColor::from(NanRgb{76, 79, 105}),
            .placeholder_font_color = NanColor::from(NanRgb{154, 157, 180}),
            .bg = NanColor::from(NanRgb{247, 248, 250}),
            .border = NanColor::from(NanRgb{186, 194, 222}),
            .border_focus = NanColor::from(NanRgb{92, 95, 119}),
        };
        ColorFamilyStyle destructive_family{
            .font_color = NanColor::from(NanRgb{120, 36, 44}),
            .placeholder_font_color = NanColor::from(NanRgb{210, 95, 106}),
            .bg = NanColor::from(NanRgb{254, 242, 243}),
            .border = NanColor::from(NanRgb{242, 170, 178}),
            .border_focus = NanColor::from(NanRgb{230, 69, 83}),
        };
    };

    struct NanTagStyle {
        struct ColorFamilyStyle {
            NanColor bg{NanColor::from(NanRgb{230, 232, 250})};
            NanColor text{NanColor::from(NanRgb{69, 72, 200})};
            NanColor border{NanColor::from(NanRgb{190, 193, 230})};
            NanColor bg_disabled{NanColor::from(NanRgb{239, 241, 245})};
            NanColor text_disabled{NanColor::from(NanRgb{154, 157, 180})};
            NanColor border_disabled{NanColor::from(NanRgb{204, 207, 220})};
        };

        struct SizeStyle {
            float font_size{12.0f};
            float padding_h{10.0f};
            float padding_v{4.0f};
        };

        float font_size{12.0f};
        text::NanFontWeight font_weight{text::NanFontWeight::medium};
        text::TextOverflow overflow{text::TextOverflow::ellipsis};
        bool single_line{true};

        ColorVariant color_variant{ColorVariant::inherit};
        TagSize size{TagSize::md};
        float corner_radius{999.0f};
        float border_width{1.0f};

        NanColor bg{NanColor::from(NanRgb{230, 232, 250})};
        NanColor text{NanColor::from(NanRgb{69, 72, 200})};
        NanColor border{NanColor::from(NanRgb{190, 193, 230})};
        NanColor bg_disabled{NanColor::from(NanRgb{239, 241, 245})};
        NanColor text_disabled{NanColor::from(NanRgb{154, 157, 180})};
        NanColor border_disabled{NanColor::from(NanRgb{204, 207, 220})};

        ColorFamilyStyle secondary_family{
            .bg = NanColor::from(NanRgb{223, 229, 255}),
            .text = NanColor::from(NanRgb{66, 84, 186}),
            .border = NanColor::from(NanRgb{184, 195, 248}),
            .bg_disabled = NanColor::from(NanRgb{239, 241, 245}),
            .text_disabled = NanColor::from(NanRgb{154, 157, 180}),
            .border_disabled = NanColor::from(NanRgb{204, 207, 220}),
        };
        ColorFamilyStyle neutral_family{
            .bg = NanColor::from(NanRgb{247, 248, 250}),
            .text = NanColor::from(NanRgb{76, 79, 105}),
            .border = NanColor::from(NanRgb{215, 218, 228}),
            .bg_disabled = NanColor::from(NanRgb{239, 241, 245}),
            .text_disabled = NanColor::from(NanRgb{154, 157, 180}),
            .border_disabled = NanColor::from(NanRgb{204, 207, 220}),
        };
        ColorFamilyStyle destructive_family{
            .bg = NanColor::from(NanRgb{251, 225, 228}),
            .text = NanColor::from(NanRgb{178, 47, 61}),
            .border = NanColor::from(NanRgb{242, 170, 178}),
            .bg_disabled = NanColor::from(NanRgb{239, 241, 245}),
            .text_disabled = NanColor::from(NanRgb{154, 157, 180}),
            .border_disabled = NanColor::from(NanRgb{204, 207, 220}),
        };

        SizeStyle sm{};
        SizeStyle md{
            .font_size = 12.0f,
            .padding_h = 10.0f,
            .padding_v = 4.0f,
        };
        SizeStyle lg{
            .font_size = 14.0f,
            .padding_h = 12.0f,
            .padding_v = 6.0f,
        };
    };

    // ═══════════════════════════════════════════════════════════════
    // § NanCheckboxStyle — Checkbox 组件样式
    // ═══════════════════════════════════════════════════════════════

    struct NanCheckboxStyle {
        struct ColorFamilyStyle {
            NanColor box_bg{NanColor::from(NanRgb{99, 102, 241})};
            NanColor box_border{NanColor::from(NanRgb{99, 102, 241})};
            NanColor check{NanColor::from(NanRgb{255, 255, 255})};
            NanColor box_bg_disabled{NanColor::from(NanRgb{204, 207, 220})};
            NanColor box_border_disabled{NanColor::from(NanRgb{204, 207, 220})};
            NanColor check_disabled{NanColor::from(NanRgb{154, 157, 180})};
        };

        struct SizeStyle {
            float box_size{16.0f};
            float font_size{14.0f};
            float gap{8.0f};
            float corner_radius{4.0f};
        };

        ColorVariant color_variant{ColorVariant::inherit};
        CheckboxSize size{CheckboxSize::md};

        float box_size{16.0f};
        float font_size{14.0f};
        float gap{8.0f};
        float corner_radius{4.0f};

        NanColor box_bg{NanColor::from(NanRgb{99, 102, 241})};
        NanColor box_border{NanColor::from(NanRgb{99, 102, 241})};
        NanColor check{NanColor::from(NanRgb{255, 255, 255})};
        NanColor box_bg_unchecked{NanColor::from(NanRgb{255, 255, 255})};
        NanColor box_border_unchecked{NanColor::from(NanRgb{186, 194, 222})};
        NanColor box_bg_disabled{NanColor::from(NanRgb{204, 207, 220})};
        NanColor box_border_disabled{NanColor::from(NanRgb{204, 207, 220})};
        NanColor check_disabled{NanColor::from(NanRgb{154, 157, 180})};

        ColorFamilyStyle secondary_family{
            .box_bg = NanColor::from(NanRgb{114, 135, 253}),
            .box_border = NanColor::from(NanRgb{114, 135, 253}),
            .check = NanColor::from(NanRgb{255, 255, 255}),
        };
        ColorFamilyStyle neutral_family{
            .box_bg = NanColor::from(NanRgb{92, 95, 119}),
            .box_border = NanColor::from(NanRgb{92, 95, 119}),
            .check = NanColor::from(NanRgb{255, 255, 255}),
        };
        ColorFamilyStyle destructive_family{
            .box_bg = NanColor::from(NanRgb{230, 69, 83}),
            .box_border = NanColor::from(NanRgb{230, 69, 83}),
            .check = NanColor::from(NanRgb{255, 255, 255}),
        };

        SizeStyle sm{
            .box_size = 14.0f,
            .font_size = 12.0f,
            .gap = 6.0f,
            .corner_radius = 3.0f,
        };
        SizeStyle md{};
    };

    struct NanFocusRingStyle {
        NanColor color{NanColor::from(NanRgb{99, 102, 241})};
        float width{2.0f};
        float offset{2.0f};
    };

    // ═══════════════════════════════════════════════════════════════
    // § NanProgressStyle — ProgressBar 组件样式
    // ═══════════════════════════════════════════════════════════════

    struct NanProgressStyle {
        struct ColorFamilyStyle {
            NanColor track_bg{NanColor::from(NanRgb{226, 228, 240})};
            NanColor fill{NanColor::from(NanRgb{99, 102, 241})};
        };

        ColorVariant color_variant{ColorVariant::inherit};
        NanColor track_bg{NanColor::from(NanRgb{226, 228, 240})};   // surface-variant
        NanColor fill{NanColor::from(NanRgb{99, 102, 241})};        // primary
        float corner_radius{4.0f};
        float bar_height{6.0f};
        ColorFamilyStyle secondary_family{
            .track_bg = NanColor::from(NanRgb{223, 229, 255}),
            .fill = NanColor::from(NanRgb{114, 135, 253}),
        };
        ColorFamilyStyle neutral_family{
            .track_bg = NanColor::from(NanRgb{230, 233, 239}),
            .fill = NanColor::from(NanRgb{108, 111, 133}),
        };
        ColorFamilyStyle destructive_family{
            .track_bg = NanColor::from(NanRgb{251, 225, 228}),
            .fill = NanColor::from(NanRgb{230, 69, 83}),
        };
    };

    // ═══════════════════════════════════════════════════════════════
    // § NanStylePrimitives — 聚合所有组件样式
    // ═══════════════════════════════════════════════════════════════

    /**
     * NanStylePrimitives — 全局组件样式原语
     *
     * 类似 shadcn/ui 的 CSS 变量系统。
     * 开发者可覆盖任意字段来定制整个应用的视觉风格。
     *
     * 用法：
     *   auto style = NanStylePrimitives::default_style();
     *   style.button.bg = NanColor::from(NanRgb{"#e64553"});    // catppuccin red
     *   style.button.corner_radius = 12.0f;
     *   style.spacing.medium = 16.0f;
     *   NanStylePrimitives::set_current(style);
     */
    struct NanStylePrimitives {
        // ── 组件样式 ──────────────────────────────────────
        NanTextStyle      text;
        NanLabelStyle     label;
        NanButtonStyle    button;
        NanCardStyle      card;
        NanPanelStyle     panel;
        NanInputStyle     input;
        NanTagStyle       tag;
        NanCheckboxStyle  checkbox;
        NanFocusRingStyle focus_ring;
        NanProgressStyle  progress;

        // ── 设计 Token ────────────────────────────────────
        NanSpacingTokens    spacing;
        NanRadiusTokens     radius;
        NanTypographyTokens typography;
        NanElevationTokens  elevation;
        NanOpacityTokens    opacity;

        // ── 工厂 ──────────────────────────────────────────

        /// 获取默认样式（Catppuccin Latte 浅色风格）
        [[nodiscard]] static auto default_style() -> NanStylePrimitives {
            return NanStylePrimitives{};
        }

        /// 获取当前全局样式（如果未设置，返回默认）
        [[nodiscard]] static auto current() -> const NanStylePrimitives& {
            return s_current ? *s_current : s_default_instance();
        }

        /// 设置全局样式
        static auto set_current(const NanStylePrimitives& style) -> void {
            if (!s_current) {
                s_current = new NanStylePrimitives();
            }
            *s_current = style;
        }

        /// 重置为默认样式
        static auto reset_to_default() -> void {
            delete s_current;
            s_current = nullptr;
        }

    private:
        static auto s_default_instance() -> const NanStylePrimitives& {
            static const NanStylePrimitives instance{};
            return instance;
        }

        inline static NanStylePrimitives* s_current = nullptr;
    };

} // namespace nandina::theme
