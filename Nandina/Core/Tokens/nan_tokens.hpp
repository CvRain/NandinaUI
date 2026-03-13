//
// Created by cvrain on 2026/3/1.
//

#ifndef NANDINA_NAN_TOKENS_HPP
#define NANDINA_NAN_TOKENS_HPP

#include <QObject>
#include <QQmlEngine>

#include "theme_type.hpp"

namespace Nandina::Core::Tokens {

    // ═══════════════════════════════════════════════════════════════
    //  NanTokens — Design-system constants exposed to QML
    //
    //  Centralises:
    //    - ColorVariantTypes int values (Primary … Surface)
    //    - PresetTypes int values (Filled … Link)
    //    - SizeTypes int values (Sm / Md / Lg)
    //    - shadeIndex(int shadeNum) → 0-based array index in ColorPalette
    //
    //  QML usage:
    //    import Nandina.Tokens
    //
    //    property int colorVariant: NanTokens.colorPrimary
    //    _palette.shade(NanTokens.shadeIndex(500))  // → shade at index 5
    // ═══════════════════════════════════════════════════════════════

    class NanTokens : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        // ── ColorVariantTypes ──────────────────────────────────────
        Q_PROPERTY(int colorPrimary   READ colorPrimary   CONSTANT)
        Q_PROPERTY(int colorSecondary READ colorSecondary CONSTANT)
        Q_PROPERTY(int colorTertiary  READ colorTertiary  CONSTANT)
        Q_PROPERTY(int colorSuccess   READ colorSuccess   CONSTANT)
        Q_PROPERTY(int colorWarning   READ colorWarning   CONSTANT)
        Q_PROPERTY(int colorError     READ colorError     CONSTANT)
        Q_PROPERTY(int colorSurface   READ colorSurface   CONSTANT)

        // ── PresetTypes ────────────────────────────────────────────
        Q_PROPERTY(int presetFilled   READ presetFilled   CONSTANT)
        Q_PROPERTY(int presetTonal    READ presetTonal    CONSTANT)
        Q_PROPERTY(int presetOutlined READ presetOutlined CONSTANT)
        Q_PROPERTY(int presetGhost    READ presetGhost    CONSTANT)
        Q_PROPERTY(int presetLink     READ presetLink     CONSTANT)

        // ── SizeTypes ──────────────────────────────────────────────
        Q_PROPERTY(int sizeSm READ sizeSm CONSTANT)
        Q_PROPERTY(int sizeMd READ sizeMd CONSTANT)
        Q_PROPERTY(int sizeLg READ sizeLg CONSTANT)

    public:
        explicit NanTokens(QObject *parent = nullptr);

        // ── ColorVariantTypes getters ──────────────────────────────
        [[nodiscard]] int colorPrimary()   const;
        [[nodiscard]] int colorSecondary() const;
        [[nodiscard]] int colorTertiary()  const;
        [[nodiscard]] int colorSuccess()   const;
        [[nodiscard]] int colorWarning()   const;
        [[nodiscard]] int colorError()     const;
        [[nodiscard]] int colorSurface()   const;

        // ── PresetTypes getters ────────────────────────────────────
        [[nodiscard]] int presetFilled()   const;
        [[nodiscard]] int presetTonal()    const;
        [[nodiscard]] int presetOutlined() const;
        [[nodiscard]] int presetGhost()    const;
        [[nodiscard]] int presetLink()     const;

        // ── SizeTypes getters ──────────────────────────────────────
        [[nodiscard]] int sizeSm() const;
        [[nodiscard]] int sizeMd() const;
        [[nodiscard]] int sizeLg() const;

        // ── Shade lookup ───────────────────────────────────────────
        /// Maps a Tailwind shade number (50…950) to its 0-based array index
        /// used by ColorPalette::shade(int idx).
        /// Returns 5 (shade-500) for any unrecognised input.
        Q_INVOKABLE [[nodiscard]] int shadeIndex(int shadeNum) const;
    };

} // namespace Nandina::Core::Tokens

#endif // NANDINA_NAN_TOKENS_HPP
