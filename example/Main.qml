import Nandina.Color
import Nandina.Theme
import QtQuick
import QtQuick.Controls

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")


    ThemeManager {
        id: themeManager

        property ColorCollection legacyColorCollection
        property PaletteCollection legacyPaletteCollection

        customColorCollection: legacyColorCollection
        customPaletteCollection: legacyPaletteCollection

        legacyColorCollection: ColorCollection {
            rosewater: "#c9f6e8"
            flamingo: "#a9e8d3"
            pink: "#80ddbf"
            mauve: "#5ed1aa"
            red: "#d31b76"
            maroon: "#a80a5c"
            peach: "#eab312"
            yellow: "#f0ca55"
            green: "#85cc21"
            teal: "#4f46e5"
            sky: "#54c1f1"
            sapphire: "#01a5ea"
            blue: "#00a470"
            lavender: "#8a84ec"
            text: "#1f2741"
            subtext1: "#303b62"
            subtext0: "#39466e"
            overlay2: "#41507f"
            overlay1: "#495a90"
            overlay0: "#6876a2"
            surface2: "#8892b3"
            surface1: "#a6adc8"
            surface0: "#c6c9d7"
            base: "#e4e5ec"
            mantle: "#c6c9d7"
            crust: "#a6adc8"
        }

        legacyPaletteCollection: PaletteCollection {
            backgroundPane: "#e4e5ec"
            secondaryPane: "#c6c9d7"
            surfaceElement0: "#a6adc8"
            surfaceElement1: "#8892b3"
            surfaceElement2: "#6876a2"
            overlay0: "#495a90"
            overlay1: "#41507f"
            overlay2: "#39466e"
            bodyCopy: "#1f2741"
            mainHeadline: "#1f2741"
            subHeadlines0: "#303b62"
            subHeadlines1: "#39466e"
            subtle: "#41507f"
            onAccent: "#e4e5ec"
            links: "#11ba81"
            success: "#85cc21"
            warning: "#eab312"
            error: "#d31b76"
            tags: "#01a5ea"
            selectionBackground: "#a6adc8"
            cursor: "#11ba81"
            cursorText: "#e4e5ec"
            activeBorder: "#11ba81"
            inactiveBorder: "#6876a2"
            bellBorder: "#eab312"
            color0: "#39466e"
            color1: "#d31b76"
            color2: "#85cc21"
            color3: "#eab312"
            color4: "#11ba81"
            color5: "#4f46e5"
            color6: "#01a5ea"
            color7: "#a6adc8"
            color8: "#495a90"
            color9: "#a80a5c"
            color10: "#85cc21"
            color11: "#f0ca55"
            color12: "#5ed1aa"
            color13: "#8a84ec"
            color14: "#54c1f1"
            color15: "#8892b3"
            color16: "#a9e8d3"
            color17: "#c9f6e8"
            mark1: "#11ba81"
            mark2: "#4f46e5"
            mark3: "#01a5ea"
            mark1Text: "#e4e5ec"
            mark2Text: "#e4e5ec"
            mark3Text: "#e4e5ec"
        }

    }

    Button {
        id: button

        property int paletteIndex: 0

        width: 140
        height: 55
        anchors.top: parent.top
        anchors.topMargin: 15
        anchors.right: parent.right
        anchors.rightMargin: 15
        onClicked: {
            paletteIndex = (paletteIndex + 1) % 5;
            const paletteTypes = [NandinaColor.PaletteType.Latte, NandinaColor.PaletteType.Frappe, NandinaColor.PaletteType.Macchiato, NandinaColor.PaletteType.Mocha, NandinaColor.PaletteType.Custom];
            themeManager.currentPaletteType = paletteTypes[paletteIndex];
        }
    }

    Rectangle {
        id: rectangle

        width: 200
        height: 60
        anchors.centerIn: parent
        radius: 15
        color: themeManager.currentPaletteCollection.backgroundPane

        Text {
            id: someText

            text: themeManager.currentPaletteType
            width: 120
            height: 45
            anchors.centerIn: parent
            font.pixelSize: 28
            color: themeManager.currentPaletteCollection.bodyCopy
        }

    }

}
