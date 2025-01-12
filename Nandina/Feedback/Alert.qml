import QtQuick
import Nandina.Color
import Nandina.Utils

Rectangle {
    enum AlertType{
        Success,
        Error,
        Warning,
        Info
    }
    
    required property NandinaStyle style 
    property int level: Alert.AlertType.Info
    property string message: ""
    property Image icon: null
    property real duration: 3000
    property bool autoClose: true

    signal closed()
    signal showed()

    id: root
    implicitWidth: 300
    implicitHeight: 35
    radius: 5
    color: {
        switch(level){
            case Alert.AlertType.Success:
                return root.style.green
            case Alert.AlertType.Error:
                return root.style.red
            case Alert.AlertType.Warning:
                return root.style.yellow
            case Alert.AlertType.Info:
                return root.style.blue
        }
    }
    
    Row{
        id: row
        spacing: 10
        width: parent.width * 0.9
        height: parent.height * 0.9
        anchors.centerIn: parent

        Image {
            id: icon
            width: row.width
            height: row.width
        }

        Text{
            width: row.width - icon.width
            text: message
            color: root.style.text
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }
    }

}
