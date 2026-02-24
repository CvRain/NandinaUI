import QtQuick

QtObject {
    id: root

    property bool focused: false
    property bool disabled: false
    property bool hasError: false
    property bool hasSuccess: false

    readonly property string visualState: {
        if (disabled)
            return "disabled";
        if (hasError)
            return "error";
        if (hasSuccess)
            return "success";
        if (focused)
            return "focused";
        return "default";
    }
}
