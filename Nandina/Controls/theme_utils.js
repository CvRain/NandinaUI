function looksLikeFont(candidate) {
    if (candidate === undefined || candidate === null)
        return false

    return candidate.pixelSize !== undefined
        || candidate.pointSize !== undefined
        || candidate.family !== undefined
}

function resolveFont(_item, explicitFont, fallbackFont) {
    if (looksLikeFont(explicitFont))
        return explicitFont

    if (looksLikeFont(NanStyle.font))
        return NanStyle.font

    if (looksLikeFont(NanTheme.font))
        return NanTheme.font

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
