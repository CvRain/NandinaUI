// Test-only translation of historical dual-palette scenarios into canonical scheme selections.
#ifndef NANDINA_TEST_THEME_TEST_SUPPORT_HPP
#define NANDINA_TEST_THEME_TEST_SUPPORT_HPP
#include <map>
#include <nandina/theme/builtin_themes.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <stdexcept>
#include <string>

namespace theme_test
{
    namespace theme = nandina::theme;
    inline auto id(std::string_view text) -> theme::ThemeSchemeId {
        return theme::ThemeSchemeId::parse(text).value();
    }
    inline auto encoded(std::string_view text) -> std::string {
        std::string result = "x";
        constexpr char digits[] = "0123456789abcdef";
        for (unsigned char ch: text) {
            result += digits[ch >> 4];
            result += digits[ch & 15];
        }
        return result;
    }
    /// style_document 把文件主题/家族编译为 `styles:x<hex>[/(light|dark)]` 形式的
    /// 目录 ID；兼容查询从这里解码回文件级名字（测试专用，生产层用 active_scheme_id()）。
    inline auto decode_styles_name(std::string_view text) -> std::optional<std::string> {
        constexpr std::string_view prefix = "styles:x";
        if (!text.starts_with(prefix)) {
            return std::nullopt;
        }
        auto hex = text.substr(prefix.size());
        if (const auto slash = hex.find('/'); slash != std::string_view::npos) {
            hex = hex.substr(0, slash);
        }
        if (hex.size() % 2 != 0) {
            return std::nullopt;
        }
        const auto nibble = [](const char value) -> int {
            return value >= '0' && value <= '9' ? value - '0' : value - 'a' + 10;
        };
        std::string decoded;
        decoded.reserve(hex.size() / 2);
        for (std::size_t index = 0; index < hex.size(); index += 2) {
            decoded.push_back(
                static_cast<char>((nibble(hex[index]) << 4) | nibble(hex[index + 1]))
            );
        }
        return decoded;
    }
    class Manager: public theme::ThemeManager {
        struct Pair {
            theme::ThemeSchemeId light;
            theme::ThemeSchemeId dark;
        };
        std::map<std::string, Pair> named_;
        std::map<std::string, std::pair<std::string, std::string>> families_;
        std::map<std::string, Pair> full_;
        std::shared_ptr<const theme::DesignSystem> applied_;
        std::string active_name_;
        std::string active_family_;
        theme::ThemePreference preference_ = theme::ThemePreference::system;
        auto install(std::string_view name, const theme::DesignSystem& system) -> Pair {
            Pair pair {
                id("tests:" + encoded(name) + "/light"),
                id("tests:" + encoded(name) + "/dark")
            };
            for (auto appearance: {theme::ColorAppearance::light, theme::ColorAppearance::dark}) {
                auto key = appearance == theme::ColorAppearance::light ? pair.light : pair.dark;
                theme::ThemeScheme entry {
                    key,
                    std::string(name),
                    appearance,
                    {},
                    std::make_shared<const theme::ThemeSchemeContent>(
                        theme::ThemeSchemeContent::from(system, appearance)
                    )
                };
                auto result = catalog().find(key) ? replace_scheme(entry) : register_scheme(entry);
                if (!result) {
                    throw std::invalid_argument(theme::describe(result.error()));
                }
            }
            return pair;
        }
        auto use(const Pair& pair) -> bool {
            return select(theme::AppearanceThemeSelection {pair.light, pair.dark, preference_})
                .has_value();
        }

