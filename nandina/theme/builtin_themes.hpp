//
// theme/builtin_themes - framework built-in theme families.
//
// A family is authored as reference color scales + a variant policy + tokens,
// then compiled into a full DesignSystem snapshot (embedded light/dark). The
// family catalog is registered into ThemeManager so applications select one
// coherent look by name and override it per brand.
//

#ifndef NANDINA_EXPERIMENT_THEME_BUILTIN_THEMES_HPP
#define NANDINA_EXPERIMENT_THEME_BUILTIN_THEMES_HPP

#include "design_system.hpp"
#include "theme.hpp"

#include <string>
#include <vector>

namespace nandina::theme
{
    class ThemeManager;

    /** 内置主题族定义：一组参考色阶 + 变体策略 + tokens + 配方规则定制。 */
    struct ThemeFamilyDefinition {
        std::string name;
        NanReferencePalette reference;
        PaletteVariantPolicy policy;
        NanTokens tokens;
        /// 追加到 Card 配方的规则（如 butter 的软阴影）；后续可扩展其它组件。
        std::vector<CardRecipeRule> card_rules;
    };

    /** @return 框架内置主题族目录（butter / fluent / material）。 */
    [[nodiscard]] auto default_theme_families() -> std::vector<ThemeFamilyDefinition>;

    /** 从主题族定义编译 DesignSystem（复用默认 typography 与 recipe，替换 tokens + light/dark）。 */
    [[nodiscard]] auto build_family_design_system(const ThemeFamilyDefinition& family)
        -> DesignSystem;

    /** 把所有内置主题族注册进 ThemeManager（幂等）。 */
    void register_default_theme_families(ThemeManager& manager);

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_BUILTIN_THEMES_HPP
