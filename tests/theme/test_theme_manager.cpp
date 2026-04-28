//
// Created by cvrain on 2026/4/28.
//
// NandinaUI — ThemeManager 单元测试
// 覆盖：
//   - NanTheme 构造与属性查询
//   - ThemeManager 单例
//   - 主题注册/激活/切换
//   - 颜色方案切换 (light/dark)
//   - 颜色取值
//   - Primitive Tokens 访问
//   - 变更监听器
//   - 自动注册 default 主题
//

#include <gtest/gtest.h>
#include <string>
#include <atomic>
#include <thread>

import nandina.theme;

using namespace nandina::theme;

// 辅助：比较两个 NanColor 是否相等（通过 RGB 值）
static auto color_eq(const nandina::NanColor& a, const nandina::NanColor& b) -> bool {
    const auto ar = a.to<nandina::NanRgb>();
    const auto br = b.to<nandina::NanRgb>();
    return ar.red() == br.red() && ar.green() == br.green() &&
           ar.blue() == br.blue() && ar.alpha() == br.alpha();
}

// ═══════════════════════════════════════════════════════════════════════════
// NanTheme 构造测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanThemeTest, DefaultConstruction) {
    NanTheme theme{"test_theme"};
    EXPECT_EQ(theme.name(), "test_theme");
    EXPECT_EQ(theme.scheme(), NanColorScheme::light);

    // 应该有默认调色板
    const auto& pal = theme.palette();
    (void)pal; // 至少不崩溃

    // 应该有默认 Token
    const auto& tokens = theme.tokens();
    EXPECT_FLOAT_EQ(tokens.spacing.medium, 12.0f);
}

TEST(NanThemeTest, CustomPalette) {
    // 用一个空的自定义调色板构造主题
    std::array<NanRoleColorPair, static_cast<std::size_t>(NanColorRole::_count)> pairs{};
    for (std::size_t i = 0; i < pairs.size(); ++i) {
        pairs[i] = NanRoleColorPair{
            .role  = static_cast<NanColorRole>(i),
            .light = nandina::NanColor::from(nandina::NanRgb{255u, 255u, 255u, 255u}),
            .dark  = nandina::NanColor::from(nandina::NanRgb{0u, 0u, 0u, 255u}),
        };
    }
    NanColorPalette custom_palette{pairs};

    NanTheme theme{"custom", std::move(custom_palette), NanPrimitiveTokens{}, NanColorScheme::dark};
    EXPECT_EQ(theme.name(), "custom");
    EXPECT_EQ(theme.scheme(), NanColorScheme::dark);

    // 验证自定义色值：dark 模式下应返回 dark 色值
    const auto& primary_color = theme.color(NanColorRole::primary);
    const auto rgb = primary_color.to<nandina::NanRgb>();
    EXPECT_EQ(rgb.red(), 0);
    EXPECT_EQ(rgb.green(), 0);
    EXPECT_EQ(rgb.blue(), 0);
}

TEST(NanThemeTest, ToggleScheme) {
    NanTheme theme{"toggle_test"};
    EXPECT_EQ(theme.scheme(), NanColorScheme::light);

    theme.toggle_scheme();
    EXPECT_EQ(theme.scheme(), NanColorScheme::dark);

    theme.toggle_scheme();
    EXPECT_EQ(theme.scheme(), NanColorScheme::light);
}

TEST(NanThemeTest, SetScheme) {
    NanTheme theme{"scheme_test"};
    theme.set_scheme(NanColorScheme::dark);
    EXPECT_EQ(theme.scheme(), NanColorScheme::dark);
}

