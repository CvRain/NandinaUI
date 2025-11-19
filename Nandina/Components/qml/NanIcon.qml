import QtQuick

Item {
    id: root

    property string icon: "none" // e.g., "maximize", "minimize", "close"
    property color color: "black"
    property real strokeWidth: 2
    property alias size: canvas.width

    Canvas {
        id: canvas

        anchors.fill: parent
        antialiasing: true
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.strokeStyle = root.color;
            ctx.lineWidth = root.strokeWidth;
            ctx.lineCap = "round";
            ctx.lineJoin = "round";
            var w = width;
            var h = height;
            var p = root.strokeWidth / 2; // padding
            switch (root.icon) {
            case "maximize":
                ctx.beginPath();
                ctx.rect(p, p, w - p * 2, h - p * 2);
                ctx.stroke();
                break;
            case "minimize":
                ctx.beginPath();
                ctx.moveTo(p, h / 2);
                ctx.lineTo(w - p, h / 2);
                ctx.stroke();
                break;
            case "close":
                ctx.beginPath();
                ctx.moveTo(p, p);
                ctx.lineTo(w - p, h - p);
                ctx.moveTo(w - p, p);
                ctx.lineTo(p, h - p);
                ctx.stroke();
                break;
            }
        }
        Component.onCompleted: {
            requestPaint();
        }
        onIconChanged: requestPaint()
        onColorChanged: requestPaint()
        onStrokeWidthChanged: requestPaint()
        onSizeChanged: requestPaint()
    }

}