    public:
        void apply(std::shared_ptr<const theme::DesignSystem> system) {
            if (!system) {
                throw std::invalid_argument("null system");
            }
            if (applied_ == system && active_name_ == "applied") {
                return;
            }
            require_idle();
            auto pair = install("applied", *system);
            applied_ = std::move(system);
            active_name_ = "applied";
            active_family_.clear();
            (void)use(pair);
        }
        void set_theme(theme::NanTheme value) {
            apply(
                std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(value))
            );
        }
        auto register_theme(std::string name, theme::NanTheme value) -> bool {
            if (name.empty()) {
                throw std::invalid_argument("empty name");
            }
            require_idle();
            const bool added = !named_.contains(name);
            named_.insert_or_assign(name, install(name, theme::design_system_from_theme(value)));
            return added;
        }
        auto contains(std::string_view name) const -> bool {
            return named_.contains(std::string(name));
        }
        auto activate(std::string_view name) -> bool {
            require_idle();
            auto found = named_.find(std::string(name));
            if (found == named_.end()) {
                return false;
            }
            active_name_ = found->first;
            active_family_.clear();
            return use(found->second);
        }
        auto register_family(std::string name, std::string light, std::string dark) -> bool {
            if (name.empty() || !named_.contains(light) || !named_.contains(dark)) {
                throw std::invalid_argument("invalid family");
            }
            require_idle();
            bool added = !families_.contains(name);
            families_.insert_or_assign(name, std::pair {light, dark});
            if (active_family_ == name) {
                (void)use({named_.at(light).light, named_.at(dark).dark});
            }
            return added;
        }
        void register_theme_family(std::string name, theme::DesignSystem system) {
            if (name.empty()) {
                throw std::invalid_argument("empty family");
            }
            require_idle();
            full_.insert_or_assign(name, install("family-" + name, system));
            if (active_family_ == name) {
                (void)use(full_.at(name));
            }
        }
        auto contains_family(std::string_view name) const -> bool {
            if (families_.contains(std::string(name)) || full_.contains(std::string(name))) {
                return true;
            }
            auto key = theme::ThemeSchemeId::parse("nandina:" + std::string(name) + "/light");
            return key && catalog().find(*key);
        }
        auto activate_family(std::string_view name) -> bool {
            require_idle();
            const auto key = std::string(name);
            if (full_.contains(key)) {
                active_family_ = key;
                return use(full_.at(key));
            }
            if (families_.contains(key)) {
                active_family_ = key;
                const auto& pair = families_.at(key);
                return use({named_.at(pair.first).light, named_.at(pair.second).dark});
            }
            if (contains_family(key)) {
                active_family_ = key;
                return use({id("nandina:" + key + "/light"), id("nandina:" + key + "/dark")});
            }
            return false;
        }
        void set_preference(theme::ThemePreference value) {
            auto next = std::get<theme::AppearanceThemeSelection>(selection());
            next.preference = value;
            auto result = select(next);
            if (!result) {
                throw std::invalid_argument(theme::describe(result.error()));
            }
            preference_ = value;
        }
        auto preference() const -> theme::ThemePreference {
            const auto& current = ThemeManager::selection();
            if (std::holds_alternative<theme::AppearanceThemeSelection>(current)) {
                return std::get<theme::AppearanceThemeSelection>(current).preference;
            }
            return preference_;
        }
        auto active_family() const -> std::string_view {
            if (const auto* active = ThemeManager::active_scheme_id(); active) {
                if (const auto* scheme = ThemeManager::catalog().find(*active);
                    scheme && scheme->family)
                {
                    if (auto decoded = decode_styles_name(scheme->family->text()); decoded) {
                        decoded_family_ = *decoded;
                        return decoded_family_;
                    }
                }
            }
            return active_family_;
        }
        auto active_name() const -> std::string_view {
            if (families_.contains(active_family_)) {
                const auto& family = families_.at(active_family_);
                return appearance() == theme::ColorAppearance::dark ? family.second : family.first;
            }
            if (const auto* active = ThemeManager::active_scheme_id(); active) {
                if (auto decoded = decode_styles_name(active->text()); decoded) {
                    decoded_name_ = *decoded;
                    return decoded_name_;
                }
                if (active->text().starts_with("nandina:")) {
                    return "default";
                }
            }
            return active_name_;
        }
        mutable std::string decoded_name_;
        mutable std::string decoded_family_;
    };
} // namespace theme_test
#endif
