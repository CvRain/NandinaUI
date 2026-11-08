// Foundation CSS Color 4 named color literals (D2 step 4c).
#include <nandina/foundation/named_color.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>

namespace foundation = nandina::foundation;

namespace
{
    struct ExpectedNamedColor {
        foundation::NanNamedColor named;
        std::string_view name;
        std::uint32_t rgb;
    };

    /// 独立固定的期望表（CSS Color 4 §6.1 快照值）；不读生产表。
    constexpr std::array<ExpectedNamedColor, 148> expected_named_colors {{
        {foundation::NanNamedColor::aliceblue, "aliceblue", 0xF0F8FFU},
        {foundation::NanNamedColor::antiquewhite, "antiquewhite", 0xFAEBD7U},
        {foundation::NanNamedColor::aqua, "aqua", 0x00FFFFU},
        {foundation::NanNamedColor::aquamarine, "aquamarine", 0x7FFFD4U},
        {foundation::NanNamedColor::azure, "azure", 0xF0FFFFU},
        {foundation::NanNamedColor::beige, "beige", 0xF5F5DCU},
        {foundation::NanNamedColor::bisque, "bisque", 0xFFE4C4U},
        {foundation::NanNamedColor::black, "black", 0x000000U},
        {foundation::NanNamedColor::blanchedalmond, "blanchedalmond", 0xFFEBCDU},
        {foundation::NanNamedColor::blue, "blue", 0x0000FFU},
        {foundation::NanNamedColor::blueviolet, "blueviolet", 0x8A2BE2U},
        {foundation::NanNamedColor::brown, "brown", 0xA52A2AU},
        {foundation::NanNamedColor::burlywood, "burlywood", 0xDEB887U},
        {foundation::NanNamedColor::cadetblue, "cadetblue", 0x5F9EA0U},
        {foundation::NanNamedColor::chartreuse, "chartreuse", 0x7FFF00U},
        {foundation::NanNamedColor::chocolate, "chocolate", 0xD2691EU},
        {foundation::NanNamedColor::coral, "coral", 0xFF7F50U},
        {foundation::NanNamedColor::cornflowerblue, "cornflowerblue", 0x6495EDU},
        {foundation::NanNamedColor::cornsilk, "cornsilk", 0xFFF8DCU},
        {foundation::NanNamedColor::crimson, "crimson", 0xDC143CU},
        {foundation::NanNamedColor::cyan, "cyan", 0x00FFFFU},
        {foundation::NanNamedColor::darkblue, "darkblue", 0x00008BU},
        {foundation::NanNamedColor::darkcyan, "darkcyan", 0x008B8BU},
        {foundation::NanNamedColor::darkgoldenrod, "darkgoldenrod", 0xB8860BU},
        {foundation::NanNamedColor::darkgray, "darkgray", 0xA9A9A9U},
        {foundation::NanNamedColor::darkgreen, "darkgreen", 0x006400U},
        {foundation::NanNamedColor::darkgrey, "darkgrey", 0xA9A9A9U},
        {foundation::NanNamedColor::darkkhaki, "darkkhaki", 0xBDB76BU},
        {foundation::NanNamedColor::darkmagenta, "darkmagenta", 0x8B008BU},
        {foundation::NanNamedColor::darkolivegreen, "darkolivegreen", 0x556B2FU},
        {foundation::NanNamedColor::darkorange, "darkorange", 0xFF8C00U},
        {foundation::NanNamedColor::darkorchid, "darkorchid", 0x9932CCU},
        {foundation::NanNamedColor::darkred, "darkred", 0x8B0000U},
        {foundation::NanNamedColor::darksalmon, "darksalmon", 0xE9967AU},
        {foundation::NanNamedColor::darkseagreen, "darkseagreen", 0x8FBC8FU},
        {foundation::NanNamedColor::darkslateblue, "darkslateblue", 0x483D8BU},
        {foundation::NanNamedColor::darkslategray, "darkslategray", 0x2F4F4FU},
        {foundation::NanNamedColor::darkslategrey, "darkslategrey", 0x2F4F4FU},
        {foundation::NanNamedColor::darkturquoise, "darkturquoise", 0x00CED1U},
        {foundation::NanNamedColor::darkviolet, "darkviolet", 0x9400D3U},
        {foundation::NanNamedColor::deeppink, "deeppink", 0xFF1493U},
        {foundation::NanNamedColor::deepskyblue, "deepskyblue", 0x00BFFFU},
        {foundation::NanNamedColor::dimgray, "dimgray", 0x696969U},
        {foundation::NanNamedColor::dimgrey, "dimgrey", 0x696969U},
        {foundation::NanNamedColor::dodgerblue, "dodgerblue", 0x1E90FFU},
        {foundation::NanNamedColor::firebrick, "firebrick", 0xB22222U},
        {foundation::NanNamedColor::floralwhite, "floralwhite", 0xFFFAF0U},
        {foundation::NanNamedColor::forestgreen, "forestgreen", 0x228B22U},
        {foundation::NanNamedColor::fuchsia, "fuchsia", 0xFF00FFU},
        {foundation::NanNamedColor::gainsboro, "gainsboro", 0xDCDCDCU},
        {foundation::NanNamedColor::ghostwhite, "ghostwhite", 0xF8F8FFU},
        {foundation::NanNamedColor::gold, "gold", 0xFFD700U},
        {foundation::NanNamedColor::goldenrod, "goldenrod", 0xDAA520U},
        {foundation::NanNamedColor::gray, "gray", 0x808080U},
        {foundation::NanNamedColor::green, "green", 0x008000U},
        {foundation::NanNamedColor::greenyellow, "greenyellow", 0xADFF2FU},
        {foundation::NanNamedColor::grey, "grey", 0x808080U},
        {foundation::NanNamedColor::honeydew, "honeydew", 0xF0FFF0U},
        {foundation::NanNamedColor::hotpink, "hotpink", 0xFF69B4U},
        {foundation::NanNamedColor::indianred, "indianred", 0xCD5C5CU},
        {foundation::NanNamedColor::indigo, "indigo", 0x4B0082U},
        {foundation::NanNamedColor::ivory, "ivory", 0xFFFFF0U},
        {foundation::NanNamedColor::khaki, "khaki", 0xF0E68CU},
        {foundation::NanNamedColor::lavender, "lavender", 0xE6E6FAU},
        {foundation::NanNamedColor::lavenderblush, "lavenderblush", 0xFFF0F5U},
        {foundation::NanNamedColor::lawngreen, "lawngreen", 0x7CFC00U},
        {foundation::NanNamedColor::lemonchiffon, "lemonchiffon", 0xFFFACDU},
        {foundation::NanNamedColor::lightblue, "lightblue", 0xADD8E6U},
        {foundation::NanNamedColor::lightcoral, "lightcoral", 0xF08080U},
        {foundation::NanNamedColor::lightcyan, "lightcyan", 0xE0FFFFU},
        {foundation::NanNamedColor::lightgoldenrodyellow, "lightgoldenrodyellow", 0xFAFAD2U},
        {foundation::NanNamedColor::lightgray, "lightgray", 0xD3D3D3U},
        {foundation::NanNamedColor::lightgreen, "lightgreen", 0x90EE90U},
        {foundation::NanNamedColor::lightgrey, "lightgrey", 0xD3D3D3U},
        {foundation::NanNamedColor::lightpink, "lightpink", 0xFFB6C1U},
        {foundation::NanNamedColor::lightsalmon, "lightsalmon", 0xFFA07AU},
        {foundation::NanNamedColor::lightseagreen, "lightseagreen", 0x20B2AAU},
        {foundation::NanNamedColor::lightskyblue, "lightskyblue", 0x87CEFAU},
        {foundation::NanNamedColor::lightslategray, "lightslategray", 0x778899U},
        {foundation::NanNamedColor::lightslategrey, "lightslategrey", 0x778899U},
        {foundation::NanNamedColor::lightsteelblue, "lightsteelblue", 0xB0C4DEU},
        {foundation::NanNamedColor::lightyellow, "lightyellow", 0xFFFFE0U},
        {foundation::NanNamedColor::lime, "lime", 0x00FF00U},
        {foundation::NanNamedColor::limegreen, "limegreen", 0x32CD32U},
        {foundation::NanNamedColor::linen, "linen", 0xFAF0E6U},
        {foundation::NanNamedColor::magenta, "magenta", 0xFF00FFU},
        {foundation::NanNamedColor::maroon, "maroon", 0x800000U},
        {foundation::NanNamedColor::mediumaquamarine, "mediumaquamarine", 0x66CDAAU},
        {foundation::NanNamedColor::mediumblue, "mediumblue", 0x0000CDU},
        {foundation::NanNamedColor::mediumorchid, "mediumorchid", 0xBA55D3U},
        {foundation::NanNamedColor::mediumpurple, "mediumpurple", 0x9370DBU},
        {foundation::NanNamedColor::mediumseagreen, "mediumseagreen", 0x3CB371U},
        {foundation::NanNamedColor::mediumslateblue, "mediumslateblue", 0x7B68EEU},
        {foundation::NanNamedColor::mediumspringgreen, "mediumspringgreen", 0x00FA9AU},
        {foundation::NanNamedColor::mediumturquoise, "mediumturquoise", 0x48D1CCU},
        {foundation::NanNamedColor::mediumvioletred, "mediumvioletred", 0xC71585U},
        {foundation::NanNamedColor::midnightblue, "midnightblue", 0x191970U},
        {foundation::NanNamedColor::mintcream, "mintcream", 0xF5FFFAU},
        {foundation::NanNamedColor::mistyrose, "mistyrose", 0xFFE4E1U},
        {foundation::NanNamedColor::moccasin, "moccasin", 0xFFE4B5U},
        {foundation::NanNamedColor::navajowhite, "navajowhite", 0xFFDEADU},
        {foundation::NanNamedColor::navy, "navy", 0x000080U},
        {foundation::NanNamedColor::oldlace, "oldlace", 0xFDF5E6U},
        {foundation::NanNamedColor::olive, "olive", 0x808000U},
        {foundation::NanNamedColor::olivedrab, "olivedrab", 0x6B8E23U},
        {foundation::NanNamedColor::orange, "orange", 0xFFA500U},
        {foundation::NanNamedColor::orangered, "orangered", 0xFF4500U},
        {foundation::NanNamedColor::orchid, "orchid", 0xDA70D6U},
        {foundation::NanNamedColor::palegoldenrod, "palegoldenrod", 0xEEE8AAU},
        {foundation::NanNamedColor::palegreen, "palegreen", 0x98FB98U},
        {foundation::NanNamedColor::paleturquoise, "paleturquoise", 0xAFEEEEU},
        {foundation::NanNamedColor::palevioletred, "palevioletred", 0xDB7093U},
        {foundation::NanNamedColor::papayawhip, "papayawhip", 0xFFEFD5U},
        {foundation::NanNamedColor::peachpuff, "peachpuff", 0xFFDAB9U},
        {foundation::NanNamedColor::peru, "peru", 0xCD853FU},
        {foundation::NanNamedColor::pink, "pink", 0xFFC0CBU},
        {foundation::NanNamedColor::plum, "plum", 0xDDA0DDU},
        {foundation::NanNamedColor::powderblue, "powderblue", 0xB0E0E6U},
        {foundation::NanNamedColor::purple, "purple", 0x800080U},
        {foundation::NanNamedColor::rebeccapurple, "rebeccapurple", 0x663399U},
        {foundation::NanNamedColor::red, "red", 0xFF0000U},
        {foundation::NanNamedColor::rosybrown, "rosybrown", 0xBC8F8FU},
        {foundation::NanNamedColor::royalblue, "royalblue", 0x4169E1U},
        {foundation::NanNamedColor::saddlebrown, "saddlebrown", 0x8B4513U},
        {foundation::NanNamedColor::salmon, "salmon", 0xFA8072U},
        {foundation::NanNamedColor::sandybrown, "sandybrown", 0xF4A460U},
        {foundation::NanNamedColor::seagreen, "seagreen", 0x2E8B57U},
        {foundation::NanNamedColor::seashell, "seashell", 0xFFF5EEU},
        {foundation::NanNamedColor::sienna, "sienna", 0xA0522DU},
        {foundation::NanNamedColor::silver, "silver", 0xC0C0C0U},
        {foundation::NanNamedColor::skyblue, "skyblue", 0x87CEEBU},
        {foundation::NanNamedColor::slateblue, "slateblue", 0x6A5ACDU},
        {foundation::NanNamedColor::slategray, "slategray", 0x708090U},
        {foundation::NanNamedColor::slategrey, "slategrey", 0x708090U},
        {foundation::NanNamedColor::snow, "snow", 0xFFFAFAU},
        {foundation::NanNamedColor::springgreen, "springgreen", 0x00FF7FU},
        {foundation::NanNamedColor::steelblue, "steelblue", 0x4682B4U},
        {foundation::NanNamedColor::tan, "tan", 0xD2B48CU},
        {foundation::NanNamedColor::teal, "teal", 0x008080U},
        {foundation::NanNamedColor::thistle, "thistle", 0xD8BFD8U},
        {foundation::NanNamedColor::tomato, "tomato", 0xFF6347U},
        {foundation::NanNamedColor::turquoise, "turquoise", 0x40E0D0U},
        {foundation::NanNamedColor::violet, "violet", 0xEE82EEU},
        {foundation::NanNamedColor::wheat, "wheat", 0xF5DEB3U},
        {foundation::NanNamedColor::white, "white", 0xFFFFFFU},
        {foundation::NanNamedColor::whitesmoke, "whitesmoke", 0xF5F5F5U},
        {foundation::NanNamedColor::yellow, "yellow", 0xFFFF00U},
        {foundation::NanNamedColor::yellowgreen, "yellowgreen", 0x9ACD32U},
    }};

} // namespace

