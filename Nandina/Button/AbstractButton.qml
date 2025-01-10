import QtQuick.Controls
import QtQuick

Control{
    property Action action
    property bool autoExclusive: false
    property bool autoRepeat
    property int autRepeatDelay
    property int autoRepeatInterval
    property bool checkable
    property bool checked
    property int dispaly
    property bool down
    property Image icon
    property real implicitIndictorWidth
    property real implicitIndictorHeight
    property Item indicator
    property real pressX
    property real pressY
    property bool pressed
    property Text text

    signal canceled()
    signal clicked()
    signal released()
    signal doubleClicked()
    signal pressAndHold()
    signal toggled()
    signal hovered()
    signal exited()

    id: root
    implicitWidth: 120
    implicitHeight: 50

    MouseArea{
        id: buttonMouseArea
        hoverEnabled: true
        anchors.fill: parent

        onClicked: function(){
        }
    }
}