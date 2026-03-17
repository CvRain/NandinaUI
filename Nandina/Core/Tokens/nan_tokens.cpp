//
// Created by cvrain on 2026/3/1.
//

#include "nan_tokens.hpp"

namespace Nandina::Core::Tokens {

    using CVT = Types::ThemeVariant::ColorVariantTypes;
    using PT  = Types::ThemeVariant::PresetTypes;
    using ST  = Types::ThemeVariant::SizeTypes;

    NanTokens::NanTokens(QObject *parent) : QObject(parent) {}

    // ── ColorVariantTypes ──────────────────────────────────────────
    int NanTokens::colorPrimary()   const { return static_cast<int>(CVT::Primary);   }
    int NanTokens::colorSecondary() const { return static_cast<int>(CVT::Secondary); }
    int NanTokens::colorTertiary()  const { return static_cast<int>(CVT::Tertiary);  }
    int NanTokens::colorSuccess()   const { return static_cast<int>(CVT::Success);   }
    int NanTokens::colorWarning()   const { return static_cast<int>(CVT::Warning);   }
    int NanTokens::colorError()     const { return static_cast<int>(CVT::Error);     }
    int NanTokens::colorSurface()   const { return static_cast<int>(CVT::Surface);   }

    // ── PresetTypes ────────────────────────────────────────────────
    int NanTokens::presetFilled()   const { return static_cast<int>(PT::Filled);   }
    int NanTokens::presetTonal()    const { return static_cast<int>(PT::Tonal);    }
    int NanTokens::presetOutlined() const { return static_cast<int>(PT::Outlined); }
    int NanTokens::presetGhost()    const { return static_cast<int>(PT::Ghost);    }
    int NanTokens::presetLink()     const { return static_cast<int>(PT::Link);     }

    // ── SizeTypes ──────────────────────────────────────────────────
    int NanTokens::sizeSm() const { return static_cast<int>(ST::Sm); }
    int NanTokens::sizeMd() const { return static_cast<int>(ST::Md); }
    int NanTokens::sizeLg() const { return static_cast<int>(ST::Lg); }

    // ── Shade lookup ───────────────────────────────────────────────
    int NanTokens::shadeIndex(int shadeNum) const {
        switch (shadeNum) {
            case 50:  return 0;
            case 100: return 1;
            case 200: return 2;
            case 300: return 3;
            case 400: return 4;
            case 500: return 5;
            case 600: return 6;
            case 700: return 7;
            case 800: return 8;
            case 900: return 9;
            case 950: return 10;
            default:  return 5; // fallback: shade-500 index
        }
    }

} // namespace Nandina::Core::Tokens
