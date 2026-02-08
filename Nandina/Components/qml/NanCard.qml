import Nandina.Components
import Nandina.Theme
import Qt5Compat.GraphicalEffects
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    id: control

    readonly property string componentName: "NanCard"
    property string type: "default"
    property bool elevated: true
    property real radius: 16
    property real borderWidth: 1
    property real shadowBlur: 28
    property real shadowOffsetY: 10
    property real shadowOpacity: 0.18
    property int contentSpacing: 12
    property Component header
    property Component footer
    default property alias content: body.data
    property color currentBackgroundColor: cardStyle().background
    property color currentBorderColor: cardStyle().border
    property color currentForegroundColor: cardStyle().foreground

    function cardStyle() {
        return ComponentManager.getStyle(control.componentName, control.type);
    }

    implicitWidth: 320
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
    padding: 16

    Connections {
        function onPaletteChanged() {
            control.currentBackgroundColor = control.cardStyle().background;
            control.currentBorderColor = control.cardStyle().border;
            control.currentForegroundColor = control.cardStyle().foreground;
        }

        target: ThemeManager
    }

    background: Item {
        anchors.fill: parent

        DropShadow {
            anchors.fill: cardRect
            horizontalOffset: 0
            verticalOffset: control.shadowOffsetY
            radius: control.shadowBlur
            samples: Math.max(8, control.shadowBlur)
            color: Qt.rgba(0, 0, 0, control.shadowOpacity)
            source: cardRect
            visible: control.elevated
        }

        Rectangle {
            id: cardRect

            anchors.fill: parent
            radius: control.radius
            color: control.currentBackgroundColor
            border.color: control.currentBorderColor
            border.width: control.borderWidth
        }

    }

    contentItem: ColumnLayout {
        id: column

        spacing: control.contentSpacing
        width: parent.width

        Loader {
            id: headerLoader

            Layout.fillWidth: true
            sourceComponent: control.header
            visible: control.header !== null
        }

        Item {
            id: body

            Layout.fillWidth: true
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
        }

        Loader {
            id: footerLoader

            Layout.fillWidth: true
            sourceComponent: control.footer
            visible: control.footer !== null
        }

    }

}
