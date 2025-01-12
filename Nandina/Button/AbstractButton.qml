import QtQuick 2.15
import QtQuick.Controls 2.15

Control {
    id: root

    property Action action: null
    property bool autoExclusive: false
    property bool autoRepeat: false
    property int autoRepeatDelay: 300
    property int autoRepeatInterval: 100
    property bool checkable: false
    property bool checked: false
    property int display: 0
    property bool down: false
    property Image icon: null
    property real implicitIndicatorWidth: 0
    property real implicitIndicatorHeight: 0
    property Item indicator: null
    property real pressX: 0
    property real pressY: 0
    property bool pressed: false
    property string text: "Abstract Button"

    signal canceled()
    signal clicked()
    signal released()
    signal doubleClicked()
    signal pressAndHold()
    signal toggled()
    signal hovered()
    signal exited()

    implicitWidth: 120
    implicitHeight: 50

    MouseArea {
        id: buttonMouseArea

        hoverEnabled: true
        anchors.fill: parent
        onClicked: {
            if (root.action !== null) {
                root.action.trigger();
                root.clicked();
            }
        }
        onPressed: {
            root.pressed = true;
        }
        onReleased: {
            root.pressed = false;
            root.released();
        }
        onEntered: {
            root.hovered();
        }
        onExited: {
            root.exited();
        }
    }

    Text {
        id: abstractButtonText
        text: root.text 
        anchors.centerIn: parent 
    }

    function hasChildComponent(parent, componentType){
        for(var i =0; i < parent.children.length; i++){
            if(parent.children[i] instanceof componentType){
                return true;
            }
        }
        return false;
    }
}
