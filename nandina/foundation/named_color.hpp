//
// foundation/named_color - CSS Color 4 named color literals.
//
// 148 个不透明命名颜色（CSS Color 4 §6.1 快照：SVG 1.1 的 147 个 + rebeccapurple）。
// 这是**色名字面量**入口，不是语义 token：默认配方不得硬编码颜色；主题作者可以把命名
// 颜色赋给既有语义角色（组件仍通过语义角色消费）。`transparent` 与 `currentColor`
// 分别是透明黑与上下文相关关键字，均不属于固定不透明色值，不在本表范围。
//
// 规范快照：CSS Color 4 候选推荐草案 2026-11-03 §6.1
// https://www.w3.org/TR/2026/CRD-css-color-4-20261103/#named-colors
//

#ifndef NANDINA_EXPERIMENT_FOUNDATION_NAMED_COLOR_HPP
#define NANDINA_EXPERIMENT_FOUNDATION_NAMED_COLOR_HPP

#include "nandina_color.hpp"

#include <optional>
#include <string_view>

namespace nandina::foundation
{
    /// CSS Color 4 的 148 个命名颜色，按规范名称字母序声明。
    ///
    /// 枚举序号**不承诺持久化稳定**：保存时使用规范名称（`nan_named_color_name`）。
    /// 名称相同 RGB 的别名（如 aqua/cyan、gray/grey）各有独立成员与表项，不去重。
    enum class NanNamedColor {
        aliceblue,
        antiquewhite,
        aqua,
        aquamarine,
        azure,
        beige,
        bisque,
        black,
        blanchedalmond,
        blue,
        blueviolet,
        brown,
        burlywood,
        cadetblue,
        chartreuse,
        chocolate,
        coral,
        cornflowerblue,
        cornsilk,
        crimson,
        cyan,
        darkblue,
        darkcyan,
        darkgoldenrod,
        darkgray,
        darkgreen,
        darkgrey,
        darkkhaki,
        darkmagenta,
        darkolivegreen,
        darkorange,
        darkorchid,
        darkred,
        darksalmon,
        darkseagreen,
        darkslateblue,
        darkslategray,
        darkslategrey,
        darkturquoise,
        darkviolet,
        deeppink,
        deepskyblue,
        dimgray,
        dimgrey,
        dodgerblue,
        firebrick,
        floralwhite,
        forestgreen,
        fuchsia,
        gainsboro,
        ghostwhite,
        gold,
        goldenrod,
        gray,
        green,
        greenyellow,
        grey,
        honeydew,
        hotpink,
        indianred,
        indigo,
        ivory,
        khaki,
        lavender,
        lavenderblush,
        lawngreen,
        lemonchiffon,
        lightblue,
        lightcoral,
        lightcyan,
        lightgoldenrodyellow,
        lightgray,
        lightgreen,
        lightgrey,
        lightpink,
        lightsalmon,
        lightseagreen,
        lightskyblue,
        lightslategray,
        lightslategrey,
        lightsteelblue,
        lightyellow,
        lime,
        limegreen,
        linen,
        magenta,
        maroon,
        mediumaquamarine,
        mediumblue,
        mediumorchid,
        mediumpurple,
        mediumseagreen,
        mediumslateblue,
        mediumspringgreen,
        mediumturquoise,
        mediumvioletred,
        midnightblue,
        mintcream,
        mistyrose,
        moccasin,
        navajowhite,
        navy,
        oldlace,
        olive,
        olivedrab,
        orange,
        orangered,
        orchid,
        palegoldenrod,
        palegreen,
        paleturquoise,
        palevioletred,
        papayawhip,
        peachpuff,
        peru,
        pink,
        plum,
        powderblue,
        purple,
        rebeccapurple,
        red,
        rosybrown,
        royalblue,
        saddlebrown,
        salmon,
        sandybrown,
        seagreen,
        seashell,
        sienna,
        silver,
        skyblue,
        slateblue,
        slategray,
        slategrey,
        snow,
        springgreen,
        steelblue,
        tan,
        teal,
        thistle,
        tomato,
        turquoise,
        violet,
        wheat,
        white,
        whitesmoke,
        yellow,
        yellowgreen,
    };

    /// 类型化入口：已声明枚举成员 → 固定 sRGB 字面量（经 `NanColor::from_hex` 规范化）。
    [[nodiscard]] auto nan_named_color(NanNamedColor named) noexcept -> NanColor;

    /// 字符串入口：ASCII 大小写不敏感；拒绝空白、嵌入 NUL、非 ASCII 与未知名称。
    /// 不扩展为完整 CSS 颜色语法（不支持 `#`、`rgb()` 等）。
    [[nodiscard]] auto nan_named_color(std::string_view name) -> std::optional<NanColor>;

    /// 已声明枚举成员的规范名称（小写）供诊断与持久化；与字符串入口往返一致。
    [[nodiscard]] auto nan_named_color_name(NanNamedColor named) noexcept -> std::string_view;
} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_FOUNDATION_NAMED_COLOR_HPP
