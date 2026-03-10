//
// Created by cvrain on 2026/3/10.
//

#include "font_manager.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFontDatabase>
#include <QDebug>

namespace Nandina::Core::Fonts {

    // ─── Static storage ────────────────────────────────────────────
    QString FontManager::s_lxgwFamily;
    QString FontManager::s_lxgwMonoFamily;
    QString FontManager::s_sarasaFamily;
    QString FontManager::s_cascadiaFamily;
    bool    FontManager::s_fontsLoaded = false;

    // ─── Constructor ───────────────────────────────────────────────

    FontManager::FontManager(QObject *parent) : QObject(parent) {}

    // ─── Instance property accessors (QML-facing) ──────────────────

    QString FontManager::defaultFamily()  const { return resolvedDefaultFamily(); }
    QString FontManager::lxgwFamily()     const { return resolvedLxgwFamily(); }
    QString FontManager::lxgwMonoFamily() const { return resolvedLxgwMonoFamily(); }
    QString FontManager::sarasaFamily()   const { return resolvedSarasaFamily(); }
    QString FontManager::cascadiaFamily() const { return resolvedCascadiaFamily(); }
    bool    FontManager::fontsLoaded()    const { return s_fontsLoaded; }

    // ─── Static accessors (C++-facing, usable after loadBundledFonts) ─

    QString FontManager::resolvedDefaultFamily()  { return resolvedLxgwFamily(); }
    QString FontManager::resolvedLxgwFamily()     { return s_lxgwFamily.isEmpty()     ? QStringLiteral("LXGW WenKai")           : s_lxgwFamily; }
    QString FontManager::resolvedLxgwMonoFamily() { return s_lxgwMonoFamily.isEmpty() ? QStringLiteral("LXGW WenKai Mono")      : s_lxgwMonoFamily; }
    QString FontManager::resolvedSarasaFamily()   { return s_sarasaFamily.isEmpty()   ? QStringLiteral("Sarasa UI SC")           : s_sarasaFamily; }
    QString FontManager::resolvedCascadiaFamily() { return s_cascadiaFamily.isEmpty() ? QStringLiteral("CaskaydiaCove Nerd Font") : s_cascadiaFamily; }

    // ─── Private helpers ───────────────────────────────────────────

    QString FontManager::resolveSearchDir(const QString &override) {
        if (!override.isEmpty() && QDir(override).exists())
            return override;

        // Environment variable override
        const QString envDir = qEnvironmentVariable("NANDINA_FONTS_DIR");
        if (!envDir.isEmpty() && QDir(envDir).exists())
            return envDir;

        // Search relative to the application executable
        const QString appDir = QCoreApplication::applicationDirPath();
        const QStringList candidates = {
            appDir + QStringLiteral("/fonts"),
            QDir::cleanPath(appDir + QStringLiteral("/../fonts")),
            QDir::cleanPath(appDir + QStringLiteral("/../../fonts")),
        };
        for (const QString &path : candidates) {
            if (QDir(path).exists())
                return path;
        }
        return {};
    }

    // ───────────────────────────────────────────────────────────────
    //  loadBundledFonts
    //  Iterates each font sub-directory, loads every .ttf/.ttc file,
    //  and captures the primary family name from each group.
    // ───────────────────────────────────────────────────────────────

    static QString firstFamilyFromFile(const QString &filePath) {
        const int id = QFontDatabase::addApplicationFont(filePath);
        if (id < 0) {
            qWarning() << "[Nandina/FontManager] Failed to load:" << filePath;
            return {};
        }
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        return families.isEmpty() ? QString{} : families.first();
    }

    static void loadAllFontsInDir(const QString &dirPath) {
        QDirIterator it(dirPath, {QStringLiteral("*.ttf"), QStringLiteral("*.ttc")},
                        QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QFontDatabase::addApplicationFont(path);
        }
    }

    bool FontManager::loadBundledFonts(const QString &fontsDir) {
        if (s_fontsLoaded)
            return true;

        const QString baseDir = resolveSearchDir(fontsDir);
        if (baseDir.isEmpty()) {
            qWarning() << "[Nandina/FontManager] Fonts directory not found. "
                          "Set NANDINA_FONTS_DIR or place a fonts/ directory "
                          "next to the executable.";
            return false;
        }

        qDebug() << "[Nandina/FontManager] Loading fonts from:" << baseDir;

        // ── LXGW WenKai ────────────────────────────────────────────
        const QString lxgwDir = baseDir + QStringLiteral("/LxgwWenkai");
        if (QDir(lxgwDir).exists()) {
            // Capture family name from the Regular variant first
            s_lxgwFamily = firstFamilyFromFile(lxgwDir + QStringLiteral("/LXGWWenKai-Regular.ttf"));
            s_lxgwMonoFamily = firstFamilyFromFile(lxgwDir + QStringLiteral("/LXGWWenKaiMono-Regular.ttf"));
            // Load the remaining weights
            loadAllFontsInDir(lxgwDir);
        } else {
            qWarning() << "[Nandina/FontManager] LxgwWenkai directory not found:" << lxgwDir;
        }

        // ── Sarasa ─────────────────────────────────────────────────
        const QString sarasaDir = baseDir + QStringLiteral("/Sarasa");
        if (QDir(sarasaDir).exists()) {
            // TTC files contain multiple families; pick the "UI SC" variant
            const int id = QFontDatabase::addApplicationFont(sarasaDir + QStringLiteral("/Sarasa-Regular.ttc"));
            if (id >= 0) {
                const QStringList families = QFontDatabase::applicationFontFamilies(id);
                // Prefer "Sarasa UI SC" for proportional CJK body text
                for (const QString &f : families) {
                    if (f.contains(QStringLiteral("UI"), Qt::CaseInsensitive)) {
                        s_sarasaFamily = f;
                        break;
                    }
                }
                if (s_sarasaFamily.isEmpty() && !families.isEmpty())
                    s_sarasaFamily = families.first();
            }
            loadAllFontsInDir(sarasaDir);
        } else {
            qWarning() << "[Nandina/FontManager] Sarasa directory not found:" << sarasaDir;
        }

        // ── CascadiaMono (Caskaydia Nerd Font) ─────────────────────
        const QString cascadiaDir = baseDir + QStringLiteral("/CascadiaMono");
        if (QDir(cascadiaDir).exists()) {
            s_cascadiaFamily = firstFamilyFromFile(
                cascadiaDir + QStringLiteral("/CaskaydiaMonoNerdFont-Regular.ttf"));
            loadAllFontsInDir(cascadiaDir);
        } else {
            qWarning() << "[Nandina/FontManager] CascadiaMono directory not found:" << cascadiaDir;
        }

        s_fontsLoaded = true;

        qDebug() << "[Nandina/FontManager] Loaded families:"
                 << "LXGW WenKai =" << s_lxgwFamily
                 << "| LXGW WenKai Mono =" << s_lxgwMonoFamily
                 << "| Sarasa =" << s_sarasaFamily
                 << "| Cascadia =" << s_cascadiaFamily;

        return true;
    }

} // namespace Nandina::Core::Fonts
