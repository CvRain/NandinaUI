//
// foundation/utf8 — small UTF-8 boundary and encoding helpers.
//

#include "utf8.hpp"

#include <utf8proc.h>

#include <algorithm>

namespace nandina::foundation::utf8
{
    namespace
    {
        /// 获取指定偏移处的字节值
        [[nodiscard]] auto byte_at(std::string_view text, std::size_t offset) -> unsigned char;

        [[nodiscard]] auto byte_at(std::string_view text, std::size_t offset) -> unsigned char {
            return static_cast<unsigned char>(text[offset]);
        }

        /// 检查指定字节是否是 UTF-8 的续字节
        [[nodiscard]] auto is_continuation(unsigned char byte) -> bool {
            return (byte & 0xC0U) == 0x80U;
        }

        /// 获取指定偏移处的有效 UTF-8 字节序列长度
        [[nodiscard]] auto valid_sequence_length(std::string_view text, std::size_t offset)
            -> std::size_t {
            if (offset >= text.size()) {
                return 0;
            }

            const auto first = byte_at(text, offset);
            if (first <= 0x7FU) {
                return 1;
            }

            if (first >= 0xC2U && first <= 0xDFU) {
                const auto result =
                    offset + 1 < text.size() && is_continuation(byte_at(text, offset + 1));
                return result ? 2 : 1;
            }

            if (first >= 0xE0U && first <= 0xEFU && offset + 2 < text.size()) {
                const auto second = byte_at(text, offset + 1);
                const auto third = byte_at(text, offset + 2);
                const bool valid_second = first == 0xE0U ? second >= 0xA0U && second <= 0xBFU
                    : first == 0xEDU                     ? second >= 0x80U && second <= 0x9FU
                                                         : is_continuation(second);
                return valid_second && is_continuation(third) ? 3 : 1;
            }
            if (first >= 0xF0U && first <= 0xF4U && offset + 3 < text.size()) {
                const auto second = byte_at(text, offset + 1);
                const bool valid_second = first == 0xF0U ? second >= 0x90U && second <= 0xBFU
                    : first == 0xF4U                     ? second >= 0x80U && second <= 0x8FU
                                                         : is_continuation(second);
                return valid_second && is_continuation(byte_at(text, offset + 2))
                        && is_continuation(byte_at(text, offset + 3))
                    ? 4
                    : 1;
            }
            return 1;
        }
    } // namespace

    auto is_valid_codepoint(char32_t codepoint) -> bool {
        return codepoint <= 0x10FFFFU && !(codepoint >= 0xD800U && codepoint <= 0xDFFFU);
    }

    auto encode(char32_t codepoint) -> std::string {
        if (!is_valid_codepoint(codepoint)) {
            codepoint = replacement_character;
        }

        std::string result;
        if (codepoint <= 0x7FU) {
            result.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint <= 0x7FFU) {
            result.push_back(static_cast<char>(0xC0U | (codepoint >> 6U)));
            result.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        }
        else if (codepoint <= 0xFFFFU) {
            result.push_back(static_cast<char>(0xE0U | (codepoint >> 12U)));
            result.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
            result.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        }
        else {
            result.push_back(static_cast<char>(0xF0U | (codepoint >> 18U)));
            result.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
            result.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
            result.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        }
        return result;
    }

    auto next_boundary(const std::string_view text, std::size_t offset) -> std::size_t {
        offset = std::min(offset, text.size());
        return std::min(text.size(), offset + valid_sequence_length(text, offset));
    }

    auto previous_boundary(const std::string_view text, std::size_t offset) -> std::size_t {
        offset = std::min(offset, text.size());
        if (offset == 0) {
            return 0;
        }

        auto candidate = offset - 1;
        while (candidate > 0 && is_continuation(byte_at(text, candidate))) {
            --candidate;
        }
        return next_boundary(text, candidate) == offset ? candidate : offset - 1;
    }

    auto clamp_boundary(const std::string_view text, std::size_t offset) -> std::size_t {
        offset = std::min(offset, text.size());
        if (offset == text.size() || offset == 0 || !is_continuation(byte_at(text, offset))) {
            return offset;
        }
        while (offset > 0 && is_continuation(byte_at(text, offset))) {
            --offset;
        }
        return offset;
    }

    auto codepoint_count(const std::string_view text) -> std::size_t {
        std::size_t count = 0;
        for (std::size_t offset = 0; offset < text.size(); offset = next_boundary(text, offset)) {
            ++count;
        }
        return count;
    }

    auto byte_offset_for_codepoints(const std::string_view text, std::size_t count) -> std::size_t {
        std::size_t offset = 0;
        while (count > 0 && offset < text.size()) {
            offset = next_boundary(text, offset);
            --count;
        }
        return offset;
    }

    auto decode(std::string_view text) -> std::vector<DecodedCodepoint> {
        std::vector<DecodedCodepoint> result;
        result.reserve(codepoint_count(text));

        std::size_t offset = 0;
        while (offset < text.size()) {
            const auto length = valid_sequence_length(text, offset);
            const auto first = byte_at(text, offset);
            char32_t value = replacement_character;
            if (length == 1 && first <= 0x7FU) {
                value = first;
            }
            else if (length == 2) {
                value = static_cast<char32_t>(first & 0x1FU) << 6U;
                value |= static_cast<char32_t>(byte_at(text, offset + 1) & 0x3FU);
            }
            else if (length == 3) {
                value = static_cast<char32_t>(first & 0x0FU) << 12U;
                value |= static_cast<char32_t>(byte_at(text, offset + 1) & 0x3FU) << 6U;
                value |= static_cast<char32_t>(byte_at(text, offset + 2) & 0x3FU);
            }
            else if (length == 4) {
                value = static_cast<char32_t>(first & 0x07U) << 18U;
                value |= static_cast<char32_t>(byte_at(text, offset + 1) & 0x3FU) << 12U;
                value |= static_cast<char32_t>(byte_at(text, offset + 2) & 0x3FU) << 6U;
                value |= static_cast<char32_t>(byte_at(text, offset + 3) & 0x3FU);
            }

            result.push_back(
                DecodedCodepoint {
                    .value = value,
                    .byte_offset = offset,
                    .byte_length = length,
                }
            );
            offset += length;
        }
        return result;
    }

    auto grapheme_ranges(std::string_view text) -> std::vector<ByteRange> {
        const auto decoded = decode(text);
        if (decoded.empty()) {
            return {};
        }

        std::vector<ByteRange> result;
        result.reserve(decoded.size());
        std::size_t cluster_offset = decoded.front().byte_offset;
        utf8proc_int32_t state = 0;
        for (std::size_t index = 1; index < decoded.size(); ++index) {
            const auto previous = static_cast<utf8proc_int32_t>(decoded[index - 1].value);
            const auto current = static_cast<utf8proc_int32_t>(decoded[index].value);
            if (utf8proc_grapheme_break_stateful(previous, current, &state)) {
                result.push_back(
                    ByteRange {
                        .offset = cluster_offset,
                        .length = decoded[index].byte_offset - cluster_offset,
                    }
                );
                cluster_offset = decoded[index].byte_offset;
                state = 0;
            }
        }
        result.push_back(
            ByteRange {
                .offset = cluster_offset,
                .length = text.size() - cluster_offset,
            }
        );
        return result;
    }

} // namespace nandina::foundation::utf8