TEST_CASE("every named color resolves to its pinned sRGB value", "[foundation][named-color]") {
    CHECK(expected_named_colors.size() == 148);
    for (const auto& expected: expected_named_colors) {
        CAPTURE(expected.name);
        const auto resolved = foundation::nan_named_color(expected.name);
        REQUIRE(resolved.has_value());
        CHECK(*resolved == foundation::NanColor::from_hex(expected.rgb));
        const auto encoded = resolved->to<foundation::NanHexRgb>();
        CHECK(static_cast<unsigned>(encoded.red) == ((expected.rgb >> 16) & 0xFF));
        CHECK(static_cast<unsigned>(encoded.green) == ((expected.rgb >> 8) & 0xFF));
        CHECK(static_cast<unsigned>(encoded.blue) == (expected.rgb & 0xFF));
        CHECK(static_cast<unsigned>(encoded.alpha) == 255);
    }
    // Spot checks for values that are easy to confuse.
    CHECK(*foundation::nan_named_color("green") == foundation::NanColor::from_hex(0x008000U));
    CHECK(*foundation::nan_named_color("lime") == foundation::NanColor::from_hex(0x00FF00U));
    CHECK(
        *foundation::nan_named_color("rebeccapurple") == foundation::NanColor::from_hex(0x663399U)
    );
}

