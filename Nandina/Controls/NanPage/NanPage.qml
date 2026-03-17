pragma ComponentBehavior: Bound

import QtQuick

NanSurface {
    id: root

    property var pageData: ({})

    // Injected by NanRouter when injectRouterContext is true
    property var routeSpec: null
    property var router: null
    property var routeParams: ({})

    default property alias content: _content.data

    bordered: false
    radius: 0

    Item {
        id: _content
        anchors.fill: parent
    }
}
