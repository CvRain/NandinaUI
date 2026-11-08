// foundation/named_color - private lookup table for CSS Color 4 named colors.
#include "named_color.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace nandina::foundation
{
    namespace
    {
        struct NamedColorEntry {
            std::uint32_t rgb;
            std::string_view name;
        };

        /// 唯一数据表（规范快照 CSS Color 4 §6.1，2026-11-03 CRD）；按名称字母序，
        /// 与 `NanNamedColor` 声明顺序一致。RGB 为规范固定的 sRGB 整数值，不做去重。
        constexpr std::array<NamedColorEntry, 148> named_colors {{
            {0xF0F8FFU, "aliceblue"},
            {0xFAEBD7U, "antiquewhite"},
            {0x00FFFFU, "aqua"},
            {0x7FFFD4U, "aquamarine"},
            {0xF0FFFFU, "azure"},
            {0xF5F5DCU, "beige"},
            {0xFFE4C4U, "bisque"},
            {0x000000U, "black"},
            {0xFFEBCDU, "blanchedalmond"},
            {0x0000FFU, "blue"},
            {0x8A2BE2U, "blueviolet"},
            {0xA52A2AU, "brown"},
            {0xDEB887U, "burlywood"},
            {0x5F9EA0U, "cadetblue"},
            {0x7FFF00U, "chartreuse"},
            {0xD2691EU, "chocolate"},
            {0xFF7F50U, "coral"},
            {0x6495EDU, "cornflowerblue"},
            {0xFFF8DCU, "cornsilk"},
            {0xDC143CU, "crimson"},
            {0x00FFFFU, "cyan"},
            {0x00008BU, "darkblue"},
            {0x008B8BU, "darkcyan"},
            {0xB8860BU, "darkgoldenrod"},
            {0xA9A9A9U, "darkgray"},
            {0x006400U, "darkgreen"},
            {0xA9A9A9U, "darkgrey"},
            {0xBDB76BU, "darkkhaki"},
            {0x8B008BU, "darkmagenta"},
            {0x556B2FU, "darkolivegreen"},
            {0xFF8C00U, "darkorange"},
            {0x9932CCU, "darkorchid"},
            {0x8B0000U, "darkred"},
            {0xE9967AU, "darksalmon"},
            {0x8FBC8FU, "darkseagreen"},
            {0x483D8BU, "darkslateblue"},
            {0x2F4F4FU, "darkslategray"},
            {0x2F4F4FU, "darkslategrey"},
            {0x00CED1U, "darkturquoise"},
            {0x9400D3U, "darkviolet"},
            {0xFF1493U, "deeppink"},
            {0x00BFFFU, "deepskyblue"},
            {0x696969U, "dimgray"},
            {0x696969U, "dimgrey"},
            {0x1E90FFU, "dodgerblue"},
            {0xB22222U, "firebrick"},
            {0xFFFAF0U, "floralwhite"},
            {0x228B22U, "forestgreen"},
            {0xFF00FFU, "fuchsia"},
            {0xDCDCDCU, "gainsboro"},
            {0xF8F8FFU, "ghostwhite"},
            {0xFFD700U, "gold"},
            {0xDAA520U, "goldenrod"},
            {0x808080U, "gray"},
            {0x008000U, "green"},
            {0xADFF2FU, "greenyellow"},
            {0x808080U, "grey"},
            {0xF0FFF0U, "honeydew"},
            {0xFF69B4U, "hotpink"},
            {0xCD5C5CU, "indianred"},
            {0x4B0082U, "indigo"},
            {0xFFFFF0U, "ivory"},
            {0xF0E68CU, "khaki"},
            {0xE6E6FAU, "lavender"},
            {0xFFF0F5U, "lavenderblush"},
            {0x7CFC00U, "lawngreen"},
            {0xFFFACDU, "lemonchiffon"},
            {0xADD8E6U, "lightblue"},
            {0xF08080U, "lightcoral"},
            {0xE0FFFFU, "lightcyan"},
            {0xFAFAD2U, "lightgoldenrodyellow"},
            {0xD3D3D3U, "lightgray"},
            {0x90EE90U, "lightgreen"},
            {0xD3D3D3U, "lightgrey"},
            {0xFFB6C1U, "lightpink"},
            {0xFFA07AU, "lightsalmon"},
            {0x20B2AAU, "lightseagreen"},
            {0x87CEFAU, "lightskyblue"},
            {0x778899U, "lightslategray"},
            {0x778899U, "lightslategrey"},
            {0xB0C4DEU, "lightsteelblue"},
            {0xFFFFE0U, "lightyellow"},
            {0x00FF00U, "lime"},
            {0x32CD32U, "limegreen"},
            {0xFAF0E6U, "linen"},
            {0xFF00FFU, "magenta"},
            {0x800000U, "maroon"},
            {0x66CDAAU, "mediumaquamarine"},
            {0x0000CDU, "mediumblue"},
            {0xBA55D3U, "mediumorchid"},
            {0x9370DBU, "mediumpurple"},
            {0x3CB371U, "mediumseagreen"},
            {0x7B68EEU, "mediumslateblue"},
            {0x00FA9AU, "mediumspringgreen"},
            {0x48D1CCU, "mediumturquoise"},
            {0xC71585U, "mediumvioletred"},
            {0x191970U, "midnightblue"},
            {0xF5FFFAU, "mintcream"},
            {0xFFE4E1U, "mistyrose"},
            {0xFFE4B5U, "moccasin"},
            {0xFFDEADU, "navajowhite"},
            {0x000080U, "navy"},
            {0xFDF5E6U, "oldlace"},
            {0x808000U, "olive"},
            {0x6B8E23U, "olivedrab"},
            {0xFFA500U, "orange"},
            {0xFF4500U, "orangered"},
            {0xDA70D6U, "orchid"},
            {0xEEE8AAU, "palegoldenrod"},
            {0x98FB98U, "palegreen"},
            {0xAFEEEEU, "paleturquoise"},
            {0xDB7093U, "palevioletred"},
            {0xFFEFD5U, "papayawhip"},
            {0xFFDAB9U, "peachpuff"},
            {0xCD853FU, "peru"},
            {0xFFC0CBU, "pink"},
            {0xDDA0DDU, "plum"},
            {0xB0E0E6U, "powderblue"},
            {0x800080U, "purple"},
            {0x663399U, "rebeccapurple"},
            {0xFF0000U, "red"},
            {0xBC8F8FU, "rosybrown"},
            {0x4169E1U, "royalblue"},
            {0x8B4513U, "saddlebrown"},
            {0xFA8072U, "salmon"},
            {0xF4A460U, "sandybrown"},
            {0x2E8B57U, "seagreen"},
            {0xFFF5EEU, "seashell"},
            {0xA0522DU, "sienna"},
            {0xC0C0C0U, "silver"},
            {0x87CEEBU, "skyblue"},
            {0x6A5ACDU, "slateblue"},
            {0x708090U, "slategray"},
            {0x708090U, "slategrey"},
            {0xFFFAFAU, "snow"},
            {0x00FF7FU, "springgreen"},
            {0x4682B4U, "steelblue"},
            {0xD2B48CU, "tan"},
            {0x008080U, "teal"},
            {0xD8BFD8U, "thistle"},
            {0xFF6347U, "tomato"},
            {0x40E0D0U, "turquoise"},
            {0xEE82EEU, "violet"},
            {0xF5DEB3U, "wheat"},
            {0xFFFFFFU, "white"},
            {0xF5F5F5U, "whitesmoke"},
            {0xFFFF00U, "yellow"},
            {0x9ACD32U, "yellowgreen"},
        }};

        /// 仅折叠 ASCII A–Z，不依赖 locale。
        constexpr auto ascii_lower(const char value) noexcept -> char {
            return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
        }
    } // namespace

    auto nan_named_color(const NanNamedColor named) noexcept -> NanColor {
        const auto index = static_cast<std::size_t>(named);
        if (index >= named_colors.size()) {
            // Defensive: only declared enumerators are well formed; never index out of bounds.
            return NanColor::from_hex(0x000000U);
        }
        return NanColor::from_hex(named_colors[index].rgb);
    }

    auto nan_named_color(const std::string_view name) -> std::optional<NanColor> {
        if (name.empty()) {
            return std::nullopt;
        }
        // Reject embedded NUL and non-ASCII bytes before any comparison.
        for (const char value: name) {
            const auto byte = static_cast<unsigned char>(value);
            if (byte == 0 || byte >= 0x80) {
                return std::nullopt;
            }
        }
        for (const auto& entry: named_colors) {
            if (entry.name.size() != name.size()) {
                continue;
            }
            bool matches = true;
            for (std::size_t index = 0; index < name.size(); ++index) {
                if (ascii_lower(name[index]) != entry.name[index]) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                return NanColor::from_hex(entry.rgb);
            }
        }
        return std::nullopt;
    }

    auto nan_named_color_name(const NanNamedColor named) noexcept -> std::string_view {
        const auto index = static_cast<std::size_t>(named);
        if (index >= named_colors.size()) {
            return "unknown";
        }
        return named_colors[index].name;
    }
} // namespace nandina::foundation
