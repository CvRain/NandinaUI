//
// Created by cvrain on 2026/4/28.
//
module;

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module nandina.theme.nan_theme;

import nandina.foundation.color;
import nandina.theme.nan_theme_types;
import nandina.theme.nan_primitive_tokens;
import nandina.theme.nan_color_palette;
import nandina.theme.nan_style;

/**
 * nandina.theme.nan_theme
 *
 * 主题核心。
 *
 * NanTheme 聚合了主题的所有素材：
 *   - 语义颜色调色板 (NanColorPalette)
 *   - 原始设计 Token (NanPrimitiveTokens)
 *   - 色彩方案开关 (NanColorScheme)
 *
 * ThemeManager 管理多个注册主题并提供切换通知（基于内部简易信号，无 reactive 依赖）。
 */

export namespace nandina::theme {

// ═════════════════════════════════════════════════════════════════════════════
// § NanTheme — 单个主题定义
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 单个主题的完整定义。
 *
 * 包含该主题的所有 Token 配置。每个注册的主题有一个名称（如 "default"、"catppuccin"）。
 *
 * 对于名称相同但需要 light/dark 区分的场景，
 * 同一个 NanTheme 中包含两套色值，通过 `scheme()` 切换。
 */
class NanTheme {
public:
    /**
     * @brief 构造一个命名主题。
     *
     * @param name        主题名称（如 "default"、"catppuccin"）
     * @param palette     语义颜色调色板
     * @param tokens      原始设计 Token
     * @param scheme      初始色彩方案
     */
    explicit NanTheme(
        std::string name,
        NanColorPalette palette = NanColorPalette{},
        NanPrimitiveTokens tokens = NanPrimitiveTokens{},
        NanColorScheme scheme = NanColorScheme::light
    );

    /// 主题名称
    [[nodiscard]] auto name() const noexcept -> const std::string&;

    /// 当前色彩方案
    [[nodiscard]] auto scheme() const noexcept -> NanColorScheme;

    /// 设置为浅色/深色方案
    auto set_scheme(NanColorScheme scheme) noexcept -> void;

    /// 切换 light ↔ dark
    auto toggle_scheme() noexcept -> void;

    /**
     * @brief 获取指定语义角色的颜色值（依据当前方案）。
     */
    [[nodiscard]] auto color(NanColorRole role) const -> const NanColor&;

    /// 获取调色板（允许修改覆盖色值）
    [[nodiscard]] auto palette() noexcept -> NanColorPalette&;

    /// 获取调色板（只读）
    [[nodiscard]] auto palette() const noexcept -> const NanColorPalette&;

    /// 获取原始 Token（允许修改覆盖 Token 值）
    [[nodiscard]] auto tokens() noexcept -> NanPrimitiveTokens&;

    /// 获取原始 Token（只读）
    [[nodiscard]] auto tokens() const noexcept -> const NanPrimitiveTokens&;

private:
    std::string       name_;
    NanColorPalette   palette_;
    NanPrimitiveTokens tokens_;
    NanColorScheme    scheme_;
};

// ═════════════════════════════════════════════════════════════════════════════
// § ThemeManager — 主题注册与切换管理器
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 全局主题管理器。
 *
 * 职责：
 *   1. 注册/移除命名主题
 *   2. 设置当前活跃主题
 *   3. 切换色彩方案（light/dark）
 *   4. 主题变更时通知已注册的监听器
 *
 * 线程安全：是（互斥锁保护）。
 *
 * 响应式集成：外部可通过 `on_changed(callback)` 注册监听器，
 *             在 theme/scheme 变更时被调用，再桥接到 reactive::Effect。
 *
 * 使用方式：
 * @code
 *   auto& mgr = ThemeManager::instance();
 *
 *   // 注册自定义主题
 *   mgr.register_theme(NanTheme{"my_theme", my_palette, my_tokens, NanColorScheme::light});
 *
 *   // 激活主题
 *   mgr.activate("my_theme");
 *
 *   // 注册变更监听器
 *   auto token = mgr.on_changed([](const std::string& theme_name) {
 *       // 主题已切换，触发重绘
 *   });
 *
 *   // 切换亮/暗模式
 *   mgr.set_scheme(NanColorScheme::dark);
 *
 *   // 注销监听器
 *   token.disconnect();
 * @endcode
 */
class ThemeManager {
public:
    /// 变更监听器类型
    using ChangeListener = std::function<void(const std::string& /* theme_name */)>;

