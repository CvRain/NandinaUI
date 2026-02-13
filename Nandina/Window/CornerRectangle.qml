import QtQuick

Item {
    id: root

    property color fillColor: "transparent"
    property color borderColor: "transparent"
    property real borderWidth: 0

    property real topLeftRadius: 0
    property real topRightRadius: 0
    property real bottomRightRadius: 0
    property real bottomLeftRadius: 0

    default property alias data: contentRoot.data

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            const ctx = getContext("2d")
            const width = root.width
            const height = root.height

            if (width <= 0 || height <= 0)
                return

            ctx.reset()

            const maxRadius = Math.min(width, height) / 2
            const tl = Math.max(0, Math.min(root.topLeftRadius, maxRadius))
            const tr = Math.max(0, Math.min(root.topRightRadius, maxRadius))
            const br = Math.max(0, Math.min(root.bottomRightRadius, maxRadius))
            const bl = Math.max(0, Math.min(root.bottomLeftRadius, maxRadius))

            ctx.beginPath()
            ctx.moveTo(tl, 0)
            ctx.lineTo(width - tr, 0)
            if (tr > 0)
                ctx.quadraticCurveTo(width, 0, width, tr)
            ctx.lineTo(width, height - br)
            if (br > 0)
                ctx.quadraticCurveTo(width, height, width - br, height)
            ctx.lineTo(bl, height)
            if (bl > 0)
                ctx.quadraticCurveTo(0, height, 0, height - bl)
            ctx.lineTo(0, tl)
            if (tl > 0)
                ctx.quadraticCurveTo(0, 0, tl, 0)
            ctx.closePath()

            ctx.fillStyle = root.fillColor
            ctx.fill()

            if (root.borderWidth > 0) {
                ctx.lineWidth = root.borderWidth
                ctx.strokeStyle = root.borderColor
                ctx.stroke()
            }
        }
    }

    Item {
        id: contentRoot
        anchors.fill: parent
    }

    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
    onFillColorChanged: canvas.requestPaint()
    onBorderColorChanged: canvas.requestPaint()
    onBorderWidthChanged: canvas.requestPaint()
    onTopLeftRadiusChanged: canvas.requestPaint()
    onTopRightRadiusChanged: canvas.requestPaint()
    onBottomRightRadiusChanged: canvas.requestPaint()
    onBottomLeftRadiusChanged: canvas.requestPaint()
}
