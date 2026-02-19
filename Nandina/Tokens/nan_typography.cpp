#include "nan_typography.hpp"

namespace Nandina::NandinaTokens {
    QFont NanTypography::makeFont(const int pixelSize, const QFont::Weight weight) {
        QFont font;
        font.setPixelSize(pixelSize);
        font.setWeight(weight);
        return font;
    }

    NanTypography::NanTypography(QObject *parent)
        : NandinaCore::Types::NanSingleton<NanTypography>(parent),
                    display_font(makeFont(36, QFont::DemiBold)),
                    title_large_font(makeFont(28, QFont::DemiBold)),
                    title_font(makeFont(22, QFont::DemiBold)),
                    subtitle_font(makeFont(18, QFont::DemiBold)),
                    body_large_font(makeFont(16, QFont::Normal)),
                    body_font(makeFont(14, QFont::Normal)),
                    body_strong_font(makeFont(14, QFont::DemiBold)),
                    caption_font(makeFont(12, QFont::Normal)) {
    }
}
