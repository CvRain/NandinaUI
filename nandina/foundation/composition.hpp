//
// foundation/composition - deterministic CPU color composition for diagnostics.
//
// 唯一 CPU 合成实现（D2 §6.7）：输入纯颜色与显式合成配置，不认识组件、token 或场景树。
// 首版只覆盖「straight-alpha 前景覆盖到不透明背景」的编码 sRGB source-over，与当前
// SDL 绘制路径的源码核对一致（§6.8）；它不宣称覆盖任意透明背景、预乘 alpha 或离屏组
// 透明度。合成空间是显式策略：新增后端不得暗改既有语义，扩展须另定输出 alpha 规则。
//

#ifndef NANDINA_EXPERIMENT_FOUNDATION_COMPOSITION_HPP
#define NANDINA_EXPERIMENT_FOUNDATION_COMPOSITION_HPP

#include "nandina_color.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <expected>
#include <span>
#include <stdexcept>
#include <string_view>

namespace nandina::foundation
{
    /// 首版唯一合成空间：编码 sRGB 通道上的 straight-alpha source-over。
    enum class CompositeSpace {
        encoded_srgb,
    };

    /// 显式合成配置；诊断与后端读回核对测试必须记录同一配置。
    struct CompositeConfig {
        CompositeSpace space = CompositeSpace::encoded_srgb;
    };

    enum class CompositionErrorKind {
        unsupported_space,
        non_finite_color,
        no_opaque_endpoint,
    };

    struct CompositionError {
        CompositionErrorKind kind = CompositionErrorKind::unsupported_space;
        /// 相关层的下标（由近及远，0 为最近的前景/背景层）。
        std::size_t index = 0;
    };

    [[nodiscard]] inline auto describe(const CompositionError& error) -> std::string_view {
        switch (error.kind) {
            case CompositionErrorKind::unsupported_space:
                return "composition space is not supported";
            case CompositionErrorKind::non_finite_color:
                return "composition input color is not finite";
            case CompositionErrorKind::no_opaque_endpoint:
                return "background stack has no opaque endpoint";
        }
        return "invalid composition input";
    }

    namespace composition_detail
    {
        [[nodiscard]] inline auto supported(const CompositeConfig& config) -> bool {
            return config.space == CompositeSpace::encoded_srgb;
        }

        [[nodiscard]] inline auto finite(const NanColor& color) -> bool {
            const auto rgb = color.to<NanRgb>();
            return std::isfinite(rgb.red) && std::isfinite(rgb.green) && std::isfinite(rgb.blue)
                && std::isfinite(rgb.alpha);
        }

        /// 编码 sRGB 通道上的 straight-alpha source-over，结果为不透明色。
        [[nodiscard]] inline auto blend(const NanColor& foreground, const NanColor& backdrop)
            -> NanColor {
            const auto source = foreground.to<NanRgb>();
            const auto destination = backdrop.to<NanRgb>();
            const float alpha = std::clamp(source.alpha, 0.0F, 1.0F);
            return NanColor::from_rgb(
                source.red * alpha + destination.red * (1.0F - alpha),
                source.green * alpha + destination.green * (1.0F - alpha),
                source.blue * alpha + destination.blue * (1.0F - alpha)
            );
        }
    } // namespace composition_detail

    /// 前景按自身 alpha 覆盖到**完全不透明**背景（straight-alpha source-over，编码 sRGB）。
    ///
    /// 非有限颜色、非不透明背景或未知合成空间属于调用方误用，抛 std::invalid_argument；
    /// 背景栈的「上下文不足」判定由调用方用 nan_composite_background_stack 得到。
    [[nodiscard]] inline auto nan_composite_over(
        const NanColor& foreground,
        const NanColor& opaque_backdrop,
        const CompositeConfig& config = {}
    ) -> NanColor {
        if (!composition_detail::supported(config)) {
            throw std::invalid_argument("composition space is not supported");
        }
        if (!composition_detail::finite(foreground) || !composition_detail::finite(opaque_backdrop))
        {
            throw std::invalid_argument("composition color is not finite");
        }
        if (opaque_backdrop.alpha() < 1.0F) {
            throw std::invalid_argument("composition backdrop must be opaque");
        }
        return composition_detail::blend(foreground, opaque_backdrop);
    }

    /// 纯颜色背景栈（由近及远）。遇到首个完全不透明层即停止，再从该端点向近端逐层合成。
    /// 无有限不透明终点、近端颜色非有限或空间不支持时返回诊断，不猜测底色。
    [[nodiscard]] inline auto nan_composite_background_stack(
        std::span<const NanColor> from_nearest,
        const CompositeConfig& config = {}
    ) -> std::expected<NanColor, CompositionError> {
        if (!composition_detail::supported(config)) {
            return std::unexpected(
                CompositionError {.kind = CompositionErrorKind::unsupported_space}
            );
        }
        std::size_t opaque = from_nearest.size();
        for (std::size_t index = 0; index < from_nearest.size(); ++index) {
            if (!composition_detail::finite(from_nearest[index])) {
                return std::unexpected(
                    CompositionError {
                        .kind = CompositionErrorKind::non_finite_color,
                        .index = index,
                    }
                );
            }
            if (from_nearest[index].alpha() >= 1.0F) {
                opaque = index;
                break;
            }
        }
        if (opaque == from_nearest.size()) {
            return std::unexpected(
                CompositionError {
                    .kind = CompositionErrorKind::no_opaque_endpoint,
                    .index = from_nearest.size(),
                }
            );
        }
        auto result = from_nearest[opaque];
        for (std::size_t index = opaque; index-- > 0;) {
            result = composition_detail::blend(from_nearest[index], result);
        }
        return result;
    }

    /// 前景先合成到背景栈结果上；任一输入不足以确定有效背景时返回诊断。
    [[nodiscard]] inline auto nan_composite_over_stack(
        const NanColor& foreground,
        std::span<const NanColor> from_nearest,
        const CompositeConfig& config = {}
    ) -> std::expected<NanColor, CompositionError> {
        auto background = nan_composite_background_stack(from_nearest, config);
        if (!background) {
            return std::unexpected(background.error());
        }
        if (!composition_detail::finite(foreground)) {
            return std::unexpected(
                CompositionError {.kind = CompositionErrorKind::non_finite_color}
            );
        }
        return composition_detail::blend(foreground, *background);
    }
} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_FOUNDATION_COMPOSITION_HPP
