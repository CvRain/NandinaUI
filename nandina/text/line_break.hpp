//
// text/line_break — 断行机会判定（UAX #14 的实用子集）。
//

#ifndef NANDINA_EXPERIMENT_TEXT_LINE_BREAK_HPP
#define NANDINA_EXPERIMENT_TEXT_LINE_BREAK_HPP

#include <cstddef>
#include <string_view>

namespace nandina::text
{

    /**
     * 该 grapheme cluster **之后**是否允许断行。
     *
     * 这是 UAX #14 的实用子集，不是完整实现。完整实现需要断行属性表（每个码点的
     * Line_Break 属性 + 成对规则），本轮不做；这里覆盖三类最常见的场合：
     *
     *   1. 空格 / 制表符之后 —— 拉丁文本的主要断点；
     *   2. 连字符、短破折号、长破折号之后 —— `well-known`、`state-of-the-art`；
     *   3. 宽字符（CJK 表意文字、假名、谚文、全角标点等）之后 —— 这些书写系统本来
     *      就允许在任意字符之间断行，所以每个字符都是断点。
     *
     * 找不到断点时会退回「按簇断行」：像超长 URL 或不可分割的长单词，必须能在词中间
     * 断开，否则会溢出盒子（见 docs/references/text_shaping_cost.md §8）。
     *
     * @param cluster_utf8 单个 grapheme cluster 的 UTF-8 字节
     */
    [[nodiscard]] auto break_opportunity_after(std::string_view cluster_utf8) -> bool;

    /**
     * 在 `[begin, end)` 内寻找**最靠后**的断行机会，返回断点字节偏移（即断在该机会之后）。
     * 找不到时返回 `begin`（调用方据此保留原来的切分点）。
     *
     * `begin`/`end` 必须落在 UTF-8 边界上。
     */
    [[nodiscard]] auto
    last_break_opportunity(std::string_view text, std::size_t begin, std::size_t end)
        -> std::size_t;

    /// 取文本首个码点（不分配；非法序列按单字节处理，空串返回 0）。
    [[nodiscard]] auto first_codepoint(std::string_view text) -> char32_t;

    /// 该码点是否属于「可以在其后断行」的宽字符集合。
    [[nodiscard]] auto is_wide_breakable(char32_t codepoint) -> bool;

} // namespace nandina::text

#endif // NANDINA_EXPERIMENT_TEXT_LINE_BREAK_HPP
