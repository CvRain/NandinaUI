import QtQuick

Item {
    id: root

    implicitWidth: labelText.implicitWidth
    implicitHeight: labelText.implicitHeight

    property string text: ""
    property var forControl: null
    property bool disabled: false
    property bool required: false
    property var themeManager: null

    readonly property var themePalette: themeManager && themeManager.currentPaletteCollection
                                      ? themeManager.currentPaletteCollection : null

    Text {
        id: labelText
        anchors.fill: parent
        text: root.required ? (root.text + " *") : root.text
        color: root.disabled
             ? (root.themePalette ? root.themePalette.subHeadlines0 : "#9d9dac")
             : (root.themePalette ? root.themePalette.bodyCopy : "#efefef")
        font.pixelSize: 13
        font.weight: Font.Medium
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        anchors.fill: parent
        enabled: !!root.forControl
        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (root.forControl && root.forControl.forceActiveFocus)
                root.forControl.forceActiveFocus()
        }
    }
}
