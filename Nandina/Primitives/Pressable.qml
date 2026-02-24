import QtQuick

Item {
    id: root

    property bool enabled: true
    readonly property bool hovered: hoverHandler.hovered
    readonly property bool pressed: tapHandler.pressed

    signal clicked
    signal pressedChanged(bool pressed)

    HoverHandler {
        id: hoverHandler
        enabled: root.enabled
    }

    TapHandler {
        id: tapHandler
        enabled: root.enabled
        onPressedChanged: root.pressedChanged(pressed)
        onTapped: root.clicked()
    }
}
