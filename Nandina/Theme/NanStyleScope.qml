import QtQuick
import Nandina.Theme

Item {
    id: root

    default property alias contentData: contentItem.data

    property var parentThemeManager: styleUtilsBridge.resolveParentThemeManager(root)
    property var themeManager: parentThemeManager ? parentThemeManager : fallbackThemeManager
    property font font: styleUtilsBridge.resolveParentFont(root)

    readonly property bool inheritsParentTheme: parentThemeManager !== null

    implicitWidth: contentItem.implicitWidth
    implicitHeight: contentItem.implicitHeight

    Item {
        id: contentItem
        anchors.fill: parent
    }

    ThemeManager {
        id: fallbackThemeManager
    }

    QtObject {
        id: styleUtilsBridge

        function resolveParentThemeManager(item) {
            var themeManagerKey = "theme" + "Manager";
            var current = item ? item.parent : null;

            while (current) {
                var candidate = current[themeManagerKey];
                if (candidate !== undefined && candidate !== null)
                    return candidate;
                current = current.parent;
            }

            return null;
        }

        function resolveParentFont(item) {
            var keys = ["font", "textFont", "titleFont"];
            var current = item ? item.parent : null;

            while (current) {
                for (var i = 0; i < keys.length; ++i) {
                    var candidate = current[keys[i]];
                    if (candidate !== undefined && candidate !== null)
                        return candidate;
                }
                current = current.parent;
            }

            return Qt.font({
                pixelSize: 14,
                weight: Font.Normal
            });
        }
    }
}
