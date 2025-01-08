import QtQuick.Controls
import QtQuick
import Nandina.Color

ApplicationWindow {
    id: root
    width: 800
    height: 600
    visible: true
    title: qsTr("Nandina")
    color: globalStyle.base

    Behavior on color {
        ColorAnimation {
            duration: 300
            easing.type: Easing.InOutQuad
        }
    }
    
    NandinaStyle {
        id: globalStyle
        onThemeChanged: {
            console.log("theme changed new base color", globalStyle.base);
        }
    }
}