import QtQuick.Controls
import Nandina.Color
import Nandina.Utils
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

    Text{
        id: buttonText
        anchors.centerIn: parent
        text: "Primary button"
        color: root.style.text
        font.pixelSize: Math.min(root.width, root.height) * 0.35
        font.family: NandinaFont.getFontFamily(NandinaFont.FontMono_Bold)
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

    Connections{
        target: root.style

        function onThemeChanged(){
            if(root.isClicked){
                buttonBackground.color = root.style.overlay2
            }else{
                buttonBackground.color = root.style.surface1
            }
        }
    }
}
