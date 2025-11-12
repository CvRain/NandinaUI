//
// Created by automated refactor on 2025/11/11.
//

#include "color_utils.hpp"
#include "../themeManager.hpp"

namespace Nandina::Theme::Utils {

    QString obtainRealColor(const QString &colorRef) {
        // 如果颜色字符串中出现了 @，则表示是引用主题颜色，需要进行转换
        if (!colorRef.startsWith("@") || colorRef == "transparent") {
            return colorRef;
        }

        // 去掉 @ 符号
        const auto colorName = colorRef.mid(1);

        // 从 ThemeManager 获取实际颜色值
        return ThemeManager::getInstance()->getColorByString(colorName);
    }

} // namespace Nandina::Theme::Utils
