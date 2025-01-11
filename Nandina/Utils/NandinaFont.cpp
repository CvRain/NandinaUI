#include "NandinaFont.hpp"

#include <QFontDatabase>

NandinaFont::NandinaFont(QObject *parent): QObject(parent) {
    loadFonts();
}

QString NandinaFont::getFontSource(const FontType &fontType) {
    if (not fontSourceMap.contains(fontType)) {
        qDebug() << "Font source not found";
        return {};
    }
    return fontSourceMap.at(fontType);
}

QString NandinaFont::getFontFamily(const FontType &fontType) {
    if (not fontFamilyMap.contains(fontType)) {
        qDebug() << "Font family not found";
        return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    }
    return fontFamilyMap.at(fontType);
}

void NandinaFont::loadFonts() {
    fontSourceMap = std::unordered_map<FontType, QString>{
        std::make_pair(FontType::Font_BoldItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-BoldItalic.ttf"),
        std::make_pair(FontType::Font_Bold,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-Bold.ttf"),
        std::make_pair(FontType::Font_ExtraLightItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-ExtraLightItalic.ttf"),
        std::make_pair(FontType::Font_ExtraLight,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-ExtraLight.ttf"),
        std::make_pair(FontType::Font_Italic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-Italic.ttf"),
        std::make_pair(FontType::Font_LightItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-LightItalic.ttf"),
        std::make_pair(FontType::Font_Light,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-Light.ttf"),
        std::make_pair(FontType::FontMono_BoldItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-BoldItalic.ttf"),
        std::make_pair(FontType::FontMono_Bold,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-Bold.ttf"),
        std::make_pair(FontType::FontMono_ExtraLightItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-ExtraLightItalic.ttf"),
        std::make_pair(FontType::FontMono_ExtraLight,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-ExtraLight.ttf"),
        std::make_pair(FontType::FontMono_Italic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-Italic.ttf"),
        std::make_pair(FontType::FontMono_LightItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-LightItalic.ttf"),
        std::make_pair(FontType::FontMono_Light,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-Light.ttf"),
        std::make_pair(FontType::FontMono_Regular,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-Regular.ttf"),
        std::make_pair(FontType::FontMono_SemiBoldItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-SemiBoldItalic.ttf"),
        std::make_pair(FontType::FontMono_SemiBold,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-SemiBold.ttf"),
        std::make_pair(FontType::FontMono_SemiLightItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-SemiLightItalic.ttf"),
        std::make_pair(FontType::FontMono_SemiLight,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-SemiLight.ttf"),
        std::make_pair(FontType::FontPropo_BoldItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-BoldItalic.ttf"),
        std::make_pair(FontType::FontPropo_Bold,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-Bold.ttf"),
        std::make_pair(FontType::FontPropo_ExtraLightItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-ExtraLightItalic.ttf"),
        std::make_pair(FontType::FontPropo_ExtraLight,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-ExtraLight.ttf"),
        std::make_pair(FontType::FontPropo_Italic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-Italic.ttf"),
        std::make_pair(FontType::FontPropo_LightItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-LightItalic.ttf"),
        std::make_pair(FontType::FontPropo_Light,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-Light.ttf"),
        std::make_pair(FontType::FontPropo_Regular,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-Regular.ttf"),
        std::make_pair(FontType::FontPropo_SemiBoldItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-SemiBoldItalic.ttf"),
        std::make_pair(FontType::FontPropo_SemiBold,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-SemiBold.ttf"),
        std::make_pair(FontType::FontPropo_SemiLightItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-SemiLightItalic.ttf"),
        std::make_pair(FontType::FontPropo_SemiLight,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-SemiLight.ttf"),
        std::make_pair(FontType::Font_Regular,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-Regular.ttf"),
        std::make_pair(FontType::Font_SemiBoldItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-SemiBoldItalic.ttf"),
        std::make_pair(FontType::Font_SemiBold,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-SemiBold.ttf"),
        std::make_pair(FontType::Font_SemiLightItalic,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-SemiLightItalic.ttf"),
        std::make_pair(FontType::Font_SemiLight,
                       ":/fonts/CascadiaMono/CaskaydiaMonoNerdFont-SemiLight.ttf")
    };

    std::ranges::for_each(fontSourceMap, [&](auto &oneFont) {
        qDebug() << "Add font:" << oneFont.second;
        const auto fontId = QFontDatabase::addApplicationFont(oneFont.second);
        if (fontId == -1) {
            qDebug() << "Add font failed:" << oneFont.second;
            return;
        }
        const auto families = QFontDatabase::applicationFontFamilies(fontId);
        if (families.isEmpty()) {
            qWarning() << "No font families found for:" << oneFont.second;
            return;
        }

        const auto family = families.at(0);
        qDebug() << "Loaded font family:" << family << "for font:" << oneFont.second;
        fontFamilyMap.insert(std::make_pair(oneFont.first, family));
    });
}