    /**
     * @brief 监听器连接令牌。
     *
     * 析构时自动断开连接。也可手动调用 disconnect()。
     */
    class Connection {
    public:
        Connection() = default;
        ~Connection() { disconnect(); }

        Connection(const Connection&) = delete;
        auto operator=(const Connection&) -> Connection& = delete;

        Connection(Connection&& other) noexcept
            : mgr_(other.mgr_), id_(other.id_) {
            other.mgr_ = nullptr;
        }

        auto operator=(Connection&& other) noexcept -> Connection& {
            if (this != &other) {
                disconnect();
                mgr_ = other.mgr_;
                id_  = other.id_;
                other.mgr_ = nullptr;
            }
            return *this;
        }

        /// 手动断开连接
        auto disconnect() -> void;

    private:
        friend class ThemeManager;
        Connection(ThemeManager* mgr, std::size_t id) : mgr_(mgr), id_(id) {}

        ThemeManager* mgr_ = nullptr;
        std::size_t   id_  = 0;
    };

    ThemeManager(const ThemeManager&) = delete;
    auto operator=(const ThemeManager&) -> ThemeManager& = delete;

    ThemeManager(ThemeManager&&) = delete;
    auto operator=(ThemeManager&&) -> ThemeManager& = delete;

    /// 获取全局单例
    [[nodiscard]] static auto instance() -> ThemeManager&;

    // ── 主题注册 ──

    /**
     * @brief 注册一个主题。
     *
     * 如果同名主题已存在，会被覆盖。
     */
    auto register_theme(NanTheme theme) -> void;

    /**
     * @brief 移除指定名称的主题。
     * @return 是否成功移除
     */
    auto unregister(const std::string& name) -> bool;

    // ── 主题激活 ──

    /**
     * @brief 激活一个已注册的主题。
     *
     * 主题变更会通知所有监听器。
     *
     * @return 是否成功激活（主题必须已注册）
     */
    auto activate(const std::string& name) -> bool;

    /// 当前活跃的主题名称（按值返回，线程安全）
    [[nodiscard]] auto active_name() const -> std::string;

    /// 当前活跃主题（只读）
    [[nodiscard]] auto active() const -> const NanTheme*;

    /// 当前活跃主题（可变）
    [[nodiscard]] auto active() -> NanTheme*;

    // ── 色彩方案 ──

    /// 设置当前活跃主题的色彩方案
    auto set_scheme(NanColorScheme scheme) -> void;

    /// 切换当前活跃主题的色彩方案 (light ↔ dark)
    auto toggle_scheme() -> void;

    /// 当前色彩方案
    [[nodiscard]] auto scheme() const -> NanColorScheme;

    // ── 颜色快捷访问 ──

    /**
     * @brief 从当前活跃主题获取语义色。
     *
     * 便捷方法，等价于 active()->color(role)。
     */
    [[nodiscard]] auto color(NanColorRole role) const -> const NanColor&;

    /// 从当前活跃主题解析组件样式原语
    [[nodiscard]] auto resolved_style() const -> NanStylePrimitives;

    // ── 监听器 ──

    /**
     * @brief 注册主题变更监听器。
     *
     * 每次 activate / set_scheme / toggle_scheme 调用后，如果实际发生了变更则通知。
     * 参数：变更后的主题名称。
     *
     * @return Connection 对象，析构时自动断开。
     */
    [[nodiscard]] auto on_changed(ChangeListener listener) -> Connection;

private:
    ThemeManager();
    ~ThemeManager() = default;

    [[nodiscard]] static auto build_style(const NanTheme& theme) -> NanStylePrimitives;

    /// 内部：触发通知
    auto notify_changed() -> void;

    /// 断开指定 ID 的监听器
    auto disconnect(std::size_t id) -> void;

