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
    property real level: Alert.AlertType.Info
    property string message: "default alert messages"
    property Image icon: null
    property real duration: 3000
    property bool autoClose: true

    
    readonly property color successColor : "#f6ffed"
    readonly property color successBorderColor: "#b7eb8f"
    readonly property color errorColor : "#fff1f0"
    readonly property color errorBorderColor: "#ffccc7"
    readonly property color warningColor : "#fffbe6"
    readonly property color warningBorderColor: "#ffe58f"
    readonly property color infoColor : "#b7dffd"
    readonly property color infoBorderColor: "#91caff"
    
    

    signal closed()
    signal showed()

    id: root
    implicitWidth: 300
    implicitHeight: 35
    radius: 5
    color: root.infoColor
    border.color: root.infoBorderColor
    visible: false
    
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
            anchors.verticalCenter: parent.verticalCenter
        }

        Text{
            width: row.width - icon.width
            text: root.message
            font.family: NandinaFont.getFontFamily(NandinaFont.FontMono_Bold)
            font.pixelSize: Math.min(root.width, root.height) * 0.35
            color: root.style.text
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    onLevelChanged: {
        upateColor()
        console.debug("Alert level changed to " + level)
    }

    Component.onCompleted: {
        upateColor()
    }
    
    function upateColor() {
        let backgroundColor = alertBgColor()
        let borderColor = alertBorderColor()
        if(style.currentTheme === NandinaType.CatppuccinThemeType.Latte){
            root.color = Qt.darker(backgroundColor, 1.09)
            root.border.color = Qt.darker(borderColor, 1.09)
        }else{
            root.color = Qt.darker(backgroundColor, 0.94)
            root.border.color = Qt.darker(borderColor, 0.94)
        }
    }

    
    function alertBgColor() {
        var color = "transparent"
        switch(level){
            case Alert.AlertType.Success:
                color = root.successColor
            case Alert.AlertType.Error:
                color =  root.errorColor
            case Alert.AlertType.Warning:
                color =  root.warningColor
            case Alert.AlertType.Info:
                color = root.infoColor
        }
        if(style.currentTheme === NandinaType.CatppuccinThemeType.Latte){
            Qt.lighter(color, 1.09)
        }else{
            Qt.lighter(color, 0.94)
        }

        return color
    }

    
    function alertBorderColor() {
        var color = "transparent"
        switch(level){
            case Alert.AlertType.Success:
                color = root.successBorderColor
            case Alert.AlertType.Error:
                color =  root.errorBorderColor
            case Alert.AlertType.Warning:
                color =  root.warningBorderColor
            case Alert.AlertType.Info:
                color = root.infoBorderColor
        }
        return color
    }

    
    function show(duration, autoClose = true) {
        root.visible = true
        
    }
}
