import Nandina.Components
import Nandina.Theme
import Nandina.Window
import Nandina.Core
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

NandinaWindow {
    id: root

    width: 900
    height: 640
    visible: true
    title: "NandinaUI Example"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: "NandinaUI Components"
            font.pixelSize: 22
            font.bold: true
            color: ThemeManager.color.text
        }

        RowLayout {
            spacing: 12

            NanButton {
                text: "Primary"
                type: "filledPrimary"
                width: 140
                height: 44
            }

            NanButton {
                text: "Secondary"
                type: "filledSecondary"
                width: 140
                height: 44
            }

            NanButton {
                vectorIcon: IconManager.ICON_SETTINGS
                iconPosition: NanButton.IconPosition.IconOnly
                type: "outlinedPrimary"
                width: 44
                height: 44
            }
        }

        RowLayout {
            spacing: 12

            NanButton {
                text: "With Icon"
                vectorIcon: IconManager.ICON_ROCKET
                iconPosition: NanButton.IconPosition.Left
                type: "filledSuccess"
                width: 170
                height: 44
            }

            NanButton {
                text: "Ghost"
                vectorIcon: IconManager.ICON_BIRD
                type: "ghost"
                width: 140
                height: 44
            }
        }

        GroupBox {
            title: "Icons"
            Layout.fillWidth: true

            RowLayout {
                anchors.margins: 12
                spacing: 16

                NanIconItem {
                    icon: IconManager.ICON_HOME
                    width: 28
                    height: 28
                }

                NanIconItem {
                    icon: IconManager.ICON_SETTINGS
                    width: 28
                    height: 28
                }

                NanIconItem {
                    icon: IconManager.ICON_BIRD
                    width: 28
                    height: 28
                }
            }
        }

        GroupBox {
            title: "Theme Colors"
            Layout.fillWidth: true

            RowLayout {
                anchors.margins: 12
                spacing: 12

                Rectangle {
                    width: 80
                    height: 40
                    radius: 6
                    color: ThemeManager.color.base
                }

                Rectangle {
                    width: 80
                    height: 40
                    radius: 6
                    color: ThemeManager.color.surface0
                }

                Rectangle {
                    width: 80
                    height: 40
                    radius: 6
                    color: ThemeManager.color.mantle
                }

                Rectangle {
                    width: 80
                    height: 40
                    radius: 6
                    color: ThemeManager.color.blue
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
