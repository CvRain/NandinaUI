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

function findParentProperty(item, keys) {
    var current = item ? item.parent : null

    while (current) {
        for (var i = 0; i < keys.length; ++i) {
            var key = keys[i]
            var candidate = current[key]
            if (candidate !== undefined && candidate !== null)
                return candidate
        }
        current = current.parent
    }

    return null
}

function looksLikeFont(candidate) {
    if (candidate === undefined || candidate === null)
        return false

    return candidate.pixelSize !== undefined
        || candidate.pointSize !== undefined
        || candidate.family !== undefined
}

/**
 * @param {Item} item
 * @return {ThemeManager|null}
 */
function resolveInheritedThemeManager(item) {
    var inheritedThemeManager = findParentThemeManager(item)
    if (inheritedThemeManager)
        return inheritedThemeManager

    var resolvedThemeManager = findParentProperty(item, ["resolvedThemeManager"])
    if (resolvedThemeManager)
        return resolvedThemeManager

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

    var inheritedThemeManager = resolveInheritedThemeManager(item)
    if (inheritedThemeManager)
        return inheritedThemeManager

    return fallbackThemeManager
}

function resolveInheritedFont(item) {
    return findParentProperty(item, ["font", "textFont", "titleFont"])
}

function resolveFont(item, explicitFont, fallbackFont) {
    if (looksLikeFont(explicitFont))
        return explicitFont

    var inheritedFont = resolveInheritedFont(item)
    if (looksLikeFont(inheritedFont))
        return inheritedFont

    return fallbackFont
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
