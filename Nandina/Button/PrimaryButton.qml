import QtQuick.Controls
import Nandina.Color
import QtQuick

Button {
    required property NandinaStyle style 
    property bool isClicked: false

    id: root
    implicitWidth: 200
    implicitHeight: 50
    hoverEnabled: true
    
    background: Rectangle {
        id: buttonBackground
        color: root.style.surface1
        radius: 5

        Behavior on color {
            ColorAnimation {
                duration: 300
                easing.type: Easing.InOutQuad
            }
        }
    }

    FontLoader{
        id: tempLoader
        source: "qrc:/fonts/CascadiaMono/CaskaydiaMonoNerdFont-Regular.ttf"
    }

    Text{
        id: buttonText
        anchors.centerIn: parent
        text: "Primary button"
        color: root.style.text
        font.pixelSize: Math.min(root.width, root.height) * 0.35
        font.family: tempLoader.name
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        
        Behavior on color {
            ColorAnimation {
                duration: 300
                easing.type: Easing.InOutQuad
            }
        }
    }

    onClicked: function(){
        if(isClicked){
            buttonBackground.color = root.style.surface1
        }else{
            buttonBackground.color = root.style.overlay2
        }
        isClicked = !isClicked        
    }
}
