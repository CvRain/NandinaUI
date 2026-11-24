//
// theme/builtin_themes - framework built-in theme families.
//

#include "builtin_themes.hpp"

#include "theme_manager.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nandina::theme
{
    auto default_theme_families() -> std::vector<ThemeFamilyDefinition> {
        std::vector<ThemeFamilyDefinition> families;

        // ─── butter：清爽暖奶油黏土（Sunlit Butter Clay / Warm Cocoa Clay）────────
        // 参考柔光黏土卡片（Claymorphism / Soft UI）美学：
        // - 亮色采用纯暖黄油色相（H≈89°）的通透浅奶油桌面 #FCF7EA（彻底消除偏粉/发灰感），
        //   上方浮起纯白黏土卡片 #FFFFFF + 暖蜜糖弥散柔影 + 轻浅奶油细边（1.25px）；
        // - 品牌主色采用明快活泼的阳光蜜橙 #E0640D 配深炭墨字（借鉴高可读暖色卡片实践，
        //   避免为迁就白字而压暗成焦褐），辅以舒缓薄荷绿、信任天蓝与柔薰衣草点缀；
        // - 暗色转为温润烘焙可可黏土 + 发光黄油金高光与半透明奶油柔边。
        {
            auto reference = default_reference_palette();
            reference.neutral = nan_color_scale({
                .shade_50 = 0xFFFFFF, // 纯白黏土卡片
                .shade_100 = 0xFCF7EA, // 阳光浅奶油画布（H≈89°，不偏粉）
                .shade_200 = 0xEDE3CE, // 饼干贴片（secondary）
                .shade_300 = 0xE5D9C3, // 浅奶油细边
                .shade_400 = 0xC2B1A0, // 暗色次文本
                .shade_500 = 0x9C9286, // 中暖石
                .shade_600 = 0x7A7167, // 暖石
                .shade_700 = 0x5C554E, // 亮色次文本
                .shade_800 = 0x413730, // 暗色饼干可可
                .shade_900 = 0x2A231F, // 暗色可可黏土卡片
                .shade_950 = 0x1E1916, // 暗色深烘焙可可画布
            });
            reference.primary = nan_color_scale({
                .shade_50 = 0xFFF9F0,
                .shade_100 = 0xFFEDD5,
                .shade_200 = 0xFED7AA,
                .shade_300 = 0xF5B959, // 暗色发光黄油金
                .shade_400 = 0xF59638,
                .shade_500 = 0xEB771C,
                .shade_600 = 0xE0640D, // 亮色阳光蜜橙（配深墨字 ≥ 5.2:1，对底色 ≥ 3.2:1）
                .shade_700 = 0xB84D08,
                .shade_800 = 0x8C3A06,
                .shade_900 = 0x5C2504,
                .shade_950 = 0x1C140E, // 深炭可可 on-primary（亮/暗同源）
            });
            reference.tertiary = nan_color_scale({
                .shade_50 = 0xFAF5FF,
                .shade_100 = 0xEDE9FE,
                .shade_200 = 0xDDD6FE,
                .shade_300 = 0xB794F4, // 暗色舒缓薰衣草
                .shade_400 = 0x9F7AEA,
                .shade_500 = 0x8B5CF6,
                .shade_600 = 0x7454D9, // 亮色柔薰衣草
                .shade_700 = 0x5B3EB5,
                .shade_800 = 0x452D8C,
                .shade_900 = 0x2F1E63,
                .shade_950 = 0x1D123D,
            });

            const auto butter_light = SemanticColorSpec {
                .background = NanColor::from_hex(0xFCF7EA),
                .foreground = NanColor::from_hex(0x231F1D),
                .card = NanColor::from_hex(0xFFFFFF),
                .card_foreground = NanColor::from_hex(0x231F1D),
                .popover = NanColor::from_hex(0xFFFFFF),
                .popover_foreground = NanColor::from_hex(0x231F1D),
                .primary = NanColor::from_hex(0xE0640D),
                .primary_foreground = NanColor::from_hex(0x1C140E),
                .secondary = NanColor::from_hex(0xEDE3CE),
                .secondary_foreground = NanColor::from_hex(0x2B241E),
                .muted = NanColor::from_hex(0xF3ECE0),
                .muted_foreground = NanColor::from_hex(0x5C554E),
                .accent = NanColor::from_hex(0xFCD694),
                .accent_foreground = NanColor::from_hex(0x24190E),
                .destructive = NanColor::from_hex(0xD63B49),
                .destructive_foreground = NanColor::from_hex(0xFFFFFF),
                .border = NanColor::from_hex(0xE5D9C3),
                .input = NanColor::from_hex(0xDCCCB0),
                .ring = NanColor::from_hex(0xE0640D),
                // surface 与 card 统一为清透纯白黏土：让面板在浅黄油画布上柔和浮起。
                .surface = NanColor::from_hex(0xFFFFFF),
                .surface_foreground = NanColor::from_hex(0x231F1D),
                .surface_variant = NanColor::from_hex(0xF3ECE0),
                .surface_variant_foreground = NanColor::from_hex(0x5C554E),
                .tertiary = NanColor::from_hex(0x7454D9),
                .tertiary_foreground = NanColor::from_hex(0xFFFFFF),
                .success = NanColor::from_hex(0x0E855B),
                .success_foreground = NanColor::from_hex(0xFFFFFF),
                .warning = NanColor::from_hex(0xF0AD24),
                .warning_foreground = NanColor::from_hex(0x1F1810),
                .error = NanColor::from_hex(0xD63B49),
                .error_foreground = NanColor::from_hex(0xFFFFFF),
                .info = NanColor::from_hex(0x2563EB),
                .info_foreground = NanColor::from_hex(0xFFFFFF),
                .selection = NanColor::from_hex(0xF59E0B, 0.30F),
            };

            const auto butter_dark = SemanticColorSpec {
                .background = NanColor::from_hex(0x1E1916),
                .foreground = NanColor::from_hex(0xFAF2E6),
                .card = NanColor::from_hex(0x2A231F),
                .card_foreground = NanColor::from_hex(0xFAF2E6),
                .popover = NanColor::from_hex(0x332B26),
                .popover_foreground = NanColor::from_hex(0xFAF2E6),
                .primary = NanColor::from_hex(0xF5B959),
                .primary_foreground = NanColor::from_hex(0x231911),
                .secondary = NanColor::from_hex(0x413730),
                .secondary_foreground = NanColor::from_hex(0xF6EBDC),
                .muted = NanColor::from_hex(0x241D1A),
                .muted_foreground = NanColor::from_hex(0xC2B1A0),
                .accent = NanColor::from_hex(0x56412C),
                .accent_foreground = NanColor::from_hex(0xFFF5E6),
                .destructive = NanColor::from_hex(0xE86A74),
                .destructive_foreground = NanColor::from_hex(0x231315),
                // 暗色用低透明暖奶油细边：在任意可可表面层级上保持柔润边界，彻底告别亮褐线框。
                .border = NanColor::from_hex(0xFFF0D9, 0.14F),
                .input = NanColor::from_hex(0xFFF0D9, 0.20F),
                .ring = NanColor::from_hex(0xF5B959),
                .surface = NanColor::from_hex(0x2A231F),
                .surface_foreground = NanColor::from_hex(0xFAF2E6),
                .surface_variant = NanColor::from_hex(0x241D1A),
                .surface_variant_foreground = NanColor::from_hex(0xC2B1A0),
                .tertiary = NanColor::from_hex(0xB794F4),
                .tertiary_foreground = NanColor::from_hex(0x1E1338),
                .success = NanColor::from_hex(0x4ADE9A),
                .success_foreground = NanColor::from_hex(0x11271C),
                .warning = NanColor::from_hex(0xF5C453),
                .warning_foreground = NanColor::from_hex(0x261C0E),
                .error = NanColor::from_hex(0xE86A74),
                .error_foreground = NanColor::from_hex(0x231315),
                .info = NanColor::from_hex(0x60A5FA),
                .info_foreground = NanColor::from_hex(0x0E1F38),
                .selection = NanColor::from_hex(0xF5B959, 0.30F),
            };

            NanTokens tokens {};
            // 饱满 squircle 圆角（8/12/20）：卡片温润圆角 20px，按钮饱满友好而不成香肠胶囊。
            tokens.radius.sm = 8.0F;
            tokens.radius.md = 12.0F;
            tokens.radius.lg = 20.0F;
            // 轻柔细边（1.25/2.25）：既保留黏土描边收口，又不让侧栏 outlined 列表显得像粗方框。
            tokens.border.thin = 1.25F;
            tokens.border.medium = 2.25F;

            const auto accent_ref = ColorOperand {ToneAccentRef {.on_accent = false}};
            families.push_back(
                ThemeFamilyDefinition {
                    .name = "butter",
                    .reference = std::move(reference),
                    .policy =
                        {
                            .light_brand = ColorShade::shade_600,
                            .dark_brand = ColorShade::shade_300,
                            .light_on_brand = ColorShade::shade_950,
                            .dark_on_brand = ColorShade::shade_950,
                        },
                    .tokens = tokens,
                    .light_palette = butter_light,
                    .dark_palette = butter_dark,
                    .card_rules =
                        {
                            // 含蓄的深可可柔影（低透明 + 紧凑弥散）：亮色下呈细腻暖影不脏边，
                            // 暗色下比可可背景更深（真正接地阴影，不会在暗底上泛出橙色外发光），
                            // 嵌套多张卡片时边缘也保持利落不杂乱。
                            CardRecipeRule {
                                .shadow_color =
                                    ThemeColor::literal(NanColor::from_hex(0x1A1008, 0.09F)),
                                .shadow_offset_y = ThemeScalar::literal(3.0F),
                                .shadow_spread = ThemeScalar::literal(8.0F),
                            },
                        },
                    .button_rules =
                        {
                            // outlined 按钮采用实心卡片底 + 柔淡半透明描边，作为导航项或次级瓷片均轻盈干净。
                            ButtonRecipeRule {
                                .selector = {.treatment = ButtonTreatment::outlined},
                                .container_fill = ThemeColor::token(ColorToken::card),
                                .container_border = ThemeColor::with_alpha(
                                    ColorToken::border,
                                    ThemeScalar::literal(0.82F)
                                ),
                                .container_border_width =
                                    ThemeScalar::token(ScalarToken::border_thin),
                                .label_color = ThemeColor::token(ColorToken::foreground),
                            },
                            // tonal 按钮采用柔粉彩蜜糖底 + 正文深墨/奶油字，兼顾糖果感与极高可读性。
                            ButtonRecipeRule {
                                .selector = {.treatment = ButtonTreatment::tonal},
                                .container_fill = ThemeColor::mix(
                                    ColorOperand {ColorToken::card},
                                    accent_ref,
                                    ThemeScalar::literal(0.22F)
                                ),
                                .container_border =
                                    ThemeColor::with_alpha(accent_ref, ThemeScalar::literal(0.24F)),
                                .container_border_width = ThemeScalar::literal(1.0F),
                                .label_color = ThemeColor::token(ColorToken::foreground),
                            },
                        },
                    .badge_rules =
                        {
                            // 徽章加一圈轻柔贴片边，确保落入蜜桃高亮选中行时仍像可爱黏土贴纸一样分明。
                            BadgeRecipeRule {
                                .container_border = ThemeColor::with_alpha(
                                    ColorToken::border,
                                    ThemeScalar::literal(0.80F)
                                ),
                                .container_border_width = ThemeScalar::literal(1.0F),
                            },
                        },
                    .dialog_rules = {
                        // 暖可可遮罩 + 柔和卡片描边，让 Dialog / Sheet 与黄油黏土氛围统一。
                        DialogRecipeRule {
                            .scrim = ThemeColor::literal(NanColor::from_hex(0x1C140E, 0.42F)),
                            .panel_fill = ThemeColor::token(ColorToken::popover),
                            .panel_border = ThemeColor::token(ColorToken::border),
                            .panel_border_width = ThemeScalar::token(ScalarToken::border_thin),
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
        system.light = family.light_palette.has_value()
            ? NanColorScheme {*family.light_palette}
            : make_color_scheme(family.reference, ColorAppearance::light, family.policy);
        system.dark = family.dark_palette.has_value()
            ? NanColorScheme {*family.dark_palette}
            : make_color_scheme(family.reference, ColorAppearance::dark, family.policy);
        for (const auto& rule: family.card_rules) {
            system.components.card.rules.push_back(rule);
        }
        for (const auto& rule: family.button_rules) {
            system.components.button.rules.push_back(rule);
        }
        for (const auto& rule: family.badge_rules) {
            system.components.badge.rules.push_back(rule);
        }
        for (const auto& rule: family.dialog_rules) {
            system.components.dialog.rules.push_back(rule);
        }
        return system;
    }

    void register_default_theme_families(ThemeManager& manager) {
        for (const auto& family: default_theme_families()) {
            const auto group = ThemeFamilyId::parse("nandina:" + family.name).value();
            const auto exists =
                std::ranges::any_of(manager.catalog().families(), [&group](const auto& entry) {
                    return entry.id == group;
                });
            if (!exists) {
                auto result = manager.register_family({group, family.name});
                if (!result) {
                    throw std::logic_error(describe(result.error()));
                }
            }
            const auto system = build_family_design_system(family);
            for (auto appearance: {ColorAppearance::light, ColorAppearance::dark}) {
                const auto suffix = appearance == ColorAppearance::light ? "/light" : "/dark";
                const auto id = ThemeSchemeId::parse("nandina:" + family.name + suffix).value();
                if (manager.catalog().find(id)) {
                    continue;
                }
                auto result = manager.register_scheme(
                    {id,
                     family.name + suffix,
                     appearance,
                     group,
                     std::make_shared<const ThemeSchemeContent>(
                         ThemeSchemeContent::from(system, appearance)
                     )}
                );
                if (!result) {
                    throw std::logic_error(describe(result.error()));
                }
            }
        }
    }
} // namespace nandina::theme
