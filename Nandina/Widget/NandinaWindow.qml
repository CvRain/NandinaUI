import QtQuick.Controls
import QtQuick
import Nandina.Color

ApplicationWindow {
    readonly property alias style: globalStyle
    property NandinaStyle nandinaStyle: globalStyle 
    
    signal themeChanged(int theme)
    
    
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
        currentTheme: NandinaType.CatppuccinThemeType.Latte
        onThemeChanged: {
            root.themeChanged(globalStyle.currentTheme)
        }
    }
}