import QtQuick

QtObject {
    readonly property font display: Qt.font({
        pixelSize: 36,
        weight: Font.DemiBold
    })
    readonly property font titleLarge: Qt.font({
        pixelSize: 28,
        weight: Font.DemiBold
    })
    readonly property font title: Qt.font({
        pixelSize: 22,
        weight: Font.DemiBold
    })
    readonly property font subtitle: Qt.font({
        pixelSize: 18,
        weight: Font.DemiBold
    })
    readonly property font bodyLarge: Qt.font({
        pixelSize: 16,
        weight: Font.Normal
    })
    readonly property font body: Qt.font({
        pixelSize: 14,
        weight: Font.Normal
    })
    readonly property font bodyStrong: Qt.font({
        pixelSize: 14,
        weight: Font.DemiBold
    })
    readonly property font caption: Qt.font({
        pixelSize: 12,
        weight: Font.Normal
    })
}
