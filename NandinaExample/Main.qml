import QtQuick
import QtQuick.Controls
import Nandina

Window {
    id: window
    width: 800
    height: 600
    visible: true
    title: "Nandina Example"

    Nandina.theme: Nandina.Latte

    Column {
        spacing: 20
        anchors.centerIn: parent

        Button {
            text: "Press me"
        }

        Row {
            spacing: 10
            Button {
                text: "Latte"
                onClicked: Nandina.theme = Nandina.Latte
            }
            Button {
                text: "Frappe"
                onClicked: Nandina.theme = Nandina.Frappe
            }
            Button {
                text: "Macchiato"
                onClicked: Nandina.theme = Nandina.Macchiato
            }
            Button {
                text: "Mocha"
                onClicked: Nandina.theme = Nandina.Mocha
            }
        }
    }
}
