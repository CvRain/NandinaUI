import Nandina.Widget
import Nandina.Button
import Nandina.Color
import Nandina.Feedback
import QtQuick

NandinaWindow {
    id: window
    title: "Nandina Example"

    Rectangle {
        id: testRectangle
        width: 100
        height: 100
        color: window.style.rosewater
        anchors.centerIn: parent
    }

    Row {
        width: parent.width
        height: 100
        spacing: 15
        anchors.top: testRectangle.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter

        PrimaryButton {
            id: primaryButton
            style: window.style
            property bool isLight: true

            onClicked: {
                if (isLight) {
                    window.style.currentTheme = NandinaType.CatppuccinThemeType.Mocha;
                } else {
                    window.style.currentTheme = NandinaType.CatppuccinThemeType.Latte;
                }
                isLight = !isLight;
                console.log("Button1 color: ", button2.background.color)
            }
        }

        PrimaryButton {
            id: button2
            style: window.style
            onClicked: {
                console.log("Button2 color: ", button2.background.color)
            }
        }
    }

    Alert{
        id: alert
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        level: Alert.AlertType.Success
        
        style: window.style
    }
}
