//
// Created by cvrain on 2026/2/3.
//

#ifndef RAINERBLOG_FONT_COLLECTION_HPP
#define RAINERBLOG_FONT_COLLECTION_HPP

#include "Core/Types/nan_singleton.hpp"

#include <QFont>
#include <QFontDatabase>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <qqmlintegration.h>

namespace Nandina::Styles {
    class FontCollection : public Core::Types::NanSingleton<FontCollection> {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

    public:
        enum class FontFamily {
            SarasaBold = 0,
            SarasaItalic,
            SarasaLight,
            SarasaRegular,
            JetBrainsMonoBold,
            JetBrainsMonoItalic,
            JetBrainsMonoLight,
            JetBrainsMonoMedium,
            JetBrainsMonoRegular,
        };

        Q_ENUM(FontFamily)

        explicit FontCollection(QObject *parent = nullptr);

        Q_INVOKABLE [[nodiscard]] QString family(FontFamily family) const;

        Q_INVOKABLE [[nodiscard]] bool isLoaded(FontFamily family) const;

        Q_INVOKABLE [[nodiscard]] QStringList availableFamilies() const;

    private:
        void loadFont(FontFamily family, const QString &resourcePath, const QString &fallbackFamily);

        QMap<FontFamily, QString> fontFamilies{};
        QSet<FontFamily> loadedFamilies{};
    };
} // namespace Nandina::Styles


#endif // RAINERBLOG_FONT_COLLECTION_HPP
