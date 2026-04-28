//
// Created by cvrain on 2026/4/28.
//
module;

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module nandina.theme.nan_theme;

import nandina.foundation.color;
import nandina.theme.nan_theme_types;
import nandina.theme.nan_primitive_tokens;
import nandina.theme.nan_color_palette;

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
}

inline auto ThemeManager::instance() -> ThemeManager& {
    static ThemeManager mgr;
    return mgr;
}

inline auto ThemeManager::register_theme(NanTheme theme) -> void {
    std::lock_guard lock(mutex_);
    const auto name = std::string{theme.name()};
    themes_.insert_or_assign(name, std::move(theme));
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
    }
    if (changed) {
        notify_changed();
    }
}

inline auto ThemeManager::toggle_scheme() -> void {
    bool changed = false;
    {
        std::lock_guard lock(mutex_);
        const auto it = themes_.find(active_name_);
        if (it == themes_.end()) {
            return;
        }
        it->second.toggle_scheme();
        changed = true;
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

} // namespace nandina::theme