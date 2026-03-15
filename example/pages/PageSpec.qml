pragma ComponentBehavior: Bound

import QtQuick

QtObject {
    property int pageType: -1
    required property string key
    required property string section
    required property string navTitle
    required property string title
    required property string summary
    required property string iconText
    property var icon: null
    property var data: ({})
    property var initialProperties: ({})
    property url pageSource: ""
}
