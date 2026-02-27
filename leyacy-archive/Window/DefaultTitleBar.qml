import QtQuick

CornerRectangle {
    id: root

    implicitHeight: 42

    required property var targetWindow
    required property var themeManager
    property string titleText: "Nandina"
    property url windowIconSource: ""
    property bool showWindowIcon: true
    property int windowIconSize: 16
    readonly property bool hasWindowIcon: showWindowIcon && windowIconSource.toString().length > 0
    property bool draggable: true
    property bool showWindowControls: true
    property bool enableDoubleClickToggleMaximize: true

    fillColor: themeManager.currentPaletteCollection ? themeManager.currentPaletteCollection.secondaryPane : "transparent"
    bottomLeftRadius: 0
    bottomRightRadius: 0

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.surfaceElement0 : "transparent"
        opacity: 0.7
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 14
        spacing: 9

        Rectangle {
            width: root.showWindowIcon ? root.windowIconSize : 10
            height: root.showWindowIcon ? root.windowIconSize : 10
            radius: root.showWindowIcon ? 4 : 5
            color: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.surfaceElement0 : "#6b7280"

            Image {
                anchors.fill: parent
                anchors.margins: 1
                visible: root.hasWindowIcon
                source: root.windowIconSource
                fillMode: Image.PreserveAspectFit
                mipmap: true
                smooth: true
            }

            Text {
                anchors.centerIn: parent
                visible: root.showWindowIcon && !root.hasWindowIcon
                text: root.titleText.length > 0 ? root.titleText.charAt(0).toUpperCase() : "N"
                color: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.bodyCopy : "#f3f4f6"
                font.pixelSize: Math.max(10, root.windowIconSize - 6)
                font.weight: Font.DemiBold
            }
        }

        Text {
            text: root.titleText
            color: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.bodyCopy : "#f3f4f6"
            font.pixelSize: 13
            font.weight: Font.Medium
        }
    }

    Row {
        visible: root.showWindowControls
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 6
        spacing: 2

        TitleBarButton {
            text: "—"
            width: 42
            height: 30
            textColor: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.bodyCopy : "white"
            hoverColor: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.overlay0 : "#4a4a4a"
            pressedColor: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.overlay1 : "#5a5a5a"
            useAccentForHover: true
            accentColor: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.activeBorder : "#4f8cff"
            onClicked: root.targetWindow.showMinimized()
        }

        TitleBarButton {
            text: root.targetWindow.visibility === Window.Maximized ? "❐" : "□"
            width: 42
            height: 30
            textColor: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.bodyCopy : "white"
            hoverColor: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.overlay0 : "#4a4a4a"
            pressedColor: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.overlay1 : "#5a5a5a"
            useAccentForHover: true
            accentColor: root.themeManager.currentPaletteCollection ? root.themeManager.currentPaletteCollection.activeBorder : "#4f8cff"
            onClicked: {
                if (root.targetWindow.visibility === Window.Maximized)
                    root.targetWindow.showNormal();
                else
                    root.targetWindow.showMaximized();
            }
        }

        TitleBarButton {
            text: "✕"
            isCloseButton: true
            width: 42
            height: 30
            textColor: "white"
            onClicked: root.targetWindow.close()
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        z: -1
        onPressed: {
            if (!root.draggable)
                return;
            if (root.targetWindow.visibility !== Window.FullScreen && root.targetWindow.visibility !== Window.Maximized) {
                root.targetWindow.startSystemMove();
            }
        }
        onDoubleClicked: {
            if (!root.enableDoubleClickToggleMaximize)
                return;
            if (root.targetWindow.visibility === Window.FullScreen)
                return;
            if (root.targetWindow.visibility === Window.Maximized)
                root.targetWindow.showNormal();
            else
                root.targetWindow.showMaximized();
        }
    }
}
