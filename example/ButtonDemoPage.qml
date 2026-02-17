pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Nandina.Controls 1.0

Item {
    id: root
    anchors.fill: parent

    property var themeManager: null
    readonly property int primary: NanButton.Primary
    readonly property int secondary: NanButton.Secondary
    readonly property int tertiary: NanButton.Tertiary
    readonly property int ghost: NanButton.Ghost
    readonly property int destructive: NanButton.Destructive
    readonly property int link: NanButton.Link
    readonly property int custom: NanButton.Custom
    readonly property int filled: NanButton.Filled
    readonly property int tonal: NanButton.Tonal
    readonly property int outlined: NanButton.Outlined
    readonly property int neutral: NanButton.Neutral
    readonly property int success: NanButton.Success
    readonly property int warn: NanButton.Warn
    readonly property int error: NanButton.Error

    property bool demoEnableHoverAnimation: true
    property bool demoEnableClickBounce: true
    property bool demoEnableHoverHighlight: true
    property real demoHoverScale: 1.03
    property real demoPressScale: 0.97
    property int demoHoverDuration: 120
    property int demoBounceInDuration: 70
    property int demoBounceOutDuration: 140
    property string demoEventLog: "等待交互..."
    readonly property string basicUsageCode: "NanButton {\n    text: \"Primary 操作\"\n    variant: NanButton.Primary\n    accent: NanButton.Filled\n    onClicked: doAction()\n}\n\nNanButton {\n    text: \"Disabled\"\n    disabled: true\n}"
    readonly property string semanticEventCode: "NanButton {\n    text: \"提交\"\n\n    onPressStarted: console.log(\"pressStarted\")\n    onClicked: console.log(\"clicked\")\n    onReleased: console.log(\"released\")\n    onCanceled: console.log(\"canceled\")\n}"
    readonly property string customThemeCode: "NanButton {\n    variant: NanButton.Custom\n    accent: NanButton.Filled\n\n    useCustomBackgroundColor: true\n    useCustomForegroundColor: true\n    useCustomHoverColor: true\n    useCustomPressedColor: true\n\n    customBackgroundColor: \"#7c3aed\"\n    customForegroundColor: \"#f8f7ff\"\n    customHoverColor: \"#6d28d9\"\n    customPressedColor: \"#5b21b6\"\n}"

    function appendDemoEvent(message) {
        demoEventLog = message;
    }

    readonly property var variantOptions: [
        {
            name: "Primary",
            value: root.primary
        },
        {
            name: "Secondary",
            value: root.secondary
        },
        {
            name: "Tertiary",
            value: root.tertiary
        },
        {
            name: "Ghost",
            value: root.ghost
        },
        {
            name: "Destructive",
            value: root.destructive
        },
        {
            name: "Link",
            value: root.link
        },
        {
            name: "Custom",
            value: root.custom
        }
    ]

    readonly property var accentOptions: [
        {
            name: "Filled",
            value: root.filled
        },
        {
            name: "Tonal",
            value: root.tonal
        },
        {
            name: "Outlined",
            value: root.outlined
        }
    ]

    readonly property var matrixEntries: {
        const entries = [];
        for (let variantIndex = 0; variantIndex < root.variantOptions.length; ++variantIndex) {
            for (let accentIndex = 0; accentIndex < root.accentOptions.length; ++accentIndex) {
                const variant = root.variantOptions[variantIndex];
                const accent = root.accentOptions[accentIndex];
                entries.push({
                    label: variant.name + " " + accent.name,
                    variant: variant.value,
                    accent: accent.value
                });
            }
        }
        return entries;
    }

    component DocsShowcaseCard: Rectangle {
        id: docsCard

        property string title: ""
        property string description: ""
        property string codeSnippet: ""
        property Component preview: null

        width: parent ? parent.width : implicitWidth
        radius: 10
        color: root.themeManager.currentPaletteCollection.secondaryPane
        border.width: 1
        border.color: root.themeManager.currentPaletteCollection.surfaceElement0
        implicitHeight: contentColumn.implicitHeight + 16

        Column {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            Text {
                text: docsCard.title
                color: root.themeManager.currentPaletteCollection.mainHeadline
                font.pixelSize: 15
                font.weight: Font.Medium
            }

            Text {
                visible: docsCard.description.length > 0
                text: docsCard.description
                color: root.themeManager.currentPaletteCollection.subHeadlines0
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }

            Rectangle {
                width: parent.width
                radius: 8
                color: root.themeManager.currentPaletteCollection.backgroundPane
                border.width: 1
                border.color: root.themeManager.currentPaletteCollection.surfaceElement0
                implicitHeight: previewLoader.active ? previewLoader.implicitHeight + 20 : 56

                Loader {
                    id: previewLoader
                    active: docsCard.preview !== null
                    sourceComponent: docsCard.preview
                    anchors.centerIn: parent
                }
            }

            Rectangle {
                width: parent.width
                radius: 8
                color: root.themeManager.currentPaletteCollection.backgroundPane
                border.width: 1
                border.color: root.themeManager.currentPaletteCollection.surfaceElement0
                implicitHeight: codeText.implicitHeight + 18

                Text {
                    id: codeText
                    anchors.fill: parent
                    anchors.margins: 9
                    text: docsCard.codeSnippet
                    color: root.themeManager.currentPaletteCollection.bodyCopy
                    font.pixelSize: 12
                    wrapMode: Text.NoWrap
                    textFormat: Text.PlainText
                }
            }
        }
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 24
        clip: true

        Column {
            id: contentColumn
            width: parent.width
            spacing: 12

            Rectangle {
                width: parent.width
                radius: 10
                color: root.themeManager.currentPaletteCollection.secondaryPane
                border.width: 1
                border.color: root.themeManager.currentPaletteCollection.surfaceElement0
                implicitHeight: controlsColumn.implicitHeight + 16

                Column {
                    id: controlsColumn
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Text {
                        text: "按钮动画调节"
                        color: root.themeManager.currentPaletteCollection.mainHeadline
                        font.pixelSize: 15
                        font.weight: Font.Medium
                    }

                    Row {
                        spacing: 12

                        CheckBox {
                            text: "Hover 动画"
                            checked: root.demoEnableHoverAnimation
                            onToggled: root.demoEnableHoverAnimation = checked
                        }

                        CheckBox {
                            text: "点击回弹"
                            checked: root.demoEnableClickBounce
                            onToggled: root.demoEnableClickBounce = checked
                        }

                        CheckBox {
                            text: "悬浮高亮"
                            checked: root.demoEnableHoverHighlight
                            onToggled: root.demoEnableHoverHighlight = checked
                        }
                    }

                    Row {
                        spacing: 10

                        Text {
                            text: "HoverScale"
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                            width: 84
                        }

                        Slider {
                            width: 180
                            from: 1.00
                            to: 1.10
                            value: root.demoHoverScale
                            onMoved: root.demoHoverScale = value
                        }

                        Text {
                            text: root.demoHoverScale.toFixed(2)
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                        }
                    }

                    Row {
                        spacing: 10

                        Text {
                            text: "PressScale"
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                            width: 84
                        }

                        Slider {
                            width: 180
                            from: 0.90
                            to: 1.00
                            value: root.demoPressScale
                            onMoved: root.demoPressScale = value
                        }

                        Text {
                            text: root.demoPressScale.toFixed(2)
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                        }
                    }

                    Row {
                        spacing: 10

                        Text {
                            text: "HoverDur"
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                            width: 84
                        }

                        Slider {
                            width: 180
                            from: 60
                            to: 260
                            value: root.demoHoverDuration
                            stepSize: 1
                            onMoved: root.demoHoverDuration = Math.round(value)
                        }

                        Text {
                            text: root.demoHoverDuration + "ms"
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                        }
                    }

                    Row {
                        spacing: 10

                        Text {
                            text: "BounceIn"
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                            width: 84
                        }

                        Slider {
                            width: 180
                            from: 40
                            to: 200
                            value: root.demoBounceInDuration
                            stepSize: 1
                            onMoved: root.demoBounceInDuration = Math.round(value)
                        }

                        Text {
                            text: root.demoBounceInDuration + "ms"
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                        }
                    }

                    Row {
                        spacing: 10

                        Text {
                            text: "BounceOut"
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                            width: 84
                        }

                        Slider {
                            width: 180
                            from: 80
                            to: 320
                            value: root.demoBounceOutDuration
                            stepSize: 1
                            onMoved: root.demoBounceOutDuration = Math.round(value)
                        }

                        Text {
                            text: root.demoBounceOutDuration + "ms"
                            color: root.themeManager.currentPaletteCollection.bodyCopy
                        }
                    }
                }
            }

            Text {
                text: "文档式展示"
                color: root.themeManager.currentPaletteCollection.mainHeadline
                font.pixelSize: 16
                font.weight: Font.Medium
            }

            DocsShowcaseCard {
                title: "基础用法"
                description: "和前端组件库类似：先看可交互预览，再看对应最小代码。"
                codeSnippet: root.basicUsageCode
                preview: Component {
                    Row {
                        spacing: 10

                        NanButton {
                            text: "Primary 操作"
                            variant: root.primary
                            accent: root.filled
                            themeManager: root.themeManager
                            onClicked: root.appendDemoEvent("Primary: clicked")
                        }

                        NanButton {
                            text: "Secondary"
                            variant: root.secondary
                            accent: root.outlined
                            themeManager: root.themeManager
                            onClicked: root.appendDemoEvent("Secondary: clicked")
                        }

                        NanButton {
                            text: "Disabled"
                            disabled: true
                            themeManager: root.themeManager
                        }
                    }
                }
            }

            DocsShowcaseCard {
                title: "语义事件"
                description: "展示 `pressStarted/clicked/released/canceled` 的触发顺序。"
                codeSnippet: root.semanticEventCode
                preview: Component {
                    Column {
                        spacing: 8

                        NanButton {
                            text: "事件按钮"
                            variant: root.primary
                            accent: root.filled
                            themeManager: root.themeManager
                            onPressStarted: root.appendDemoEvent("事件按钮: pressStarted")
                            onClicked: root.appendDemoEvent("事件按钮: clicked")
                            onReleased: root.appendDemoEvent("事件按钮: released")
                            onCanceled: root.appendDemoEvent("事件按钮: canceled")
                        }

                        Text {
                            text: "事件日志: " + root.demoEventLog
                            color: root.themeManager.currentPaletteCollection.subHeadlines0
                            font.pixelSize: 12
                        }
                    }
                }
            }

            DocsShowcaseCard {
                title: "自定义配色"
                description: "`variant: Custom` 时通过显式开关启用颜色覆盖。"
                codeSnippet: root.customThemeCode
                preview: Component {
                    NanButton {
                        text: "Custom"
                        variant: root.custom
                        accent: root.filled
                        useCustomBackgroundColor: true
                        useCustomForegroundColor: true
                        useCustomHoverColor: true
                        useCustomPressedColor: true
                        customBackgroundColor: "#7c3aed"
                        customForegroundColor: "#f8f7ff"
                        customHoverColor: "#6d28d9"
                        customPressedColor: "#5b21b6"
                        themeManager: root.themeManager
                    }
                }
            }

            Text {
                text: "Variant × Accent 全组合"
                color: root.themeManager.currentPaletteCollection.mainHeadline
                font.pixelSize: 16
                font.weight: Font.Medium
            }

            Text {
                text: "Button 基础使用示例"
                color: root.themeManager.currentPaletteCollection.mainHeadline
                font.pixelSize: 16
                font.weight: Font.Medium
            }

            Rectangle {
                width: parent.width
                radius: 10
                color: root.themeManager.currentPaletteCollection.secondaryPane
                border.width: 1
                border.color: root.themeManager.currentPaletteCollection.surfaceElement0
                implicitHeight: usageColumn.implicitHeight + 16

                Column {
                    id: usageColumn
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Row {
                        spacing: 10

                        NanButton {
                            text: "Primary 操作"
                            variant: root.primary
                            accent: root.filled
                            themeManager: root.themeManager
                            onPressStarted: root.appendDemoEvent("Primary: pressStarted")
                            onClicked: root.appendDemoEvent("Primary: clicked")
                            onReleased: root.appendDemoEvent("Primary: released")
                            onCanceled: root.appendDemoEvent("Primary: canceled")
                        }

                        NanButton {
                            text: "Secondary Outlined"
                            variant: root.secondary
                            accent: root.outlined
                            themeManager: root.themeManager
                            onClicked: root.appendDemoEvent("Secondary Outlined: clicked")
                        }

                        NanButton {
                            text: "Disabled"
                            variant: root.primary
                            accent: root.filled
                            disabled: true
                            themeManager: root.themeManager
                        }
                    }

                    Text {
                        text: "事件日志: " + root.demoEventLog
                        color: root.themeManager.currentPaletteCollection.subHeadlines0
                        font.pixelSize: 12
                    }
                }
            }

            GridLayout {
                width: parent.width
                columns: 3
                rowSpacing: 10
                columnSpacing: 10

                Repeater {
                    model: root.matrixEntries

                    delegate: NanButton {
                        required property var modelData

                        text: modelData.label
                        variant: modelData.variant
                        accent: modelData.accent
                        themeManager: root.themeManager
                        enableHoverAnimation: root.demoEnableHoverAnimation
                        enableClickBounce: root.demoEnableClickBounce
                        enableHoverHighlight: root.demoEnableHoverHighlight
                        hoverScale: root.demoHoverScale
                        pressScale: root.demoPressScale
                        hoverTransitionDuration: root.demoHoverDuration
                        clickBounceInDuration: root.demoBounceInDuration
                        clickBounceOutDuration: root.demoBounceOutDuration
                        Layout.fillWidth: true
                    }
                }
            }

            Text {
                text: "Link tone 扩展示例"
                color: root.themeManager.currentPaletteCollection.mainHeadline
                font.pixelSize: 16
                font.weight: Font.Medium
            }

            Row {
                width: parent.width
                spacing: 10

                NanButton {
                    text: "Link Neutral"
                    variant: root.link
                    accent: root.outlined
                    linkTone: root.neutral
                    themeManager: root.themeManager
                    enableHoverAnimation: root.demoEnableHoverAnimation
                    enableClickBounce: root.demoEnableClickBounce
                    enableHoverHighlight: root.demoEnableHoverHighlight
                    hoverScale: root.demoHoverScale
                    pressScale: root.demoPressScale
                    hoverTransitionDuration: root.demoHoverDuration
                    clickBounceInDuration: root.demoBounceInDuration
                    clickBounceOutDuration: root.demoBounceOutDuration
                }

                NanButton {
                    text: "Link Success"
                    variant: root.link
                    accent: root.outlined
                    linkTone: root.success
                    themeManager: root.themeManager
                    enableHoverAnimation: root.demoEnableHoverAnimation
                    enableClickBounce: root.demoEnableClickBounce
                    enableHoverHighlight: root.demoEnableHoverHighlight
                    hoverScale: root.demoHoverScale
                    pressScale: root.demoPressScale
                    hoverTransitionDuration: root.demoHoverDuration
                    clickBounceInDuration: root.demoBounceInDuration
                    clickBounceOutDuration: root.demoBounceOutDuration
                }

                NanButton {
                    text: "Link Warn"
                    variant: root.link
                    accent: root.outlined
                    linkTone: root.warn
                    themeManager: root.themeManager
                    enableHoverAnimation: root.demoEnableHoverAnimation
                    enableClickBounce: root.demoEnableClickBounce
                    enableHoverHighlight: root.demoEnableHoverHighlight
                    hoverScale: root.demoHoverScale
                    pressScale: root.demoPressScale
                    hoverTransitionDuration: root.demoHoverDuration
                    clickBounceInDuration: root.demoBounceInDuration
                    clickBounceOutDuration: root.demoBounceOutDuration
                }

                NanButton {
                    text: "Link Error"
                    variant: root.link
                    accent: root.outlined
                    linkTone: root.error
                    themeManager: root.themeManager
                    enableHoverAnimation: root.demoEnableHoverAnimation
                    enableClickBounce: root.demoEnableClickBounce
                    enableHoverHighlight: root.demoEnableHoverHighlight
                    hoverScale: root.demoHoverScale
                    pressScale: root.demoPressScale
                    hoverTransitionDuration: root.demoHoverDuration
                    clickBounceInDuration: root.demoBounceInDuration
                    clickBounceOutDuration: root.demoBounceOutDuration
                }
            }

            Text {
                text: "Custom 覆盖示例"
                color: root.themeManager.currentPaletteCollection.mainHeadline
                font.pixelSize: 16
                font.weight: Font.Medium
            }

            NanButton {
                text: "Custom"
                variant: root.custom
                accent: root.filled
                useCustomBackgroundColor: true
                useCustomForegroundColor: true
                useCustomHoverColor: true
                useCustomPressedColor: true
                useCustomBorderColor: true
                customBackgroundColor: "#7c3aed"
                customForegroundColor: "#f8f7ff"
                customHoverColor: "#6d28d9"
                customPressedColor: "#5b21b6"
                customBorderColor: "#7c3aed"
                themeManager: root.themeManager
                enableHoverAnimation: root.demoEnableHoverAnimation
                enableClickBounce: root.demoEnableClickBounce
                enableHoverHighlight: root.demoEnableHoverHighlight
                hoverScale: root.demoHoverScale
                pressScale: root.demoPressScale
                hoverTransitionDuration: root.demoHoverDuration
                clickBounceInDuration: root.demoBounceInDuration
                clickBounceOutDuration: root.demoBounceOutDuration
            }
        }
    }
}
