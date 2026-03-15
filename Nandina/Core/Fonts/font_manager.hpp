//
// Created by cvrain on 2026/3/10.
//

#ifndef NANDINA_FONT_MANAGER_HPP
#define NANDINA_FONT_MANAGER_HPP

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include "fonts_export.hpp"


namespace Nandina::Core::Fonts {

    // ═══════════════════════════════════════════════════════════════
    //  FontManager — loads bundled fonts and exposes family names
    //
    //  Three font families are bundled with Nandina:
    //    • LXGW WenKai   — CJK + Latin (default UI font)
    //    • Sarasa         — CJK + Latin (alternative)
    //    • CascadiaMono  — Latin monospace (Nerd Font)
    //
    //  Call FontManager::loadBundledFonts() once from C++ before the
    //  QML engine starts.  All font family name properties become
    //  available afterwards.
    // ═══════════════════════════════════════════════════════════════

    class NANDINA_FONTS_EXPORT FontManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        Q_PROPERTY(QString defaultFamily    READ defaultFamily    CONSTANT)
        Q_PROPERTY(QString lxgwFamily       READ lxgwFamily       CONSTANT)
        Q_PROPERTY(QString lxgwMonoFamily   READ lxgwMonoFamily   CONSTANT)
        Q_PROPERTY(QString sarasaFamily     READ sarasaFamily     CONSTANT)
        Q_PROPERTY(QString cascadiaFamily   READ cascadiaFamily   CONSTANT)
        Q_PROPERTY(bool    fontsLoaded      READ fontsLoaded      CONSTANT)

    public:
        explicit FontManager(QObject *parent = nullptr);

        // ── Font family accessors (usable directly in font.family) ─
        [[nodiscard]] QString defaultFamily()  const;
        [[nodiscard]] QString lxgwFamily()     const;
        [[nodiscard]] QString lxgwMonoFamily() const;
        [[nodiscard]] QString sarasaFamily()   const;
        [[nodiscard]] QString cascadiaFamily() const;
        [[nodiscard]] bool    fontsLoaded()    const;

        // ── C++ init API ───────────────────────────────────────────
        // Scans <fontsDir>/{LxgwWenkai,Sarasa,CascadiaMono}/ for font
        // files and registers them with QFontDatabase.
        // When fontsDir is empty it searches:
        //   1. $NANDINA_FONTS_DIR  (env override)
        //   2. <appDir>/fonts
        //   3. <appDir>/../fonts
        //   4. <appDir>/../../fonts
        static bool loadBundledFonts(const QString &fontsDir = {});

        // ── Static accessors (usable in C++ after loadBundledFonts) ─
        // Use these instead of raw string literals to avoid typos.
        [[nodiscard]] static QString resolvedDefaultFamily();   // LXGW WenKai
        [[nodiscard]] static QString resolvedLxgwFamily();
        [[nodiscard]] static QString resolvedLxgwMonoFamily();
        [[nodiscard]] static QString resolvedSarasaFamily();
        [[nodiscard]] static QString resolvedCascadiaFamily();

    private:
        static QString resolveSearchDir(const QString &override);
    };

} // namespace Nandina::Core::Fonts

#endif // NANDINA_FONT_MANAGER_HPP
