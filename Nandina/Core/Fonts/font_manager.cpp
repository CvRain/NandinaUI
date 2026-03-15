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

    namespace {
        QString &lxgwFamilyStorage() {
            static QString value;
            return value;
        }

        QString &lxgwMonoFamilyStorage() {
            static QString value;
            return value;
        }

        QString &sarasaFamilyStorage() {
            static QString value;
            return value;
        }

        QString &cascadiaFamilyStorage() {
            static QString value;
            return value;
        }

        bool &fontsLoadedStorage() {
            static bool value = false;
            return value;
        }
    }

    // ─── Constructor ───────────────────────────────────────────────

    FontManager::FontManager(QObject *parent) : QObject(parent) {}

    // ─── Instance property accessors (QML-facing) ──────────────────

    QString FontManager::defaultFamily()  const { return resolvedDefaultFamily(); }
    QString FontManager::lxgwFamily()     const { return resolvedLxgwFamily(); }
    QString FontManager::lxgwMonoFamily() const { return resolvedLxgwMonoFamily(); }
    QString FontManager::sarasaFamily()   const { return resolvedSarasaFamily(); }
    QString FontManager::cascadiaFamily() const { return resolvedCascadiaFamily(); }
    bool    FontManager::fontsLoaded()    const { return fontsLoadedStorage(); }

    // ─── Static accessors (C++-facing, usable after loadBundledFonts) ─

    QString FontManager::resolvedDefaultFamily()  { return resolvedLxgwFamily(); }
    QString FontManager::resolvedLxgwFamily()     { return lxgwFamilyStorage().isEmpty()     ? QStringLiteral("LXGW WenKai")            : lxgwFamilyStorage(); }
    QString FontManager::resolvedLxgwMonoFamily() { return lxgwMonoFamilyStorage().isEmpty() ? QStringLiteral("LXGW WenKai Mono")       : lxgwMonoFamilyStorage(); }
    QString FontManager::resolvedSarasaFamily()   { return sarasaFamilyStorage().isEmpty()   ? QStringLiteral("Sarasa UI SC")            : sarasaFamilyStorage(); }
    QString FontManager::resolvedCascadiaFamily() { return cascadiaFamilyStorage().isEmpty() ? QStringLiteral("CaskaydiaCove Nerd Font") : cascadiaFamilyStorage(); }

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
        if (fontsLoadedStorage())
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
            lxgwFamilyStorage() = firstFamilyFromFile(lxgwDir + QStringLiteral("/LXGWWenKai-Regular.ttf"));
            lxgwMonoFamilyStorage() = firstFamilyFromFile(lxgwDir + QStringLiteral("/LXGWWenKaiMono-Regular.ttf"));
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
                        sarasaFamilyStorage() = f;
                        break;
                    }
                }
                if (sarasaFamilyStorage().isEmpty() && !families.isEmpty())
                    sarasaFamilyStorage() = families.first();
            }
            loadAllFontsInDir(sarasaDir);
        } else {
            qWarning() << "[Nandina/FontManager] Sarasa directory not found:" << sarasaDir;
        }

        // ── CascadiaMono (Caskaydia Nerd Font) ─────────────────────
        const QString cascadiaDir = baseDir + QStringLiteral("/CascadiaMono");
        if (QDir(cascadiaDir).exists()) {
            cascadiaFamilyStorage() = firstFamilyFromFile(
                cascadiaDir + QStringLiteral("/CaskaydiaMonoNerdFont-Regular.ttf"));
            loadAllFontsInDir(cascadiaDir);
        } else {
            qWarning() << "[Nandina/FontManager] CascadiaMono directory not found:" << cascadiaDir;
        }

        fontsLoadedStorage() = true;

        qDebug() << "[Nandina/FontManager] Loaded families:"
                 << "LXGW WenKai =" << lxgwFamilyStorage()
                 << "| LXGW WenKai Mono =" << lxgwMonoFamilyStorage()
                 << "| Sarasa =" << sarasaFamilyStorage()
                 << "| Cascadia =" << cascadiaFamilyStorage();

        return true;
    }

} // namespace Nandina::Core::Fonts