TEST_CASE("typed and string entries share one table", "[foundation][named-color]") {
    CHECK(static_cast<std::size_t>(foundation::NanNamedColor::yellowgreen) + 1 == 148);
    std::set<std::string_view> names;
    std::set<foundation::NanNamedColor> identities;
    std::string_view previous;
    for (const auto& expected: expected_named_colors) {
        CAPTURE(expected.name);
        const auto name = foundation::nan_named_color_name(expected.named);
        CHECK(name == expected.name);
        CHECK(
            foundation::nan_named_color(expected.named)
            == foundation::NanColor::from_hex(expected.rgb)
        );
        CHECK(foundation::nan_named_color(name) == foundation::nan_named_color(expected.named));
        CHECK(names.insert(name).second);
        CHECK(identities.insert(expected.named).second);
        if (!previous.empty()) {
            CHECK(previous < name);
        }
        previous = name;
    }
    CHECK(names.size() == 148);
    CHECK(identities.size() == 148);
}

TEST_CASE("named color lookup is ASCII case insensitive", "[foundation][named-color]") {
    for (const auto& expected: expected_named_colors) {
        CAPTURE(expected.name);
        std::string upper(expected.name);
        std::string alternating(expected.name);
        for (std::size_t index = 0; index < expected.name.size(); ++index) {
            upper[index] = static_cast<char>(expected.name[index] - 'a' + 'A');
            if (index % 2 == 0) {
                alternating[index] = upper[index];
            }
        }
        CHECK(foundation::nan_named_color(upper) == foundation::NanColor::from_hex(expected.rgb));
        CHECK(
            foundation::nan_named_color(alternating) == foundation::NanColor::from_hex(expected.rgb)
        );
    }
    // Non-ASCII bytes are not folded (no locale-dependent case mapping).
    CHECK_FALSE(foundation::nan_named_color("steelbl\u00fce"));
    CHECK_FALSE(
        foundation::nan_named_color("\uFF33\uFF54\uFF45\uFF45\uFF4C\uFF42\uFF4C\uFF55\uFF45")
    );
}

