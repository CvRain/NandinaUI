// theme/theme_scheme — stable scheme identities, catalog entries and selection values.
#ifndef NANDINA_EXPERIMENT_THEME_THEME_SCHEME_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_SCHEME_HPP

#include "appearance.hpp"
#include "design_system.hpp"
#include "nan_style.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace nandina::theme
{
    enum class ThemeSchemeIdErrorKind {
        empty,
        too_long,
        missing_separator,
        invalid_namespace,
        invalid_name,
    };

    struct ThemeSchemeIdError {
        ThemeSchemeIdErrorKind kind = ThemeSchemeIdErrorKind::empty;
        std::size_t offset = 0;
    };

    class ThemeSchemeId {
    public:
        /// Empty is an invalid sentinel, never an implicit default scheme.
        ThemeSchemeId() = default;

        [[nodiscard]] static auto parse(std::string_view text)
            -> std::expected<ThemeSchemeId, ThemeSchemeIdError>;
        [[nodiscard]] auto valid() const noexcept -> bool {
            return !text_.empty();
        }
        [[nodiscard]] auto text() const noexcept -> std::string_view {
            return text_;
        }

        friend auto operator==(const ThemeSchemeId&, const ThemeSchemeId&) -> bool = default;

    private:
        explicit ThemeSchemeId(std::string text): text_(std::move(text)) {}
        std::string text_;
    };

    class ThemeFamilyId {
    public:
        ThemeFamilyId() = default;
        [[nodiscard]] static auto parse(std::string_view text)
            -> std::expected<ThemeFamilyId, ThemeSchemeIdError>;
        [[nodiscard]] auto valid() const noexcept -> bool {
            return !text_.empty();
        }
        [[nodiscard]] auto text() const noexcept -> std::string_view {
            return text_;
        }
        friend auto operator==(const ThemeFamilyId&, const ThemeFamilyId&) -> bool = default;

    private:
        explicit ThemeFamilyId(std::string text): text_(std::move(text)) {}
        std::string text_;
    };

    struct ThemeSchemeFamily {
        ThemeFamilyId id;
        std::string display_name;
    };

    /// 显式语义色覆盖：键是既有语义角色，值是作者给定的 NanColor（OKLCH）。
    /// 不新增 red/blue 等原始色 token；需要固定色值时使用本类型或参考色阶档位。
    struct SemanticColorOverride {
        ColorToken role;
        NanColor color;
    };

    enum class ThemeColorErrorKind {
        /// NanColor 构造已归一化 light/hue/alpha 并把负 chroma 归零，作者入口只需拒绝
        /// 非有限分量（NaN/inf）——不在这里静默裁剪合法输入，也不冒充色域处理。
        non_finite_color,
    };

    struct ThemeColorError {
        ThemeColorErrorKind kind = ThemeColorErrorKind::non_finite_color;
        ColorToken role = ColorToken::background;
    };

    [[nodiscard]] auto describe(const ThemeColorError& error) -> std::string_view;

    /// 兼容别名（on_* / outline* / focus_ring）映射到 shadcn 规范名；规范名原样返回。
    [[nodiscard]] auto canonical_color_token(ColorToken role) noexcept -> ColorToken;
    /// 读取已声明的语义角色（规范名或别名）；别名读取规范字段。
    [[nodiscard]] auto semantic_color(const NanColorScheme& palette, ColorToken role) noexcept
        -> const NanColor&;
    /// 写入已声明的语义角色并同步兼容别名；规范化存储值非有限时返回错误且不修改。
    [[nodiscard]] auto set_semantic_color(NanColorScheme& palette, ColorToken role, NanColor color)
        -> std::expected<void, ThemeColorError>;
    /// 批量显式覆盖：先验证全部再一次性写入；失败不修改，重复角色（含别名）最后一项生效。
    [[nodiscard]] auto
    apply_semantic_colors(NanColorScheme& palette, std::span<const SemanticColorOverride> overrides)
        -> std::expected<void, ThemeColorError>;

    /// One appearance's complete immutable content; DesignSystem is only a renderer adapter.
    struct ThemeSchemeContent {
        NanTokens tokens;
        NanColorScheme palette;
        TypographyRoles typography;
        ComponentRecipes components;
        /// 派生来源的 primitive palette（可选）；palette 仍是唯一权威语义事实。
        std::optional<NanReferencePalette> reference;

        [[nodiscard]] static auto from(const DesignSystem& system, ColorAppearance appearance)
            -> ThemeSchemeContent;

        /// 从参考色阶派生完整内容：palette 由色阶生成、记录来源；其余字段取默认设计系统模板。
        [[nodiscard]] static auto from_reference(
            const NanReferencePalette& reference,
            ColorAppearance appearance,
            PaletteVariantPolicy policy = {}
        ) -> ThemeSchemeContent;

        /// 从显式语义色声明构造完整内容；不伪造参考色阶来源。
        [[nodiscard]] static auto from_semantic_colors(const SemanticColorSpec& spec)
            -> ThemeSchemeContent;

        [[nodiscard]] auto design_system() const -> DesignSystem;
    };

    struct UnregisteredThemeSource {
        std::shared_ptr<const ThemeSchemeContent> content;
        ColorAppearance appearance = ColorAppearance::light;

        friend auto operator==(const UnregisteredThemeSource&, const UnregisteredThemeSource&)
            -> bool = default;
    };

    using FixedThemeSource = std::variant<ThemeSchemeId, UnregisteredThemeSource>;

    struct ThemeScheme {
        ThemeSchemeId id;
        std::string display_name;
        ColorAppearance appearance = ColorAppearance::light;
        std::optional<ThemeFamilyId> family;
        std::shared_ptr<const ThemeSchemeContent> content;

        /// Content equality is immutable snapshot identity; recipes are not deep-compared.
        friend auto operator==(const ThemeScheme&, const ThemeScheme&) -> bool = default;
    };

    struct FixedThemeSelection {
        FixedThemeSource source;

        friend auto operator==(const FixedThemeSelection&, const FixedThemeSelection&)
            -> bool = default;
    };

    struct AppearanceThemeSelection {
        ThemeSchemeId light;
        ThemeSchemeId dark;
        ThemePreference preference = ThemePreference::system;

        friend auto operator==(const AppearanceThemeSelection&, const AppearanceThemeSelection&)
            -> bool = default;
    };

    using ThemeSelection = std::variant<FixedThemeSelection, AppearanceThemeSelection>;

    enum class ThemeCatalogErrorKind {
        invalid_scheme_id,
        invalid_family_id,
        duplicate_scheme,
        duplicate_family,
        scheme_not_found,
        family_not_found,
        scheme_in_use,
        family_in_use,
        null_content,
        unknown_family,
        appearance_mismatch,
    };

    struct ThemeCatalogError {
        ThemeCatalogErrorKind kind = ThemeCatalogErrorKind::invalid_scheme_id;
        /// Present for scheme operations and when a scheme references a missing family.
        std::optional<ThemeSchemeId> scheme;
        /// Present for family operations and when a scheme references an unknown family.
        std::optional<ThemeFamilyId> family;
    };

    enum class ThemeSelectionErrorKind {
        unknown_scheme,
        null_content,
        appearance_mismatch,
    };

    struct ThemeSelectionError {
        ThemeSelectionErrorKind kind = ThemeSelectionErrorKind::unknown_scheme;
        ThemeSchemeId id;
    };

    /// Value directory used by ThemeManager; a staged copy permits failure-atomic commits.
    class ThemeSchemeCatalog {
    public:
        ThemeSchemeCatalog() = default;

        [[nodiscard]] auto register_family(ThemeSchemeFamily family)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto remove_family(const ThemeFamilyId& id)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto families() const -> std::vector<ThemeSchemeFamily>;
        [[nodiscard]] auto contains_family(const ThemeFamilyId& id) const -> bool;

        [[nodiscard]] auto register_scheme(ThemeScheme scheme)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto replace_scheme(ThemeScheme scheme)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto remove_scheme(const ThemeSchemeId& id)
            -> std::expected<void, ThemeCatalogError>;
        /// Borrowed entry; replacement may invalidate all entry pointers in this value catalog.
        [[nodiscard]] auto find(const ThemeSchemeId& id) const -> const ThemeScheme*;
        [[nodiscard]] auto schemes() const -> std::vector<ThemeScheme>;
        [[nodiscard]] auto schemes_in_family(const ThemeFamilyId& id) const
            -> std::vector<ThemeScheme>;

        /// Directory revision advances for actual add/replace/remove operations.
        /// Selection and equivalent entry replacement leave it unchanged.
        [[nodiscard]] auto version() const noexcept -> std::uint64_t;

        [[nodiscard]] auto select(ThemeSelection selection, ColorAppearance system_appearance)
            -> std::expected<void, ThemeSelectionError>;
        [[nodiscard]] auto selection() const noexcept -> const std::optional<ThemeSelection>&;
        [[nodiscard]] auto active() const noexcept -> const ThemeScheme*;
        [[nodiscard]] auto active_content() const noexcept -> const ThemeSchemeContent*;
        [[nodiscard]] auto active_appearance() const noexcept -> ColorAppearance;

    private:
        struct Resolved {
            const ThemeScheme* scheme = nullptr;
            std::shared_ptr<const ThemeSchemeContent> anonymous_content;
            ColorAppearance appearance = ColorAppearance::light;
        };
        [[nodiscard]] auto
        resolve(const ThemeSelection& selection, ColorAppearance system_appearance) const
            -> std::expected<Resolved, ThemeSelectionError>;
        [[nodiscard]] auto uses(const ThemeSchemeId& id) const -> bool;

        std::map<std::string, ThemeSchemeFamily, std::less<>> families_;
        std::map<std::string, ThemeScheme, std::less<>> schemes_;
        std::uint64_t version_ = 0;
        std::optional<ThemeSelection> selection_;
        std::optional<ThemeSchemeId> active_id_;
        std::shared_ptr<const ThemeSchemeContent> active_anonymous_content_;
        ColorAppearance active_appearance_ = ColorAppearance::light;
        ColorAppearance system_appearance_ = ColorAppearance::light;
    };

    // ─── 统一作者入口：派生与结构校验（第 4 步） ────────────────────────────
    //
    // 派生总是深拷贝内容快照：修改派生方案不会污染基础方案，后续替换基础方案也不会
    // 隐式重写已注册的派生方案（重新派生是显式动作，见 D2 §6.3）。
    [[nodiscard]] auto derive_content(const ThemeSchemeContent& base) -> ThemeSchemeContent;
    /// 继承基础方案的外观分类与家族归属，内容为独立深拷贝。
    [[nodiscard]] auto
    derive_scheme(const ThemeScheme& base, ThemeSchemeId id, std::string display_name)
        -> ThemeScheme;

    enum class ThemeSchemeErrorKind {
        invalid_id,
        empty_display_name,
        null_content,
        duplicate_id,
        unknown_family,
    };

    struct ThemeSchemeError {
        ThemeSchemeErrorKind kind = ThemeSchemeErrorKind::invalid_id;
        ThemeSchemeId id;
    };

    /// 结构校验（不含配色对比度门槛，后者见 D2 §6.4/4a 子项）：
    /// ID 有效、显示名非空、内容非空、未与目录既有项冲突、家族存在。
    [[nodiscard]] auto validate_scheme(const ThemeScheme& scheme, const ThemeSchemeCatalog& catalog)
        -> std::expected<void, ThemeSchemeError>;
    [[nodiscard]] auto describe(const ThemeSchemeError& error) -> std::string;

    [[nodiscard]] auto describe(const ThemeSchemeIdError& error) -> std::string;
    [[nodiscard]] auto describe(const ThemeCatalogError& error) -> std::string;
    [[nodiscard]] auto describe(const ThemeSelectionError& error) -> std::string;
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_SCHEME_HPP
