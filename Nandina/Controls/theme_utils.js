function findParentThemeManager(item) {
    var themeManagerKey = "theme" + "Manager"
    var current = item ? item.parent : null

    while (current) {
        var candidate = current[themeManagerKey]
        if (candidate !== undefined && candidate !== null)
            return candidate
        current = current.parent
    }

    return null
}

function resolveThemeManager(item, explicitThemeManager, fallbackThemeManager) {
    if (explicitThemeManager)
        return explicitThemeManager

    var parentThemeManager = findParentThemeManager(item)
    if (parentThemeManager)
        return parentThemeManager

    return fallbackThemeManager
}