TEST_CASE("named color lookup rejects malformed input", "[foundation][named-color]") {
    for (const std::string_view input:
         {"", " ", "steelblue ", " steelblue", "steel blue", "steel\tblue"})
    {
        CAPTURE(input);
        CHECK_FALSE(foundation::nan_named_color(input));
    }
    // Explicit length with an embedded NUL is rejected even though a prefix matches.
    const std::string embedded_nul("steelblue\0", 10);
    CHECK_FALSE(foundation::nan_named_color(std::string_view(embedded_nul)));
    // A C-string literal truncates at the NUL; use an explicit length to keep it embedded.
    CHECK_FALSE(foundation::nan_named_color(std::string_view("steelblue\0extra", 15)));
    // Contextual keywords and CSS syntax are out of scope.
    CHECK_FALSE(foundation::nan_named_color("transparent"));
    CHECK_FALSE(foundation::nan_named_color("currentColor"));
    CHECK_FALSE(foundation::nan_named_color("#4682B4"));
    CHECK_FALSE(foundation::nan_named_color("rgb(70,130,180)"));
    CHECK_FALSE(foundation::nan_named_color("notacolor"));
}

TEST_CASE("same-value aliases keep distinct identities", "[foundation][named-color]") {
    struct AliasPair {
        foundation::NanNamedColor first;
        foundation::NanNamedColor second;
    };
    const std::array pairs {
        AliasPair {foundation::NanNamedColor::aqua, foundation::NanNamedColor::cyan},
        AliasPair {foundation::NanNamedColor::fuchsia, foundation::NanNamedColor::magenta},
        AliasPair {foundation::NanNamedColor::gray, foundation::NanNamedColor::grey},
        AliasPair {foundation::NanNamedColor::darkgray, foundation::NanNamedColor::darkgrey},
        AliasPair {foundation::NanNamedColor::dimgray, foundation::NanNamedColor::dimgrey},
        AliasPair {foundation::NanNamedColor::lightgray, foundation::NanNamedColor::lightgrey},
        AliasPair {foundation::NanNamedColor::slategray, foundation::NanNamedColor::slategrey},
        AliasPair {
            foundation::NanNamedColor::lightslategray,
            foundation::NanNamedColor::lightslategrey
        },
        AliasPair {
            foundation::NanNamedColor::darkslategray,
            foundation::NanNamedColor::darkslategrey
        },
    };
    for (const auto& pair: pairs) {
        CAPTURE(pair.first, pair.second);
        const auto first_name = foundation::nan_named_color_name(pair.first);
        const auto second_name = foundation::nan_named_color_name(pair.second);
        REQUIRE(foundation::nan_named_color(first_name).has_value());
        REQUIRE(foundation::nan_named_color(second_name).has_value());
        // Equal RGB, but both names exist as separate entries.
        CHECK(foundation::nan_named_color(pair.first) == foundation::nan_named_color(pair.second));
        CHECK(pair.first != pair.second);
        CHECK(first_name != second_name);
    }
}

