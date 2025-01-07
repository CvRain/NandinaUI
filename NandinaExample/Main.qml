import QtQuick
import QtQuick.Controls
import Nandina.Color

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    color: globalStyle.base

    Behavior on color {
        ColorAnimation {
            duration: 300
            easing.type: Easing.InOutQuad
        }
    }

    Rectangle {
        id: testRectangle
        width: 300
        height: 300
        radius: 15
        anchors.centerIn: parent
        color: globalStyle.surface0

        MouseArea {
            anchors.fill: parent
            onClicked: {
                globalStyle.currentTheme = NandinaType.CatppuccinThemeType.Macchiato;
            }
        }

        Behavior on color {
            ColorAnimation {
                duration: 300
                easing.type: Easing.InOutQuad
            }
        }
    }

    Button {
        id: changeThemeButton
        width: 200
        height: 50
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20

        // 添加文本显示当前主题
        text: "Switch Theme"

        // 添加背景色过渡
        background: Rectangle {
            color: globalStyle.surface0
            radius: 6

            Behavior on color {
                ColorAnimation {
                    duration: 300
                    easing.type: Easing.InOutQuad
                }
            }
        }

        // 添加文本颜色过渡
        contentItem: Text {
            text: changeThemeButton.text
            color: globalStyle.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            Behavior on color {
                ColorAnimation {
                    duration: 300
                    easing.type: Easing.InOutQuad
                }
            }
        }

        // 循环切换主题
        property int currentIndex: 0
        onClicked: {
            currentIndex = (currentIndex + 1) % 4;
            switch (currentIndex) {
            case 0:
                globalStyle.currentTheme = NandinaType.CatppuccinThemeType.Latte;
                break;
            case 1:
                globalStyle.currentTheme = NandinaType.CatppuccinThemeType.Frappe;
                break;
            case 2:
                globalStyle.currentTheme = NandinaType.CatppuccinThemeType.Macchiato;
                break;
            case 3:
                globalStyle.currentTheme = NandinaType.CatppuccinThemeType.Mocha;
                break;
            }
        }
    }

    Component.onCompleted: {
        console.log("Hello world!");
    }

    NandinaStyle {
        id: globalStyle
        onThemeChanged: {
            console.log("theme changed new base color", globalStyle.base);
        }
    }
}
