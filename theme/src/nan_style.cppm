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

export module nandina.theme.nan_style;

export import nandina.foundation.color;
export import nandina.foundation.nan_insets;
export import nandina.theme.nan_primitive_tokens;
export import nandina.text.nan_font;

export namespace nandina::theme {

    // ═══════════════════════════════════════════════════════════════
    // § NanLabelStyle — Label 组件样式
    // ═══════════════════════════════════════════════════════════════

    struct NanLabelStyle {
        float           font_size{14.0f};
        text::NanFontWeight font_weight{text::NanFontWeight::regular};
        NanColor        font_color{NanColor::from(NanRgb{76, 79, 105})};
        text::TextOverflow overflow{text::TextOverflow::ellipsis};
        bool            single_line{true};
        int             max_lines{1};
    };

    // ═══════════════════════════════════════════════════════════════
    // § NanButtonStyle — Button 组件样式
    // ═══════════════════════════════════════════════════════════════

    struct NanButtonStyle {
        float           font_size{14.0f};
        text::NanFontWeight font_weight{text::NanFontWeight::medium};
        NanColor        font_color{NanColor::from(NanRgb{255, 255, 255})};
        text::TextOverflow overflow{text::TextOverflow::ellipsis};
        bool            single_line{true};

        NanColor bg{NanColor::from(NanRgb{99, 102, 241})};
        NanColor bg_hover{NanColor::from(NanRgb{120, 123, 255})};
        NanColor bg_pressed{NanColor::from(NanRgb{80, 82, 200})};
        NanColor bg_disabled{NanColor::from(NanRgb{60, 62, 80})};
        NanColor text{NanColor::from(NanRgb{255, 255, 255})};
        NanColor text_disabled{NanColor::from(NanRgb{110, 112, 130})};

        float corner_radius{6.0f};
        geometry::NanInsets padding{12.0f, 8.0f, 12.0f, 8.0f};
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
        float           font_size{14.0f};
        text::NanFontWeight font_weight{text::NanFontWeight::regular};
        NanColor        font_color{NanColor::from(NanRgb{30, 30, 46})};
        float           placeholder_font_size{14.0f};
        NanColor        placeholder_font_color{NanColor::from(NanRgb{154, 157, 180})};

        NanColor bg{NanColor::from(NanRgb{239, 241, 245})};
        NanColor border{NanColor::from(NanRgb{204, 207, 220})};
        NanColor border_focus{NanColor::from(NanRgb{99, 102, 241})};
        float corner_radius{6.0f};
        float border_width{1.0f};
        geometry::NanInsets padding{12.0f, 10.0f, 12.0f, 10.0f};
    };

    // ═══════════════════════════════════════════════════════════════
    // § NanProgressStyle — ProgressBar 组件样式
    // ═══════════════════════════════════════════════════════════════

    struct NanProgressStyle {
        NanColor track_bg{NanColor::from(NanRgb{226, 228, 240})};   // surface-variant
        NanColor fill{NanColor::from(NanRgb{99, 102, 241})};        // primary
        float corner_radius{4.0f};
        float bar_height{6.0f};
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
        NanLabelStyle     label;
        NanButtonStyle    button;
        NanCardStyle      card;
        NanPanelStyle     panel;
        NanInputStyle     input;
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
                s_current = std::make_unique<NanStylePrimitives>();
            }
            *s_current = style;
        }

        /// 重置为默认样式
        static auto reset_to_default() -> void {
            s_current.reset();
        }

    private:
        static auto s_default_instance() -> const NanStylePrimitives& {
            static const NanStylePrimitives instance{};
            return instance;
        }

        inline static std::unique_ptr<NanStylePrimitives> s_current;
    };

} // namespace nandina::theme
