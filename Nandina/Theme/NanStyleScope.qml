import QtQuick
import Nandina.Theme

Item {
    id: root

    default property alias contentData: contentItem.data

    implicitWidth: contentItem.implicitWidth
    implicitHeight: contentItem.implicitHeight

    Item {
        id: contentItem
        anchors.fill: parent
    }
}
