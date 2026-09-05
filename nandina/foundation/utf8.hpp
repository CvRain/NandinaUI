//
// foundation/utf8 — small UTF-8 boundary and encoding helpers.
//

#ifndef NANDINA_EXPERIMENT_FOUNDATION_UTF8_HPP
#define NANDINA_EXPERIMENT_FOUNDATION_UTF8_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::foundation::utf8
{

    inline constexpr char32_t replacement_character = U'\uFFFD';

    struct DecodedCodepoint {
        char32_t value = replacement_character;
        std::size_t byte_offset = 0;
        std::size_t byte_length = 0;
    };

    struct ByteRange {
        std::size_t offset = 0;
        std::size_t length = 0;
    };

    /**
     * 检查传入的 codepoint 是否是有效的 UTF-8 codepoint。
     */
    [[nodiscard]] auto is_valid_codepoint(char32_t codepoint) -> bool;

    /**
     * 将传入的 codepoint 编码为 UTF-8 字节序列。
     *
     * @param codepoint 待编码的 codepoint
     * @return 编码后的 UTF-8 字节序列
     */
    [[nodiscard]] auto encode(char32_t codepoint) -> std::string;

    /**
     * 获取下一个 UTF-8 边界。
     *
     * @param text 文本视图
     * @param offset 偏移量
     * @return 下一个边界的位置
     */
    [[nodiscard]] auto next_boundary(const std::string_view text, std::size_t offset)
        -> std::size_t;

    /**
     * 获取上一个 UTF-8 边界。
     *
     * @param text 文本视图
     * @param offset 偏移量
     * @return 上一个边界的位置
     */
    [[nodiscard]] auto previous_boundary(const std::string_view text, std::size_t offset)
        -> std::size_t;

    /**
     * 限制偏移量在有效的 UTF-8 边界内。
     *
     * @param text 文本视图
     * @param offset 偏移量
     * @return 限制后的边界位置
     */
    [[nodiscard]] auto clamp_boundary(const std::string_view text, std::size_t offset)
        -> std::size_t;

    /**
     * 计算文本视图中的 UTF-8 codepoint 数量。
     *
     * @param text 文本视图
     * @return codepoint 数量
     */
    [[nodiscard]] auto codepoint_count(const std::string_view text) -> std::size_t;

    /**
     * 获取指定数量的 UTF-8 codepoint 对应的字节偏移量。
     *
     * @param text 文本视图
     * @param count codepoint 数量
     * @return 对应的字节偏移量
     */
    [[nodiscard]] auto byte_offset_for_codepoints(const std::string_view text, std::size_t count)
        -> std::size_t;

    /** 解码文本并保留每个 codepoint 对应的源字节范围。 */
    [[nodiscard]] auto decode(std::string_view text) -> std::vector<DecodedCodepoint>;

    /** 按 UAX #29 扩展 grapheme cluster 返回源 UTF-8 字节范围。 */
    [[nodiscard]] auto grapheme_ranges(std::string_view text) -> std::vector<ByteRange>;

} // namespace nandina::foundation::utf8

#endif // NANDINA_EXPERIMENT_FOUNDATION_UTF8_HPP
