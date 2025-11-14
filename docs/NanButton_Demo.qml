import Nandina.Components
import QtQuick
import QtQuick.Layouts

Item {
    width: 800
    height: 600

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Text {
            text: "NanButton 功能演示"
            font.pixelSize: 24
            font.bold: true
        }

        // 第一行: 自动字体 vs 手动字体
        GroupBox {
            title: "字体控制"
            Layout.fillWidth: true

            RowLayout {
                spacing: 16

                NanButton {
                    text: "自动字体"
                    width: 150
                    height: 60
                    autoFitText: true
                }

                NanButton {
                    text: "手动字体(14pt)"
                    width: 150
                    height: 60
                    autoFitText: false
                    manualFontSize: 14
                }

                NanButton {
                    text: "手动字体(20pt)"
                    width: 150
                    height: 60
                    autoFitText: false
                    manualFontSize: 20
                }

            }

        }

        // 第二行: 图标位置
        GroupBox {
            title: "图标位置"
            Layout.fillWidth: true

            RowLayout {
                spacing: 16

                NanButton {
                    text: "左侧图标"
                    width: 150
                    height: 50
                    iconSource: "qrc:/icons/left.svg" // 替换为实际图标路径
                    iconPosition: NanButton.IconPosition.Left
                    iconSize: 20
                }

                NanButton {
                    text: "右侧图标"
                    width: 150
                    height: 50
                    iconSource: "qrc:/icons/right.svg" // 替换为实际图标路径
                    iconPosition: NanButton.IconPosition.Right
                    iconSize: 20
                }

                NanButton {
                    width: 50
                    height: 50
                    iconSource: "qrc:/icons/icon.svg" // 替换为实际图标路径
                    iconPosition: NanButton.IconPosition.IconOnly
                    iconSize: 28
                }

            }

        }

        // 第三行: 不同尺寸
        GroupBox {
            title: "不同尺寸"
            Layout.fillWidth: true

            RowLayout {
                spacing: 16
                alignment: Qt.AlignVCenter

                NanButton {
                    text: "小"
                    width: 80
                    height: 40
                    autoFitText: true
                }

                NanButton {
                    text: "中等"
                    width: 120
                    height: 50
                    autoFitText: true
                }

                NanButton {
                    text: "大"
                    width: 160
                    height: 60
                    autoFitText: true
                }

                NanButton {
                    text: "超大"
                    width: 200
                    height: 80
                    autoFitText: true
                }

            }

        }

        // 第四行: 带图标的不同尺寸
        GroupBox {
            title: "图标 + 自动字体"
            Layout.fillWidth: true

            RowLayout {
                spacing: 16
                alignment: Qt.AlignVCenter

                NanButton {
                    text: "保存"
                    width: 100
                    height: 45
                    iconSource: "qrc:/icons/save.svg" // 替换为实际图标路径
                    iconPosition: NanButton.IconPosition.Left
                    autoFitText: true
                }

                NanButton {
                    text: "上传文件"
                    width: 160
                    height: 55
                    iconSource: "qrc:/icons/upload.svg" // 替换为实际图标路径
                    iconPosition: NanButton.IconPosition.Left
                    autoFitText: true
                }

                NanButton {
                    text: "下一步"
                    width: 140
                    height: 50
                    iconSource: "qrc:/icons/next.svg" // 替换为实际图标路径
                    iconPosition: NanButton.IconPosition.Right
                    autoFitText: true
                }

            }

        }

        // 第五行: 自定义动画参数
        GroupBox {
            title: "自定义动画"
            Layout.fillWidth: true

            RowLayout {
                spacing: 16

                NanButton {
                    text: "默认动画"
                    width: 140
                    height: 50
                }

                NanButton {
                    text: "强烈动画"
                    width: 140
                    height: 50
                    hoverScale: 1.1
                    pressScale: 0.9
                    hoverBrightness: 1.3
                    pressBrightness: 0.7
                }

                NanButton {
                    text: "轻微动画"
                    width: 140
                    height: 50
                    hoverScale: 1.02
                    pressScale: 0.98
                    hoverBrightness: 1.05
                    pressBrightness: 0.95
                }

            }

        }

        Item {
            Layout.fillHeight: true
        }

    }

}
