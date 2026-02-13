import QtQuick

Rectangle {
    id: root

    width: 42
    height: 30
    radius: 4
    color: "transparent"

    property string text: ""
    property bool isCloseButton: false
    property color textColor: "white"
    property color hoverColor: "transparent"
    property color pressedColor: "transparent"
    property color closeHoverColor: "#d9534f"
    property color closePressedColor: "#c64541"
    property bool useAccentForHover: false
    property color accentColor: "#4f8cff"
    property real accentHoverAlpha: 0.28
    property real accentPressedAlpha: 0.40
    property real iconLineWidth: 1.4

    readonly property color resolvedHoverColor: {
        if (isCloseButton)
            return closeHoverColor
        if (!useAccentForHover)
            return hoverColor
        return Qt.rgba(accentColor.r, accentColor.g, accentColor.b, accentHoverAlpha)
    }

    readonly property color resolvedPressedColor: {
        if (isCloseButton)
            return closePressedColor
        if (!useAccentForHover)
            return pressedColor
        return Qt.rgba(accentColor.r, accentColor.g, accentColor.b, accentPressedAlpha)
    }

    readonly property string iconType: {
        if (isCloseButton)
            return "close"
        if (text === "—" || text === "-")
            return "minimize"
        if (text === "□")
            return "maximize"
        if (text === "❐")
            return "restore"
        return "text"
    }

    signal clicked

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: {
            if (mouseArea.pressed)
                return root.resolvedPressedColor
            if (mouseArea.containsMouse)
                return root.resolvedHoverColor
            return "transparent"
        }

        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }
    }

    Text {
        visible: root.iconType === "text"
        anchors.centerIn: parent
        text: root.text
        color: root.textColor
        font.pixelSize: 12
        font.bold: root.isCloseButton
    }

    Canvas {
        id: iconCanvas
        visible: root.iconType !== "text"
        anchors.centerIn: parent
        width: 12
        height: 12
        antialiasing: true

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()

            const w = width
            const h = height
            const stroke = root.isCloseButton ? "white" : root.textColor

            ctx.strokeStyle = stroke
            ctx.fillStyle = stroke
            ctx.lineWidth = root.iconLineWidth
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            if (root.iconType === "close") {
                ctx.beginPath()
                ctx.moveTo(2, 2)
                ctx.lineTo(w - 2, h - 2)
                ctx.moveTo(w - 2, 2)
                ctx.lineTo(2, h - 2)
                ctx.stroke()
                return
            }

            if (root.iconType === "minimize") {
                ctx.beginPath()
                ctx.moveTo(2, h - 3)
                ctx.lineTo(w - 2, h - 3)
                ctx.stroke()
                return
            }

            if (root.iconType === "maximize") {
                ctx.strokeRect(2, 2, w - 4, h - 4)
                return
            }

            if (root.iconType === "restore") {
                ctx.strokeRect(4, 2, w - 6, h - 6)
                ctx.strokeRect(2, 4, w - 6, h - 6)
            }
        }
    }

    onIconTypeChanged: iconCanvas.requestPaint()
    onTextColorChanged: iconCanvas.requestPaint()
    onIconLineWidthChanged: iconCanvas.requestPaint()

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
