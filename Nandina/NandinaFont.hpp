#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QQmlModuleRegistration>
#include <qtmetamacros.h>

class NandinaFont : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum class FontType : int {
        Font_BoldItalic,
        Font_Bold,
        Font_ExtraLightItalic,
        Font_ExtraLight,
        Font_Italic,
        Font_LightItalic,
        Font_Light,
        FontMono_BoldItalic,
        FontMono_Bold,
        FontMono_ExtraLightItalic,
        FontMono_ExtraLight,
        FontMono_Italic,
        FontMono_LightItalic,
        FontMono_Light,
        FontMono_Regular,
        FontMono_SemiBoldItalic,
        FontMono_SemiBold,
        FontMono_SemiLightItalic,
        FontMono_SemiLight,
        FontPropo_BoldItalic,
        FontPropo_Bold,
        FontPropo_ExtraLightItalic,
        FontPropo_ExtraLight,
        FontPropo_Italic,
        FontPropo_LightItalic,
        FontPropo_Light,
        FontPropo_Regular,
        FontPropo_SemiBoldItalic,
        FontPropo_SemiBold,
        FontPropo_SemiLightItalic,
        FontPropo_SemiLight,
        Font_Regular,
        Font_SemiBoldItalic,
        Font_SemiBold,
        Font_SemiLightItalic,
        Font_SemiLight,
    };
    Q_ENUM(FontType)

    explicit NandinaFont(QObject *parent = nullptr);

    [[nodiscard]] Q_INVOKABLE QString getFontSource(const FontType &fontType);

    [[nodiscard]] Q_INVOKABLE QString getFontFamily(const FontType &fontType);

signals:
    void fontLoaded(const FontType &fontType);

private:
    std::unordered_map<FontType, QString> fontSourceMap;
    std::unordered_map<FontType, QString> fontFamilyMap;
    void loadFonts();
};
