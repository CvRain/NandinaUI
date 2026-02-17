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

/**
 * @param {Item} item
 * @param {ThemeManager} explicitThemeManager
 * @param {ThemeManager} fallbackThemeManager
 * @return {ThemeManager}
 */
function resolveThemeManager(item, explicitThemeManager, fallbackThemeManager) {
    if (explicitThemeManager)
        return explicitThemeManager

    var parentThemeManager = findParentThemeManager(item)
    if (parentThemeManager)
        return parentThemeManager

    return fallbackThemeManager
}

function looksLikeSideBar(candidate) {
    if (!candidate)
        return false

    return candidate["side"] !== undefined
        && candidate["collapsed"] !== undefined
        && candidate["toggle"] !== undefined
}

/**
 * @param {Item} item
 * @return {Item|null}
 */
function resolveSidebar(item) {
    var current = item ? item.parent : null

    while (current) {
        if (looksLikeSideBar(current))
            return current

        var directSidebar = current["sidebar"]
        if (looksLikeSideBar(directSidebar))
            return directSidebar

        var resolvedSidebar = current["resolvedSidebar"]
        if (looksLikeSideBar(resolvedSidebar))
            return resolvedSidebar

        current = current.parent
    }

    return null
}
