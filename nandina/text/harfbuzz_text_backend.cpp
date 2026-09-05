//
// text/harfbuzz_text_backend — HarfBuzz shaping into TextLayoutResult glyph runs.
//

#include "harfbuzz_text_backend.hpp"

#include "../foundation/utf8.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <fribidi.h>
#include <hb-ft.h>
#include <hb.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nandina::text
{
    namespace
    {
        [[nodiscard]] auto pixels(hb_position_t value) -> float {
            return static_cast<float>(value) / 64.0F;
        }
    } // namespace

    class HarfBuzzTextLayoutBackend::Impl {
    public:
        struct BidiRun {
            std::size_t offset = 0;
            std::size_t length = 0;
            FriBidiLevel level = 0;

            [[nodiscard]] auto right_to_left() const -> bool {
                return (level & 1U) != 0;
            }
        };

        struct BidiRuns {
            std::vector<BidiRun> visual;
            bool paragraph_right_to_left = false;
        };

        struct BidiParagraph {
            std::vector<foundation::utf8::DecodedCodepoint> decoded;
            std::vector<FriBidiCharType> types;
            std::vector<FriBidiLevel> levels;
            FriBidiParType base_direction = FRIBIDI_PAR_LTR;
        };

        struct ClusterRange {
            std::size_t offset = 0;
            std::size_t length = 0;
            float width = 0.0F;
        };

        struct FontRange {
            std::size_t offset = 0;
            std::size_t length = 0;
            std::size_t font_index = 0;
            bool supported = true;
        };

        struct FontChoice {
            std::size_t index = 0;
            bool supported = false;
        };

        struct FontSlot {
            std::shared_ptr<FreeTypeFontFace> face;
            hb_font_t* font = nullptr;
        };

        Impl(
            std::shared_ptr<FreeTypeFontFace> primary,
            std::vector<std::shared_ptr<FreeTypeFontFace>> fallbacks
        ) {
            if (!primary) {
                throw std::invalid_argument("HarfBuzzTextLayoutBackend requires a font face");
            }
            add_font(std::move(primary));
            for (auto& fallback: fallbacks) {
                if (!fallback) {
                    throw std::invalid_argument("HarfBuzzTextLayoutBackend fallback face is null");
                }
                add_font(std::move(fallback));
            }
        }

        ~Impl() {
            for (const auto& slot: fonts) {
                hb_font_destroy(slot.font);
            }
        }

        void add_font(std::shared_ptr<FreeTypeFontFace> face) {
            auto* font =
                hb_ft_font_create_referenced(static_cast<FT_Face>(face->native_face_handle()));
            if (font == nullptr) {
                throw std::runtime_error("Failed to create HarfBuzz font from FreeType face");
            }
            fonts.push_back(FontSlot {.face = std::move(face), .font = font});
        }

        [[nodiscard]] auto combined_metrics(float pixel_size) const -> FontMetrics {
            auto result = fonts.front().face->metrics(pixel_size);
            for (std::size_t index = 1; index < fonts.size(); ++index) {
                const auto current = fonts[index].face->metrics(pixel_size);
                result.ascender = std::max(result.ascender, current.ascender);
                result.descender = std::min(result.descender, current.descender);
                result.line_height = std::max(result.line_height, current.line_height);
            }
            result.line_height = std::max(result.line_height, result.ascender - result.descender);
            return result;
        }

        [[nodiscard]] auto shape_line(
            std::string_view text,
            std::size_t source_offset,
            std::size_t source_length,
            float pixel_size,
            float line_height,
            float baseline,
            std::optional<bool> right_to_left = std::nullopt,
            std::size_t font_index = 0
        ) const -> widget::primitives::TextLayoutLine {
            if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                throw std::length_error("Text line is too large for HarfBuzz");
            }

            const auto& slot = fonts.at(font_index);
            (void)slot.face->metrics(pixel_size);
            hb_ft_font_changed(slot.font);

            const std::unique_ptr<hb_buffer_t, decltype(&hb_buffer_destroy)> buffer(
                hb_buffer_create(),
                &hb_buffer_destroy
            );
            hb_buffer_add_utf8(
                buffer.get(),
                text.data(),
                static_cast<int>(text.size()),
                0,
                static_cast<int>(text.size())
            );
            hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
            if (right_to_left.has_value()) {
                hb_buffer_set_direction(
                    buffer.get(),
                    *right_to_left ? HB_DIRECTION_RTL : HB_DIRECTION_LTR
                );
            }
            hb_buffer_guess_segment_properties(buffer.get());
            const auto direction = hb_buffer_get_direction(buffer.get());
            hb_shape(slot.font, buffer.get(), nullptr, 0);

            unsigned int glyph_count = 0;
            const auto* infos = hb_buffer_get_glyph_infos(buffer.get(), &glyph_count);
            const auto* positions = hb_buffer_get_glyph_positions(buffer.get(), &glyph_count);

            widget::primitives::TextLayoutLine line {
                .text_offset = source_offset,
                .text_length = source_length,
                .visible_text = std::string(text),
                .size = foundation::NanSize(0.0F, line_height),
                .baseline = baseline,
                .right_to_left = HB_DIRECTION_IS_BACKWARD(direction),
            };
            line.glyphs.reserve(glyph_count);

            float width = 0.0F;
            for (unsigned int index = 0; index < glyph_count; ++index) {
                const float advance = pixels(positions[index].x_advance);
                line.glyphs.push_back(
                    widget::primitives::TextLayoutLine::Glyph {
                        .glyph_index = infos[index].codepoint,
                        .font_index = font_index,
                        .cluster = source_offset
                            + std::min<std::size_t>(infos[index].cluster, source_length),
                        .x_advance = advance,
                        .y_advance = pixels(positions[index].y_advance),
                        .x_offset = pixels(positions[index].x_offset),
                        .y_offset = pixels(positions[index].y_offset),
                    }
                );
                width += advance;
            }
            line.missing_glyphs = std::ranges::any_of(line.glyphs, [](const auto& glyph) {
                return glyph.glyph_index == 0;
            });
            line.size = foundation::NanSize(std::abs(width), line_height);
            return line;
        }

        [[nodiscard]] auto analyze_bidi(
            std::string_view text,
            std::optional<bool> paragraph_right_to_left = std::nullopt
        ) const -> BidiParagraph {
            BidiParagraph result;
            result.decoded = foundation::utf8::decode(text);
            result.base_direction = paragraph_right_to_left.has_value()
                ? (*paragraph_right_to_left ? FRIBIDI_PAR_RTL : FRIBIDI_PAR_LTR)
                : FRIBIDI_PAR_ON;
            if (result.decoded.empty()) {
                return result;
            }
            if (result.decoded.size()
                > static_cast<std::size_t>(std::numeric_limits<FriBidiStrIndex>::max()))
            {
                throw std::length_error("Text line is too large for FriBidi");
            }

            std::vector<FriBidiChar> codepoints;
            codepoints.reserve(result.decoded.size());
            for (const auto& codepoint: result.decoded) {
                codepoints.push_back(static_cast<FriBidiChar>(codepoint.value));
            }

            result.types.resize(result.decoded.size());
            std::vector<FriBidiBracketType> brackets(result.decoded.size());
            result.levels.resize(result.decoded.size());
            const auto length = static_cast<FriBidiStrIndex>(result.decoded.size());
            fribidi_get_bidi_types(codepoints.data(), length, result.types.data());
            fribidi_get_bracket_types(
                codepoints.data(),
                length,
                result.types.data(),
                brackets.data()
            );
            if (fribidi_get_par_embedding_levels_ex(
                    result.types.data(),
                    brackets.data(),
                    length,
                    &result.base_direction,
                    result.levels.data()
                )
                == 0)
            {
                throw std::runtime_error("FriBidi failed to resolve paragraph embedding levels");
            }
            return result;
        }

        [[nodiscard]] auto bidi_runs(
            const BidiParagraph& paragraph,
            std::string_view text,
            std::size_t line_offset,
            std::size_t line_length
        ) const -> BidiRuns {
            if (paragraph.decoded.empty() || line_length == 0) {
                return {
                    .visual = {},
                    .paragraph_right_to_left = FRIBIDI_IS_RTL(paragraph.base_direction) != 0,
                };
            }

            const auto first_codepoint = std::ranges::lower_bound(
                paragraph.decoded,
                line_offset,
                {},
                &foundation::utf8::DecodedCodepoint::byte_offset
            );
            const auto last_codepoint = std::ranges::lower_bound(
                paragraph.decoded,
                line_offset + line_length,
                {},
                &foundation::utf8::DecodedCodepoint::byte_offset
            );
            const auto first_index =
                static_cast<std::size_t>(first_codepoint - paragraph.decoded.begin());
            const auto last_index =
                static_cast<std::size_t>(last_codepoint - paragraph.decoded.begin());
            const auto line_codepoints = last_index - first_index;
            if (line_codepoints == 0) {
                return {
                    .visual = {},
                    .paragraph_right_to_left = FRIBIDI_IS_RTL(paragraph.base_direction) != 0,
                };
            }

            auto levels = paragraph.levels;
            std::vector<FriBidiStrIndex> visual_to_logical(paragraph.decoded.size());
            std::iota(visual_to_logical.begin(), visual_to_logical.end(), 0);
            if (fribidi_reorder_line(
                    FRIBIDI_FLAGS_DEFAULT,
                    paragraph.types.data(),
                    static_cast<FriBidiStrIndex>(line_codepoints),
                    static_cast<FriBidiStrIndex>(first_index),
                    paragraph.base_direction,
                    levels.data(),
                    nullptr,
                    visual_to_logical.data()
                )
                == 0)
            {
                throw std::runtime_error("FriBidi failed to reorder paragraph line");
            }
            std::vector<std::size_t> logical_to_visual(paragraph.decoded.size());
            for (std::size_t visual = 0; visual < visual_to_logical.size(); ++visual) {
                logical_to_visual[static_cast<std::size_t>(visual_to_logical[visual])] = visual;
            }

            std::vector<BidiRun> logical;
            std::size_t begin = first_index;
            while (begin < last_index) {
                std::size_t end = begin + 1;
                while (end < last_index && levels[end] == levels[begin]) {
                    ++end;
                }
                const auto byte_begin = paragraph.decoded[begin].byte_offset;
                const auto byte_end = end < paragraph.decoded.size()
                    ? paragraph.decoded[end].byte_offset
                    : text.size();
                logical.push_back(
                    BidiRun {
                        .offset = byte_begin,
                        .length = byte_end - byte_begin,
                        .level = levels[begin],
                    }
                );
                begin = end;
            }

            const auto visual_start = [&](const BidiRun& run) {
                const auto first = std::ranges::lower_bound(
                    paragraph.decoded,
                    run.offset,
                    {},
                    &foundation::utf8::DecodedCodepoint::byte_offset
                );
                const auto last = std::ranges::lower_bound(
                    paragraph.decoded,
                    run.offset + run.length,
                    {},
                    &foundation::utf8::DecodedCodepoint::byte_offset
                );
                std::size_t result = paragraph.decoded.size();
                for (auto current = first; current != last; ++current) {
                    const auto index =
                        static_cast<std::size_t>(current - paragraph.decoded.begin());
                    result = std::min(result, logical_to_visual[index]);
                }
                return result;
            };
            std::ranges::stable_sort(logical, [&](const BidiRun& lhs, const BidiRun& rhs) {
                return visual_start(lhs) < visual_start(rhs);
            });

            return {
                .visual = std::move(logical),
                .paragraph_right_to_left = FRIBIDI_IS_RTL(paragraph.base_direction) != 0,
            };
        }

        [[nodiscard]] auto bidi_runs(
            std::string_view text,
            std::optional<bool> paragraph_right_to_left = std::nullopt
        ) const -> BidiRuns {
            const auto paragraph = analyze_bidi(text, paragraph_right_to_left);
            return bidi_runs(paragraph, text, 0, text.size());
        }

        [[nodiscard]] auto
        font_for_text(std::string_view text, float pixel_size, bool right_to_left) const
            -> FontChoice {
            const auto decoded = foundation::utf8::decode(text);
            const bool has_zwj = std::ranges::any_of(decoded, [](const auto& codepoint) {
                return codepoint.value == U'\u200D';
            });
            std::optional<std::size_t> best_font;
            std::size_t best_glyph_count = std::numeric_limits<std::size_t>::max();
            for (std::size_t font_index = 0; font_index < fonts.size(); ++font_index) {
                bool supported = true;
                for (std::size_t index = 0; index < decoded.size(); ++index) {
                    const auto& codepoint = decoded[index];
                    const bool default_ignorable = codepoint.value == U'\u200C'
                        || codepoint.value == U'\u200D'
                        || (codepoint.value >= U'\uFE00' && codepoint.value <= U'\uFE0F')
                        || (codepoint.value >= U'\U000E0100' && codepoint.value <= U'\U000E01EF');
                    supported = supported
                        && (default_ignorable
                            || fonts[font_index].face->glyph_index(codepoint.value) != 0);

                    const bool variation_selector =
                        (codepoint.value >= U'\uFE00' && codepoint.value <= U'\uFE0F')
                        || (codepoint.value >= U'\U000E0100' && codepoint.value <= U'\U000E01EF');
                    if (supported && variation_selector && index > 0) {
                        hb_codepoint_t variation_glyph = 0;
                        supported = hb_font_get_variation_glyph(
                                        fonts[font_index].font,
                                        static_cast<hb_codepoint_t>(decoded[index - 1].value),
                                        static_cast<hb_codepoint_t>(codepoint.value),
                                        &variation_glyph
                                    )
                            != 0;
                    }
                }
                if (!supported) {
                    continue;
                }

                const auto shaped = shape_line(
                    text,
                    0,
                    text.size(),
                    pixel_size,
                    pixel_size * 1.2F,
                    pixel_size,
                    right_to_left,
                    font_index
                );
                const bool has_missing = std::ranges::any_of(shaped.glyphs, [](const auto& glyph) {
                    return glyph.glyph_index == 0;
                });
                if (has_missing) {
                    continue;
                }
                if (!has_zwj) {
                    return {.index = font_index, .supported = true};
                }
                if (shaped.glyphs.size() < best_glyph_count) {
                    best_font = font_index;
                    best_glyph_count = shaped.glyphs.size();
                }
            }
            return best_font.has_value() ? FontChoice {.index = *best_font, .supported = true}
                                         : FontChoice {.index = 0, .supported = false};
        }

        [[nodiscard]] auto shape_fallback_run(
            std::string_view text,
            std::size_t source_offset,
            float pixel_size,
            float line_height,
            float baseline,
            bool right_to_left
        ) const -> widget::primitives::TextLayoutLine {
            if (fonts.size() == 1 || text.empty()) {
                return shape_line(
                    text,
                    source_offset,
                    text.size(),
                    pixel_size,
                    line_height,
                    baseline,
                    right_to_left,
                    0
                );
            }

            std::vector<FontRange> ranges;
            for (const auto& grapheme: foundation::utf8::grapheme_ranges(text)) {
                const auto choice = font_for_text(
                    text.substr(grapheme.offset, grapheme.length),
                    pixel_size,
                    right_to_left
                );
                const auto absolute_offset = source_offset + grapheme.offset;
                if (!ranges.empty() && ranges.back().font_index == choice.index
                    && ranges.back().supported == choice.supported
                    && ranges.back().offset + ranges.back().length == absolute_offset)
                {
                    ranges.back().length += grapheme.length;
                }
                else {
                    ranges.push_back(
                        FontRange {
                            .offset = absolute_offset,
                            .length = grapheme.length,
                            .font_index = choice.index,
                            .supported = choice.supported,
                        }
                    );
                }
            }
            if (right_to_left) {
                std::ranges::reverse(ranges);
            }

            widget::primitives::TextLayoutLine result {
                .text_offset = source_offset,
                .text_length = text.size(),
                .visible_text = std::string(text),
                .size = foundation::NanSize(0.0F, line_height),
                .baseline = baseline,
                .right_to_left = right_to_left,
            };
            float width = 0.0F;
            for (const auto& range: ranges) {
                const auto relative_offset = range.offset - source_offset;
                auto shaped = shape_line(
                    text.substr(relative_offset, range.length),
                    range.offset,
                    range.length,
                    pixel_size,
                    line_height,
                    baseline,
                    right_to_left,
                    range.font_index
                );
                width += shaped.size.get_width();
                result.missing_glyphs =
                    result.missing_glyphs || !range.supported || shaped.missing_glyphs;
                for (auto& glyph: shaped.glyphs) {
                    result.glyphs.push_back(std::move(glyph));
                }
            }
            result.size = foundation::NanSize(width, line_height);
            return result;
        }

        static void append_run_caret_stops(
            widget::primitives::TextLayoutLine& line,
            const widget::primitives::TextLayoutLine& shaped,
            const std::size_t source_begin,
            const std::size_t source_end,
            const bool right_to_left,
            const float visual_origin
        ) {
            std::vector<std::size_t> cluster_starts;
            for (const auto& glyph: shaped.glyphs) {
                if (glyph.cluster >= source_begin && glyph.cluster < source_end) {
                    cluster_starts.push_back(glyph.cluster);
                }
            }
            std::ranges::sort(cluster_starts);
            const auto unique_end = std::ranges::unique(cluster_starts).begin();
            cluster_starts.erase(unique_end, cluster_starts.end());

            float pen = visual_origin;
            std::size_t glyph_index = 0;
            while (glyph_index < shaped.glyphs.size()) {
                const auto cluster = shaped.glyphs[glyph_index].cluster;
                const float cluster_start_x = pen;
                do {
                    pen += shaped.glyphs[glyph_index].x_advance;
                    ++glyph_index;
                } while (glyph_index < shaped.glyphs.size()
                         && shaped.glyphs[glyph_index].cluster == cluster);

                if (cluster < source_begin || cluster >= source_end) {
                    continue;
                }
                const auto next = std::ranges::upper_bound(cluster_starts, cluster);
                const auto cluster_end = next == cluster_starts.end() ? source_end : *next;
                const float cluster_end_x = pen;
                line.caret_stops.push_back(
                    widget::primitives::TextCaretStop {
                        .source_offset = cluster,
                        .x = right_to_left ? cluster_end_x : cluster_start_x,
                        .affinity = widget::primitives::TextAffinity::downstream,
                    }
                );
                line.caret_stops.push_back(
                    widget::primitives::TextCaretStop {
                        .source_offset = cluster_end,
                        .x = right_to_left ? cluster_start_x : cluster_end_x,
                        .affinity = widget::primitives::TextAffinity::upstream,
                    }
                );
            }
        }

        static void normalize_caret_stops(widget::primitives::TextLayoutLine& line) {
            if (line.caret_stops.empty()) {
                line.caret_stops.push_back(
                    widget::primitives::TextCaretStop {
                        .source_offset = line.text_offset,
                    }
                );
                return;
            }

            std::ranges::stable_sort(line.caret_stops, [](const auto& left, const auto& right) {
                return left.x < right.x;
            });
            std::vector<widget::primitives::TextCaretStop> normalized;
            normalized.reserve(line.caret_stops.size());
            constexpr float epsilon = 0.001F;
            for (const auto& stop: line.caret_stops) {
                const auto duplicate = std::ranges::find_if(normalized, [&](const auto& existing) {
                    return existing.source_offset == stop.source_offset
                        && std::abs(existing.x - stop.x) <= epsilon;
                });
                if (duplicate == normalized.end()) {
                    normalized.push_back(stop);
                }
                else if (stop.affinity == widget::primitives::TextAffinity::downstream) {
                    duplicate->affinity = stop.affinity;
                }
            }
            line.caret_stops = std::move(normalized);
        }

        [[nodiscard]] auto shape_analyzed_line(
            std::string_view paragraph_text,
            const BidiParagraph& paragraph,
            std::size_t line_offset,
            std::size_t line_length,
            std::size_t paragraph_source_offset,
            std::size_t output_source_length,
            float pixel_size,
            float line_height,
            float baseline
        ) const -> widget::primitives::TextLayoutLine {
            const auto runs = bidi_runs(paragraph, paragraph_text, line_offset, line_length);
            const auto output_offset = paragraph_source_offset + line_offset;
            widget::primitives::TextLayoutLine line {
                .text_offset = output_offset,
                .text_length = output_source_length,
                .visible_text = std::string(paragraph_text.substr(line_offset, line_length)),
                .size = foundation::NanSize(0.0F, line_height),
                .baseline = baseline,
                .right_to_left = runs.paragraph_right_to_left,
            };
            float width = 0.0F;
            for (const auto& run: runs.visual) {
                auto shaped = shape_fallback_run(
                    paragraph_text.substr(run.offset, run.length),
                    paragraph_source_offset + run.offset,
                    pixel_size,
                    line_height,
                    baseline,
                    run.right_to_left()
                );
                append_run_caret_stops(
                    line,
                    shaped,
                    paragraph_source_offset + run.offset,
                    std::min(
                        paragraph_source_offset + run.offset + run.length,
                        output_offset + output_source_length
                    ),
                    run.right_to_left(),
                    width
                );
                for (auto& glyph: shaped.glyphs) {
                    glyph.cluster = std::min(glyph.cluster, output_offset + output_source_length);
                    line.glyphs.push_back(std::move(glyph));
                }
                line.missing_glyphs = line.missing_glyphs || shaped.missing_glyphs;
                width += shaped.size.get_width();
            }
            normalize_caret_stops(line);
            line.size = foundation::NanSize(width, line_height);
            return line;
        }

        [[nodiscard]] auto shape_bidi_line(
            std::string_view text,
            std::size_t source_offset,
            std::size_t source_length,
            float pixel_size,
            float line_height,
            float baseline,
            std::optional<bool> paragraph_right_to_left = std::nullopt
        ) const -> widget::primitives::TextLayoutLine {
            const auto paragraph = analyze_bidi(text, paragraph_right_to_left);
            return shape_analyzed_line(
                text,
                paragraph,
                0,
                text.size(),
                source_offset,
                source_length,
                pixel_size,
                line_height,
                baseline
            );
        }

        [[nodiscard]] auto shape_source_line(
            std::string_view text,
            std::size_t source_offset,
            float pixel_size,
            float line_height,
            float baseline
        ) const -> widget::primitives::TextLayoutLine {
            return shape_bidi_line(
                text,
                source_offset,
                text.size(),
                pixel_size,
                line_height,
                baseline
            );
        }

        [[nodiscard]] auto max_shaped_width(std::string_view text, float pixel_size) const
            -> float {
            const auto font_metrics = combined_metrics(pixel_size);
            const float line_height =
                font_metrics.line_height > 0.0F ? font_metrics.line_height : pixel_size * 1.2F;
            float width = 0.0F;
            std::size_t offset = 0;
            bool pending_empty_line = text.empty();
            while (offset < text.size() || pending_empty_line) {
                const auto newline = text.find('\n', offset);
                const auto end = newline != std::string_view::npos ? newline : text.size();
                const auto line = shape_source_line(
                    text.substr(offset, end - offset),
                    offset,
                    pixel_size,
                    line_height,
                    font_metrics.ascender
                );
                width = std::max(width, line.size.get_width());
                pending_empty_line = false;
                offset = end;
                if (offset < text.size() && text[offset] == '\n') {
                    ++offset;
                    pending_empty_line = offset == text.size();
                }
            }
            return width;
        }

        [[nodiscard]] auto
        clusters(const widget::primitives::TextLayoutLine& line, std::size_t paragraph_end) const
            -> std::vector<ClusterRange> {
            std::vector<ClusterRange> result;
            if (line.glyphs.empty()) {
                return result;
            }

            std::map<std::size_t, float> cluster_widths;
            for (const auto& glyph: line.glyphs) {
                cluster_widths[glyph.cluster] += glyph.x_advance;
            }
            for (auto current = cluster_widths.begin(); current != cluster_widths.end(); ++current)
            {
                const auto next_entry = std::next(current);
                const auto next =
                    next_entry != cluster_widths.end() ? next_entry->first : paragraph_end;
                result.push_back(
                    ClusterRange {
                        .offset = current->first,
                        .length = next - current->first,
                        .width = std::abs(current->second),
                    }
                );
            }
            return result;
        }

        [[nodiscard]] auto fitted_source_length(
            const widget::primitives::TextLayoutLine& line,
            std::size_t paragraph_end,
            float width_limit
        ) const -> std::size_t {
            float width = 0.0F;
            std::size_t end = line.text_offset;
            for (const auto& cluster: clusters(line, paragraph_end)) {
                if (width + cluster.width > width_limit) {
                    break;
                }
                width += cluster.width;
                end = cluster.offset + cluster.length;
            }
            return end - line.text_offset;
        }

        [[nodiscard]] auto wrapped_ranges(
            const widget::primitives::TextLayoutLine& line,
            std::size_t paragraph_end,
            float width_limit
        ) const -> std::vector<ClusterRange> {
            const auto shaped_clusters = clusters(line, paragraph_end);
            if (shaped_clusters.empty()) {
                return {
                    {.offset = line.text_offset,
                     .length = line.text_length,
                     .width = line.size.get_width()}
                };
            }

            std::vector<ClusterRange> result;
            std::size_t line_offset = shaped_clusters.front().offset;
            std::size_t line_end = line_offset;
            float line_width = 0.0F;
            for (const auto& cluster: shaped_clusters) {
                if (line_width > 0.0F && line_width + cluster.width > width_limit) {
                    result.push_back(
                        ClusterRange {
                            .offset = line_offset,
                            .length = line_end - line_offset,
                            .width = line_width,
                        }
                    );
                    line_offset = cluster.offset;
                    line_width = 0.0F;
                }
                line_width += cluster.width;
                line_end = cluster.offset + cluster.length;
            }
            result.push_back(
                ClusterRange {
                    .offset = line_offset,
                    .length = line_end - line_offset,
                    .width = line_width,
                }
            );
            return result;
        }

        std::vector<FontSlot> fonts;
    };

    HarfBuzzTextLayoutBackend::HarfBuzzTextLayoutBackend(std::shared_ptr<FreeTypeFontFace> face):
        HarfBuzzTextLayoutBackend(std::move(face), {}) {}

    HarfBuzzTextLayoutBackend::HarfBuzzTextLayoutBackend(
        std::shared_ptr<FreeTypeFontFace> primary,
        std::vector<std::shared_ptr<FreeTypeFontFace>> fallbacks
    ):
        impl_(std::make_unique<Impl>(std::move(primary), std::move(fallbacks))) {}

    HarfBuzzTextLayoutBackend::~HarfBuzzTextLayoutBackend() = default;

    auto HarfBuzzTextLayoutBackend::layout(widget::primitives::TextLayoutInput input) const
        -> widget::primitives::TextLayoutResult {
        const bool has_width_limit =
            std::isfinite(input.constraints.max_width) && input.constraints.max_width >= 0.0F;
        const float width_limit =
            has_width_limit ? input.constraints.max_width : std::numeric_limits<float>::infinity();

        float font_size = std::max(1.0F, input.style.font_size);
        if (input.style.overflow == widget::primitives::TextOverflow::scale && has_width_limit) {
            const float natural_width = impl_->max_shaped_width(input.text, font_size);
            if (natural_width > width_limit) {
                float lower = 1.0F;
                float upper = font_size;
                if (impl_->max_shaped_width(input.text, lower) <= width_limit) {
                    for (int iteration = 0; iteration < 12; ++iteration) {
                        const float candidate = (lower + upper) * 0.5F;
                        if (impl_->max_shaped_width(input.text, candidate) <= width_limit) {
                            lower = candidate;
                        }
                        else {
                            upper = candidate;
                        }
                    }
                }
                font_size = lower;
            }
        }

        const auto metrics = impl_->combined_metrics(font_size);
        const float line_height =
            metrics.line_height > 0.0F ? metrics.line_height : font_size * 1.2F;
        const float baseline = metrics.ascender;
        const auto max_lines = static_cast<std::size_t>(std::max(1, input.style.max_lines));

        widget::primitives::TextLayoutResult result;
        result.font_size = font_size;
        result.baseline = baseline;

        std::size_t offset = 0;
        bool pending_empty_line = input.text.empty();
        while ((offset < input.text.size() || pending_empty_line)
               && result.lines.size() < max_lines)
        {
            const auto newline = input.text.find('\n', offset);
            const auto end = newline != std::string_view::npos ? newline : input.text.size();
            const auto paragraph = input.text.substr(offset, end - offset);
            const auto bidi_paragraph = impl_->analyze_bidi(paragraph);
            auto shaped = impl_->shape_analyzed_line(
                paragraph,
                bidi_paragraph,
                0,
                paragraph.size(),
                offset,
                paragraph.size(),
                font_size,
                line_height,
                baseline
            );

            if (input.style.overflow == widget::primitives::TextOverflow::wrap && has_width_limit
                && shaped.size.get_width() > width_limit)
            {
                const auto ranges = impl_->wrapped_ranges(shaped, end, width_limit);
                for (std::size_t range_index = 0;
                     range_index < ranges.size() && result.lines.size() < max_lines;
                     ++range_index)
                {
                    const auto& range = ranges[range_index];
                    const auto relative_offset = range.offset - offset;
                    auto line = impl_->shape_analyzed_line(
                        paragraph,
                        bidi_paragraph,
                        relative_offset,
                        range.length,
                        offset,
                        range.length,
                        font_size,
                        line_height,
                        baseline
                    );
                    result.overflowed = result.overflowed || line.size.get_width() > width_limit;
                    result.lines.push_back(std::move(line));
                    if (range_index + 1 < ranges.size() && result.lines.size() == max_lines) {
                        result.overflowed = true;
                    }
                }
            }
            else {
                if (has_width_limit && shaped.size.get_width() > width_limit) {
                    result.overflowed = true;
                    if (input.style.overflow == widget::primitives::TextOverflow::ellipsis) {
                        const auto dots = impl_->shape_source_line(
                            "...",
                            offset,
                            font_size,
                            line_height,
                            baseline
                        );
                        if (dots.size.get_width() <= width_limit) {
                            const auto length = impl_->fitted_source_length(
                                shaped,
                                end,
                                width_limit - dots.size.get_width()
                            );
                            auto visible = std::string(paragraph.substr(0, length));
                            visible += "...";
                            const bool paragraph_right_to_left = shaped.right_to_left;
                            shaped = impl_->shape_bidi_line(
                                visible,
                                offset,
                                length,
                                font_size,
                                line_height,
                                baseline,
                                paragraph_right_to_left
                            );
                        }
                        else {
                            const bool paragraph_right_to_left = shaped.right_to_left;
                            shaped = impl_->shape_bidi_line(
                                {},
                                offset,
                                0,
                                font_size,
                                line_height,
                                baseline,
                                paragraph_right_to_left
                            );
                        }
                    }
                }
                result.lines.push_back(std::move(shaped));
            }

            pending_empty_line = false;
            offset = end;
            if (offset < input.text.size() && input.text[offset] == '\n') {
                ++offset;
                pending_empty_line = offset == input.text.size();
            }
        }

        float width = 0.0F;
        for (const auto& line: result.lines) {
            width = std::max(width, line.size.get_width());
            result.missing_glyphs = result.missing_glyphs || line.missing_glyphs;
        }
        if (has_width_limit && width > width_limit) {
            result.overflowed = true;
        }
        result.overflowed = result.overflowed || offset < input.text.size() || pending_empty_line;
        result.size = input.constraints.constrain(
            foundation::NanSize(width, line_height * static_cast<float>(result.lines.size()))
        );
        return result;
    }

    auto HarfBuzzTextLayoutBackend::font_face() const -> const std::shared_ptr<FreeTypeFontFace>& {
        return impl_->fonts.front().face;
    }

    auto HarfBuzzTextLayoutBackend::font_face(std::size_t index) const
        -> const std::shared_ptr<FreeTypeFontFace>& {
        return impl_->fonts.at(index).face;
    }

    auto HarfBuzzTextLayoutBackend::font_count() const -> std::size_t {
        return impl_->fonts.size();
    }

} // namespace nandina::text
