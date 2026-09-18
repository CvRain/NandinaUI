//
// widget/key_codes.hpp — 键盘键码常量（单一定义处）。
//
// 这些值是 GLFW 的键码（raylib 直接透传）。此前 editable_text / slider / select /
// tabs / radio_button / chip / text_field 各自抄了一份局部常量，既重复又容易出现
// 不一致（例如 home/end/page 各写各的）。新增导航与 typeahead 需要更多键码，统一收
// 到这里。
//
// 只放**跨组件通用**的键码；某个控件专属的编辑键（如撤销/重做）留在自己文件里。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_KEY_CODES_HPP
#define NANDINA_EXPERIMENT_WIDGET_KEY_CODES_HPP

namespace nandina::widget::keys
{
    // ─── 编辑 / 提交 ──────────────────────────────────────────────────────
    inline constexpr int space = 32;
    inline constexpr int apostrophe = 39;
    inline constexpr int comma = 44;
    inline constexpr int minus = 45;
    inline constexpr int period = 46;
    inline constexpr int slash = 47;
    inline constexpr int num_0 = 48;
    inline constexpr int num_9 = 57;
    inline constexpr int semicolon = 59;
    inline constexpr int equal = 61;
    inline constexpr int a = 65;
    inline constexpr int c = 67;
    inline constexpr int v = 86;
    inline constexpr int x = 88;
    inline constexpr int y = 89;
    inline constexpr int z = 90;
    inline constexpr int left_bracket = 91;
    inline constexpr int backslash = 92;
    inline constexpr int right_bracket = 93;
    inline constexpr int grave_accent = 96;

    inline constexpr int escape = 256;
    inline constexpr int enter = 257;
    inline constexpr int tab = 258;
    inline constexpr int backspace = 259;
    inline constexpr int insert = 260;
    inline constexpr int delete_key = 261;

    // ─── 导航 ────────────────────────────────────────────────────────────
    inline constexpr int right = 262;
    inline constexpr int left = 263;
    inline constexpr int down = 264;
    inline constexpr int up = 265;
    inline constexpr int page_up = 266;
    inline constexpr int page_down = 267;
    inline constexpr int home = 268;
    inline constexpr int end = 269;
} // namespace nandina::widget::keys

#endif // NANDINA_EXPERIMENT_WIDGET_KEY_CODES_HPP
