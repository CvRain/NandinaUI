import QtQuick
import Nandina.Theme

Item {
    id: root

    default property alias contentData: contentItem.data

    property var parentThemeManager: themeUtilsBridge.resolveParentThemeManager(root)
    property var themeManager: parentThemeManager ? parentThemeManager : fallbackThemeManager

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
        id: themeUtilsBridge

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
    }
}
