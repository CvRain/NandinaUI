//
// text/line_break — 断行机会判定。
//

#include "line_break.hpp"

#include "../foundation/utf8.hpp"

#include <algorithm>

namespace nandina::text
{
    namespace
    {
        [[nodiscard]] auto is_space_byte(char byte) -> bool {
            return byte == ' ' || byte == '\t' || byte == '\v' || byte == '\f';
        }

        /// 连字符与破折号：U+002D、U+2010..U+2014、U+2212。
        [[nodiscard]] auto is_hyphen(char32_t codepoint) -> bool {
            return codepoint == U'-' || (codepoint >= 0x2010 && codepoint <= 0x2015)
                || codepoint == 0x2212;
        }

        /// 取最后一个码点（用于「连字符之后」判定）。
        [[nodiscard]] auto last_codepoint(std::string_view text) -> char32_t {
            if (text.empty()) {
                return 0;
            }
            // 从末尾回退到最后一个非续字节，那就是最后一个码点的起点。
            auto start = text.size() - 1;
            while (start > 0 && (static_cast<unsigned char>(text[start]) & 0xC0U) == 0x80U) {
                --start;
            }
            return first_codepoint(text.substr(start));
        }
    } // namespace

    auto first_codepoint(std::string_view text) -> char32_t {
        if (text.empty()) {
            return 0;
        }
        const auto continuation = [&text](std::size_t index) -> char32_t {
            return index < text.size() ? (static_cast<unsigned char>(text[index]) & 0x3FU) : 0U;
        };
        const auto lead = static_cast<unsigned char>(text[0]);
        if (lead < 0x80U) {
            return lead;
        }
        if ((lead & 0xE0U) == 0xC0U) {
            return ((lead & 0x1FU) << 6U) | continuation(1);
        }
        if ((lead & 0xF0U) == 0xE0U) {
            return ((lead & 0x0FU) << 12U) | (continuation(1) << 6U) | continuation(2);
        }
        if ((lead & 0xF8U) == 0xF0U) {
            return ((lead & 0x07U) << 18U) | (continuation(1) << 12U) | (continuation(2) << 6U)
                | continuation(3);
        }
        // 非法引导字节：按单字节处理，保证调用方总能前进。
        return lead;
    }

    auto is_wide_breakable(char32_t codepoint) -> bool {
        return
            // 谚文字母
            (codepoint >= 0x1100 && codepoint <= 0x11FF)
            // CJK 部首扩展、康熙部首、CJK 符号与标点、平假名、片假名、注音、谚文兼容字母
            || (codepoint >= 0x2E80 && codepoint <= 0x33FF)
            // CJK 扩展 A + 基本区
            || (codepoint >= 0x3400 && codepoint <= 0x9FFF)
            // 彝文
            || (codepoint >= 0xA000 && codepoint <= 0xA4CF)
            // 谚文音节
            || (codepoint >= 0xAC00 && codepoint <= 0xD7A3)
            // CJK 兼容表意文字
            || (codepoint >= 0xF900 && codepoint <= 0xFAFF)
            // CJK 兼容形式
            || (codepoint >= 0xFE30 && codepoint <= 0xFE4F)
            // 全角形式与全角符号
            || (codepoint >= 0xFF00 && codepoint <= 0xFF60)
            || (codepoint >= 0xFFE0 && codepoint <= 0xFFE6)
            // CJK 扩展 B 及以上
            || (codepoint >= 0x20000 && codepoint <= 0x3FFFF);
    }

    auto break_opportunity_after(std::string_view cluster_utf8) -> bool {
        if (cluster_utf8.empty()) {
            return false;
        }
        for (const auto byte: cluster_utf8) {
            if (is_space_byte(byte)) {
                return true;
            }
        }
        if (is_hyphen(last_codepoint(cluster_utf8))) {
            return true;
        }
        return is_wide_breakable(first_codepoint(cluster_utf8));
    }

    auto last_break_opportunity(std::string_view text, std::size_t begin, std::size_t end)
        -> std::size_t {
        // 宽容处理越界：夹到文本范围内；退化范围直接返回 begin（调用方保留原切分点）。
        end = std::min(end, text.size());
        if (begin >= end) {
            return begin;
        }

        auto result = begin;
        auto cursor = begin;
        while (cursor < end) {
            const auto next = foundation::utf8::next_grapheme_boundary(text, cursor);
            if (next > end) {
                break;
            }
            if (break_opportunity_after(text.substr(cursor, next - cursor))) {
                result = next;
            }
            cursor = next;
        }
        return result;
    }
} // namespace nandina::text
