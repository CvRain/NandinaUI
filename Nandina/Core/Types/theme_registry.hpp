//
// Created by cvrain on 2026/3/13.
//

#ifndef NANDINA_THEME_REGISTRY_HPP
#define NANDINA_THEME_REGISTRY_HPP

#include <QString>
#include <QStringList>
#include <array>

#include "theme_type.hpp"
#include "global_export.hpp"

namespace Nandina::Core::Types {

    struct ThemeRegistration {
        ThemeVariant::ThemeTypes type;
        const char *name;
    };

    class ThemeRegistry {
    public:
        static constexpr std::array<ThemeRegistration, ThemeVariant::ThemeCount> Registrations = {{
                {ThemeVariant::ThemeTypes::Aurora, "Aurora"},
                {ThemeVariant::ThemeTypes::Catppuccin, "Catppuccin"},
                {ThemeVariant::ThemeTypes::Cerberus, "Cerberus"},
                {ThemeVariant::ThemeTypes::Concord, "Concord"},
                {ThemeVariant::ThemeTypes::Crimson, "Crimson"},
                {ThemeVariant::ThemeTypes::Fennec, "Fennec"},
                {ThemeVariant::ThemeTypes::Legacy, "Legacy"},
                {ThemeVariant::ThemeTypes::AdwaitaSoft, "AdwaitaSoft"},
                {ThemeVariant::ThemeTypes::Orchis, "Orchis"},
        }};

        static const ThemeRegistration &registration(ThemeVariant::ThemeTypes type) {
            for (const auto &entry: Registrations) {
                if (entry.type == type)
                    return entry;
            }
            return Registrations.front();
        }

        static ThemeVariant::ThemeTypes themeFromName(const QString &name) {
            for (const auto &entry: Registrations) {
                if (QString::compare(QString::fromLatin1(entry.name), name, Qt::CaseInsensitive) == 0)
                    return entry.type;
            }
            return Registrations.front().type;
        }

        static QString themeName(ThemeVariant::ThemeTypes type) {
            return QString::fromLatin1(registration(type).name);
        }

        static QString colorResourcePath(ThemeVariant::ThemeTypes type) {
            return resourcePath(QStringLiteral("Color"), type);
        }

        static QString primitiveResourcePath(ThemeVariant::ThemeTypes type) {
            return resourcePath(QStringLiteral("Primitives"), type);
        }

        static QStringList availableThemeNames() {
            QStringList names;
            names.reserve(static_cast<qsizetype>(Registrations.size()));
            for (const auto &entry: Registrations) {
                names.append(QString::fromLatin1(entry.name));
            }
            return names;
        }

    private:
        static QString resourcePath(const QString &moduleName, ThemeVariant::ThemeTypes type) {
            return QStringLiteral(":/Nandina/%1/themes/%2.json").arg(moduleName, themeName(type).toLower());
        }
    };

} // namespace Nandina::Core::Types

#endif // NANDINA_THEME_REGISTRY_HPP
