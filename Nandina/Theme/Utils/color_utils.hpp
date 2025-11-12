//
// Created by automated refactor on 2025/11/11.
//

#ifndef NANDINAUI_COLOR_UTILS_HPP
#define NANDINAUI_COLOR_UTILS_HPP

#include <QString>

namespace Nandina::Theme::Utils {

    /**
     * @brief 将颜色引用（如 @lavender）转换为实际颜色值
     *
     * 此函数用于解析 JSON 配置文件中的颜色引用。
     * 如果颜色字符串以 @ 开头，表示引用主题颜色，需要从 ThemeManager 获取实际颜色值。
     * 如果是 "transparent" 或不以 @ 开头，则直接返回原值。
     *
     * @param colorRef 颜色引用字符串，如 "@lavender" 或 "#FF0000" 或 "transparent"
     * @return 实际的颜色值字符串
     *
     * @example
     *   obtainRealColor("@lavender")    -> "#b4befe" (从当前主题获取)
     *   obtainRealColor("#FF0000")      -> "#FF0000" (直接返回)
     *   obtainRealColor("transparent")  -> "transparent" (直接返回)
     */
    QString obtainRealColor(const QString &colorRef);

} // namespace Nandina::Theme::Utils

#endif // NANDINAUI_COLOR_UTILS_HPP
