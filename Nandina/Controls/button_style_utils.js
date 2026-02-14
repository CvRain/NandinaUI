function withAlpha(colorValue, alphaValue) {
    return Qt.rgba(colorValue.r, colorValue.g, colorValue.b, alphaValue)
}

function resolveVariantBaseColor(palette, variant, linkTone, constants) {
    if (variant === constants.variantPrimary)
        return palette ? palette.activeBorder : "#4f8cff"
    if (variant === constants.variantSecondary)
        return palette ? palette.mainHeadline : "#9aa3bf"
    if (variant === constants.variantTertiary)
        return palette ? palette.overlay1 : "#6f7ba0"
    if (variant === constants.variantGhost)
        return palette ? palette.bodyCopy : "#e5e8f2"
    if (variant === constants.variantDestructive)
        return palette ? palette.error : "#d9534f"
    if (variant === constants.variantLink) {
        if (linkTone === constants.toneSuccess)
            return palette ? palette.success : "#6bbf59"
        if (linkTone === constants.toneWarn)
            return palette ? palette.warning : "#eab312"
        if (linkTone === constants.toneError)
            return palette ? palette.error : "#d9534f"
        return palette ? palette.links : "#6c8cff"
    }

    return palette ? palette.activeBorder : "#4f8cff"
}

function resolveColors(args) {
    var variantBaseColor = resolveVariantBaseColor(args.palette, args.variant, args.linkTone, args.constants)

    var foregroundColor = args.accent === args.constants.accentFilled
        ? (args.palette ? args.palette.onAccent : "white")
        : variantBaseColor
    if (args.variant === args.constants.variantCustom && args.useCustomForegroundColor)
        foregroundColor = args.customForegroundColor

    var backgroundColor = "transparent"
    if (args.accent === args.constants.accentFilled)
        backgroundColor = variantBaseColor
    else if (args.accent === args.constants.accentTonal)
        backgroundColor = withAlpha(variantBaseColor, 0.22)
    if (args.variant === args.constants.variantCustom && args.useCustomBackgroundColor)
        backgroundColor = args.customBackgroundColor

    var hoverColor = withAlpha(variantBaseColor, 0.12)
    if (args.accent === args.constants.accentFilled)
        hoverColor = withAlpha(variantBaseColor, 0.86)
    else if (args.accent === args.constants.accentTonal)
        hoverColor = withAlpha(variantBaseColor, 0.30)
    if (args.variant === args.constants.variantCustom && args.useCustomHoverColor)
        hoverColor = args.customHoverColor

    var pressedColor = withAlpha(variantBaseColor, 0.20)
    if (args.accent === args.constants.accentFilled)
        pressedColor = withAlpha(variantBaseColor, 0.72)
    else if (args.accent === args.constants.accentTonal)
        pressedColor = withAlpha(variantBaseColor, 0.38)
    if (args.variant === args.constants.variantCustom && args.useCustomPressedColor)
        pressedColor = args.customPressedColor

    var borderColor = "transparent"
    if (args.accent === args.constants.accentOutlined)
        borderColor = variantBaseColor
    else if (args.focused)
        borderColor = args.palette ? args.palette.activeBorder : "#4f8cff"
    if (args.variant === args.constants.variantCustom && args.useCustomBorderColor)
        borderColor = args.customBorderColor

    return {
        variantBaseColor: variantBaseColor,
        foregroundColor: foregroundColor,
        backgroundColor: backgroundColor,
        hoverColor: hoverColor,
        pressedColor: pressedColor,
        borderColor: borderColor
    }
}
