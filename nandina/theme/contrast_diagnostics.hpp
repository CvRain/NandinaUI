/**
 * theme/contrast_diagnostics — explicit, scenario-scoped contrast diagnostics for schemes.
 *
 * D2 §6.4–§6.7：规则显式声明前景与从近到远的背景栈；theme 负责按角色解析并检查终点，
 * 合成交给 foundation 的唯一 CPU 合成器；阈值比较使用未格式化的计算值。诊断保留解析后的
 * OKLCH 值、转换/裁剪后的 sRGB 值、实测比值与所需阈值，并把「上下文不足」与「实测失败」
 * 分开。本模块只做诊断，不自动替作者改色；注册门槛要等真实后端像素读回核对合成空间与
 * 色域策略后再接通（§8 第 4a 步），在此之前不宣称注册通过等于配色门槛通过。
 */

#ifndef NANDINA_EXPERIMENT_THEME_CONTRAST_DIAGNOSTICS_HPP
#define NANDINA_EXPERIMENT_THEME_CONTRAST_DIAGNOSTICS_HPP

#include "../foundation/composition.hpp"
#include "../foundation/contrast.hpp"
#include "nan_style.hpp"
#include "theme_scheme.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::theme
{
    /// 一条可读性规则：显式的角色对与背景栈，不做隐式推断。
    struct ContrastRule {
        /// 稳定的规则标识，用于诊断与显式例外；建议使用「角色/场景」形式。
        std::string name;
        /// 适用状态（如 "normal" / "hover"）；空表示规则不区分状态。
        std::string state;
        ThemeColor foreground;
        /// 从近到远的背景栈；在首个不透明终点停止（缺少终点则报告上下文不足）。
        std::vector<ThemeColor> background;
        float required_ratio = foundation::nan_contrast_aa_text;
        /// required 失败在未来的注册门槛中阻止提交；false 表示建议级别。
        bool required = true;
        /// tone 引用（含变换操作数）的显式场景上下文；缺少时不猜测 primary。
        std::optional<ButtonTone> tone;
    };

    struct ContrastProfile {
        std::vector<ContrastRule> rules;
    };

    /// 参与判定的颜色层：保留 NanColor 规范化存储的 OKLCH 与转换/裁剪后的 sRGB。
    /// 不是导入前作者原文；CPU 路径不额外进行 8-bit 量化。
    struct ContrastLayer {
        foundation::NanColor resolved;
        foundation::NanRgb converted;
    };

    enum class ContrastStatus {
        passed,
        failed,
        insufficient_context,
    };

    struct ContrastDiagnostic {
        std::string rule;
        std::string state;
        /// 匿名来源（未注册快照）没有 scheme id；不伪造注册身份。
        std::optional<ThemeSchemeId> scheme;
        ColorAppearance appearance = ColorAppearance::light;
        ContrastStatus status = ContrastStatus::failed;
        bool required = true;
        float required_ratio = 0.0F;
        /// 实测比值；上下文不足时不存在。
        std::optional<float> measured_ratio;
        std::optional<foundation::NanColor> effective_foreground;
        std::optional<foundation::NanColor> effective_background;
        /// 前景与背景栈的角色路径，变换包含操作及操作数。
        std::string foreground_role;
        std::vector<std::string> background_roles;
        /// 缺少前景解析上下文时为空，避免伪造解析值。
        std::optional<ContrastLayer> foreground;
        std::vector<ContrastLayer> background;
        std::string message;
        /// 与本次计算一致的合成配置；现行色域为 linear RGB 通道裁剪、浮点 sRGB，无 u8 量化。
        foundation::CompositeConfig config;
        std::optional<ButtonTone> tone;
    };

    /// 与组件默认配方对应的首批显式规则档案（不含隐式整表断言，也不做风格建议）。
    [[nodiscard]] auto default_contrast_profile() -> ContrastProfile;

    /// 按档案评估一份方案内容（content.palette 即该外观下的语义色板）。
    [[nodiscard]] auto evaluate_contrast(
        const ThemeSchemeContent& content,
        ColorAppearance appearance,
        const ContrastProfile& profile,
        std::optional<ThemeSchemeId> scheme = std::nullopt,
        foundation::CompositeConfig config = {}
    ) -> std::vector<ContrastDiagnostic>;

    /// 是否存在会阻止（未来）注册提交的必需规则失败；上下文不足不计作失败。
    [[nodiscard]] auto
    has_required_contrast_failure(std::span<const ContrastDiagnostic> diagnostics) -> bool;

    /// 是否存在上下文不足的诊断（严格模式可选择拒绝，但不伪报为实测失败）。
    [[nodiscard]] auto
    has_insufficient_contrast_context(std::span<const ContrastDiagnostic> diagnostics) -> bool;

    [[nodiscard]] auto describe(const ContrastDiagnostic& diagnostic) -> std::string;
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_CONTRAST_DIAGNOSTICS_HPP
