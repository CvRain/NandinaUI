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
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-BoldItalic.ttf"),
        std::make_pair(FontType::Font_Bold,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-Bold.ttf"),
        std::make_pair(FontType::Font_ExtraLightItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-ExtraLightItalic.ttf"),
        std::make_pair(FontType::Font_ExtraLight,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-ExtraLight.ttf"),
        std::make_pair(FontType::Font_Italic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-Italic.ttf"),
        std::make_pair(FontType::Font_LightItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-LightItalic.ttf"),
        std::make_pair(FontType::Font_Light,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-Light.ttf"),
        std::make_pair(FontType::FontMono_BoldItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-BoldItalic.ttf"),
        std::make_pair(FontType::FontMono_Bold,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-Bold.ttf"),
        std::make_pair(FontType::FontMono_ExtraLightItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-ExtraLightItalic.ttf"),
        std::make_pair(FontType::FontMono_ExtraLight,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-ExtraLight.ttf"),
        std::make_pair(FontType::FontMono_Italic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-Italic.ttf"),
        std::make_pair(FontType::FontMono_LightItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-LightItalic.ttf"),
        std::make_pair(FontType::FontMono_Light,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-Light.ttf"),
        std::make_pair(FontType::FontMono_Regular,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-Regular.ttf"),
        std::make_pair(FontType::FontMono_SemiBoldItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-SemiBoldItalic.ttf"),
        std::make_pair(FontType::FontMono_SemiBold,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-SemiBold.ttf"),
        std::make_pair(FontType::FontMono_SemiLightItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-SemiLightItalic.ttf"),
        std::make_pair(FontType::FontMono_SemiLight,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontMono-SemiLight.ttf"),
        std::make_pair(FontType::FontPropo_BoldItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-BoldItalic.ttf"),
        std::make_pair(FontType::FontPropo_Bold,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-Bold.ttf"),
        std::make_pair(FontType::FontPropo_ExtraLightItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-ExtraLightItalic.ttf"),
        std::make_pair(FontType::FontPropo_ExtraLight,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-ExtraLight.ttf"),
        std::make_pair(FontType::FontPropo_Italic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-Italic.ttf"),
        std::make_pair(FontType::FontPropo_LightItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-LightItalic.ttf"),
        std::make_pair(FontType::FontPropo_Light,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-Light.ttf"),
        std::make_pair(FontType::FontPropo_Regular,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-Regular.ttf"),
        std::make_pair(FontType::FontPropo_SemiBoldItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-SemiBoldItalic.ttf"),
        std::make_pair(FontType::FontPropo_SemiBold,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-SemiBold.ttf"),
        std::make_pair(FontType::FontPropo_SemiLightItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-SemiLightItalic.ttf"),
        std::make_pair(FontType::FontPropo_SemiLight,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFontPropo-SemiLight.ttf"),
        std::make_pair(FontType::Font_Regular,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-Regular.ttf"),
        std::make_pair(FontType::Font_SemiBoldItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-SemiBoldItalic.ttf"),
        std::make_pair(FontType::Font_SemiBold,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-SemiBold.ttf"),
        std::make_pair(FontType::Font_SemiLightItalic,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-SemiLightItalic.ttf"),
        std::make_pair(FontType::Font_SemiLight,
                       "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-SemiLight.ttf")
    };

    std::ranges::for_each(fontSourceMap, [&](auto &oneFont) {
        qDebug() << "Add font:" << oneFont.second;
        const auto fontId = QFontDatabase::addApplicationFont(oneFont.second);
        if(fontId == -1){
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
