pragma ComponentBehavior: Bound

import QtQuick

// Route descriptor registered in NanRouter.routes.
// Identified by the string `key` — no integer enum required.
QtObject {
    required property string key
    required property string section
    required property string navTitle
    required property string title
    required property string summary
    required property string iconText
    property var icon: null
    property var pageData: ({})
    property var initialProperties: ({})
    property url pageSource: ""
}
