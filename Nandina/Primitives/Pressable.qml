import QtQuick

Item {
    id: root

    property bool enabled: true
    property int cursorShape: Qt.PointingHandCursor
    readonly property bool hovered: hoverHandler.hovered
    readonly property bool pressed: tapHandler.pressed

    signal clicked
    signal released
    signal pressStarted
    signal canceled
    signal pressedChanged(bool pressed)

    HoverHandler {
        id: hoverHandler
        enabled: root.enabled
        cursorShape: root.enabled ? root.cursorShape : Qt.ArrowCursor
    }

    TapHandler {
        id: tapHandler
        enabled: root.enabled
        onPressedChanged: {
            root.pressedChanged(pressed);
            if (pressed)
                root.pressStarted();
        }
        onTapped: {
            root.clicked();
            root.released();
        }
        onCanceled: root.canceled()
    }
}