TEST_CASE(
    "named color lookup respects string view bounds and every surrounding byte",
    "[foundation][named-color]"
) {
    const std::string storage = "prefixSteelBluesuffix";
    const auto bounded = std::string_view(storage).substr(6, 9);
    REQUIRE(bounded == "SteelBlue");
    CHECK(foundation::nan_named_color(bounded) == foundation::NanColor::from_hex(0x4682B4U));
    const auto name = std::string_view("steelblue");
    for (unsigned byte = 0; byte < 256; ++byte) {
        CAPTURE(byte);
        const auto extra = static_cast<char>(byte);
        const auto prefix = std::string(1, extra) + std::string(name);
        const auto suffix = std::string(name) + extra;
        const auto internal = std::string("steel") + extra + "blue";
        CHECK_FALSE(foundation::nan_named_color(std::string_view(prefix)));
        CHECK_FALSE(foundation::nan_named_color(std::string_view(suffix)));
        CHECK_FALSE(foundation::nan_named_color(std::string_view(internal)));
    }
    const std::string long_name(4096, 'a');
    CHECK_FALSE(foundation::nan_named_color(long_name));
    for (const auto keyword:
         {"TRANSPARENT",
          "CURRENTCOLOR",
          "inherit",
          "initial",
          "unset",
          "none",
          "color(srgb 1 0 0)"})
    {
        CHECK_FALSE(foundation::nan_named_color(keyword));
    }
}
