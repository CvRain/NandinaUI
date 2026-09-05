//
// theme/builtin_themes - framework built-in theme families.
//

#include "builtin_themes.hpp"

#include "theme_manager.hpp"

#include <utility>

namespace nandina::theme
{
    auto default_theme_families() -> std::vector<ThemeFamilyDefinition> {
        std::vector<ThemeFamilyDefinition> families;

        // ─── butter：黄油卡片 + Catppuccin（暖奶油中性 + 琥珀主色 + 暖粉辅色）───
        // 色阶按 50 → 950（浅 → 深）命名；950 档 = base，实现「On Accent = Base」。
        // 主色/辅色以 Catppuccin Peach/Rosewater、Flamingo/Maroon 为参考做暖调微调。
        {
            auto reference = default_reference_palette();
            reference.neutral = nan_color_scale({
                .shade_50 = 0xFAF6EC, // 奶白（背景）
                .shade_100 = 0xF2EAD9, // 米黄（表面）
                .shade_200 = 0xE8DCC4, // 浅米（表面变体）
                .shade_300 = 0xD9C8A8, // 米
                .shade_400 = 0xB3A184, // 暖灰
                .shade_500 = 0x96856A, // 暖灰（outline）
                .shade_600 = 0x7A6B54, // 暖灰深
                .shade_700 = 0x5A4F3E, // 暖深灰（次文本）
                .shade_800 = 0x3F372B, // 深暖
                .shade_900 = 0x2A241B, // 近黑暖（暗表面）
                .shade_950 = 0x1C1811, // 最深暖（文字 / base）
            });
            reference.primary = nan_color_scale({
                .shade_50 = 0xFFF6E2, // 奶油
                .shade_100 = 0xFFEBC4, // 浅黄
                .shade_200 = 0xFFD997,
                .shade_300 = 0xFCC36C, // 暗色品牌（Peach 提亮）
                .shade_400 = 0xF5A845,
                .shade_500 = 0xE78D28, // 亮色品牌（琥珀）
                .shade_600 = 0xC97217,
                .shade_700 = 0xA15710,
                .shade_800 = 0x7A3E0A,
                .shade_900 = 0x522804,
                .shade_950 = 0x1C1811, // on-primary（= base）
            });
            reference.tertiary = nan_color_scale({
                .shade_50 = 0xF9E4E0,
                .shade_100 = 0xF4CCCC,
                .shade_200 = 0xEBA5AE,
                .shade_300 = 0xDD7F8A, // 暗色辅色（Flamingo 提亮）
                .shade_400 = 0xD45A68,
                .shade_500 = 0xC23A4E, // 亮色辅色（Maroon）
                .shade_600 = 0xA62A3C,
                .shade_700 = 0x841F2F,
                .shade_800 = 0x61151F,
                .shade_900 = 0x3E0C12,
                .shade_950 = 0x24060A,
            });

            NanTokens tokens {};
            tokens.radius.sm = 12.0F;
            tokens.radius.md = 16.0F;
            tokens.radius.lg = 24.0F;
            tokens.border.thin = 2.0F;
            tokens.border.medium = 3.0F;

            families.push_back(
                ThemeFamilyDefinition {
                    .name = "butter",
                    .reference = std::move(reference),
                    .policy =
                        {
                            .light_brand = ColorShade::shade_500,
                            .dark_brand = ColorShade::shade_300,
                            .light_on_brand = ColorShade::shade_950,
                            .dark_on_brand = ColorShade::shade_950,
                        },
                    .tokens = tokens,
                    .card_rules = {
                        // 卡片软阴影：暖棕、向下偏移、柔和衰减（claymorphism elevation）。
                        CardRecipeRule {
                            .shadow_color =
                                ThemeColor::literal(NanColor::from_hex(0x1C1811, 0.16F)),
                            .shadow_offset_y = ThemeScalar::literal(4.0F),
                            .shadow_spread = ThemeScalar::literal(10.0F),
                        },
                    },
                }
            );
        }

        // ─── fluent：Fluent 2 / Windows（冷灰中性 + 蓝色 accent，扁平锐角、8px 网格）───
        // 蓝 accent 在亮色下较深（白字）、暗色下较浅（深字），故 on_brand 随外观翻转。
        {
            auto reference = default_reference_palette();
            reference.neutral = nan_color_scale({
                .shade_50 = 0xF3F3F3, // 亮背景（近白冷灰）
                .shade_100 = 0xFFFFFF, // 亮表面（纯白）
                .shade_200 = 0xEBEBEB, // 表面变体
                .shade_300 = 0xE0E0E0, // outline 变体
                .shade_400 = 0xC8C8C8, // 暗 on_surface_variant
                .shade_500 = 0xA0A0A0, // outline（亮）
                .shade_600 = 0x808080, // outline（暗）
                .shade_700 = 0x616161, // on_surface_variant（亮，fluentui gray）
                .shade_800 = 0x424242, // 表面变体（暗，fluentui gray）
                .shade_900 = 0x2B2B2B, // 暗表面
                .shade_950 = 0x202020, // 暗背景 / 文字
            });
            reference.primary = nan_color_scale({
                .shade_50 = 0xFFFFFF, // on-primary（亮）
                .shade_100 = 0xE0F1FF,
                .shade_200 = 0xC2E5FF,
                .shade_300 = 0x4CC2FF, // 暗品牌（Fluent 亮蓝）
                .shade_400 = 0x2B88D8,
                .shade_500 = 0x0067C0, // 亮品牌（Fluent 蓝）
                .shade_600 = 0x005AA3,
                .shade_700 = 0x004A86,
                .shade_800 = 0x003A6A,
                .shade_900 = 0x002A4F,
                .shade_950 = 0x0A1F33, // on-primary（暗）
            });

            NanTokens tokens {};
            tokens.radius.sm = 4.0F;
            tokens.radius.md = 8.0F;
            tokens.radius.lg = 12.0F;

            families.push_back(
                ThemeFamilyDefinition {
                    .name = "fluent",
                    .reference = std::move(reference),
                    .policy =
                        {
                            .light_brand = ColorShade::shade_500,
                            .dark_brand = ColorShade::shade_300,
                            .light_on_brand = ColorShade::shade_50,
                            .dark_on_brand = ColorShade::shade_950,
                        },
                    .tokens = tokens,
                }
            );
        }

        // ─── material：Material 3（灰紫中性 + 紫主色 + 粉辅色，圆角、4px 密度）───
        {
            auto reference = default_reference_palette();
            reference.neutral = nan_color_scale({
                .shade_50 = 0xFEF7FF, // 亮背景（M3 surface）
                .shade_100 = 0xFFFFFF, // 亮表面（surface-container-lowest）
                .shade_200 = 0xE7E0EC, // 表面变体（M3 surface-variant）
                .shade_300 = 0xCAC4D0, // outline 变体（M3 outline-variant）
                .shade_400 = 0xCAC4D0, // 暗 on_surface_variant（M3 对称翻转）
                .shade_500 = 0x79747E, // outline（亮，M3）
                .shade_600 = 0x938F99, // outline（暗，M3 更亮）
                .shade_700 = 0x49454F, // on_surface_variant（亮）
                .shade_800 = 0x49454F, // 表面变体 / outline 变体（暗）
                .shade_900 = 0x211F26, // 暗表面（surface-container）
                .shade_950 = 0x141218, // 暗背景（M3 surface）
            });
            reference.primary = nan_color_scale({
                .shade_50 = 0xFFFFFF, // on-primary（亮，tone 100）
                .shade_100 = 0xF6EDFF, // tone 95
                .shade_200 = 0xEADDFF, // tone 90（primary-container）
                .shade_300 = 0xD0BCFF, // 暗品牌（tone 80）
                .shade_400 = 0xB69DF8, // tone 70
                .shade_500 = 0x6750A4, // 亮品牌（tone 40）
                .shade_600 = 0x54408C, // tone ~34
                .shade_700 = 0x453275, // tone ~28
                .shade_800 = 0x38245F, // tone ~23
                .shade_900 = 0x21005D, // tone 10（on-primary-container）
                .shade_950 = 0x381E72, // on-primary（暗，tone 20）
            });
            reference.tertiary = nan_color_scale({
                .shade_50 = 0xFFF8F9,
                .shade_100 = 0xFFE1E6,
                .shade_200 = 0xFFC4D0,
                .shade_300 = 0xEFB8C8, // 暗辅色（M3 粉）
                .shade_400 = 0xE89BB0,
                .shade_500 = 0x7D5260, // 亮辅色（M3 紫红）
                .shade_600 = 0x66434F,
                .shade_700 = 0x53363F,
                .shade_800 = 0x402A31,
                .shade_900 = 0x2E1E24,
                .shade_950 = 0x1D1216,
            });

            NanTokens tokens {};
            tokens.radius.sm = 8.0F;
            tokens.radius.md = 12.0F;
            tokens.radius.lg = 16.0F;

            families.push_back(
                ThemeFamilyDefinition {
                    .name = "material",
                    .reference = std::move(reference),
                    .policy =
                        {
                            .light_brand = ColorShade::shade_500,
                            .dark_brand = ColorShade::shade_300,
                            .light_on_brand = ColorShade::shade_50,
                            .dark_on_brand = ColorShade::shade_950,
                        },
                    .tokens = tokens,
                }
            );
        }

        return families;
    }

    auto build_family_design_system(const ThemeFamilyDefinition& family) -> DesignSystem {
        auto system = default_design_system();
        system.tokens = family.tokens;
        system.light = make_color_scheme(family.reference, ColorAppearance::light, family.policy);
        system.dark = make_color_scheme(family.reference, ColorAppearance::dark, family.policy);
        for (const auto& rule: family.card_rules) {
            system.components.card.rules.push_back(rule);
        }
        return system;
    }

    void register_default_theme_families(ThemeManager& manager) {
        for (const auto& family: default_theme_families()) {
            manager.register_theme_family(family.name, build_family_design_system(family));
        }
    }
} // namespace nandina::theme