    mutable std::mutex                         mutex_;
    std::unordered_map<std::string, NanTheme>   themes_;
    std::string                                active_name_;
    std::unordered_map<std::size_t, ChangeListener> listeners_;
    std::size_t                                next_listener_id_ = 0;
};

// ═════════════════════════════════════════════════════════════════════════════
// § Inline 实现
// ═════════════════════════════════════════════════════════════════════════════

// ── NanTheme ──

inline NanTheme::NanTheme(
    std::string name,
    NanColorPalette palette,
    NanPrimitiveTokens tokens,
    NanColorScheme scheme
)
    : name_(std::move(name))
    , palette_(std::move(palette))
    , tokens_(std::move(tokens))
    , scheme_(scheme) {}

inline auto NanTheme::name() const noexcept -> const std::string& {
    return name_;
}

inline auto NanTheme::scheme() const noexcept -> NanColorScheme {
    return scheme_;
}

inline auto NanTheme::set_scheme(NanColorScheme scheme) noexcept -> void {
    scheme_ = scheme;
}

inline auto NanTheme::toggle_scheme() noexcept -> void {
    scheme_ = (scheme_ == NanColorScheme::light) ? NanColorScheme::dark : NanColorScheme::light;
}

inline auto NanTheme::color(NanColorRole role) const -> const NanColor& {
    return palette_.get(role, scheme_);
}

inline auto NanTheme::palette() noexcept -> NanColorPalette& {
    return palette_;
}

inline auto NanTheme::palette() const noexcept -> const NanColorPalette& {
    return palette_;
}

inline auto NanTheme::tokens() noexcept -> NanPrimitiveTokens& {
    return tokens_;
}

inline auto NanTheme::tokens() const noexcept -> const NanPrimitiveTokens& {
    return tokens_;
}

// ── ThemeManager ──

inline ThemeManager::ThemeManager()
    : active_name_("default") {
    themes_.emplace("default", NanTheme{"default"});
    if (const auto it = themes_.find(active_name_); it != themes_.end()) {
        NanStylePrimitives::set_current(build_style(it->second));
    }
}

inline auto ThemeManager::instance() -> ThemeManager& {
    static ThemeManager mgr;
    return mgr;
}

inline auto ThemeManager::register_theme(NanTheme theme) -> void {
    bool should_sync = false;
    std::optional<NanStylePrimitives> resolved;
    std::lock_guard lock(mutex_);
    const auto name = std::string{theme.name()};
    should_sync = (name == active_name_);
    themes_.insert_or_assign(name, std::move(theme));
    if (should_sync) {
        resolved = build_style(themes_.at(active_name_));
    }
    if (resolved.has_value()) {
        NanStylePrimitives::set_current(*resolved);
    }
}

inline auto ThemeManager::unregister(const std::string& name) -> bool {
    std::lock_guard lock(mutex_);
    if (name == active_name_) {
        return false;
    }
    return themes_.erase(name) > 0;
}

inline auto ThemeManager::activate(const std::string& name) -> bool {
    std::string changed_name;
    std::optional<NanStylePrimitives> resolved;
    {
        std::lock_guard lock(mutex_);
        if (!themes_.contains(name)) {
            return false;
        }
        if (active_name_ == name) {
            return true;
        }
        active_name_ = name;
        changed_name = name;
        resolved = build_style(themes_.at(active_name_));
    }
    if (resolved.has_value()) {
        NanStylePrimitives::set_current(*resolved);
    }
    if (!changed_name.empty()) {
        notify_changed();
    }
    return true;
}

inline auto ThemeManager::active_name() const -> std::string {
    std::lock_guard lock(mutex_);
    return active_name_;
}

inline auto ThemeManager::active() const -> const NanTheme* {
    std::lock_guard lock(mutex_);
    const auto it = themes_.find(active_name_);
    if (it != themes_.end()) {
        return &it->second;
    }
    return nullptr;
}

inline auto ThemeManager::active() -> NanTheme* {
    std::lock_guard lock(mutex_);
    const auto it = themes_.find(active_name_);
    if (it != themes_.end()) {
        return const_cast<NanTheme*>(&it->second);
    }
    return nullptr;
}

inline auto ThemeManager::set_scheme(NanColorScheme scheme) -> void {
    bool changed = false;
    std::optional<NanStylePrimitives> resolved;
    {
        std::lock_guard lock(mutex_);
        const auto it = themes_.find(active_name_);
        if (it == themes_.end()) {
            return;
        }
        auto& t = it->second;
        if (t.scheme() == scheme) {
            return;
        }
        t.set_scheme(scheme);
        changed = true;
        resolved = build_style(t);
    }
    if (resolved.has_value()) {
        NanStylePrimitives::set_current(*resolved);
    }
    if (changed) {
        notify_changed();
    }
}

inline auto ThemeManager::toggle_scheme() -> void {
    bool changed = false;
    std::optional<NanStylePrimitives> resolved;
    {
        std::lock_guard lock(mutex_);
        const auto it = themes_.find(active_name_);
        if (it == themes_.end()) {
            return;
        }
        it->second.toggle_scheme();
        changed = true;
        resolved = build_style(it->second);
    }
    if (resolved.has_value()) {
        NanStylePrimitives::set_current(*resolved);
    }
    if (changed) {
        notify_changed();
    }
}

inline auto ThemeManager::scheme() const -> NanColorScheme {
    std::lock_guard lock(mutex_);
    const auto it = themes_.find(active_name_);
    if (it != themes_.end()) {
        return it->second.scheme();
    }
    return NanColorScheme::light;
}

inline auto ThemeManager::color(NanColorRole role) const -> const NanColor& {
    static const NanColor fallback = [] {
        // 返回一个透明的黑色 Oklab
        return NanColor::from(NanRgb{0u, 0u, 0u, 0u});
    }();
    std::lock_guard lock(mutex_);
    const auto it = themes_.find(active_name_);
    if (it != themes_.end()) {
        return it->second.color(role);
    }
    return fallback;
}

inline auto ThemeManager::resolved_style() const -> NanStylePrimitives {
    std::lock_guard lock(mutex_);
    const auto it = themes_.find(active_name_);
    if (it != themes_.end()) {
        return build_style(it->second);
    }
    return NanStylePrimitives::default_style();
}

inline auto ThemeManager::on_changed(ChangeListener listener) -> Connection {
    std::lock_guard lock(mutex_);
    const auto id = next_listener_id_++;
    listeners_[id] = std::move(listener);
    return Connection{this, id};
}

inline auto ThemeManager::notify_changed() -> void {
    // 在锁外快照监听器列表，避免通知期间死锁
    decltype(listeners_) snapshot;
    std::string active;
    {
        std::lock_guard lock(mutex_);
        snapshot = listeners_;
        active = active_name_;
    }
    for (const auto& [id, listener] : snapshot) {
        if (listener) {
            listener(active);
        }
    }
}

inline auto ThemeManager::disconnect(std::size_t id) -> void {
    std::lock_guard lock(mutex_);
    listeners_.erase(id);
}

// ── Connection ──

inline auto ThemeManager::Connection::disconnect() -> void {
    if (mgr_) {
        mgr_->disconnect(id_);
        mgr_ = nullptr;
    }
}

inline auto ThemeManager::build_style(const NanTheme& theme) -> NanStylePrimitives {
    auto style = NanStylePrimitives::default_style();
    const auto& tokens = theme.tokens();

    auto map_weight = [](const NanFontWeight weight) -> text::NanFontWeight {
        switch (weight) {
        case NanFontWeight::thin:       return text::NanFontWeight::thin;
        case NanFontWeight::extraLight: return text::NanFontWeight::extraLight;
        case NanFontWeight::light:      return text::NanFontWeight::light;
        case NanFontWeight::regular:    return text::NanFontWeight::regular;
        case NanFontWeight::medium:     return text::NanFontWeight::medium;
        case NanFontWeight::semiBold:   return text::NanFontWeight::semiBold;
        case NanFontWeight::bold:       return text::NanFontWeight::bold;
        case NanFontWeight::extraBold:  return text::NanFontWeight::extraBold;
        case NanFontWeight::black:      return text::NanFontWeight::black;
        }
        return text::NanFontWeight::regular;
    };

    const auto role = [&theme](const NanColorRole color_role) -> const NanColor& {
        return theme.color(color_role);
    };

    const auto transparent = NanColor::from(NanRgb{0, 0, 0, 0});

    const auto make_button_family = [&tokens, &role, &transparent](
        const NanColorRole filled_bg_role,
        const NanColorRole filled_hover_role,
        const NanColorRole filled_pressed_role,
        const NanColorRole filled_text_role,
        const NanColorRole tonal_bg_role,
        const NanColorRole tonal_hover_role,
        const NanColorRole tonal_pressed_role,
        const NanColorRole tonal_text_role,
        const NanColorRole accent_text_role,
        const NanColorRole accent_border_role) -> NanButtonStyle::ColorFamilyStyle {
        return NanButtonStyle::ColorFamilyStyle{
            .filled = NanButtonStyle::PresetStyle{
                .bg = role(filled_bg_role),
                .bg_hover = role(filled_hover_role),
                .bg_pressed = role(filled_pressed_role),
                .bg_disabled = role(NanColorRole::surfaceVariant),
                .text = role(filled_text_role),
                .text_disabled = role(NanColorRole::outline),
                .border = transparent,
                .border_width = tokens.border.none,
            },
            .tonal = NanButtonStyle::PresetStyle{
                .bg = role(tonal_bg_role),
                .bg_hover = role(tonal_hover_role),
                .bg_pressed = role(tonal_pressed_role),
                .bg_disabled = role(NanColorRole::surfaceVariant),
                .text = role(tonal_text_role),
                .text_disabled = role(NanColorRole::outline),
                .border = transparent,
                .border_width = tokens.border.none,
            },
            .outlined = NanButtonStyle::PresetStyle{
                .bg = role(NanColorRole::surface),
                .bg_hover = role(NanColorRole::surfaceVariant),
                .bg_pressed = role(tonal_bg_role),
                .bg_disabled = role(NanColorRole::surface),
                .text = role(accent_text_role),
                .text_disabled = role(NanColorRole::outline),
                .border = role(accent_border_role),
                .border_width = tokens.border.thin,
            },
            .ghost = NanButtonStyle::PresetStyle{
                .bg = transparent,
                .bg_hover = role(NanColorRole::surfaceVariant),
                .bg_pressed = role(tonal_bg_role),
                .bg_disabled = transparent,
                .text = role(accent_text_role),
                .text_disabled = role(NanColorRole::outline),
                .border = transparent,
                .border_width = tokens.border.none,
            },
            .destructive = NanButtonStyle::PresetStyle{
                .bg = role(filled_bg_role),
                .bg_hover = role(filled_hover_role),
                .bg_pressed = role(filled_pressed_role),
                .bg_disabled = role(NanColorRole::surfaceVariant),
                .text = role(filled_text_role),
                .text_disabled = role(NanColorRole::outline),
                .border = transparent,
                .border_width = tokens.border.none,
            },
            .link = NanButtonStyle::PresetStyle{
                .bg = transparent,
                .bg_hover = transparent,
                .bg_pressed = transparent,
                .bg_disabled = transparent,
                .text = role(accent_text_role),
                .text_disabled = role(NanColorRole::outline),
                .border = transparent,
                .border_width = tokens.border.none,
            },
        };
    };

    const auto make_input_family = [&role](
        const NanColorRole bg_role,
        const NanColorRole border_role,
        const NanColorRole border_focus_role,
        const NanColorRole font_role,
        const NanColorRole placeholder_role) -> NanInputStyle::ColorFamilyStyle {
        return NanInputStyle::ColorFamilyStyle{
            .font_color = role(font_role),
            .placeholder_font_color = role(placeholder_role),
            .bg = role(bg_role),
            .border = role(border_role),
            .border_focus = role(border_focus_role),
        };
    };

    const auto make_progress_family = [&role](
        const NanColorRole track_role,
        const NanColorRole fill_role) -> NanProgressStyle::ColorFamilyStyle {
        return NanProgressStyle::ColorFamilyStyle{
            .track_bg = role(track_role),
            .fill = role(fill_role),
        };
    };

    const auto make_tag_family = [&role](
        const NanColorRole bg_role,
        const NanColorRole text_role,
        const NanColorRole border_role) -> NanTagStyle::ColorFamilyStyle {
        return NanTagStyle::ColorFamilyStyle{
            .bg = role(bg_role),
            .text = role(text_role),
            .border = role(border_role),
            .bg_disabled = role(NanColorRole::surfaceVariant),
            .text_disabled = role(NanColorRole::outline),
            .border_disabled = role(NanColorRole::outlineVariant),
        };
    };

    style.spacing = tokens.spacing;
    style.radius = tokens.radius;
    style.elevation = tokens.elevation;
    style.opacity = tokens.opacity;
    style.typography = tokens.typography;

    style.text.font_size = tokens.typography.body_medium.font_size;
    style.text.font_weight = map_weight(tokens.typography.body_medium.font_weight);
    style.text.font_color = role(NanColorRole::onSurface);
    style.text.overflow = text::TextOverflow::wrap;
    style.text.single_line = false;
    style.text.max_lines = 0;

    style.label.font_size = tokens.typography.body_medium.font_size;
    style.label.font_weight = map_weight(tokens.typography.body_medium.font_weight);
    style.label.font_color = role(NanColorRole::onSurfaceVariant);
    style.label.disabled_font_color = role(NanColorRole::outline);
    style.label.error_font_color = role(NanColorRole::error);
    style.label.required_indicator_color = role(NanColorRole::error);
    style.label.required_indicator_gap = tokens.spacing.xsmall;

    style.button.corner_radius = tokens.radius.small;
    style.button.font_size = tokens.typography.label_large.font_size;
    style.button.font_weight = map_weight(tokens.typography.label_large.font_weight);
    style.button.font_color = role(NanColorRole::onPrimary);
    style.button.color_variant = ColorVariant::inherit;
    style.button.primary_family = make_button_family(
        NanColorRole::primary,
        NanColorRole::primaryContainer,
        NanColorRole::primary,
        NanColorRole::onPrimary,
        NanColorRole::primaryContainer,
        NanColorRole::primary,
        NanColorRole::primary,
        NanColorRole::onPrimaryContainer,
        NanColorRole::primary,
        NanColorRole::primary);
    style.button.secondary_family = make_button_family(
        NanColorRole::secondary,
        NanColorRole::secondaryContainer,
        NanColorRole::secondary,
        NanColorRole::onSecondary,
        NanColorRole::secondaryContainer,
        NanColorRole::secondary,
        NanColorRole::secondary,
        NanColorRole::onSecondaryContainer,
        NanColorRole::secondary,
        NanColorRole::secondary);
    style.button.neutral_family = make_button_family(
        NanColorRole::onSurfaceVariant,
        NanColorRole::outlineVariant,
        NanColorRole::onSurfaceVariant,
        NanColorRole::surface,
        NanColorRole::surfaceVariant,
        NanColorRole::outlineVariant,
        NanColorRole::surfaceVariant,
        NanColorRole::onSurfaceVariant,
        NanColorRole::onSurfaceVariant,
        NanColorRole::outline);
    style.button.destructive_family = make_button_family(
        NanColorRole::error,
        NanColorRole::errorContainer,
        NanColorRole::error,
        NanColorRole::onError,
        NanColorRole::errorContainer,
        NanColorRole::error,
        NanColorRole::error,
        NanColorRole::onErrorContainer,
        NanColorRole::error,
        NanColorRole::error);

    style.button.filled = style.button.primary_family.filled;
    style.button.tonal = style.button.secondary_family.tonal;
    style.button.outlined = NanButtonStyle::PresetStyle{
        .bg = role(NanColorRole::surface),
        .bg_hover = role(NanColorRole::surfaceVariant),
        .bg_pressed = role(NanColorRole::secondaryContainer),
        .bg_disabled = role(NanColorRole::surface),
        .text = role(NanColorRole::primary),
        .text_disabled = role(NanColorRole::outline),
        .border = role(NanColorRole::outline),
        .border_width = tokens.border.thin,
    };
    style.button.ghost = NanButtonStyle::PresetStyle{
        .bg = transparent,
        .bg_hover = role(NanColorRole::surfaceVariant),
        .bg_pressed = role(NanColorRole::secondaryContainer),
        .bg_disabled = transparent,
        .text = role(NanColorRole::primary),
        .text_disabled = role(NanColorRole::outline),
        .border = transparent,
        .border_width = tokens.border.none,
    };
    style.button.destructive = style.button.destructive_family.destructive;
    style.button.link = style.button.primary_family.link;

    style.button.xs.height = 24.0f;
    style.button.xs.font_size = tokens.typography.label_small.font_size;
    style.button.xs.padding_h = tokens.spacing.small;
    style.button.xs.padding_v = tokens.spacing.xxsmall;
    style.button.xs.gap = tokens.spacing.xsmall;
    style.button.xs.icon_size = 14.0f;

    style.button.sm.height = 32.0f;
    style.button.sm.font_size = tokens.typography.label_medium.font_size;
    style.button.sm.padding_h = tokens.spacing.medium;
    style.button.sm.padding_v = tokens.spacing.xsmall;
    style.button.sm.gap = tokens.spacing.small - tokens.spacing.xxsmall;
    style.button.sm.icon_size = 16.0f;

    style.button.md.height = 40.0f;
    style.button.md.font_size = tokens.typography.label_large.font_size;
    style.button.md.padding_h = tokens.spacing.large;
    style.button.md.padding_v = tokens.spacing.small;
    style.button.md.gap = tokens.spacing.small;
    style.button.md.icon_size = 18.0f;

    style.button.lg.height = 48.0f;
    style.button.lg.font_size = tokens.typography.title_small.font_size;
    style.button.lg.padding_h = tokens.spacing.xlarge;
    style.button.lg.padding_v = tokens.spacing.medium;
    style.button.lg.gap = tokens.spacing.medium - tokens.spacing.xxsmall;
    style.button.lg.icon_size = 22.0f;

    style.button.icon.height = 40.0f;
    style.button.icon.font_size = 0.0f;
    style.button.icon.padding_h = 0.0f;
    style.button.icon.padding_v = 0.0f;
    style.button.icon.gap = 0.0f;
    style.button.icon.icon_size = 20.0f;
    style.button.icon.square = true;

    style.card.bg = role(NanColorRole::surface);
    style.card.border = role(NanColorRole::outlineVariant);
    style.card.corner_radius = tokens.radius.small;
    style.card.border_width = tokens.border.thin;
    style.card.padding = geometry::NanInsets{tokens.spacing.large};
    style.card.title_font_size = tokens.typography.title_medium.font_size;
    style.card.title_font_weight = map_weight(tokens.typography.title_medium.font_weight);
    style.card.title_font_color = role(NanColorRole::onSurface);

    style.panel.bg = role(NanColorRole::surface);
    style.panel.border = role(NanColorRole::outlineVariant);
    style.panel.corner_radius = tokens.radius.small;
    style.panel.border_width = tokens.border.thin;
    style.panel.padding = geometry::NanInsets{tokens.spacing.medium};
    style.panel.title_font_size = tokens.typography.title_small.font_size;
    style.panel.title_font_weight = map_weight(tokens.typography.title_small.font_weight);
    style.panel.title_font_color = role(NanColorRole::onSurfaceVariant);

    style.input.font_size = tokens.typography.body_medium.font_size;
    style.input.font_weight = map_weight(tokens.typography.body_medium.font_weight);
    style.input.font_color = role(NanColorRole::onSurface);
    style.input.placeholder_font_size = tokens.typography.body_medium.font_size;
    style.input.placeholder_font_color = role(NanColorRole::outline);
    style.input.color_variant = ColorVariant::inherit;
    style.input.bg = role(NanColorRole::surface);
    style.input.border = role(NanColorRole::outlineVariant);
    style.input.border_focus = role(NanColorRole::primary);
    style.input.corner_radius = tokens.radius.small;
    style.input.border_width = tokens.border.thin;
    style.input.padding = geometry::NanInsets{tokens.spacing.large, tokens.spacing.medium - tokens.spacing.xxsmall, tokens.spacing.large, tokens.spacing.medium - tokens.spacing.xxsmall};
    style.input.secondary_family = make_input_family(
        NanColorRole::secondaryContainer,
        NanColorRole::secondary,
        NanColorRole::secondary,
        NanColorRole::onSecondaryContainer,
        NanColorRole::secondary);
    style.input.neutral_family = make_input_family(
        NanColorRole::surfaceVariant,
        NanColorRole::outline,
        NanColorRole::onSurfaceVariant,
        NanColorRole::onSurface,
        NanColorRole::outline);
    style.input.destructive_family = make_input_family(
        NanColorRole::errorContainer,
        NanColorRole::error,
        NanColorRole::error,
        NanColorRole::onErrorContainer,
        NanColorRole::error);

    style.tag.font_size = tokens.typography.label_medium.font_size;
    style.tag.font_weight = map_weight(tokens.typography.label_medium.font_weight);
    style.tag.overflow = text::TextOverflow::ellipsis;
    style.tag.single_line = true;
    style.tag.color_variant = ColorVariant::inherit;
    style.tag.size = TagSize::md;
    style.tag.corner_radius = tokens.radius.full;
    style.tag.border_width = tokens.border.thin;
    style.tag.bg = role(NanColorRole::primaryContainer);
    style.tag.text = role(NanColorRole::onPrimaryContainer);
    style.tag.border = role(NanColorRole::primary);
    style.tag.bg_disabled = role(NanColorRole::surfaceVariant);
    style.tag.text_disabled = role(NanColorRole::outline);
    style.tag.border_disabled = role(NanColorRole::outlineVariant);
    style.tag.secondary_family = make_tag_family(
        NanColorRole::secondaryContainer,
        NanColorRole::onSecondaryContainer,
        NanColorRole::secondary);
    style.tag.neutral_family = make_tag_family(
        NanColorRole::surfaceVariant,
        NanColorRole::onSurfaceVariant,
        NanColorRole::outlineVariant);
    style.tag.destructive_family = make_tag_family(
        NanColorRole::errorContainer,
        NanColorRole::onErrorContainer,
        NanColorRole::error);
    style.tag.sm.font_size = tokens.typography.label_small.font_size;
    style.tag.sm.padding_h = tokens.spacing.small;
    style.tag.sm.padding_v = tokens.spacing.xxsmall;
    style.tag.md.font_size = tokens.typography.label_medium.font_size;
    style.tag.md.padding_h = tokens.spacing.medium;
    style.tag.md.padding_v = tokens.spacing.xsmall;
    style.tag.lg.font_size = tokens.typography.label_large.font_size;
    style.tag.lg.padding_h = tokens.spacing.large;
    style.tag.lg.padding_v = tokens.spacing.small;

    const auto make_checkbox_family = [&role](
        const NanColorRole box_bg_role,
        const NanColorRole box_border_role,
        const NanColorRole check_role) -> NanCheckboxStyle::ColorFamilyStyle {
        return NanCheckboxStyle::ColorFamilyStyle{
            .box_bg = role(box_bg_role),
            .box_border = role(box_border_role),
            .check = role(check_role),
            .box_bg_disabled = role(NanColorRole::surfaceVariant),
            .box_border_disabled = role(NanColorRole::outlineVariant),
            .check_disabled = role(NanColorRole::outline),
        };
    };

    style.checkbox.color_variant = ColorVariant::inherit;
    style.checkbox.size = CheckboxSize::md;
    style.checkbox.box_size = tokens.spacing.medium;
    style.checkbox.font_size = tokens.typography.body_medium.font_size;
    style.checkbox.gap = tokens.spacing.small;
    style.checkbox.corner_radius = tokens.radius.xsmall;
    style.checkbox.box_bg = role(NanColorRole::primary);
    style.checkbox.box_border = role(NanColorRole::primary);
    style.checkbox.check = role(NanColorRole::onPrimary);
    style.checkbox.box_bg_unchecked = role(NanColorRole::surface);
    style.checkbox.box_border_unchecked = role(NanColorRole::outlineVariant);
    style.checkbox.box_bg_disabled = role(NanColorRole::surfaceVariant);
    style.checkbox.box_border_disabled = role(NanColorRole::outlineVariant);
    style.checkbox.check_disabled = role(NanColorRole::outline);
    style.checkbox.secondary_family = make_checkbox_family(
        NanColorRole::secondary, NanColorRole::secondary, NanColorRole::onSecondary);
    style.checkbox.neutral_family = make_checkbox_family(
        NanColorRole::onSurfaceVariant, NanColorRole::onSurfaceVariant, NanColorRole::surface);
    style.checkbox.destructive_family = make_checkbox_family(
        NanColorRole::error, NanColorRole::error, NanColorRole::onError);
    style.checkbox.sm.box_size = tokens.spacing.small + tokens.spacing.xxsmall;
    style.checkbox.sm.font_size = tokens.typography.label_medium.font_size;
    style.checkbox.sm.gap = tokens.spacing.xsmall;
    style.checkbox.sm.corner_radius = 3.0f;
    style.checkbox.md.box_size = tokens.spacing.medium;
    style.checkbox.md.font_size = tokens.typography.body_medium.font_size;
    style.checkbox.md.gap = tokens.spacing.small;
    style.checkbox.md.corner_radius = tokens.radius.xsmall;

    style.focus_ring.color = role(NanColorRole::primary);
    style.focus_ring.width = tokens.border.focus_ring;
    style.focus_ring.offset = tokens.spacing.xxsmall;

    style.progress.track_bg = role(NanColorRole::surfaceVariant);
    style.progress.fill = role(NanColorRole::primary);
    style.progress.color_variant = ColorVariant::inherit;
    style.progress.corner_radius = tokens.radius.xsmall;
    style.progress.bar_height = tokens.spacing.small - tokens.spacing.xxsmall;
    style.progress.secondary_family = make_progress_family(NanColorRole::secondaryContainer, NanColorRole::secondary);
    style.progress.neutral_family = make_progress_family(NanColorRole::outlineVariant, NanColorRole::onSurfaceVariant);
    style.progress.destructive_family = make_progress_family(NanColorRole::errorContainer, NanColorRole::error);

    return style;
}

} // namespace nandina::theme