TEST(NanThemeTest, TokensMutable) {
    NanTheme theme{"tokens_test"};
    auto& tokens = theme.tokens();
    tokens.spacing.large = 20.0f;
    EXPECT_FLOAT_EQ(theme.tokens().spacing.large, 20.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// ThemeManager 单例与默认主题
// ═══════════════════════════════════════════════════════════════════════════

TEST(ThemeManagerTest, Singleton) {
    auto& mgr1 = ThemeManager::instance();
    auto& mgr2 = ThemeManager::instance();
    EXPECT_EQ(&mgr1, &mgr2);
}

TEST(ThemeManagerTest, DefaultThemeExists) {
    auto& mgr = ThemeManager::instance();
    EXPECT_EQ(mgr.active_name(), "default");
    EXPECT_NE(mgr.active(), nullptr);
}

TEST(ThemeManagerTest, DefaultThemeHasColors) {
    auto& mgr = ThemeManager::instance();
    const auto* theme = mgr.active();
    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->name(), "default");

    // 默认 light scheme 下 primary 应有色值
    const auto& primary = theme->color(NanColorRole::primary);
    const auto rgb = primary.to<nandina::NanRgb>();
    EXPECT_EQ(rgb.red(), 103);
    EXPECT_EQ(rgb.green(), 80);
    EXPECT_EQ(rgb.blue(), 217);
}

// ═══════════════════════════════════════════════════════════════════════════
// 主题注册与激活
// ═══════════════════════════════════════════════════════════════════════════

TEST(ThemeManagerTest, RegisterAndActivate) {
    auto& mgr = ThemeManager::instance();

    // 注册自定义主题
    NanTheme custom_theme{"my_theme"};
    mgr.register_theme(std::move(custom_theme));

    // 激活
    bool activated = mgr.activate("my_theme");
    EXPECT_TRUE(activated);
    EXPECT_EQ(mgr.active_name(), "my_theme");

    // 激活不存在的主题应失败
    bool failed = mgr.activate("nonexistent");
    EXPECT_FALSE(failed);

    // 恢复默认
    mgr.activate("default");
}

TEST(ThemeManagerTest, ActiveMutable) {
    auto& mgr = ThemeManager::instance();

    // 注册并激活
    NanTheme t{"mutable_test"};
    mgr.register_theme(std::move(t));
    mgr.activate("mutable_test");

    // 获取可变指针并修改
    auto* active = mgr.active();
    ASSERT_NE(active, nullptr);
    active->tokens().spacing.medium = 100.0f;

    // 重新获取验证
    const auto* same_active = mgr.active();
    ASSERT_NE(same_active, nullptr);
    EXPECT_FLOAT_EQ(same_active->tokens().spacing.medium, 100.0f);

    mgr.activate("default");
}

// ═══════════════════════════════════════════════════════════════════════════
// 颜色方案切换
// ═══════════════════════════════════════════════════════════════════════════

TEST(ThemeManagerTest, SetScheme) {
    auto& mgr = ThemeManager::instance();
    EXPECT_EQ(mgr.scheme(), NanColorScheme::light);

    mgr.set_scheme(NanColorScheme::dark);
    EXPECT_EQ(mgr.scheme(), NanColorScheme::dark);

    mgr.set_scheme(NanColorScheme::light);
    EXPECT_EQ(mgr.scheme(), NanColorScheme::light);
}

TEST(ThemeManagerTest, ToggleScheme) {
    auto& mgr = ThemeManager::instance();
    auto initial = mgr.scheme();

    mgr.toggle_scheme();
    EXPECT_NE(mgr.scheme(), initial);

    mgr.toggle_scheme();
    EXPECT_EQ(mgr.scheme(), initial);
}

TEST(ThemeManagerTest, ColorChangesWithScheme) {
    auto& mgr = ThemeManager::instance();

    // 注册专门测试主题
    std::array<NanRoleColorPair, static_cast<std::size_t>(NanColorRole::_count)> pairs{};
    for (std::size_t i = 0; i < pairs.size(); ++i) {
        pairs[i] = NanRoleColorPair{
            .role  = static_cast<NanColorRole>(i),
            .light = nandina::NanColor::from(nandina::NanRgb{255u, 0u, 0u, 255u}),   // 红色 light
            .dark  = nandina::NanColor::from(nandina::NanRgb{0u, 0u, 255u, 255u}),   // 蓝色 dark
        };
    }
    NanColorPalette custom_palette{pairs};
    NanTheme theme{"scheme_color_test", std::move(custom_palette)};
    mgr.register_theme(std::move(theme));
    mgr.activate("scheme_color_test");

    // Light 模式应该是红色
    mgr.set_scheme(NanColorScheme::light);
    {
        const auto& primary = mgr.color(NanColorRole::primary);
        const auto rgb = primary.to<nandina::NanRgb>();
        EXPECT_EQ(rgb.red(), 255);
        EXPECT_EQ(rgb.green(), 0);
        EXPECT_EQ(rgb.blue(), 0);
    }

    // Dark 模式应该是蓝色
    mgr.set_scheme(NanColorScheme::dark);
    {
        const auto& primary = mgr.color(NanColorRole::primary);
        const auto rgb = primary.to<nandina::NanRgb>();
        EXPECT_EQ(rgb.red(), 0);
        EXPECT_EQ(rgb.green(), 0);
        EXPECT_EQ(rgb.blue(), 255);
    }

    mgr.activate("default");
    mgr.set_scheme(NanColorScheme::light);
}

// ═══════════════════════════════════════════════════════════════════════════
// 变更监听器
// ═══════════════════════════════════════════════════════════════════════════

TEST(ThemeManagerTest, ChangeListenerOnActivate) {
    auto& mgr = ThemeManager::instance();
    std::string notified_theme;
    auto token = mgr.on_changed([&notified_theme](const std::string& name) {
        notified_theme = name;
    });

    // 注册并激活一个新主题
    NanTheme t{"listener_test"};
    mgr.register_theme(std::move(t));
    mgr.activate("listener_test");

    EXPECT_EQ(notified_theme, "listener_test");

    // 清理
    mgr.activate("default");
}

TEST(ThemeManagerTest, ChangeListenerOnSchemeToggle) {
    auto& mgr = ThemeManager::instance();
    int notification_count = 0;
    auto token = mgr.on_changed([&notification_count](const std::string&) {
        ++notification_count;
    });

    mgr.toggle_scheme();
    EXPECT_GE(notification_count, 1);

    // 切回 light
    mgr.set_scheme(NanColorScheme::light);
    EXPECT_GE(notification_count, 2);
}

TEST(ThemeManagerTest, ChangeListenerDisconnect) {
    auto& mgr = ThemeManager::instance();
    int notification_count = 0;

    {
        auto token = mgr.on_changed([&notification_count](const std::string&) {
            ++notification_count;
        });
        // token 在此作用域结束时析构 → disconnect
    }

    mgr.toggle_scheme();
    // disconnect 后不应再通知
    mgr.set_scheme(NanColorScheme::light);

    // 计数应仍为 0（因为 token 已销毁）
    EXPECT_EQ(notification_count, 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// 颜色快捷访问
// ═══════════════════════════════════════════════════════════════════════════

TEST(ThemeManagerTest, ColorAccess) {
    auto& mgr = ThemeManager::instance();
    mgr.activate("default");
    mgr.set_scheme(NanColorScheme::light);

    // 验证可以通过 manager 直接获取颜色
    const auto& primary = mgr.color(NanColorRole::primary);
    const auto rgb = primary.to<nandina::NanRgb>();
    EXPECT_EQ(rgb.red(), 103);
    EXPECT_EQ(rgb.green(), 80);
    EXPECT_EQ(rgb.blue(), 217);
}

// ═══════════════════════════════════════════════════════════════════════════
// 线程安全测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(ThemeManagerTest, ThreadSafety) {
    auto& mgr = ThemeManager::instance();
    std::atomic<int> errors{0};

    std::thread t1([&]() {
        try {
            mgr.activate("default");
            mgr.set_scheme(NanColorScheme::dark);
            mgr.toggle_scheme();
        } catch (...) {
            errors.fetch_add(1);
        }
    });

    std::thread t2([&]() {
        try {
            auto name = mgr.active_name();
            (void)name;
            auto* active = mgr.active();
            (void)active;
            auto scheme = mgr.scheme();
            (void)scheme;
        } catch (...) {
            errors.fetch_add(1);
        }
    });

    t1.join();
    t2.join();

    EXPECT_EQ(errors.load(), 0);
    mgr.set_scheme(NanColorScheme::light);
}

// ═══════════════════════════════════════════════════════════════════════════
// 取消注册主题
// ═══════════════════════════════════════════════════════════════════════════

TEST(ThemeManagerTest, Unregister) {
    auto& mgr = ThemeManager::instance();

    // 不能取消注册当前活跃主题
    mgr.activate("default");
    EXPECT_FALSE(mgr.unregister("default"));

    // 可以取消注册非活跃主题
    NanTheme t{"to_be_removed"};
    mgr.register_theme(std::move(t));
    EXPECT_TRUE(mgr.unregister("to_be_removed"));

    // 重复取消应失败
    EXPECT_FALSE(mgr.unregister("to_be_removed"));
}