// ResizeHandle.qml
import QtQuick

Rectangle {
    id: resizeHandle
    color: "transparent"

    property int resizeMargin: 5
    property int cursorShape: Qt.ArrowCursor
    signal resize(real deltaX, real deltaY)

    MouseArea {
        id: resizeMouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: parent.cursorShape
        property point clickPos: "0,0"

        onPressed: mouse => {
                       clickPos = Qt.point(mouse.x, mouse.y)
                   }

        onPositionChanged: mouse => {
                               if (pressed) {
                                   var delta = Qt.point(mouse.x - clickPos.x,
                                                        mouse.y - clickPos.y)
                                   resize(delta.x, delta.y)
                               }
                           }
    }
}
